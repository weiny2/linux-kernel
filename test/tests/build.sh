#!/bin/bash

# Build script for WFR driver.
#
# Author: Dennis Dalessandro (dennis.dalessandro@intel.com)
#
# Feed in three args, the path to the kernel build dir, your WFR sourece dir and sparse flag

kernel_build=$1
if [ -z $1 ]; then
	kernel_build="/lib/modules/3.12.18-wfr+/build"
fi

wfr_src=$2
if [ -z $2 ]; then
	wfr_src=$(git rev-parse --show-toplevel)
fi

sparse=$3
if [ -z $3 ]; then
	sparse=0
fi

checkpatch=$4
if [ -z $4 ]; then
	checkpatch=0
fi

if [ ! -d $kernel_build ]; then
	echo "Could not find dir: $kernel_build for kernel build"
	exit 1
fi

if [ ! -d $wfr_src ]; then
	echo "Could not find dir: $wfr_src for WFR source dir"
	exit 1
fi

echo "Changing to $wfr_src and building against $kernel_build"
cd $wfr_src

echo "Doing clean build"
KBUILD=$kernel_build 
export KBUILD
make clean
if [ $? -ne 0 ]; then
	echo "Failed to do clean build"
	exit $?
fi

if [ -e hfi1.ko ]; then
	echo "Could not remove hfi.ko by clean"
	exit 1
fi

if [ $checkpatch -eq 1 ];then
	cd $wfr_src
	scripts/checkpatch.pl -F --no-tree *.[ch] | scripts/build_checkpatch_whitelist.sh > checkpatch.current
	if ! diff -c test/tests/checkpatch.whitelist checkpatch.current; then
		echo "Failed checkpatch whitelist comparison"
	fi
	rm -f checkpatch.current
fi

echo "Doing build in $PWD"
if [ $sparse -eq 1 ]; then
	make C=2
else
	make
fi

if [ $? -ne 0 ]; then
	echo "Failed to do build"
	exit 1
fi

if [ -e hfi1.ko ]; then
	echo "Build sucess."
	exit 0
else
	echo "Could not find hfi1.ko failed!"
	exit 1
fi


