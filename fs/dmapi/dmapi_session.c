/*
 * Copyright (c) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Further, this software is distributed without any warranty that it is
 * free of the rightful claim of any third person regarding infringement
 * or the like.	 Any license provided herein, whether implied or
 * otherwise, applies only to this software file.  Patent licenses, if
 * any, provided herein do not apply to combinations of this program with
 * other software, or any other product whatsoever.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Contact information: Silicon Graphics, Inc., 1600 Amphitheatre Pkwy,
 * Mountain View, CA  94043, or:
 *
 * http://www.sgi.com
 *
 * For further information regarding this notice, see:
 *
 * http://oss.sgi.com/projects/GenInfo/SGIGPLNoticeExplan/
 */

#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#ifdef CONFIG_PROC_FS
#include <linux/module.h>
#endif
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/seq_file.h>
#include "dmapi.h"
#include "dmapi_kern.h"
#include "dmapi_private.h"

dm_session_t	*dm_sessions = NULL;	/* head of session list */
u_int		dm_sessions_active = 0; /* # sessions currently active */
dm_sessid_t	dm_next_sessid = 1;	/* next session ID to use */
DEFINE_SPINLOCK(dm_session_lock);/* lock for session list */

dm_token_t	dm_next_token = 1;	/* next token ID to use */
dm_sequence_t	dm_next_sequence = 1;	/* next sequence number to use */
DEFINE_SPINLOCK(dm_token_lock);/* dm_next_token/dm_next_sequence lock */

int	dm_max_queued_msgs = 2048;	/* max # undelivered msgs/session */

int	dm_hash_buckets = 1009;		/* prime -- number of buckets */

#define DM_SHASH(sess,inodenum)	\
		((sess)->sn_sesshash + do_mod((inodenum), dm_hash_buckets))

extern struct proc_dir_entry    *dm_proc_sessions_entry;

#ifdef CONFIG_PROC_FS
static int
sessions_read_pfs(struct seq_file *seq, void *data)
{
	dm_session_t	*sessp = (dm_session_t*)seq->private;

	while(1){
		seq_printf(seq, "sessp=0x%p\n", sessp);
		seq_printf(seq, "sn_next=0x%p\n", sessp->sn_next);
		seq_printf(seq, "sn_sessid=%d\n", sessp->sn_sessid);
		seq_printf(seq, "sn_flags=%x\n", sessp->sn_flags);
		seq_printf(seq, "sn_qlock=%c\n", '?');
		seq_printf(seq, "sn_readerq=%c\n", '?');
		seq_printf(seq, "sn_writerq=%c\n", '?');
		seq_printf(seq, "sn_readercnt=%u\n", sessp->sn_readercnt);
		seq_printf(seq, "sn_writercnt=%u\n", sessp->sn_writercnt);

		seq_printf(seq, "sn_newq.eq_head=0x%p\n", sessp->sn_newq.eq_head);
		seq_printf(seq, "sn_newq.eq_tail=0x%p\n", sessp->sn_newq.eq_tail);
		seq_printf(seq, "sn_newq.eq_count=%d\n", sessp->sn_newq.eq_count);

		seq_printf(seq, "sn_delq.eq_head=0x%p\n", sessp->sn_delq.eq_head);
		seq_printf(seq, "sn_delq.eq_tail=0x%p\n", sessp->sn_delq.eq_tail);
		seq_printf(seq, "sn_delq.eq_count=%d\n", sessp->sn_delq.eq_count);

		seq_printf(seq, "sn_evt_writerq.eq_head=0x%p\n", sessp->sn_evt_writerq.eq_head);
		seq_printf(seq, "sn_evt_writerq.eq_tail=0x%p\n", sessp->sn_evt_writerq.eq_tail);
		seq_printf(seq, "sn_evt_writerq.eq_count=%d\n", sessp->sn_evt_writerq.eq_count);

		seq_printf(seq, "sn_info=\"%s\"\n", sessp->sn_info);

		break;
	}

	return 0;
}

static int sessions_pfs_open(struct inode *inode, struct file *file)
{
	dm_fsreg_t	*fsrp = PDE_DATA(inode);

	return single_open(file, sessions_read_pfs, fsrp);
}

static const struct file_operations sessions_pfs_fops = {
	.owner		= THIS_MODULE,
	.open		= sessions_pfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif


/* Link a session to the end of the session list.  New sessions are always
   added at the end of the list so that dm_enqueue_mount_event() doesn't
   miss a session.  The caller must have obtained dm_session_lock before
   calling this routine.
*/

static void
link_session(
	dm_session_t	*s)
{
	dm_session_t	*tmp;

	if ((tmp = dm_sessions) == NULL) {
		dm_sessions = s;
	} else {
		while (tmp->sn_next != NULL)
			tmp = tmp->sn_next;
		tmp->sn_next = s;
	}
	s->sn_next = NULL;
	dm_sessions_active++;
}


/* Remove a session from the session list.  The caller must have obtained
   dm_session_lock before calling this routine.	 unlink_session() should only
   be used in situations where the session is known to be on the dm_sessions
   list; otherwise it panics.
*/

static void
unlink_session(
	dm_session_t	*s)
{
	dm_session_t	*tmp;

	if (dm_sessions == s) {
		dm_sessions = dm_sessions->sn_next;
	} else {
		for (tmp = dm_sessions; tmp; tmp = tmp->sn_next) {
			if (tmp->sn_next == s)
				break;
		}
		if (tmp == NULL) {
			panic("unlink_session: corrupt DMAPI session list, "
				"dm_sessions %p, session %p\n",
				dm_sessions, s);
		}
		tmp->sn_next = s->sn_next;
	}
	s->sn_next = NULL;
	dm_sessions_active--;
}


/* Link an event to the end of an event queue.	The caller must have obtained
   the session's sn_qlock before calling this routine.
*/

void
dm_link_event(
	dm_tokevent_t	*tevp,
	dm_eventq_t	*queue)
{
	if (queue->eq_tail) {
		queue->eq_tail->te_next = tevp;
		queue->eq_tail = tevp;
	} else {
		queue->eq_head = queue->eq_tail = tevp;
	}
	tevp->te_next = NULL;
	queue->eq_count++;
}


/* Remove an event from an event queue.	 The caller must have obtained the
   session's sn_qlock before calling this routine.  dm_unlink_event() should
   only be used in situations where the event is known to be on the queue;
   otherwise it panics.
*/

void
dm_unlink_event(
	dm_tokevent_t	*tevp,
	dm_eventq_t	*queue)
{
	dm_tokevent_t	*tmp;

	if (queue->eq_head == tevp) {
		queue->eq_head = tevp->te_next;
		if (queue->eq_head == NULL)
			queue->eq_tail = NULL;
	} else {
		tmp = queue->eq_head;
		while (tmp && tmp->te_next != tevp)
			tmp = tmp->te_next;
		if (tmp == NULL) {
			panic("dm_unlink_event: corrupt DMAPI queue %p, "
				"tevp %p\n", queue, tevp);
		}
		tmp->te_next = tevp->te_next;
		if (tmp->te_next == NULL)
			queue->eq_tail = tmp;
	}
	tevp->te_next = NULL;
	queue->eq_count--;
}

/* Link a regular file event to a hash bucket.	The caller must have obtained
   the session's sn_qlock before calling this routine.
   The tokevent must be for a regular file object--DM_TDT_REG.
*/

static void
hash_event(
	dm_session_t	*s,
	dm_tokevent_t	*tevp)
{
	dm_sesshash_t	*sh;
	dm_ino_t	ino;

	if (s->sn_sesshash == NULL) {
		s->sn_sesshash = kmalloc(dm_hash_buckets * sizeof(dm_sesshash_t), GFP_KERNEL);
		if (s->sn_sesshash == NULL) {
			printk("%s/%d: kmalloc returned NULL\n", __FUNCTION__, __LINE__);
			return;
		}
		memset(s->sn_sesshash, 0, dm_hash_buckets * sizeof(dm_sesshash_t));
	}

	ino = (&tevp->te_tdp->td_handle.ha_fid)->dm_fid_ino;
	sh = DM_SHASH(s, ino);

#ifdef DM_SHASH_DEBUG
	if (sh->h_next == NULL) {
		s->sn_buckets_in_use++;
		if (s->sn_buckets_in_use > s->sn_max_buckets_in_use)
			s->sn_max_buckets_in_use++;
	}
	sh->maxlength++;
	sh->curlength++;
	sh->num_adds++;
#endif

	tevp->te_flags |= DM_TEF_HASHED;
	tevp->te_hashnext = sh->h_next;
	sh->h_next = tevp;
}


/* Remove a regular file event from a hash bucket.  The caller must have
   obtained the session's sn_qlock before calling this routine.
   The tokevent must be for a regular file object--DM_TDT_REG.
*/

static void
unhash_event(
	dm_session_t	*s,
	dm_tokevent_t	*tevp)
{
	dm_sesshash_t	*sh;
	dm_tokevent_t	*tmp;
	dm_ino_t	ino;

	if (s->sn_sesshash == NULL)
		return;

	ino = (&tevp->te_tdp->td_handle.ha_fid)->dm_fid_ino;
	sh = DM_SHASH(s, ino);

	if (sh->h_next == tevp) {
		sh->h_next = tevp->te_hashnext; /* leap frog */
	} else {
		tmp = sh->h_next;
		while (tmp->te_hashnext != tevp) {
			tmp = tmp->te_hashnext;
		}
		tmp->te_hashnext = tevp->te_hashnext; /* leap frog */
	}
	tevp->te_hashnext = NULL;
	tevp->te_flags &= ~DM_TEF_HASHED;

#ifdef DM_SHASH_DEBUG
	if (sh->h_next == NULL)
		s->sn_buckets_in_use--;
	sh->curlength--;
	sh->num_dels++;
#endif
}


/* Determine if this is a repeat event.	 The caller MUST be holding
   the session lock.
   The tokevent must be for a regular file object--DM_TDT_REG.
   Returns:
	0 == match not found
	1 == match found
*/

static int
repeated_event(
	dm_session_t	*s,
	dm_tokevent_t	*tevp)
{
	dm_sesshash_t	*sh;
	dm_data_event_t *d_event1;
	dm_data_event_t *d_event2;
	dm_tokevent_t	*tevph;
	dm_ino_t	ino1;
	dm_ino_t	ino2;

	if ((!s->sn_newq.eq_tail) && (!s->sn_delq.eq_tail)) {
		return(0);
	}
	if (s->sn_sesshash == NULL) {
		return(0);
	}

	ino1 = (&tevp->te_tdp->td_handle.ha_fid)->dm_fid_ino;
	sh = DM_SHASH(s, ino1);

	if (sh->h_next == NULL) {
		/* bucket is empty, no match here */
		return(0);
	}

	d_event1 = (dm_data_event_t *)((char *)&tevp->te_msg + tevp->te_msg.ev_data.vd_offset);
	tevph = sh->h_next;
	while (tevph) {
		/* find something with the same event type and handle type */
		if ((tevph->te_msg.ev_type == tevp->te_msg.ev_type) &&
		    (tevph->te_tdp->td_type == tevp->te_tdp->td_type)) {

			ino2 = (&tevp->te_tdp->td_handle.ha_fid)->dm_fid_ino;
			d_event2 = (dm_data_event_t *)((char *)&tevph->te_msg + tevph->te_msg.ev_data.vd_offset);

			/* If the two events are operating on the same file,
			   and the same part of that file, then we have a
			   match.
			*/
			if ((ino1 == ino2) &&
			    (d_event2->de_offset == d_event1->de_offset) &&
			    (d_event2->de_length == d_event1->de_length)) {
				/* found a match */
#ifdef DM_SHASH_DEBUG
				sh->dup_hits++;
#endif
				return(1);
			}
		}
		tevph = tevph->te_hashnext;
	}

	/* No match found */
	return(0);
}


/* Return a pointer to a session given its session ID, or EINVAL if no session
   has the session ID (per the DMAPI spec).  The caller must have obtained
   dm_session_lock before calling this routine.
*/

static int
dm_find_session(
	dm_sessid_t	sid,
	dm_session_t	**sessionpp)
{
	dm_session_t	*s;

	for (s = dm_sessions; s; s = s->sn_next) {
		if (s->sn_sessid == sid) {
			*sessionpp = s;
			return(0);
		}
	}
	return(-EINVAL);
}


/* Return a pointer to a locked session given its session ID.  '*lcp' is
   used to obtain the session's sn_qlock.  Caller is responsible for eventually
   unlocking it.
*/

int
dm_find_session_and_lock(
	dm_sessid_t	sid,
	dm_session_t	**sessionpp,
	unsigned long	*lcp)		/* addr of returned lock cookie */
{
	int		error;

	for (;;) {
		*lcp = mutex_spinlock(&dm_session_lock);

		if ((error = dm_find_session(sid, sessionpp)) != 0) {
			mutex_spinunlock(&dm_session_lock, *lcp);
			return(error);
		}
		if (spin_trylock(&(*sessionpp)->sn_qlock)) {
			nested_spinunlock(&dm_session_lock);
			return(0);	/* success */
		}

		/* If the second lock is not available, drop the first and
		   start over.	This gives the CPU a chance to process any
		   interrupts, and also allows processes which want a sn_qlock
		   for a different session to proceed.
		*/

		mutex_spinunlock(&dm_session_lock, *lcp);
	}
}


/* Return a pointer to the event on the specified session's sn_delq which
   contains the given token.  The caller must have obtained the session's
   sn_qlock before calling this routine.
*/

static int
dm_find_msg(
	dm_session_t	*s,
	dm_token_t	token,
	dm_tokevent_t	**tevpp)
{
	dm_tokevent_t	*tevp;

	if (token <= DM_INVALID_TOKEN)
		return(-EINVAL);

	for (tevp = s->sn_delq.eq_head; tevp; tevp = tevp->te_next) {
		if (tevp->te_msg.ev_token == token) {
			*tevpp = tevp;
			return(0);
		}
	}
	return(-ESRCH);
}


/* Given a session ID and token, find the tevp on the specified session's
   sn_delq which corresponds to that session ID/token pair.  If a match is
   found, lock the tevp's te_lock and return a pointer to the tevp.
   '*lcp' is used to obtain the tevp's te_lock.	 The caller is responsible
   for eventually unlocking it.
*/

int
dm_find_msg_and_lock(
	dm_sessid_t	sid,
	dm_token_t	token,
	dm_tokevent_t	**tevpp,
	unsigned long	*lcp)		/* address of returned lock cookie */
{
	dm_session_t	*s;
	int		error;

	if ((error = dm_find_session_and_lock(sid, &s, lcp)) != 0)
		return(error);

	if ((error = dm_find_msg(s, token, tevpp)) != 0) {
		mutex_spinunlock(&s->sn_qlock, *lcp);
		return(error);
	}
	nested_spinlock(&(*tevpp)->te_lock);
	nested_spinunlock(&s->sn_qlock);
	return(0);
}


/* Create a new session, or resume an old session if one is given. */

int
dm_create_session(
	dm_sessid_t	old,
	char		__user *info,
	dm_sessid_t	__user *new)
{
	dm_session_t	*s;
	dm_sessid_t	sid;
	char		sessinfo[DM_SESSION_INFO_LEN];
	size_t		len;
	int		error;
	unsigned long		lc;		/* lock cookie */

	len = strnlen_user(info, DM_SESSION_INFO_LEN-1);
	/* must check length from strnlen_user since v3.4-7800-ga08c535 */
	if (len > DM_SESSION_INFO_LEN)
		len = DM_SESSION_INFO_LEN;
	if (copy_from_user(sessinfo, info, len))
		return(-EFAULT);
	lc = mutex_spinlock(&dm_session_lock);
	sid = dm_next_sessid++;
	mutex_spinunlock(&dm_session_lock, lc);
	if (copy_to_user(new, &sid, sizeof(sid)))
		return(-EFAULT);

	if (old == DM_NO_SESSION) {
		s = kmem_cache_alloc(dm_session_cachep, GFP_KERNEL);
		if (s == NULL) {
			printk("%s/%d: kmem_cache_alloc(dm_session_cachep) returned NULL\n", __FUNCTION__, __LINE__);
			return -ENOMEM;
		}
		memset(s, 0, sizeof(*s));

		sv_init(&s->sn_readerq, SV_DEFAULT, "dmreadq");
		sv_init(&s->sn_writerq, SV_DEFAULT, "dmwritq");
		spinlock_init(&s->sn_qlock, "sn_qlock");
	} else {
		lc = mutex_spinlock(&dm_session_lock);
		if ((error = dm_find_session(old, &s)) != 0) {
			mutex_spinunlock(&dm_session_lock, lc);
			return(error);
		}
		unlink_session(s);
		mutex_spinunlock(&dm_session_lock, lc);
#ifdef CONFIG_PROC_FS
		{
		char buf[100];
		sprintf(buf, DMAPI_DBG_PROCFS "/sessions/0x%p", s);
		remove_proc_entry(buf, NULL);
		}
#endif
	}
	memcpy(s->sn_info, sessinfo, len);
	s->sn_info[len-1] = 0;		/* if not NULL, then now 'tis */
	s->sn_sessid = sid;
	lc = mutex_spinlock(&dm_session_lock);
	link_session(s);
	mutex_spinunlock(&dm_session_lock, lc);
#ifdef CONFIG_PROC_FS
	{
	char buf[100];

	sprintf(buf, "0x%p", s);
	proc_create_data(buf, 0, dm_proc_sessions_entry, &sessions_pfs_fops, s);
	}
#endif
	return(0);
}


int
dm_destroy_session(
	dm_sessid_t	sid)
{
	dm_session_t	*s;
	int		error;
	unsigned long		lc;		/* lock cookie */

	/* The dm_session_lock must be held until the session is unlinked. */

	lc = mutex_spinlock(&dm_session_lock);

	if ((error = dm_find_session(sid, &s)) != 0) {
		mutex_spinunlock(&dm_session_lock, lc);
		return(error);
	}
	nested_spinlock(&s->sn_qlock);

	/* The session exists.	Check to see if it is still in use.  If any
	   messages still exist on the sn_newq or sn_delq, or if any processes
	   are waiting for messages to arrive on the session, then the session
	   must not be destroyed.
	*/

	if (s->sn_newq.eq_head || s->sn_readercnt || s->sn_delq.eq_head) {
		nested_spinunlock(&s->sn_qlock);
		mutex_spinunlock(&dm_session_lock, lc);
		return(-EBUSY);
	}

	/* The session is not in use.  Dequeue it from the session chain. */

	unlink_session(s);
	nested_spinunlock(&s->sn_qlock);
	mutex_spinunlock(&dm_session_lock, lc);

#ifdef CONFIG_PROC_FS
	{
	char buf[100];
	sprintf(buf, DMAPI_DBG_PROCFS "/sessions/0x%p", s);
	remove_proc_entry(buf, NULL);
	}
#endif

	/* Now clear the sessions's disposition registration, and then destroy
	   the session structure.
	*/

	dm_clear_fsreg(s);

	spinlock_destroy(&s->sn_qlock);
	sv_destroy(&s->sn_readerq);
	sv_destroy(&s->sn_writerq);
	if (s->sn_sesshash)
		kfree(s->sn_sesshash);
	kmem_cache_free(dm_session_cachep, s);
	return(0);
}


/*
 *  Return a list of all active sessions.
 */

int
dm_getall_sessions(
	u_int		nelem,
	dm_sessid_t	__user *sidp,
	u_int		__user *nelemp)
{
	dm_session_t	*s;
	u_int		sesscnt;
	dm_sessid_t	*sesslist;
	unsigned long		lc;		/* lock cookie */
	int		error;
	int		i;

	/* Loop until we can get the right amount of temp space, being careful
	   not to hold a mutex during the allocation.  Usually only one trip.
	*/

	for (;;) {
		if ((sesscnt = dm_sessions_active) == 0) {
			/*if (suword(nelemp, 0))*/
			if (put_user(0, nelemp))
				return(-EFAULT);
			return(0);
		}
		sesslist = kmalloc(sesscnt * sizeof(*sidp), GFP_KERNEL);
		if (sesslist == NULL) {
			printk("%s/%d: kmalloc returned NULL\n", __FUNCTION__, __LINE__);
			return -ENOMEM;
		}

		lc = mutex_spinlock(&dm_session_lock);
		if (sesscnt == dm_sessions_active)
			break;

		mutex_spinunlock(&dm_session_lock, lc);
		kfree(sesslist);
	}

	/* Make a temp copy of the data, then release the mutex. */

	for (i = 0, s = dm_sessions; i < sesscnt; i++, s = s->sn_next)
		sesslist[i] = s->sn_sessid;

	mutex_spinunlock(&dm_session_lock, lc);

	/* Now copy the data to the user. */

	if(put_user(sesscnt, nelemp)) {
		error = -EFAULT;
	} else if (sesscnt > nelem) {
		error = -E2BIG;
	} else if (copy_to_user(sidp, sesslist, sesscnt * sizeof(*sidp))) {
		error = -EFAULT;
	} else {
		error = 0;
	}
	kfree(sesslist);
	return(error);
}


/*
 *  Return the descriptive string associated with a session.
 */

int
dm_query_session(
	dm_sessid_t	sid,
	size_t		buflen,
	void		__user *bufp,
	size_t		__user *rlenp)
{
	dm_session_t	*s;		/* pointer to session given by sid */
	int		len;		/* length of session info string */
	int		error;
	char		sessinfo[DM_SESSION_INFO_LEN];
	unsigned long	lc;		/* lock cookie */

	if ((error = dm_find_session_and_lock(sid, &s, &lc)) != 0)
		return(error);

	len = strlen(s->sn_info) + 1;	/* NULL terminated when created */
	memcpy(sessinfo, s->sn_info, len);

	mutex_spinunlock(&s->sn_qlock, lc);

	/* Now that the mutex is released, copy the sessinfo to the user. */

	if (put_user(len, rlenp)) {
		error = -EFAULT;
	} else if (len > buflen) {
		error = -E2BIG;
	} else if (copy_to_user(bufp, sessinfo, len)) {
		error = -EFAULT;
	} else {
		error = 0;
	}
	return(error);
}


/*
 *  Return all of the previously delivered tokens (that is, their IDs)
 *  for the given session.
 */

int
dm_getall_tokens(
	dm_sessid_t	sid,		/* session obtaining tokens from */
	u_int		nelem,		/* size of tokenbufp */
	dm_token_t	__user *tokenbufp,/* buffer to copy token IDs to */
	u_int		__user *nelemp)	/* return number copied to tokenbufp */
{
	dm_session_t	*s;		/* pointer to session given by sid */
	dm_tokevent_t	*tevp;		/* event message queue traversal */
	unsigned long	lc;		/* lock cookie */
	int		tokcnt;
	dm_token_t	*toklist;
	int		error;
	int		i;

	/* Loop until we can get the right amount of temp space, being careful
	   not to hold a mutex during the allocation.  Usually only one trip.
	*/

	for (;;) {
		if ((error = dm_find_session_and_lock(sid, &s, &lc)) != 0)
			return(error);
		tokcnt = s->sn_delq.eq_count;
		mutex_spinunlock(&s->sn_qlock, lc);

		if (tokcnt == 0) {
			/*if (suword(nelemp, 0))*/
			if (put_user(0, nelemp))
				return(-EFAULT);
			return(0);
		}
		toklist = kmalloc(tokcnt * sizeof(*tokenbufp), GFP_KERNEL);
		if (toklist == NULL) {
			printk("%s/%d: kmalloc returned NULL\n", __FUNCTION__, __LINE__);
			return -ENOMEM;
		}

		if ((error = dm_find_session_and_lock(sid, &s, &lc)) != 0) {
			kfree(toklist);
			return(error);
		}

		if (tokcnt == s->sn_delq.eq_count)
			break;

		mutex_spinunlock(&s->sn_qlock, lc);
		kfree(toklist);
	}

	/* Make a temp copy of the data, then release the mutex. */

	tevp = s->sn_delq.eq_head;
	for (i = 0; i < tokcnt; i++, tevp = tevp->te_next)
		toklist[i] = tevp->te_msg.ev_token;

	mutex_spinunlock(&s->sn_qlock, lc);

	/* Now copy the data to the user. */

	if (put_user(tokcnt, nelemp)) {
		error = -EFAULT;
	} else if (tokcnt > nelem) {
		error = -E2BIG;
	} else if (copy_to_user(tokenbufp,toklist,tokcnt*sizeof(*tokenbufp))) {
		error = -EFAULT;
	} else {
		error = 0;
	}
	kfree(toklist);
	return(error);
}


/*
 *  Return the message identified by token.
 */

int
dm_find_eventmsg(
	dm_sessid_t	sid,
	dm_token_t	token,
	size_t		buflen,
	void		__user *bufp,
	size_t		__user *rlenp)
{
	dm_tokevent_t	*tevp;		/* message identified by token */
	int		msgsize;	/* size of message to copy out */
	void		*msg;
	int		error;
	unsigned long	lc;		/* lock cookie */

	/* Because some of the events (dm_data_event_t in particular) contain
	   __u64 fields, we need to make sure that the buffer provided by the
	   caller is aligned such that he can read those fields successfully.
	*/

	if (((unsigned long)bufp & (sizeof(__u64) - 1)) != 0)
		return(-EFAULT);

	/* Allocate the right amount of temp space, being careful not to hold
	   a mutex during the allocation.
	*/

	if ((error = dm_find_msg_and_lock(sid, token, &tevp, &lc)) != 0)
		return(error);
	msgsize = tevp->te_allocsize - offsetof(dm_tokevent_t, te_msg);
	mutex_spinunlock(&tevp->te_lock, lc);

	msg = kmalloc(msgsize, GFP_KERNEL);
	if (msg == NULL) {
		printk("%s/%d: kmalloc returned NULL\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	if ((error = dm_find_msg_and_lock(sid, token, &tevp, &lc)) != 0) {
		kfree(msg);
		return(error);
	}

	/* Make a temp copy of the data, then release the mutex. */

	memcpy(msg, &tevp->te_msg, msgsize);
	mutex_spinunlock(&tevp->te_lock, lc);

	/* Now copy the data to the user. */

	if (put_user(msgsize,rlenp)) {
		error = -EFAULT;
	} else if (msgsize > buflen) {		/* user buffer not big enough */
		error = -E2BIG;
	} else if (copy_to_user( bufp, msg, msgsize )) {
		error = -EFAULT;
	} else {
		error = 0;
	}
	kfree(msg);
	return(error);
}


int
dm_move_event(
	dm_sessid_t	srcsid,
	dm_token_t	token,
	dm_sessid_t	targetsid,
	dm_token_t	__user *rtokenp)
{
	dm_session_t	*s1;
	dm_session_t	*s2;
	dm_tokevent_t	*tevp;
	int		error;
	unsigned long		lc;		/* lock cookie */
	int		hash_it = 0;

	lc = mutex_spinlock(&dm_session_lock);

	if ((error = dm_find_session(srcsid, &s1)) != 0 ||
	    (error = dm_find_session(targetsid, &s2)) != 0 ||
	    (error = dm_find_msg(s1, token, &tevp)) != 0) {
		mutex_spinunlock(&dm_session_lock, lc);
		return(error);
	}
	dm_unlink_event(tevp, &s1->sn_delq);
	if (tevp->te_flags & DM_TEF_HASHED) {
		unhash_event(s1, tevp);
		hash_it = 1;
	}
	dm_link_event(tevp, &s2->sn_delq);
	if (hash_it)
		hash_event(s2, tevp);
	mutex_spinunlock(&dm_session_lock, lc);

	if (copy_to_user(rtokenp, &token, sizeof(token)))
		return(-EFAULT);
	return(0);
}


/* ARGSUSED */
int
dm_pending(
	dm_sessid_t	sid,
	dm_token_t	token,
	dm_timestruct_t	__user *delay)		/* unused */
{
	dm_tokevent_t	*tevp;
	int		error;
	unsigned long	lc;		/* lock cookie */

	if ((error = dm_find_msg_and_lock(sid, token, &tevp, &lc)) != 0)
		return(error);

	tevp->te_flags |= DM_TEF_INTERMED;
	if (tevp->te_evt_ref > 0)	/* if event generation threads exist */
		sv_broadcast(&tevp->te_evt_queue);

	mutex_spinunlock(&tevp->te_lock, lc);
	return(0);
}


int
dm_get_events(
	dm_sessid_t	sid,
	u_int		maxmsgs,
	u_int		flags,
	size_t		buflen,
	void		__user *bufp,
	size_t		__user *rlenp)
{
	dm_session_t	*s;		/* pointer to session given by sid */
	dm_tokevent_t	*tevp;		/* next event message on queue */
	int		error;
	unsigned long	lc1;		/* first lock cookie */
	unsigned long	lc2 = 0;	/* second lock cookie */
	int		totalsize;
	int		msgsize;
	dm_eventmsg_t	__user *prevmsg;
	int		prev_msgsize = 0;
	u_int		msgcnt;

	/* Because some of the events (dm_data_event_t in particular) contain
	   __u64 fields, we need to make sure that the buffer provided by the
	   caller is aligned such that he can read those fields successfully.
	*/

	if (((unsigned long)bufp & (sizeof(__u64) - 1)) != 0)
		return(-EFAULT);

	/* Find the indicated session and lock it. */

	if ((error = dm_find_session_and_lock(sid, &s, &lc1)) != 0)
		return(error);

	/* Check for messages on sn_newq.  If there aren't any that haven't
	   already been grabbed by another process, and if we are supposed to
	   to wait until one shows up, then go to sleep interruptibly on the
	   sn_readerq semaphore.  The session can't disappear out from under
	   us as long as sn_readerq is non-zero.
	*/

	for (;;) {
		int		rc;

		for (tevp = s->sn_newq.eq_head; tevp; tevp = tevp->te_next) {
			lc2 = mutex_spinlock(&tevp->te_lock);
			if (!(tevp->te_flags & DM_TEF_LOCKED))
				break;
			mutex_spinunlock(&tevp->te_lock, lc2);
		}
		if (tevp)
			break;		/* got one! */

		if (!(flags & DM_EV_WAIT)) {
			mutex_spinunlock(&s->sn_qlock, lc1);
			return(-EAGAIN);
		}
		s->sn_readercnt++;

		sv_wait_sig(&s->sn_readerq, 1, &s->sn_qlock, lc1);
		rc = signal_pending(current);

		lc1 = mutex_spinlock(&s->sn_qlock);
		s->sn_readercnt--;
		if (rc) {	/* if signal was received */
			mutex_spinunlock(&s->sn_qlock, lc1);
			return(-EINTR);
		}
	}

	/* At least one message is available for delivery, and we have both the
	   session lock and event lock.	 Mark the event so that it is not
	   grabbed by other daemons, then drop both locks prior copying the
	   data to the caller's buffer.	 Leaving the event on the queue in a
	   marked state prevents both the session and the event from
	   disappearing out from under us while we don't have the locks.
	*/

	tevp->te_flags |= DM_TEF_LOCKED;
	mutex_spinunlock(&tevp->te_lock, lc2);	/* reverse cookie order */
	mutex_spinunlock(&s->sn_qlock, lc1);

	/* Continue to deliver messages until there are no more, the
	   user's buffer becomes full, or we hit his maxmsgs limit.
	*/

	totalsize = 0;		/* total bytes transferred to the user */
	prevmsg = NULL;
	msgcnt = 0;

	while (tevp) {
		/* Compute the number of bytes to be moved, rounding up to an
		   8-byte boundary so that any subsequent messages will also be
		   aligned.
		*/

		msgsize = tevp->te_allocsize - offsetof(dm_tokevent_t, te_msg);
		msgsize = (msgsize + sizeof(__u64) - 1) & ~(sizeof(__u64) - 1);
		totalsize += msgsize;

		/* If it fits, copy the message into the user's buffer and
		   update his 'rlenp'.	Update the _link pointer for any
		   previous message.
		*/

		if (totalsize > buflen) {	/* no more room */
			error = -E2BIG;
		} else if (put_user(totalsize, rlenp)) {
			error = -EFAULT;
		} else if (copy_to_user(bufp, &tevp->te_msg, msgsize)) {
			error = -EFAULT;
		} else if (prevmsg && put_user(prev_msgsize, &prevmsg->_link)) {
			error = -EFAULT;
		} else {
			error = 0;
		}

		/* If an error occurred, just unmark the event and leave it on
		   the queue for someone else.	Note that other daemons may
		   have gone to sleep because this event was marked, so wake
		   them up.  Also, if at least one message has already been
		   delivered, then an error here is not really an error.
		*/

		lc1 = mutex_spinlock(&s->sn_qlock);
		lc2 = mutex_spinlock(&tevp->te_lock);
		tevp->te_flags &= ~DM_TEF_LOCKED;	/* drop the mark */

		if (error) {
			if (s->sn_readercnt)
				sv_signal(&s->sn_readerq);

			mutex_spinunlock(&tevp->te_lock, lc2);	/* rev. order */
			mutex_spinunlock(&s->sn_qlock, lc1);
			if (prevmsg)
				return(0);
			if (error == -E2BIG && put_user(totalsize,rlenp))
				error = -EFAULT;
			return(error);
		}

		/* The message was successfully delivered.  Unqueue it. */

		dm_unlink_event(tevp, &s->sn_newq);

		/* Wake up the first of any processes waiting for room on the
		   sn_newq.
		*/

		if (s->sn_writercnt)
			sv_signal(&s->sn_writerq);

		/* If the message is synchronous, add it to the sn_delq while
		   still holding the lock.  If it is asynchronous, free it.
		*/

		if (tevp->te_msg.ev_token != DM_INVALID_TOKEN) { /* synch */
			dm_link_event(tevp, &s->sn_delq);
			mutex_spinunlock(&tevp->te_lock, lc2);
		} else {
			tevp->te_flags |= DM_TEF_FINAL;
			if (tevp->te_flags & DM_TEF_HASHED)
				unhash_event(s, tevp);
			mutex_spinunlock(&tevp->te_lock, lc2);
			dm_put_tevp(tevp, NULL);/* can't cause destroy events */
		}

		/* Update our notion of where we are in the user's buffer.  If
		   he doesn't want any more messages, then stop.
		*/

		prevmsg = (dm_eventmsg_t __user *)bufp;
		prev_msgsize = msgsize;
		bufp = (char __user *)bufp + msgsize;

		msgcnt++;
		if (maxmsgs && msgcnt >= maxmsgs) {
			mutex_spinunlock(&s->sn_qlock, lc1);
			break;
		}

		/* While still holding the sn_qlock,  see if any additional
		   messages are available for delivery.
		*/

		for (tevp = s->sn_newq.eq_head; tevp; tevp = tevp->te_next) {
			lc2 = mutex_spinlock(&tevp->te_lock);
			if (!(tevp->te_flags & DM_TEF_LOCKED)) {
				tevp->te_flags |= DM_TEF_LOCKED;
				mutex_spinunlock(&tevp->te_lock, lc2);
				break;
			}
			mutex_spinunlock(&tevp->te_lock, lc2);
		}
		mutex_spinunlock(&s->sn_qlock, lc1);
	}
	return(0);
}


/*
 *  Remove an event message from the delivered queue, set the returned
 *  error where the event generator wants it, and wake up the generator.
 *  Also currently have the user side release any locks it holds...
 */

/* ARGSUSED */
int
dm_respond_event(
	dm_sessid_t	sid,
	dm_token_t	token,
	dm_response_t	response,
	int		reterror,
	size_t		buflen,		/* unused */
	void		__user *respbufp) /* unused */
{
	dm_session_t	*s;		/* pointer to session given by sid */
	dm_tokevent_t	*tevp;		/* event message queue traversal */
	int		error;
	unsigned long	lc;		/* lock cookie */

	/* Sanity check the input parameters. */

	switch (response) {
	case DM_RESP_CONTINUE:	/* continue must have reterror == 0 */
		if (reterror != 0)
			return(-EINVAL);
		break;
	case DM_RESP_ABORT:	/* abort must have errno set */
		if (reterror <= 0)
			return(-EINVAL);
		break;
	case DM_RESP_DONTCARE:
		reterror = -1;	/* to distinguish DM_RESP_DONTCARE */
		break;
	default:
		return(-EINVAL);
	}

	/* Hold session lock until the event is unqueued. */

	if ((error = dm_find_session_and_lock(sid, &s, &lc)) != 0)
		return(error);

	if ((error = dm_find_msg(s, token, &tevp)) != 0) {
		mutex_spinunlock(&s->sn_qlock, lc);
		return(error);
	}
	nested_spinlock(&tevp->te_lock);

	if ((response == DM_RESP_DONTCARE) &&
	    (tevp->te_msg.ev_type != DM_EVENT_MOUNT)) {
		error = -EINVAL;
		nested_spinunlock(&tevp->te_lock);
		mutex_spinunlock(&s->sn_qlock, lc);
	} else {
		dm_unlink_event(tevp, &s->sn_delq);
		if (tevp->te_flags & DM_TEF_HASHED)
			unhash_event(s, tevp);
		tevp->te_reply = -reterror; /* linux wants negative errno */
		tevp->te_flags |= DM_TEF_FINAL;
		if (tevp->te_evt_ref)
			sv_broadcast(&tevp->te_evt_queue);
		nested_spinunlock(&tevp->te_lock);
		mutex_spinunlock(&s->sn_qlock, lc);
		error = 0;

		/* Absolutely no locks can be held when calling dm_put_tevp! */

		dm_put_tevp(tevp, NULL);  /* this can generate destroy events */
	}
	return(error);
}

/* The caller must hold sn_qlock.
   This will return the tokevent locked.
 */
static dm_tokevent_t *
__find_match_event_no_waiters_locked(
	dm_tokevent_t	*tevp1,
	dm_eventq_t	*queue)
{
	dm_tokevent_t	*tevp2, *next_tevp;
	dm_tokdata_t	*tdp1 = tevp1->te_tdp;
	dm_tokdata_t	*tdp2;
	dm_data_event_t *d_event1;
	dm_data_event_t *d_event2;

	d_event1 = (dm_data_event_t *)((char *)&tevp1->te_msg + tevp1->te_msg.ev_data.vd_offset);

	for(tevp2 = queue->eq_head; tevp2; tevp2 = next_tevp) {
		nested_spinlock(&tevp2->te_lock);
		next_tevp = tevp2->te_next;

		/* Just compare the first tdp's in each--there should
		   be just one, if it's the match we want.
		 */
		tdp2 = tevp2->te_tdp;
		if ((tevp2->te_msg.ev_type == tevp1->te_msg.ev_type) &&
		    (tevp2->te_tdp->td_type == tevp1->te_tdp->td_type) &&
		    (tevp2->te_evt_ref == 0) && (tdp2->td_next == NULL) &&
		    (memcmp(&tdp1->td_handle, &tdp2->td_handle,
			   sizeof(dm_handle_t)) == 0)) {

			d_event2 = (dm_data_event_t *)((char *)&tevp2->te_msg + tevp2->te_msg.ev_data.vd_offset);


			if ((d_event2->de_offset == d_event1->de_offset) &&
			    (d_event2->de_length == d_event1->de_length)) {
				/* Match -- return it locked */
				return tevp2;
			}
		}
		nested_spinunlock(&tevp2->te_lock);
	}
	return NULL;
}

/* The caller must hold the sn_qlock.
   The returned tokevent will be locked with nested_spinlock.
 */
static dm_tokevent_t *
find_match_event_no_waiters_locked(
	dm_session_t	*s,
	dm_tokevent_t	*tevp)
{
	dm_tokevent_t	*tevp2;

	if ((!s->sn_newq.eq_tail) && (!s->sn_delq.eq_tail))
		return NULL;
	if (!tevp->te_tdp)
		return NULL;
	if (tevp->te_tdp->td_next) {
		/* If it has multiple tdp's then don't bother trying to
		   find a match.
		 */
		return NULL;
	}
	tevp2 = __find_match_event_no_waiters_locked(tevp, &s->sn_newq);
	if (tevp2 == NULL)
		tevp2 = __find_match_event_no_waiters_locked(tevp, &s->sn_delq);
	/* returns a locked tokevent */
	return tevp2;
}



/* Queue the filled in event message pointed to by tevp on the session s, and
   (if a synchronous event) wait for the reply from the DMAPI application.
   The caller MUST be holding the session lock before calling this routine!
   The session lock is always released upon exit.
   Returns:
	 -1 == don't care
	  0 == success (or async event)
	> 0 == errno describing reason for failure
*/

static int
dm_enqueue(
	dm_session_t	*s,
	unsigned long	lc,		/* input lock cookie */
	dm_tokevent_t	**tevpp,		/* in/out parameter */
	int		sync,
	int		flags,
	int		interruptable)
{
	int		is_unmount = 0;
	int		is_hashable = 0;
	int		reply;
	dm_tokevent_t	*tevp = *tevpp;

	/* If the caller isn't planning to stick around for the result
	   and this request is identical to one that is already on the
	   queues then just give the caller an EAGAIN.	Release the
	   session lock before returning.

	   We look only at NDELAY requests with an event type of READ,
	   WRITE, or TRUNCATE on objects that are regular files.
	*/

	if ((flags & DM_FLAGS_NDELAY) && DM_EVENT_RDWRTRUNC(tevp) &&
	    (tevp->te_tdp->td_type == DM_TDT_REG)) {
		if (repeated_event(s, tevp)) {
			mutex_spinunlock(&s->sn_qlock, lc);
			return -EAGAIN;
		}
		is_hashable = 1;
	}

	/* If the caller is a sync event then look for a matching sync
	   event.  If there is a match and it doesn't currently have
	   event threads waiting on it, then we will drop our own
	   tokevent and jump on the matching event.
	 */
	if (((flags & DM_FLAGS_NDELAY) == 0) && DM_EVENT_RDWRTRUNC(tevp) &&
	    (tevp->te_tdp->td_type == DM_TDT_REG)) {
		dm_tokevent_t	*tevp2;
		if ((tevp2 = find_match_event_no_waiters_locked(s, tevp))) {
			ASSERT(tevp2->te_evt_ref == 0);
			tevp2->te_evt_ref++;
			nested_spinunlock(&tevp2->te_lock);
			nested_spinlock(&tevp->te_lock);
			tevp->te_evt_ref--;
			nested_spinunlock(&tevp->te_lock);
			mutex_spinunlock(&s->sn_qlock, lc);
			/* All locks have been released */
			dm_evt_rele_tevp(tevp, 1);
			*tevpp = tevp = tevp2;
			goto wait_on_tevp;
		}
	}

	if (tevp->te_msg.ev_type == DM_EVENT_UNMOUNT)
		is_unmount = 1;

	/* Check for room on sn_newq.  If there is no room for new messages,
	   then go to sleep on the sn_writerq semaphore.  The
	   session cannot disappear out from under us as long as sn_writercnt
	   is non-zero.
	*/

	while (s->sn_newq.eq_count >= dm_max_queued_msgs) {	/* no room */
		s->sn_writercnt++;
		dm_link_event(tevp, &s->sn_evt_writerq);
		if (interruptable) {
			sv_wait_sig(&s->sn_writerq, 1, &s->sn_qlock, lc);
			if (signal_pending(current)) {
				s->sn_writercnt--;
				return -EINTR;
			}
		} else {
			sv_wait(&s->sn_writerq, 1, &s->sn_qlock, lc);
		}
		lc = mutex_spinlock(&s->sn_qlock);
		s->sn_writercnt--;
		dm_unlink_event(tevp, &s->sn_evt_writerq);
#ifdef HAVE_DM_QUEUE_FLUSH
		/* We hold the sn_qlock, from here to after we get into
		 * the sn_newq.  Any thread going through
		 * dm_release_threads() looking for us is already past us
		 * and has set the DM_TEF_FLUSH flag for us or is blocked on
		 * sn_qlock and will find us in sn_newq after we release
		 * the sn_qlock.
		 * We check for dop->flushing anyway, in case the
		 * dm_release_threads() already completed before we
		 * could enter dmapi.
		 */
		if (!sync) {
			/* async events are forced into the newq */
			break;
		}
		if (tevp->te_flags & DM_TEF_FLUSH) {
			mutex_spinunlock(&s->sn_qlock, lc);
			return tevp->te_reply;
		}
		else {
			struct filesystem_dmapi_operations *dops;
			dm_tokdata_t	*tdp;
			int		errno = 0;

			nested_spinlock(&tevp->te_lock);
			for (tdp = tevp->te_tdp; tdp; tdp = tdp->td_next) {
				if (tdp->td_ip) {
					dops = dm_fsys_ops(tdp->td_ip->i_sb);
					ASSERT(dops);
					if (dops->flushing)
						errno = dops->flushing(tdp->td_ip);
					if (errno) {
						nested_spinunlock(&tevp->te_lock);
						mutex_spinunlock(&s->sn_qlock, lc);
						return errno;
					}
				}
			}
			nested_spinunlock(&tevp->te_lock);
		}
#endif /* HAVE_DM_QUEUE_FLUSH */
	}

	/* Assign a sequence number and token to the event and bump the
	   application reference count by one.	We don't need 'te_lock' here
	   because this thread is still the only thread that can see the event.
	*/

	nested_spinlock(&dm_token_lock);
	tevp->te_msg.ev_sequence = dm_next_sequence++;
	if (sync) {
		tevp->te_msg.ev_token = dm_next_token++;
	} else {
		tevp->te_msg.ev_token = DM_INVALID_TOKEN;
	}
	nested_spinunlock(&dm_token_lock);

	tevp->te_app_ref++;

	/* Room exists on the sn_newq queue, so add this request.  If the
	   queue was previously empty, wake up the first of any processes
	   that are waiting for an event.
	*/

	dm_link_event(tevp, &s->sn_newq);
	if (is_hashable)
		hash_event(s, tevp);

	if (s->sn_readercnt)
		sv_signal(&s->sn_readerq);

	mutex_spinunlock(&s->sn_qlock, lc);

	/* Now that the message is queued, processes issuing asynchronous
	   events or DM_EVENT_UNMOUNT events are ready to continue.
	*/

	if (!sync || is_unmount)
		return 0;

	/* Synchronous requests wait until a final reply is received.  If the
	   caller supplied the DM_FLAGS_NDELAY flag, the process will return
	   EAGAIN if dm_pending() sets DM_TEF_INTERMED.	 We also let users
	   Cntl-C out of a read, write, and truncate requests.
	*/

wait_on_tevp:
	lc = mutex_spinlock(&tevp->te_lock);

	while (!(tevp->te_flags & DM_TEF_FINAL)) {
		if ((tevp->te_flags & DM_TEF_INTERMED) &&
		    (flags & DM_FLAGS_NDELAY)) {
			mutex_spinunlock(&tevp->te_lock, lc);
			return -EAGAIN;
		}
		if (tevp->te_msg.ev_type == DM_EVENT_READ ||
		    tevp->te_msg.ev_type == DM_EVENT_WRITE ||
		    tevp->te_msg.ev_type == DM_EVENT_TRUNCATE) {
			sv_wait_sig(&tevp->te_evt_queue, 1, &tevp->te_lock, lc);
			if (signal_pending(current)){
				return -EINTR;
			}
		} else {
			sv_wait(&tevp->te_evt_queue, 1, &tevp->te_lock, lc);
		}
		lc = mutex_spinlock(&tevp->te_lock);
#ifdef HAVE_DM_QUEUE_FLUSH
		/* Did we pop out because of queue flushing? */
		if (tevp->te_flags & DM_TEF_FLUSH) {
			mutex_spinunlock(&tevp->te_lock, lc);
			return tevp->te_reply;
		}
#endif /* HAVE_DM_QUEUE_FLUSH */
	}

	/* Return both the tevp and the reply which was stored in the tevp by
	   dm_respond_event.  The tevp structure has already been removed from
	   the reply queue by this point in dm_respond_event().
	*/

	reply = tevp->te_reply;
	mutex_spinunlock(&tevp->te_lock, lc);
	return reply;
}


/* The filesystem is guaranteed to stay mounted while this event is
   outstanding.
*/

int
dm_enqueue_normal_event(
	struct super_block *sb,
	dm_tokevent_t	**tevpp,
	int		flags)
{
	dm_session_t	*s;
	int		error;
	int		sync;
	unsigned long	lc;		/* lock cookie */

	switch ((*tevpp)->te_msg.ev_type) {
	case DM_EVENT_READ:
	case DM_EVENT_WRITE:
	case DM_EVENT_TRUNCATE:
	case DM_EVENT_PREUNMOUNT:
	case DM_EVENT_UNMOUNT:
	case DM_EVENT_NOSPACE:
	case DM_EVENT_CREATE:
	case DM_EVENT_REMOVE:
	case DM_EVENT_RENAME:
	case DM_EVENT_SYMLINK:
	case DM_EVENT_LINK:
	case DM_EVENT_DEBUT:		/* not currently supported */
		sync = 1;
		break;

	case DM_EVENT_DESTROY:
	case DM_EVENT_POSTCREATE:
	case DM_EVENT_POSTREMOVE:
	case DM_EVENT_POSTRENAME:
	case DM_EVENT_POSTSYMLINK:
	case DM_EVENT_POSTLINK:
	case DM_EVENT_ATTRIBUTE:
	case DM_EVENT_CLOSE:		/* not currently supported */
	case DM_EVENT_CANCEL:		/* not currently supported */
		sync = 0;
		break;

	default:
		return(-EIO);		/* garbage event number */
	}

	/* Wait until a session selects disposition for the event.  The session
	   is locked upon return from dm_waitfor_disp_session().
	*/

	if ((error = dm_waitfor_disp_session(sb, *tevpp, &s, &lc)) != 0)
		return(error);

	return(dm_enqueue(s, lc, tevpp, sync, flags, 0));
}


/* Traverse the session list checking for sessions with the WANTMOUNT flag
   set.	 When one is found, send it the message.  Possible responses to the
   message are one of DONTCARE, CONTINUE, or ABORT.  The action taken in each
   case is:
	DONTCARE (-1)  - Send the event to the next session with WANTMOUNT set
	CONTINUE ( 0) - Proceed with the mount, errno zero.
	ABORT	 (>0) - Fail the mount, return the returned errno.

   The mount request is sent to sessions in ascending session ID order.
   Since the session list can change dramatically while this process is
   sleeping in dm_enqueue(), this routine must use session IDs rather than
   session pointers when keeping track of where it is in the list.  Since
   new sessions are always added at the end of the queue, and have increasing
   session ID values, we don't have to worry about missing any session.
*/

int
dm_enqueue_mount_event(
	struct super_block *sb,
	dm_tokevent_t	*tevp)
{
	dm_session_t	*s;
	dm_sessid_t	sid;
	int		error;
	unsigned long	lc;		/* lock cookie */

	/* Make the mounting filesystem visible to other DMAPI calls. */

	if ((error = dm_add_fsys_entry(sb, tevp)) != 0){
		return(error);
	}

	/* Walk through the session list presenting the mount event to each
	   session that is interested until a session accepts or rejects it,
	   or until all sessions ignore it.
	*/

	for (sid = DM_NO_SESSION, error = 1; error > 0; sid = s->sn_sessid) {

		lc = mutex_spinlock(&dm_session_lock);
		for (s = dm_sessions; s; s = s->sn_next) {
			if (s->sn_sessid > sid && s->sn_flags & DM_SN_WANTMOUNT) {
				nested_spinlock(&s->sn_qlock);
				nested_spinunlock(&dm_session_lock);
				break;
			}
		}
		if (s == NULL) {
			mutex_spinunlock(&dm_session_lock, lc);
			break;		/* noone wants it; proceed with mount */
		}
		error = dm_enqueue(s, lc, &tevp, 1, 0, 0);
	}

	/* If the mount will be allowed to complete, then update the fsrp entry
	   accordingly.	 If the mount is to be aborted, remove the fsrp entry.
	*/

	if (error >= 0) {
		dm_change_fsys_entry(sb, DM_STATE_MOUNTED);
		error = 0;
	} else {
		dm_remove_fsys_entry(sb);
	}
	return(error);
}

int
dm_enqueue_sendmsg_event(
	dm_sessid_t	targetsid,
	dm_tokevent_t	*tevp,
	int		sync)
{
	dm_session_t	*s;
	int		error;
	unsigned long	lc;		/* lock cookie */

	if ((error = dm_find_session_and_lock(targetsid, &s, &lc)) != 0)
		return(error);

	return(dm_enqueue(s, lc, &tevp, sync, 0, 1));
}


dm_token_t
dm_enqueue_user_event(
	dm_sessid_t	sid,
	dm_tokevent_t	*tevp,
	dm_token_t	*tokenp)
{
	dm_session_t	*s;
	int		error;
	unsigned long	lc;		/* lock cookie */

	/* Atomically find and lock the session whose session id is 'sid'. */

	if ((error = dm_find_session_and_lock(sid, &s, &lc)) != 0)
		return(error);

	/* Assign a sequence number and token to the event, bump the
	   application reference count by one, and decrement the event
	   count because the caller gives up all ownership of the event.
	   We don't need 'te_lock' here because this thread is still the
	   only thread that can see the event.
	*/

	nested_spinlock(&dm_token_lock);
	tevp->te_msg.ev_sequence = dm_next_sequence++;
	*tokenp = tevp->te_msg.ev_token = dm_next_token++;
	nested_spinunlock(&dm_token_lock);

	tevp->te_flags &= ~(DM_TEF_INTERMED|DM_TEF_FINAL);
	tevp->te_app_ref++;
	tevp->te_evt_ref--;

	/* Add the request to the tail of the sn_delq.	Now it's visible. */

	dm_link_event(tevp, &s->sn_delq);
	mutex_spinunlock(&s->sn_qlock, lc);

	return(0);
}

#ifdef HAVE_DM_QUEUE_FLUSH
/* If inode is non-null, find any tdp referencing that inode and flush the
 * thread waiting on that inode and set DM_TEF_FLUSH for that tokevent.
 * Otherwise, if inode is null, find any tdp referencing the specified fsid
 * and flush that thread and set DM_TEF_FLUSH for that tokevent.
 */
static int
dm_flush_events(
	dm_session_t	*s,
	dm_fsid_t	*fsidp,
	struct inode	*inode, /* may be null */
	dm_eventq_t	*queue,
	int		is_writerq,
	int		errno)
{
	dm_tokevent_t	*tevp, *next_tevp;
	dm_tokdata_t	*tdp;
	int		found_events = 0;

	ASSERT(fsidp);
	for (tevp = queue->eq_head; tevp; tevp = next_tevp) {
		nested_spinlock(&tevp->te_lock);
		next_tevp = tevp->te_next;

		for (tdp = tevp->te_tdp; tdp; tdp = tdp->td_next) {
			if( inode ) {
				if( tdp->td_ip == inode ) {
					break;
				}
			}
			else if(memcmp(fsidp, &tdp->td_handle.ha_fsid, sizeof(*fsidp)) == 0) {
				break;
			}
		}

		if (tdp != NULL) {
			/* found a handle reference in this event */
			++found_events;
			tevp->te_flags |= DM_TEF_FLUSH;

			/* Set the reply value, unless dm_get_events is
			   already on this one.
			 */
			if (! (tevp->te_flags & DM_TEF_LOCKED))
				tevp->te_reply = errno;

			/* If it is on the sn_evt_writerq or is being
			   used by dm_get_events then we're done with it.
			*/
			if (is_writerq || (tevp->te_flags & DM_TEF_LOCKED)) {
				nested_spinunlock(&tevp->te_lock);
				continue;
			}

			/* If there is a thread waiting on a synchronous
			   event then be like dm_respond_event.
			*/

			if ((tevp->te_evt_ref) &&
			    (tevp->te_msg.ev_token != DM_INVALID_TOKEN)) {

				tevp->te_flags |= DM_TEF_FINAL;
				dm_unlink_event(tevp, queue);
				if (tevp->te_flags & DM_TEF_HASHED)
					unhash_event(s, tevp);
				sv_broadcast(&tevp->te_evt_queue);
				nested_spinunlock(&tevp->te_lock);
				dm_put_tevp(tevp, NULL);
				continue;
			}
		}
		nested_spinunlock(&tevp->te_lock);
	}

	return(found_events);
}


/* If inode is non-null then find any threads that have a reference to that
 * inode and flush them with the specified errno.
 * Otherwise,if inode is null, then find any threads that have a reference
 * to that sb and flush them with the specified errno.
 * We look for these threads in each session's sn_evt_writerq, sn_newq,
 * and sn_delq.
 */
int
dm_release_threads(
	struct super_block	*sb,
	struct inode		*inode, /* may be null */
	int			errno)
{
	dm_sessid_t	sid;
	dm_session_t	*s;
	unsigned long	lc;
	u_int		sesscnt;
	dm_sessid_t	*sidlist;
	int		i;
	int		found_events = 0;
	dm_fsid_t	fsid;
	struct filesystem_dmapi_operations *dops;

	ASSERT(sb);
	dops = dm_fsys_ops(sb);
	ASSERT(dops);
	dops->get_fsid(sb, &fsid);
	dm_release_disp_threads(&fsid, inode, errno);

	/* Loop until we can get the right amount of temp space, being careful
	   not to hold a mutex during the allocation.  Usually only one trip.
	*/

	for (;;) {
		lc = mutex_spinlock(&dm_session_lock);
		sesscnt = dm_sessions_active;
		mutex_spinunlock(&dm_session_lock, lc);

		if (sesscnt == 0)
			return 0;

		sidlist = kmalloc(sesscnt * sizeof(sid), GFP_KERNEL);

		lc = mutex_spinlock(&dm_session_lock);
		if (sesscnt == dm_sessions_active)
			break;

		mutex_spinunlock(&dm_session_lock, lc);
		kfree(sidlist);
	}

	for (i = 0, s = dm_sessions; i < sesscnt; i++, s = s->sn_next)
		sidlist[i] = s->sn_sessid;

	mutex_spinunlock(&dm_session_lock, lc);


	for (i = 0; i < sesscnt; i++) {
		sid = sidlist[i];
		if( dm_find_session_and_lock( sid, &s, &lc ) == 0 ){
			found_events = dm_flush_events( s, &fsid, inode,
							&s->sn_evt_writerq, 1,
							errno );
			if (found_events)
				sv_broadcast(&s->sn_writerq);

			dm_flush_events(s, &fsid, inode, &s->sn_newq, 0, errno);
			dm_flush_events(s, &fsid, inode, &s->sn_delq, 0, errno);

			mutex_spinunlock( &s->sn_qlock, lc );
		}
	}
	kfree(sidlist);

	return 0;
}
#endif /* HAVE_DM_QUEUE_FLUSH */
