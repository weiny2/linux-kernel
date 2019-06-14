/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/bits.h>
#include <linux/err.h>
#include <linux/memcontrol.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/oom.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/swap.h>
#include <asm/cet.h>
#include <asm/fpu/internal.h>

#define MMAP_MAX	(unsigned long)(test_thread_flag(TIF_ADDR32) ? \
				TASK_SIZE : TASK_SIZE_MAX)
#define IBT_BITMAP_SIZE	(round_up(MMAP_MAX, PAGE_SIZE * BITS_PER_BYTE) / \
				(PAGE_SIZE * BITS_PER_BYTE))

/*
 * For read fault, provide the zero page.  For write fault coming from
 * get_user_pages(), clear the page already allocated.
 */
static vm_fault_t bitmap_fault(const struct vm_special_mapping *sm,
			       struct vm_area_struct *vma, struct vm_fault *vmf)
{
	if (!(vmf->flags & FAULT_FLAG_WRITE)) {
		vmf->page = ZERO_PAGE(vmf->address);
		return 0;
	} else {
		vm_fault_t r;

		if (!vmf->cow_page)
			return VM_FAULT_ERROR;

		clear_user_highpage(vmf->cow_page, vmf->address);
		__SetPageUptodate(vmf->cow_page);
		r = finish_fault(vmf);
		return r ? r : VM_FAULT_DONE_COW;
	}
}

static int bitmap_mremap(const struct vm_special_mapping *sm,
			 struct vm_area_struct *vma)
{
	return -EINVAL;
}

static const struct vm_special_mapping bitmap_mapping = {
	.name	= "[ibt_bitmap]",
	.fault	= bitmap_fault,
	.mremap	= bitmap_mremap,
};

static int alloc_bitmap(void)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long addr;
	u64 msr_val;
	int r = 0;

	if (down_write_killable(&mm->mmap_sem))
		return -EINTR;

	addr = get_unmapped_area(NULL, 0, IBT_BITMAP_SIZE, 0, 0);
	if (IS_ERR_VALUE(addr)) {
		up_write(&mm->mmap_sem);
		return PTR_ERR((void *)addr);
	}

	vma = _install_special_mapping(mm, addr, IBT_BITMAP_SIZE,
				       VM_READ | VM_MAYREAD | VM_MAYWRITE,
				       &bitmap_mapping);

	if (IS_ERR(vma))
		r = PTR_ERR(vma);

	up_write(&mm->mmap_sem);

	if (r)
		return r;

	fpregs_lock();
	if (test_thread_flag(TIF_NEED_FPU_LOAD))
		__fpregs_load_activate();

	rdmsrl(MSR_IA32_U_CET, msr_val);
	msr_val |= (addr | MSR_IA32_CET_LEG_IW_EN);
	wrmsrl(MSR_IA32_U_CET, msr_val);
	fpregs_unlock();
	current->thread.cet.ibt_bitmap_used = 1;
	current->thread.cet.ibt_bitmap_base = addr;
	return 0;
}

/*
 * Set bits in the IBT legacy code bitmap, which is read-only user memory.
 */
static int set_bits(unsigned long start_bit, unsigned long end_bit,
		    unsigned long set)
{
	unsigned long start_ul, end_ul, nr_ul;
	unsigned long start_ul_addr, tmp_addr, len;
	int i, j;

	start_ul = start_bit / BITS_PER_LONG;
	end_ul = end_bit / BITS_PER_LONG;
	i = start_bit % BITS_PER_LONG;
	j = end_bit % BITS_PER_LONG;

	tmp_addr = current->thread.cet.ibt_bitmap_base;
	start_ul_addr = tmp_addr + start_ul * sizeof(0UL);
	nr_ul = end_ul - start_ul + 1;

	tmp_addr = start_ul_addr;
	len = nr_ul * sizeof(0UL);

	down_read(&current->mm->mmap_sem);
	while (len) {
		unsigned long *first, *last, mask, bytes;
		int ret, offset;
		void *kern_page_addr;
		struct page *page = NULL;

		ret = get_user_pages(tmp_addr, 1, FOLL_WRITE | FOLL_FORCE,
				     &page, NULL);

		if (ret <= 0) {
			up_read(&current->mm->mmap_sem);
			return ret;
		}

		kern_page_addr = kmap(page);

		bytes = len;
		offset = tmp_addr & (PAGE_SIZE - 1);

		/* Is end_ul in this page? */
		if (bytes > (PAGE_SIZE - offset)) {
			bytes = PAGE_SIZE - offset;
			last = NULL;
		} else {
			last = (unsigned long *)(kern_page_addr + offset + bytes) - 1;
		}

		/* Is start_ul in this page? */
		if (tmp_addr == start_ul_addr)
			first = (unsigned long *)(kern_page_addr + offset);
		else
			first = NULL;

		if (nr_ul == 1) {
			mask = GENMASK(j, i);

			if (set)
				*first |= mask;
			else
				*first &= ~mask;
		} else {
			if (first) {
				mask = GENMASK(BITS_PER_LONG - 1, i);

				if (set)
					*first |= mask;
				else
					*first &= ~mask;
			}

			if (last) {
				mask = GENMASK(j, 0);

				if (set)
					*last |= mask;
				else
					*last &= ~mask;
			}

			if (nr_ul > 2) {
				void *p = kern_page_addr + offset;
				int cnt = bytes;

				if (first) {
					p += sizeof(*first);
					cnt -= sizeof(*first);
				}

				if (last)
					cnt -= sizeof(*last);

				if (set)
					memset(p, 0xff, cnt);
				else
					memset(p, 0, cnt);
			}
		}

		set_page_dirty_lock(page);
		kunmap(page);
		put_page(page);

		len -= bytes;
		tmp_addr += bytes;
	}
	up_read(&current->mm->mmap_sem);
	return 0;
}

int cet_mark_legacy_code(unsigned long addr, unsigned long size, unsigned long set)
{
	int r;

	if (!current->thread.cet.ibt_enabled)
		return -EINVAL;

	if ((addr >= MMAP_MAX) || (addr + size > MMAP_MAX))
		return -EINVAL;

	if (!current->thread.cet.ibt_bitmap_used) {
		r = alloc_bitmap();
		if (r)
			return r;
	}

	return set_bits(addr / PAGE_SIZE, (addr + size - 1) / PAGE_SIZE, set);
}
