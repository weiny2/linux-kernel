#!/bin/bash

# Simple build script in case you forget the command.

# Takes a path to the root of your kernel tree

sparse=0
kw=0
cocci=0
cur_dir=`pwd`/static

function print_help
{
	echo "Usage: `basename $0` -k kernel_src -m [hfi1|rdmavt|qib|clean|full] -t [cocci|kw|sparse]"
	echo "required: -k, -m"
	echo "optional: -t"
	exit 1
}


while getopts "k:m:t:" opt
do
        case $opt in
        k)
                linux_src=$OPTARG
                ;;
	m)
		module=$OPTARG
		;;
	t)
		build_type=$OPTARG
		;;
        *)
                print_help
                ;;
        esac
done

if [[ -d $linux_src ]]; then
	echo "Build dir OK"
else
	echo "Could not find build dir!"
	print_help
fi

if [[ -z "$KW_PATH" ]]; then
	echo "KW_PATH not defined using default"
	KW_PATH="/nfs/sc/disks/fabric_work/klocwork/bin/"
fi

function do_build
{
	# Already in the linux kernel source tree

	dir=$1
	comp=$2
	echo "Building $comp in $dir"
	if [[ $sparse -ne 0 ]]; then
		echo "Doing sparse build"
		make SUBDIRS=$dir/$comp clean
		make C=2 CF="-D__CHECK_ENDIAN__" SUBDIRS=$dir/$comp 2>&1 | tee $cur_dir/${comp}.sparse
		if [[ $fail -ne 0 ]]; then
			echo "XXXXXXXXXXXXXX"
			echo "ERROR IN BUILD"
			echo "XXXXXXXXXXXXXX"
			exit 1
		fi

		# Count total number of warnings + errors in sparse whitelist
		count1="$(awk -F \| '{ total += $1 } END { print total }' $cur_dir/${comp}_sparse.whitelist)"
		# Count total number of warnings + errors in current build log
		count2="$(grep -v "make" $cur_dir/${comp}.sparse | grep -c "error\|warning" -)"
		if [[ $count1 -ne $count2 ]]; then
			echo "Sparse check failed found $count2 expected $count1!"
			echo "XXXXXXXXXXXXXXXXXXXXXXXX"
			echo "ERROR IN SPARSE CHECKING"
			echo "XXXXXXXXXXXXXXXXXXXXXXXX"
			exit 1
		fi

		echo "Build Passes Sparse Check"

		return
	fi

	if [[ $cocci -ne 0 ]]; then
		echo "Doing cocci check"
		fail=0
		make coccicheck MODE=report M=$dir/$comp | tee $cur_dir/${comp}.cocci
		c_err=`grep -c ".c:" $cur_dir/${comp}.cocci`
		h_err=`grep -c ".c:" $cur_dir/${comp}.cocci`
		wc_err=`grep -c ".c:" $cur_dir/${comp}_cocci.whitelist`
		wh_err=`grep -c ".c:" $cur_dir/${comp}_cocci.whitelist`
		if [[ c_err -ne wc_err ]]; then
			echo "Found $c_err errors in C files expecting $wc_err"
			fail=1
		fi
		if [[ h_err -ne wh_err ]]; then
			echo "Found $h_err errors in H files expecting $wh_err"
			fail=1
		fi

		if [[ $fail -ne 0 ]]; then
			echo "XXXXXXXXXXXXXXXXXX"
			echo "Failed Cocci Check"
			echo "XXXXXXXXXXXXXXXXXX"
			exit 1
		fi
		echo "Build Passes Cocci Check"
		return
	fi

	if [[ $kw -ne 0 ]]; then
		echo "Doing klocworks build"
		if [[ -d .kwlp ]]; then
			echo "Dir already inited for kw, continuing"
			rm -rf ./.kwlp/workingcache/*
			rm -f ./.kwlp/buildspec.txt
			rm -f ./.kwlp/buildspec.vars\#lock_file.lock
			make SUBDIRS=$dir/$comp clean
			$KW_PATH/kwshell make SUBDIRS=$dir/$comp
			$KW_PATH/kwcheck run
			$KW_PATH/kwcheck list -F detailed > $cur_dir/${comp}.kw

			# kw check logic
			cur_log=$cur_dir/${comp}.kw
			echo "Checking log: $cur_log"
			cur_total=`grep "Total Issue" $cur_log | awk '{print $1}'`
			cur_crit=`grep -c "1:Critical" $cur_log`
			cur_err=`grep -c "2:Error" $cur_log`
			cur_rev=`grep -c "4:Review" $cur_log`
			echo "Total issues found in scan: $cur_total"
			echo "    Critical: $cur_crit"
			echo "    Error: $cur_err"
			echo "    Review: $cur_rev"
			echo "Checking whitelist"
			cur_log=$cur_dir/${comp}_kw.whitelist
			echo "Checking log: $cur_log"
			wl_total=`grep "Total Issue" $cur_log | awk '{print $1}'`
			wl_crit=`grep -c "1:Critical" $cur_log`
			wl_err=`grep -c "2:Error" $cur_log`
			wl_rev=`grep -c "4:Review" $cur_log`
			fail=0
			if [[ $wl_total -ne $cur_total ]]; then
				echo "Error in total issues found"
				fail=1
			fi

			if [[ $wl_crit -ne $cur_crit ]]; then
				echo "Error in sev1 issues found"
				fail=1
			fi

			if [[ $wl_err -ne $cur_err ]]; then
				echo "Error in sev2 issues found"
				fail=1
			fi

			if [[ $wl_rev -ne $cur_rev ]]; then
				echo "Error in sev4 issues found"
				fail=1
			fi

			if [[ $fail -ne 0 ]]; then
				echo "XXXXXXXXXXXXXXX"
				echo "Failed KW check"
				echo "XXXXXXXXXXXXXXX"
				exit 1
			fi

			echo "KW Check With Whitelist Passes"
		else
			echo "kwcheck has not been run here before please initialize"
			echo "See README"
			exit 1
		fi
		return
	fi

	echo "Skipping clean build. Re-run with \"full\" in case of symbol errors"

	make -j `nproc` SUBDIRS=$dir/$comp
	if [[ $? -ne 0 ]]; then
		echo "XXXXXXXXXXXXXX"
		echo "ERROR IN BUILD"
		echo "XXXXXXXXXXXXXX"
		exit 1
	fi
}

HFI_D="drivers/staging/rdma"
HFI_C="hfi1"

QIB_D="drivers/infiniband/hw"
QIB_C="qib"

RVT_D="drivers/infiniband/sw"
RVT_C="rdmavt"

cd $linux_src

if [[ $build_type == "cocci" ]]; then
	cocci=1
fi

if [[ $build_type == "sparse" ]]; then
	sparse=1
fi

if [[ $build_type == "kw" ]]; then
	kw=1
fi

if [[ -z $module ]]; then
	print_help
fi

if [[ $module == "full" ]]; then
	echo "Cleaning hfi1, qib, and rdmavt before full build"
	make SUBDIRS=$RVT_D/$RVT_C clean
	make SUBDIRS=$HFI_D/$HFI_C clean
	make SUBDIRS=$QIB_D/$QIB_C clean
	echo "Doing full kernel build "
	make -j `nproc`
	exit $?
fi

if [[ $module == "qib" ]]; then
	echo "Building just qib"
	do_build $QIB_D $QIB_C
	exit $?
fi

if [[ $module == "hfi1" ]]; then
	echo "Building just hfi1"
	do_build $HFI_D $HFI_C
	exit $?
fi

if [[ $module == "rdmavt" ]]; then
	echo "Building just rdmavt"
	do_build $RVT_D $RVT_C
	exit $?
fi

if [[ $module == "clean" ]]; then
	echo "Doing clean build"

	make clean && make -j `nproc`
	if [[ $? -ne 0 ]]; then
		echo "Failed clean kernel build"
		exit 1
	fi

	echo "Finished clean build successfully"
	exit 0
fi

echo "No valid targets to build"
exit 1


