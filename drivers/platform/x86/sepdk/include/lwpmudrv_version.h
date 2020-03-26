/* ****************************************************************************
 *  Copyright(C) 2009-2018 Intel Corporation.  All Rights Reserved.
 *
 *  This file is part of SEP Development Kit
 *
 *  SEP Development Kit is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 *
 *  SEP Development Kit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  As a special exception, you may use this file as part of a free software
 *  library without restriction.  Specifically, if other files instantiate
 *  templates or use macros or inline functions from this file, or you
 *  compile this file and link it with other files to produce an executable
 *  this file does not by itself cause the resulting executable to be
 *  covered by the GNU General Public License.  This exception does not
 *  however invalidate any other reasons why the executable file might be
 *  covered by the GNU General Public License.
 * ****************************************************************************
 */

#ifndef _LWPMUDRV_VERSION_H_
#define _LWPMUDRV_VERSION_H_

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY_W(x) L#x
#define STRINGIFY_W(x) _STRINGIFY_W(x)

#define SEP_MAJOR_VERSION 5
#define SEP_MINOR_VERSION 0
#define SEP_UPDATE_VERSION 0
#define SEP_API_VERSION SEP_UPDATE_VERSION
#if SEP_UPDATE_VERSION > 0
#define SEP_UPDATE_STRING " Update " STRINGIFY(SEP_UPDATE_VERSION)
#else
#define SEP_UPDATE_STRING ""
#endif
#define SEP_RELEASE_STRING ""

#define EMON_MAJOR_VERSION SEP_MAJOR_VERSION
#define EMON_MINOR_VERSION SEP_MINOR_VERSION
#define EMON_PRODUCT_RELEASE_STRING SEP_UPDATE_VERSION

#if defined(SEP_ENABLE_PRIVATE_CPUS)
#define PRODUCT_TYPE "private"
#define SEP_NAME "sepint"
#define SEP_NAME_W L"sepint"
#else
#define PRODUCT_TYPE "public"
#define SEP_NAME "sep"
#define SEP_NAME_W L"sep"
#endif

#if !defined(PRODUCT_BUILDER)
#define PRODUCT_BUILDER unknown
#endif

#define TB_FILE_EXT ".tb7"
#define TB_FILE_EXT_W L".tb7"

#define SEP_PRODUCT_NAME "Sampling Enabling Product"
#define EMON_PRODUCT_NAME "EMON"

#define PRODUCT_VERSION_DATE __DATE__ " at " __TIME__

#define SEP_PRODUCT_COPYRIGHT                                                  \
	"Copyright(C) 2007-2018 Intel Corporation. All rights reserved."
#define EMON_PRODUCT_COPYRIGHT                                                 \
	"Copyright(C) 1993-2018 Intel Corporation. All rights reserved."

#define PRODUCT_DISCLAIMER                                                    \
	"Warning: This computer program is protected under U.S. and \n"       \
	"international copyright laws, and may only be used or copied in \n"  \
	"accordance with the terms of the license agreement.  Except as \n"   \
	"permitted by such license, no part of this computer program may  \n" \
	"be reproduced, stored in a retrieval  system, or transmitted  \n"    \
	"in any form or by any means without the express written consent \n"  \
	"of Intel Corporation."

#define PRODUCT_VERSION                                                        \
	STRINGIFY(SEP_MAJOR_VERSION) "." STRINGIFY(SEP_MINOR_VERSION)

#define SEP_MSG_PREFIX                                                         \
	SEP_NAME "" STRINGIFY(SEP_MAJOR_VERSION) "_" STRINGIFY(                \
		SEP_MINOR_VERSION) ":"
#define SEP_VERSION_STR                                                        \
	STRINGIFY(SEP_MAJOR_VERSION)                                           \
	"." STRINGIFY(SEP_MINOR_VERSION) "." STRINGIFY(SEP_API_VERSION)

#if defined(DRV_OS_WINDOWS)

#define SEP_DRIVER_NAME SEP_NAME "drv" STRINGIFY(SEP_MAJOR_VERSION)
#define SEP_DRIVER_NAME_W SEP_NAME_W L"drv" STRINGIFY_W(SEP_MAJOR_VERSION)
#define SEP_DEVICE_NAME SEP_DRIVER_NAME

#endif

#if defined(DRV_OS_LINUX) || defined(DRV_OS_SOLARIS) ||                        \
	defined(DRV_OS_ANDROID) || defined(DRV_OS_FREEBSD)

#define SEP_DRIVER_NAME SEP_NAME "" STRINGIFY(SEP_MAJOR_VERSION)
#define SEP_SAMPLES_NAME SEP_DRIVER_NAME "_s"
#define SEP_UNCORE_NAME SEP_DRIVER_NAME "_u"
#define SEP_SIDEBAND_NAME SEP_DRIVER_NAME "_b"
#define SEP_DEVICE_NAME "/dev/" SEP_DRIVER_NAME

#endif

#if defined(DRV_OS_MAC)

#define SEP_DRIVER_NAME SEP_NAME "" STRINGIFY(SEP_MAJOR_VERSION)
#define SEP_SAMPLES_NAME SEP_DRIVER_NAME "_s"
#define SEP_DEVICE_NAME SEP_DRIVER_NAME

#endif

#endif
