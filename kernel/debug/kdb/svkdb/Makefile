#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (c) 1999-2004 Silicon Graphics, Inc.  All Rights Reserved.
#

CCVERSION       := $(shell $(CC) -v 2>&1 | sed -ne '$$p')

obj-$(CONFIG_KGDB_KDB)	:= kdb_id.o kdba_id.o svos_kdb.o

ifeq ($(CONFIG_X86_64),y)
obj-$(CONFIG_KGDB_KDB) += x86_64-dis.o x86_64-kdbasupport.o
endif
ifeq ($(CONFIG_X86_32),y)
obj-$(CONFIG_KGDB_KDB) += i386-dis.o i386-kdbasupport.o
endif

override CFLAGS := $(CFLAGS:%-pg=% )

