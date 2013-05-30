#ifndef _WFR_H
#define _WFR_H
/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
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

/*
 * This file contains all of the defines that is specific to the WFR chip
 */

/* sizes */
#define WFR_CCE_NUM_INT_CSRS 8
#define WFR_CCE_NUM_MSIX_VECTORS 256
#define WFR_CCE_NUM_INT_MAP_CSRS 64
#define WFR_NUM_INTERRUPT_SOURCES 512
#define WFR_TXE_NUM_SDMA_ENGINES 16
#define WFR_RXE_NUM_CONTEXTS 160	/* will change */
#define WFR_TXE_NUM_CONTEXTS 160	/* will change */

/* block offsets */
#define WFR_CCE_OFFSET 0x0000000
#define WFR_RXE_OFFSET 0x1000000
#define WFR_TXE_OFFSET 0x1800000

/* registers */
#define WFR_CCE_REVISION	(WFR_CCE_OFFSET + 0x000000)
#define WFR_CCE_INT_STATUS	(WFR_CCE_OFFSET + 0x000100)
#define WFR_CCE_INT_MASK	(WFR_CCE_OFFSET + 0x000200)
#define WFR_CCE_INT_CLEAR	(WFR_CCE_OFFSET + 0x000300)
#define WFR_CCE_INT_MAP		(WFR_CCE_OFFSET + 0x000600)

#define WFR_RXE_RCVCONTEXTS	(WFR_RXE_OFFSET + 0x000010)

#define WFR_TXE_SENDCONTEXTS	(WFR_TXE_OFFSET + 0x000010)
#define WFR_TXE_SENDDMAENGINES	(WFR_TXE_OFFSET + 0x000018)

/* register accessors */
/* CCE Revision field accessors */
#define WFR_CCE_REVISION_MINOR_SHIFT	0
#define WFR_CCE_REVISION_MINOR_MASK	0xffull
#define WFR_CCE_REVISION_MAJOR_SHIFT	8
#define WFR_CCE_REVISION_MAJOR_MASK	0xffull
#define WFR_CCE_REVISION_BOARDID_SHIFT	32
#define WFR_CCE_REVISION_BOARDID_MASK	0xffull

/* interrupt source details */
#define WFR_IS_RCVAVAILINT_START 128
#define WFR_IS_SDMAINT_START	 448

#endif /* _WFR_H */
