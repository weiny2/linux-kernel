#!/bin/bash

function print_help
{
	echo "Usage: `basename $0` -k kernel_src"
	echo "required: -k"
	exit 1
}


while getopts "k:b:t:" opt
do
        case $opt in
        k)
                linux_src=$OPTARG
                ;;
        *)
                print_help
                ;;
        esac
done


curdir=`pwd`

function basic_build
 {
 	echo "-------------------------"
	echo "Running basic build tests"
 	echo "-------------------------"
	./tree_build.sh -k $linux_src -m full
	if [[ $? -ne 0 ]]; then
		echo "Basic build failed"
		exit 1
	fi
}

function special_build
{
	comp=$1
	build=$2
	echo "---------------------"
	echo "Doing $build of $comp"
	echo "---------------------"
	./tree_build.sh -k $linux_src -m $comp -t $build
	if [[ $? -ne 0 ]]; then
		echo "BUILD_FAILURE: $comp $build" >> .build_fail
	fi
}

if [[ -z "$linux_src" ]]; then
	print_help
fi

logfile=$curdir/check_log.`date +%s`

cd $linux_src
if [[ $? -ne 0 ]]; then
	echo "Could not chdir to $linux_src"
	exit 1
fi

echo "Logging results to $logfile"

exec > >(tee $logfile)
exec 2>&1

echo "Listing patches in series, not all may be applied though"
patch_count=0
for i in `stg series | awk '{print $2}'`; do
	stg show $i > .stgshow
	if [[ $? -ne 0 ]]; then
		echo "Could not get change"
		exit 1
	fi
	patch_count=$((patch_count+1))
	head -n 5 .stgshow
	rm .stgshow
	echo "----------------------------------------------------------------------"
done

echo "Listing applied and not applied:"
stg series

cd $curdir

rm -f .build_fail
touch .build_fail
#basic_build

special_build rdmavt sparse
special_build hfi1 sparse
special_build qib sparse

special_build rdmavt kw
special_build hfi1 kw
special_build qib kw

#special_build rdmavt cocci
#special_build hfi1 cocci
#special_build qib cocci

special_build rdmavt kedr
special_build hfi1 kedr
special_build qib kedr

special_build rdmavt smatch
special_build hfi1 smatch
special_build hfi1 smatch

grep BUILD_FAILURE .build_fail
if [[ $? -eq 0 ]]; then
	echo "One or more tests failed!"
	exit 1
fi

echo "All tests passed. Ok to submit"
exit 0
