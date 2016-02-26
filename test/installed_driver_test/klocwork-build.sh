#!/bin/sh

export PATH="/nfs/sc/disks/fabric_work/klocwork/bin/:$PATH";

test_repo=`dirname $0`
pushd $test_repo > /dev/null
test_repo=`pwd`
popd > /dev/null

if [ "$1" == "" ]; then
	echo "usage: $0 <kern_src>"
	exit 1
fi

kern_src=$1
dts=`date +%Y-%m-%d-%H%M`
output_file="$kern_src-$dts.output"

echo "Checking kernel src : $kern_src"

pushd $kern_src

##
# Prepare the kernel tree
#
cp $test_repo/driver-config ./.config
make olddefconfig
make modules_prepare

make M=drivers/infiniband clean
make M=drivers/staging/rdma clean

##
# Prepare klockwork
#
rm -f klocwork.current
if [ ! -d "$PWD/.kwlp" ]; then
	kwcheck create --license-host kwlic.intel.com --license-port 7500
	if [ $? -ne 0 ]; then
		echo "Failed to do kwcheck create"
		exit 1
	fi
fi
rm -rf $PWD/.kwlp/workingcache/*
kwshell make M=drivers/infiniband
kwshell make M=drivers/staging/rdma
if [ $? -ne 0 ]; then
	echo "Failed to do kwshell make"
	exit 1
fi
kwcheck run
if [ $? -ne 0 ]; then
	echo "Failed to do kwcheck run"
	exit 1
fi
kwcheck list -F detailed > $test_repo/klocwork.current

popd

echo "Testing installed kernel version : $kern_ver"
echo "     with driver repo : $driver_repo"
echo "     nodelist : $nodelist"
echo "     output : $test_repo/$output_file"

exit 0

