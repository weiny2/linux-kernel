#!/bin/sh

driver_repo=`dirname $0`
pushd $driver_repo > /dev/null
driver_repo=`pwd`/../..
popd > /dev/null

if [ "$1" == "" ]; then
	echo "usage: $0 <nodelist>"
	exit 1
fi

nodelist=$1
kern_ver=`uname -r`
dts=`date +%Y-%m-%d-%H%M`
output_file="$driver_repo/test/upstream/$kern_ver-$dts.output"

echo "Testing installed kernel version : $kern_ver"
echo "     test repo : $driver_repo"
echo "     nodelist : $nodelist"

pushd $driver_repo/test > /dev/null
./harness.py --nodelist=$nodelist --type=upstream | tee $output_file
popd > /dev/null

echo "Testing installed kernel version : $kern_ver"
echo "     test repo : $driver_repo"
echo "     nodelist : $nodelist"
echo "     output : $output_file"

exit 0

