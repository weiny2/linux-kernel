#!/bin/bash

# Build script for WFR driver.
#
# Author: Dennis Dalessandro (dennis.dalessandro@intel.com)
#
# Feed in six args, the path to the kernel build dir, your WFR source dir, sparse flag,
# checkpatch flag, the path to the kernel dir for coccinelle, and klocwork flag.

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

kernel_cocci=$5
# For RHEL 6 aka Santiago we disable cocci check
grep Santiago /etc/redhat-release
if [[ $? -eq 0 ]]; then
	echo "RHEL 6 detected. Skipping cocci check"
	kernel_cocci=""
fi

klocwork=$6
if [ -z $6 ]; then
	klocwork=0
fi

smatch=$7
if [ -z $7 ]; then
	smatch=0
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
	scripts/checkpatch.pl -F --no-tree --strict *.[ch] | scripts/build_checkpatch_whitelist.sh > checkpatch.current
	if ! diff -c test/tests/checkpatch.whitelist checkpatch.current; then
		echo "Failed checkpatch whitelist comparison"
	else
		echo "Checkpatch test passed"
	fi
	rm -f checkpatch.current
fi

if [ ! -z $kernel_cocci ];then
	if [ -d $kernel_cocci ]; then
		cd $kernel_cocci
		make coccicheck MODE=report M=$wfr_src > $wfr_src/coccinelle.current
		cd $wfr_src
		if ! diff -c test/tests/coccinelle.whitelist coccinelle.current; then
			echo "Failed coccinelle whitelist comparison"
		fi
		rm -f coccinelle.current
	else
		echo "Failed to do coccicheck. Could not find dir: $kernel_cocci"
	fi
fi

if [ $klocwork -eq 1 ];then
	cd $wfr_src
	rm -f klocwork.current
	if [ ! -d "$PWD/.kwlp" ]; then
		kwcheck create --license-host kwlic.intel.com --license-port 7500
		if [ $? -ne 0 ]; then
			echo "Failed to do kwcheck create"
			exit 1
		fi
	fi
	rm -rf ./.kwlp/workingcache/*
	kwshell make
	if [ $? -ne 0 ]; then
		echo "Failed to do kwshell make"
		exit 1
	fi
	kwcheck run
	if [ $? -ne 0 ]; then
		echo "Failed to do kwcheck run"
		exit 1
	fi
	kwcheck list -F detailed > ./test/tests/klocwork.current
	exit
fi

if [ $smatch -eq 1 ];then
	cd $wfr_src
	make CHECK="/nfs/site/proj/fabric/files/tools/smatch/smatch --full-path" CC=/nfs/site/proj/fabric/files/tools/smatch/cgcc
	exit 0
fi

echo "Doing build in $PWD"
if [ $sparse -eq 1 ]; then
	make C=2 CF="-D__CHECK_ENDIAN__"
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


