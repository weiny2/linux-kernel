#!/bin/bash
#
# this script moves patches from the master branch
# to the specific for-<branch> in the driver repo
#
# A version string is created for commits
# from the source branch unless the -v is
# used to override.
#
set -e
while getopts "b:v:" opt
do
	case $opt in
	v)
		mversion=$OPTARG
		;;
	b)
		branch=$OPTARG
		;;
	*)
		echo "Usage: `basename $0` [ -v version ] -b branch tag_to_merge_name"}
		exit 1
		;;
	esac
done
shift $(($OPTIND - 1))
if [ -z "$branch" -o -z "$1" ]
then
	echo "Usage: `basename $0` [ -v version ] -b branch tag_to_merge_name"}
	exit 1
fi
#wfr-driver repo
echo -e "\n1.Clone wfr-driver"
rm -rf wfr-driver-merge
git clone ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git wfr-driver-merge
cd wfr-driver-merge
dir=$PWD
git checkout -b branch-$1 $1
echo -e "\n2.Determine driver version string"
if [ -z "$mversion" ]
then
	mversion=`make version`
	if [ -z "$mversion" ]
	then
		echo "Unable to determine driver version"
		exit 1
	fi
fi
echo "Version: $mversion"
git checkout ${branch}
echo -e "\n3.Update driver version string to $mversion"
sed -i "/^#define .*_DRIVER_VERSION_BASE/s/\".*\"/\"$mversion\"/" common.h
git commit -m"Update driver version string to ${mversion}" common.h
echo -e "\n4.Merge $1 tag into ${branch}"
if ! git merge --log -m"Merge driver from ${mversion}" branch-$1
then
	# allow for manual conflict resolution
	#
	# This is usually just the makefile
	cd $dir
	/bin/sh
fi
