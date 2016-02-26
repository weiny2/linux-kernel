#!/bin/bash


function print_help
{
	echo "Usage: `basename $0` -k kernel_src -m [hfi1|qib|simics] -t [quick|loadonly] -o [nocopy|nobuild] -s simics_host"
	echo "required:"
	echo "		k: path to kernel source"
	echo "		m: hfi1 or qib?"
	echo "optional:"
	echo "		t: quick or loadonly"
	echo "          s: run on this host for simics"
	echo "		o: skip copying for simics, or do not build, nocopy implies nobuild"
	exit 1
}

function lock
{
	$lock_dir/lock wait $lock
	if [[ $? -ne 0 ]]; then
		echo "Could not get lock!"
		exit 1
	fi
}

function unlock
{
	$lock_dir/lock drop $lock
	if [[ $? -ne 0 ]]; then
		echo "Could not release lock!"
		exit 1
	fi
}

while getopts "k:s:m:t:o:" opt
do
        case $opt in
        k)
                kernel_build=$OPTARG
                ;;
	s)
		simics_host=$OPTARG	
		;;
	m)
		module=$OPTARG
		;;
	t)
		test_type=$OPTARG
		;;
	o)
		build_opt=$OPTARG
		;;
        *)
                print_help
                ;;
        esac
done

#shift $(($OPTIND - 1)) #skip to the end so $1 and $2 are our hostnames

test_dir=`pwd`

if [[ -z $kernel_build ]]; then
	echo "Need to define -k"
	print_help
fi

if [[ $build_opt != "nobuild" ]]; then
	cd tests
	./tree_build.sh -k $kernel_build -m full
	if [[ $? -ne 0 ]]; then
		echo "Build failed"
		exit 1
	fi
	cd ..
fi

# Choose which hosts to use. right now only one qib and one hfi test bed are
# supported. If we add more we will need a more complicated scheme to chose
# which setup to run on. Maybe keep a count of tests in queue or something and
# round robin across, or just increment a running counter of tests run and
# choose the lower one.
if [[ $module == "qib" ]]; then
	host1="knc-03"
	host2="knc-04"
elif [[ $module == "hfi1" ]]; then
	host1="knc-09"
	host2="knc-10"
fi

lock_dir=/nfs/sc/disks/fabric_work/test_locks
lock=${host1}_${host2}

# wait on the test bed to be ready if its valid
if [[ $module != "simics" ]]; then
	if [[ -f $lock_dir/resources/$lock ]]; then
		echo "Found lock $lock"
	else
		echo "Did not find valid lock: $lock"
		print_help
	fi
fi

if [[ $module == "qib" ]]; then
	echo "test is $test_type"
	if [[ $test_type == "quick" ]]; then
		echo "quick not availble for qib"
		print_help
	elif [[ $test_type == "loadonly" ]]; then
		test_args="--sm remote --qib"
	else
		test_args=" --sm remote --type qib --qib"
	fi
	lock #do not lock before we check for quick test error
elif [[ $module == "hfi1" ]]; then
	lock
	if [[ $test_type == "quick" ]]; then

		test_args="--type quick"
		curdir=`pwd`
		echo "Loading driver on $host1,$host2"
		cd $test_dir/tests
		./LoadModule.py --nodelist $host1,$host2  --linuxsrc $kernel_build
		if [[ $? -ne 0 ]]; then
			echo "Driver Load Failed!"
			unlock
			exit 1
		fi
		cd $curdir
	elif [[ $test_type == "loadonly" ]]; then
		test_args=""
	else
		test_args="--type default"
	fi

elif [[ $module == "simics" ]]; then
	echo "Simics busted, nothing to do."
	exit 1
else
	echo "need to specify qib, hfi1, or simics"
	print_help
fi

if [[ $test_type == "loadonly" ]]; then
	echo "Loading driver on $host1,$host2"
	cd $test_dir/tests
	./LoadModule.py --nodelist $host1,$host2  --linuxsrc $kernel_build $test_args 
	exit $? #leave the lock held
fi

echo "Running tests on $host1,$host2"
cd $test_dir
./harness.py --nodelist $host1,$host2  --linuxsrc $kernel_build $test_args
ret=$?
unlock
exit $ret

