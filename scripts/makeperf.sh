#!/bin/bash

# Build kernel relative perf tool.

kernel_build=$1
if [ -z $1 ]; then
	kernel_build="/lib/modules/3.12.18-wfr+/source"
fi

if [ ! -d $kernel_build ]; then
	echo "Could not find dir: $kernel_build for kernel build"
	exit 1
fi

install_path=$2
if [ -z $2 ]; then
	install_path="/usr/perf/`uname -r`"
fi

if [ -d $install_path ]; then
	echo "Error: $install_path already exists!"
	exit 1
fi

mkdir -p $install_path
if [ $? -ne 0 ]; then
	echo "Could not create $install_path"
	exit 1
fi

cd $kernel_build
make -j 8 -C tools/perf prefix=$install_path install install-doc
if [ $? -ne 0 ]; then
	echo "Could not install"
	exit 1
fi

exit 0

