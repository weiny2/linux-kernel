/* SPDX-License-Identifier: Apache-2.0 */
/*
 *  Copyright 2010-2019, Intel Corporation
 */
#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#else
#define __declspec(x)
#endif /* _WIN32 */

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _TYPEDEFS_H
