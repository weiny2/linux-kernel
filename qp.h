#ifndef _QP_H
#define _QP_H
/*
 * Copyright (c) 2013, 2014 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "verbs.h"

#define QPN_MAX                 (1 << 24)
#define QPNMAP_ENTRIES          (QPN_MAX / PAGE_SIZE / BITS_PER_BYTE)

/*
 * QPN-map pages start out as NULL, they get allocated upon
 * first use and are never deallocated. This way,
 * large bitmaps are not allocated unless large numbers of QPs are used.
 */
struct qpn_map {
	void *page;
};

struct hfi_qpn_table {
	spinlock_t lock; /* protect changes in this struct */
	unsigned flags;         /* flags for QP0/1 allocated for each port */
	u32 last;               /* last QP number allocated */
	u32 nmaps;              /* size of the map table */
	u16 limit;
	u8  incr;
	/* bit map of free QP numbers other than 0/1 */
	struct qpn_map map[QPNMAP_ENTRIES];
};

struct hfi_qp_ibdev {
	u32 qp_table_size;
	u32 qp_rnd;
	struct qib_qp __rcu **qp_table;
	spinlock_t qpt_lock;
	struct hfi_qpn_table qpn_table;
};

/**
 * hfi1_lookup_qpn - return the QP with the given QPN
 * @ibp: a pointer to the IB port
 * @qpn: the QP number to look up
 *
 * The caller is responsible for decrementing the QP reference count
 * when done.
 */
struct qib_qp *hfi1_lookup_qpn(struct qib_ibport *ibp, u32 qpn);

/**
 * hfi1_error_qp - put a QP into the error state
 * @qp: the QP to put into the error state
 * @err: the receive completion error to signal if a RWQE is active
 *
 * Flushes both send and receive work queues.
 * Returns true if last WQE event should be generated.
 * The QP r_lock and s_lock should be held and interrupts disabled.
 * If we are already in error state, just return.
 */
int hfi1_error_qp(struct qib_qp *qp, enum ib_wc_status err);

/**
 * hfi1_modify_qp - modify the attributes of a queue pair
 * @ibqp: the queue pair who's attributes we're modifying
 * @attr: the new attributes
 * @attr_mask: the mask of attributes to modify
 * @udata: user data for libibverbs.so
 *
 * Returns 0 on success, otherwise returns an errno.
 */
int hfi1_modify_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		  int attr_mask, struct ib_udata *udata);

int hfi1_query_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
		 int attr_mask, struct ib_qp_init_attr *init_attr);

/**
 * hfi1_compute_aeth - compute the AETH (syndrome + MSN)
 * @qp: the queue pair to compute the AETH for
 *
 * Returns the AETH.
 */
__be32 hfi1_compute_aeth(struct qib_qp *qp);

/**
 * hfi1_create_qp - create a queue pair for a device
 * @ibpd: the protection domain who's device we create the queue pair for
 * @init_attr: the attributes of the queue pair
 * @udata: user data for libibverbs.so
 *
 * Returns the queue pair on success, otherwise returns an errno.
 *
 * Called by the ib_create_qp() core verbs function.
 */
struct ib_qp *hfi1_create_qp(struct ib_pd *ibpd,
			    struct ib_qp_init_attr *init_attr,
			    struct ib_udata *udata);
/**
 * hfi1_destroy_qp - destroy a queue pair
 * @ibqp: the queue pair to destroy
 *
 * Returns 0 on success.
 *
 * Note that this can be called while the QP is actively sending or
 * receiving!
 */
int hfi1_destroy_qp(struct ib_qp *ibqp);

/**
 * hfi1_get_credit - flush the send work queue of a QP
 * @qp: the qp who's send work queue to flush
 * @aeth: the Acknowledge Extended Transport Header
 *
 * The QP s_lock should be held.
 */
void hfi1_get_credit(struct qib_qp *qp, u32 aeth);

/**
 * qib_qp_init - allocate QP tables
 * @dev: a pointer to the qib_ibdev
 */
int qib_qp_init(struct qib_ibdev *dev);

/**
 * qib_qp_exit - free the QP related structures
 * @dev: a pointer to the qib_ibdev
 */
void qib_qp_exit(struct qib_ibdev *dev);

/**
 * qib_qp_waitup - wakeup on the indicated event
 * @qp: the QP
 * @flag: flag the qp on which the qp is stalled
 */
void qib_qp_wakeup(struct qib_qp *qp, u32 flag);

struct sdma_engine *qp_to_sdma_engine(struct qib_qp *qp, u8 sc5);

struct qp_iter;

/**
 * qp_iter_init - wakeup on the indicated event
 * @dev: the qib_ib_dev
 */
struct qp_iter *qp_iter_init(struct qib_ibdev *dev);

/**
 * qp_iter_next - wakeup on the indicated event
 * @iter: the iterator for the qp hash list
 */
int qp_iter_next(struct qp_iter *iter);

/**
 * qp_iter_next - wakeup on the indicated event
 * @s: the seq_file to emit the qp information on
 * @iter: the iterator for the qp hash list
 */
void qp_iter_print(struct seq_file *s, struct qp_iter *iter);

#endif /* _PIO_H */
