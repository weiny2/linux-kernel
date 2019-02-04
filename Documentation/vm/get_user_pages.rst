.. _get_user_pages:

==============
get_user_pages
==============

.. contents:: :local:

Overview
========

Some kernel components (file systems, device drivers) need to access
memory that is specified via process virtual address. For a long time, the
API to achieve that was get_user_pages ("GUP") and its variations. However,
GUP has critical limitations that have been overlooked; in particular, GUP
does not interact correctly with filesystems in all situations. That means
that file-backed memory + GUP is a recipe for potential problems, some of
which have already occurred in the field.

GUP was first introduced for Direct IO (O_DIRECT), allowing filesystem code
to get the struct page behind a virtual address and to let storage hardware
perform a direct copy to or from that page. This is a short-lived access
pattern, and as such, the window for a concurrent writeback of GUP'd page
was small enough that there were not (we think) any reported problems.
Also, userspace was expected to understand and accept that Direct IO was
not synchronized with memory-mapped access to that data, nor with any
process address space changes such as munmap(), mremap(), etc.

Over the years, more GUP uses have appeared (virtualization, device
drivers, RDMA) that can keep the pages they get via GUP for a long period
of time (seconds, minutes, hours, days, ...). This long-term pinning makes
an underlying design problem more obvious.

In fact, there are a number of key problems inherent to GUP:

Interactions with file systems
==============================

File systems expect to be able to write back data, both to reclaim pages,
and for data integrity. Allowing other hardware (NICs, GPUs, etc) to gain
write access to the file memory pages means that such hardware can dirty the
pages, without the filesystem being aware. This can, in some cases
(depending on filesystem, filesystem options, block device, block device
options, and other variables), lead to data corruption, and also to kernel
bugs of the form:

::

    kernel BUG at /build/linux-fQ94TU/linux-4.4.0/fs/ext4/inode.c:1899!
    backtrace:

	ext4_writepage
	__writepage
	write_cache_pages
	ext4_writepages
	do_writepages
	__writeback_single_inode
	writeback_sb_inodes
	__writeback_inodes_wb
	wb_writeback
	wb_workfn
	process_one_work
	worker_thread
	kthread
	ret_from_fork

...which is due to the file system asserting that there are still buffer
heads attached:

::

 /* If we *know* page->private refers to buffer_heads */
 #define page_buffers(page)                                      \
        ({                                                      \
                BUG_ON(!PagePrivate(page));                     \
                ((struct buffer_head *)page_private(page));     \
        })
 #define page_has_buffers(page)  PagePrivate(page)

Dave Chinner's description of this is very clear:

    "The fundamental issue is that ->page_mkwrite must be called on every
    write access to a clean file backed page, not just the first one.
    How long the GUP reference lasts is irrelevant, if the page is clean
    and you need to dirty it, you must call ->page_mkwrite before it is
    marked writeable and dirtied. Every. Time."

This is just one symptom of the larger design problem: filesystems do not
actually support get_user_pages() being called on their pages, and letting
hardware write directly to those pages--even though that pattern has been
going on since about 2005 or so.

Long term GUP
=============

Long term GUP is an issue when FOLL_WRITE is specified to GUP (so, a
writeable mapping is created), and the pages are file-backed. That can lead
to filesystem corruption. What happens is that when a file-backed page is
being written back, it is first mapped read-only in all of the CPU page
tables; the file system then assumes that nobody can write to the page, and
that the page content is therefore stable. Unfortunately, the GUP callers
generally do not monitor changes to the CPU pages tables; they instead
assume that the following pattern is safe (it's not):

::

    get_user_pages()

    Hardware then keeps a reference to those pages for some potentially
    long time. During this time, hardware may write to the pages. Because
    "hardware" here means "devices that are not a CPU", this activity
    occurs without any interaction with the kernel's file system code.

    for each page:
	set_page_dirty()
	put_page()

In fact, the GUP documentation even recommends that pattern.

Anyway, the file system assumes that the page is stable (nothing is writing
to the page), and that is a problem: stable page content is necessary for
many filesystem actions during writeback, such as checksum, encryption,
RAID striping, etc. Furthermore, filesystem features like COW (copy on
write) or snapshot also rely on being able to use a new page for as memory
for that memory range inside the file.

Corruption during write back is clearly possible here. To solve that, one
idea is to identify pages that have active GUP, so that we can use a bounce
page to write stable data to the filesystem. The filesystem would work
on the bounce page, while any of the active GUP might write to the
original page. This would avoid the stable page violation problem, but note
that it is only part of the overall solution, because other problems
remain.

Other filesystem features that need to replace the page with a new one can
be inhibited for pages that are GUP-pinned. This will, however, alter and
limit some of those filesystem features. The only fix for that would be to
require GUP users to monitor and respond to CPU page table updates.
Subsystems such as ODP and HMM do this, for example. This aspect of the
problem is still under discussion.

Direct IO
=========

Direct IO can cause corruption, if userspace does Direct-IO that writes to
a range of virtual addresses that are mmap'd to a file.  The pages written
to are file-backed pages that can be under write back, while the Direct IO
is taking place.  Here, Direct IO races with a write back: it calls
GUP before page_mkclean() has replaced the CPU pte with a read-only entry.
The race window is pretty small, which is probably why years have gone by
before we noticed this problem: Direct IO is generally very quick, and
tends to finish up before the filesystem gets around to do anything with
the page contents.  However, it's still a real problem.  The solution is
to never let GUP return pages that are under write back, but instead,
force GUP to take a write fault on those pages.  That way, GUP will
properly synchronize with the active write back.  This does not change the
required GUP behavior, it just avoids that race.

Measurement and visibility
==========================

There are several /proc/vmstat items, in order to provide some visibility
into what get_user_pages() and put_user_page() are doing.

After booting and running fio (https://github.com/axboe/fio)
a few times on an NVMe device, as a way to get lots of
get_user_pages_fast() calls, the counters look like this:

::

 $ cat /proc/vmstat | grep gup
 nr_gup_slow_pages_requested 21319
 nr_gup_fast_pages_requested 11533792
 nr_gup_fast_page_backoffs 0
 nr_gup_page_count_overflows 0
 nr_gup_pages_returned 11555104

Interpretation of the above:

::

 Total gup requests (slow + fast): 11555111
 Total put_user_page calls:        11555104

This shows 7 more calls to get_user_pages(), than to put_user_page().
That may, or may not, represent a problem worth investigating.

Normally, those last two numbers should be equal, but a couple of things
may cause them to differ:

1. Inherent race condition in reading /proc/vmstat values.

2. Bugs at any of the get_user_pages*() call sites. Those
sites need to match get_user_pages() and put_user_page() calls.



