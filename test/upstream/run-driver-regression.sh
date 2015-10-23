#!/bin/sh

test_repo=`dirname $0`
pushd $test_repo
test_repo=`pwd`
popd

if [ "$1" == "" ]; then
	echo "usage: $0 <nodelist>"
	exit 1
fi

nodelist=$1
kern_ver=`uname -r`
driver_repo=$test_repo/..
dts=`date +%Y-%m-%d-%H%M`
output_file="$kern_ver-$dts.output"

echo "Testing installed kernel version : $kern_ver"
echo "     test repo : $test_repo"
echo "     with driver repo : $driver_repo"
echo "     nodelist : $nodelist"

pushd $driver_repo

if [ -f hfi1.ko ]; then
	rm -f hfi1.ko
fi
ln -s /lib/modules/$kern_ver/kernel/drivers/staging/rdma/hfi1/hfi1.ko

pushd $driver_repo/test

./harness.py --nodelist=$nodelist --type=quick | tee $test_repo/$output_file

popd
popd

echo "Testing installed kernel version : $kern_ver"
echo "     with driver repo : $driver_repo"
echo "     nodelist : $nodelist"
echo "     output : $test_repo/$output_file"

exit 0

