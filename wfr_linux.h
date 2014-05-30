#ifndef _WFR_LINUX_H
#define _WFR_LINUX_H
/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
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
 * This header file is for STL-specific definitions which are
 * required by the WFR driver, and which aren't yet in the linux
 * IB core. We'll collect these all here, then merge them into
 * the kernel when that's convenient.
 */

/* Unless CONFIG_STL_MGMT is defined, this file is empty */
#ifdef CONFIG_STL_MGMT

/* STL SMA attribute IDs */
#define STL_ATTRIB_ID_CONGESTION_INFO		cpu_to_be16(0x008b)
#define STL_ATTRIB_ID_HFI_CONGESTION_SETTING	cpu_to_be16(0x0090)
#define STL_ATTRIB_ID_CONGESTION_CONTROL_TABLE	cpu_to_be16(0x0091)

/* STL PMA attribute IDs */
#define STL_PM_ATTRIB_ID_PORT_STATUS		cpu_to_be16(0x0040)
#define STL_PM_ATTRIB_ID_CLEAR_PORT_STATUS	cpu_to_be16(0x0041)

/* STL status codes */
#define STL_PM_STATUS_REQUEST_TOO_LARGE		cpu_to_be16(0x100)

#endif /* CONFIG_STL_MGMT */
#endif /* _WFR_LINUX_H */
