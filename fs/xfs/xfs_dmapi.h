/*
 * Copyright (c) 2000-2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef __XFS_DMAPI_H__
#define __XFS_DMAPI_H__

/*	Values used to define the on-disk version of dm_attrname_t. All
 *	on-disk attribute names start with the 8-byte string "SGI_DMI_".
 *
 *      In the on-disk inode, DMAPI attribute names consist of the user-provided
 *      name with the DMATTR_PREFIXSTRING pre-pended.  This string must NEVER be
 *      changed.
 */

#define DMATTR_PREFIXLEN	8
#define DMATTR_PREFIXSTRING	"SGI_DMI_"

typedef enum {
	DM_EVENT_INVALID	= -1,
	DM_EVENT_CANCEL		= 0,		/* not supported */
	DM_EVENT_MOUNT		= 1,
	DM_EVENT_PREUNMOUNT	= 2,
	DM_EVENT_UNMOUNT	= 3,
	DM_EVENT_DEBUT		= 4,		/* not supported */
	DM_EVENT_CREATE		= 5,
	DM_EVENT_CLOSE		= 6,		/* not supported */
	DM_EVENT_POSTCREATE	= 7,
	DM_EVENT_REMOVE		= 8,
	DM_EVENT_POSTREMOVE	= 9,
	DM_EVENT_RENAME		= 10,
	DM_EVENT_POSTRENAME	= 11,
	DM_EVENT_LINK		= 12,
	DM_EVENT_POSTLINK	= 13,
	DM_EVENT_SYMLINK	= 14,
	DM_EVENT_POSTSYMLINK	= 15,
	DM_EVENT_READ		= 16,
	DM_EVENT_WRITE		= 17,
	DM_EVENT_TRUNCATE	= 18,
	DM_EVENT_ATTRIBUTE	= 19,
	DM_EVENT_DESTROY	= 20,
	DM_EVENT_NOSPACE	= 21,
	DM_EVENT_USER		= 22,
	DM_EVENT_MAX		= 23
} dm_eventtype_t;
#define HAVE_DM_EVENTTYPE_T

typedef enum {
	DM_RIGHT_NULL,
	DM_RIGHT_SHARED,
	DM_RIGHT_EXCL
} dm_right_t;
#define HAVE_DM_RIGHT_T

/* Defines for determining if an event message should be sent. */
#ifdef HAVE_DMAPI
#define	DM_EVENT_ENABLED(ip, event) ( \
	unlikely ((ip)->i_mount->m_flags & XFS_MOUNT_DMAPI) && \
		( ((ip)->i_d.di_dmevmask & (1 << event)) || \
		  ((ip)->i_mount->m_dmevmask & (1 << event)) ) \
	)
#else
#define DM_EVENT_ENABLED(ip, event)	(0)
#endif

#define DM_XFS_VALID_FS_EVENTS		( \
	(1 << DM_EVENT_PREUNMOUNT)	| \
	(1 << DM_EVENT_UNMOUNT)		| \
	(1 << DM_EVENT_NOSPACE)		| \
	(1 << DM_EVENT_DEBUT)		| \
	(1 << DM_EVENT_CREATE)		| \
	(1 << DM_EVENT_POSTCREATE)	| \
	(1 << DM_EVENT_REMOVE)		| \
	(1 << DM_EVENT_POSTREMOVE)	| \
	(1 << DM_EVENT_RENAME)		| \
	(1 << DM_EVENT_POSTRENAME)	| \
	(1 << DM_EVENT_LINK)		| \
	(1 << DM_EVENT_POSTLINK)	| \
	(1 << DM_EVENT_SYMLINK)		| \
	(1 << DM_EVENT_POSTSYMLINK)	| \
	(1 << DM_EVENT_ATTRIBUTE)	| \
	(1 << DM_EVENT_DESTROY)		)

/* Events valid in dm_set_eventlist() when called with a file handle for
   a regular file or a symlink.  These events are persistent.
*/

#define	DM_XFS_VALID_FILE_EVENTS	( \
	(1 << DM_EVENT_ATTRIBUTE)	| \
	(1 << DM_EVENT_DESTROY)		)

/* Events valid in dm_set_eventlist() when called with a file handle for
   a directory.  These events are persistent.
*/

#define	DM_XFS_VALID_DIRECTORY_EVENTS	( \
	(1 << DM_EVENT_CREATE)		| \
	(1 << DM_EVENT_POSTCREATE)	| \
	(1 << DM_EVENT_REMOVE)		| \
	(1 << DM_EVENT_POSTREMOVE)	| \
	(1 << DM_EVENT_RENAME)		| \
	(1 << DM_EVENT_POSTRENAME)	| \
	(1 << DM_EVENT_LINK)		| \
	(1 << DM_EVENT_POSTLINK)	| \
	(1 << DM_EVENT_SYMLINK)		| \
	(1 << DM_EVENT_POSTSYMLINK)	| \
	(1 << DM_EVENT_ATTRIBUTE)	| \
	(1 << DM_EVENT_DESTROY)		)

/* Events supported by the XFS filesystem. */
#define	DM_XFS_SUPPORTED_EVENTS		( \
	(1 << DM_EVENT_MOUNT)		| \
	(1 << DM_EVENT_PREUNMOUNT)	| \
	(1 << DM_EVENT_UNMOUNT)		| \
	(1 << DM_EVENT_NOSPACE)		| \
	(1 << DM_EVENT_CREATE)		| \
	(1 << DM_EVENT_POSTCREATE)	| \
	(1 << DM_EVENT_REMOVE)		| \
	(1 << DM_EVENT_POSTREMOVE)	| \
	(1 << DM_EVENT_RENAME)		| \
	(1 << DM_EVENT_POSTRENAME)	| \
	(1 << DM_EVENT_LINK)		| \
	(1 << DM_EVENT_POSTLINK)	| \
	(1 << DM_EVENT_SYMLINK)		| \
	(1 << DM_EVENT_POSTSYMLINK)	| \
	(1 << DM_EVENT_READ)		| \
	(1 << DM_EVENT_WRITE)		| \
	(1 << DM_EVENT_TRUNCATE)	| \
	(1 << DM_EVENT_ATTRIBUTE)	| \
	(1 << DM_EVENT_DESTROY)		)


/*
 *	Definitions used for the flags field on dm_send_*_event().
 */

#define DM_FLAGS_NDELAY		0x001	/* return EAGAIN after dm_pending() */
#define DM_FLAGS_UNWANTED	0x002	/* event not in fsys dm_eventset_t */
#define DM_FLAGS_IMUX		0x004	/* thread holds i_mutex */
#define DM_FLAGS_IALLOCSEM_RD	0x010	/* thread holds i_alloc_sem rd */
#define DM_FLAGS_IALLOCSEM_WR	0x020	/* thread holds i_alloc_sem wr */

/*
 *	Pull in platform specific event flags defines
 */
#include "xfs_dmapi_priv.h"

/*
 *	Macros to turn caller specified delay/block flags into
 *	dm_send_xxxx_event flag DM_FLAGS_NDELAY.
 */

#define FILP_DELAY_FLAG(filp) ((filp->f_flags&(O_NDELAY|O_NONBLOCK)) ? \
			DM_FLAGS_NDELAY : 0)
#define AT_DELAY_FLAG(f) ((f & XFS_ATTR_NONBLOCK) ? DM_FLAGS_NDELAY : 0)

/*
 * Prototypes and functions for the Data Migration subsystem.
 */

typedef int	(*xfs_send_data_t)(int, struct xfs_inode *,
			loff_t, size_t, int, int *);
typedef int	(*xfs_send_mmap_t)(struct vm_area_struct *, uint);
typedef int	(*xfs_send_destroy_t)(struct xfs_inode *, dm_right_t);
typedef int	(*xfs_send_namesp_t)(dm_eventtype_t, struct xfs_mount *,
			struct xfs_inode *, dm_right_t,
			struct xfs_inode *, dm_right_t,
			const unsigned char *, const unsigned char *,
			mode_t, int, int);
typedef int	(*xfs_send_mount_t)(struct xfs_mount *, dm_right_t,
			char *, char *);
typedef void	(*xfs_send_unmount_t)(struct xfs_mount *, struct xfs_inode *,
			dm_right_t, mode_t, int, int);

typedef struct xfs_dmops {
	xfs_send_data_t		xfs_send_data;
	xfs_send_mmap_t		xfs_send_mmap;
	xfs_send_destroy_t	xfs_send_destroy;
	xfs_send_namesp_t	xfs_send_namesp;
	xfs_send_mount_t	xfs_send_mount;
	xfs_send_unmount_t	xfs_send_unmount;
} xfs_dmops_t;

#define XFS_DMAPI_UNMOUNT_FLAGS(mp) \
	(((mp)->m_dmevmask & (1 << DM_EVENT_UNMOUNT)) ? 0 : DM_FLAGS_UNWANTED)

#define XFS_DMAPI_PREUNMOUNT_FLAGS(mp) \
	(((mp)->m_dmevmask & (1 << DM_EVENT_PREUNMOUNT)) \
		? 0 : DM_FLAGS_UNWANTED)

#define XFS_SEND_DATA(mp, ev,ip,off,len,fl,lock) \
	(*(mp)->m_dm_ops->xfs_send_data)(ev,ip,off,len,fl,lock)
#define XFS_SEND_MMAP(mp, vma,fl) \
	(*(mp)->m_dm_ops->xfs_send_mmap)(vma,fl)
#define XFS_SEND_DESTROY(mp, ip,right) \
	(*(mp)->m_dm_ops->xfs_send_destroy)(ip,right)
#define XFS_SEND_NAMESP(mp, ev,b1,r1,b2,r2,n1,n2,mode,rval,fl) \
	(*(mp)->m_dm_ops->xfs_send_namesp)(ev,NULL,b1,r1,b2,r2,n1,n2,mode,rval,fl)
#define XFS_SEND_MOUNT(mp,right,path,name) \
	(*(mp)->m_dm_ops->xfs_send_mount)(mp,right,path,name)
#define XFS_SEND_PREUNMOUNT(mp) \
do { \
	if (mp->m_flags & XFS_MOUNT_DMAPI) { \
		(*(mp)->m_dm_ops->xfs_send_namesp)(DM_EVENT_PREUNMOUNT, mp, \
			(mp)->m_rootip, DM_RIGHT_NULL, \
			(mp)->m_rootip, DM_RIGHT_NULL, \
			NULL, NULL, 0, 0, XFS_DMAPI_PREUNMOUNT_FLAGS(mp)); \
	} \
} while (0)
#define XFS_SEND_UNMOUNT(mp) \
do { \
	if (mp->m_flags & XFS_MOUNT_DMAPI) { \
		(*(mp)->m_dm_ops->xfs_send_unmount)(mp, (mp)->m_rootip, \
			DM_RIGHT_NULL, 0, 0, XFS_DMAPI_UNMOUNT_FLAGS(mp)); \
	} \
} while (0)





#endif  /* __XFS_DMAPI_H__ */
