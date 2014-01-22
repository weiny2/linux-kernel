#
# Copyright (c) 2013, Intel Corporation.  All rights reserved.
#
# common/Makefile - Makefile for running checkpatch on the common code
#

ROOT_DIR=$(abspath ../..)
FILES=$(wildcard *.c)
FILES+=$(wildcard *.h)

checkpatch:
	$(foreach file,$(FILES),cd $(ROOT_DIR)/tools; ./checkpatch.pl $(abspath $(file));)

.PHONY : checkpatch