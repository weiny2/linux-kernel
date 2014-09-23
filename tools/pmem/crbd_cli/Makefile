#
# Copyright (c) 2013, Intel Corporation.  All rights reserved.
#
# common/test/makefile - Makefile for Common code unit tests
#

# ---- BUILD ENVIRONMENT ---------------------------------------------------------------------------

ROOT_DIR=$(abspath ../../..)
RM=rm -f
CC=gcc
CFLAGS=-m64 -Wall -Werror -Wfatal-errors

LIBS= ./CUnit/libcunit.la
INCS=-I./CUnit/Headers \
		-I./include/

# ---- FILES ---------------------------------------------------------------------------------------
OBJS=$(wildcard *.c)

TARGETNAME=crbd_cli
TARGET=$(TARGETNAME)

# ---- RECIPES -------------------------------------------------------------------------------------

all : $(TARGET) 

$(TARGET): 
	$(CC) $(CFLAGS) $(OBJS) $(INCS) $(LIBS) -o $@
	
clean:
	$(RM) crbd_cli 
