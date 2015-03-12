#!/bin/bash

# Build script for WFR driver.
#
# Author: Dennis Dalessandro (dennis.dalessandro@intel.com)
#
# Feed in two arg, the path to the kernel build dir and your WFR sourece dir.

kernel_build=$1
if [ -z $1 ]; then
	kernel_build="/lib/modules/3.12.18-wfr+/build"
fi

fxr_src=$2
if [ -z $2 ]; then
	fxr_src=$(git rev-parse --show-toplevel)
fi

if [ ! -d $kernel_build ]; then
	echo "Could not find dir: $kernel_build for kernel build"
	exit 1
fi

if [ ! -d $fxr_src ]; then
	echo "Could not find dir: $fxr_src for WFR source dir"
	exit 1
fi

echo "Changing to $fxr_src and building against $kernel_build"
cd $fxr_src

echo "Doing clean build"
make clean
if [ $? -ne 0 ]; then
	echo "Failed to do clean build"
	exit $?
fi

if [ -e ./opa2/opa2_hfi.ko ]; then
	echo "Could not remove opa2_hfi.ko by clean"
	exit 1
fi

if [ -e ./opa_core/opa_core.ko ]; then
	echo "Could not remove opa_core.ko by clean"
	exit 1
fi

if [ -e ./user/opa2_user.ko ]; then
	echo "Could not remove opa2_user.ko by clean"
	exit 1
fi
echo "Doing build in $PWD"
KBUILD=$kernel_build
export KBUILD
make

if [ $? -ne 0 ]; then
	echo "Failed to do build"
	exit 1
fi

if [ -e ./opa2/opa2_hfi.ko ] && [ -e ./opa_core/opa_core.ko ] \
		&& [ -e ./user/opa2_user.ko ]; then
	echo " Driver Build success."
else
	echo "build failed!"
	exit 1
fi
