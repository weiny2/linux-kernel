#!/bin/bash
#
# this script moves patches from the master brach
# to the specific for-<branch>.
#
# A version string is created for commits
# get branch
set -e
while getops "b:" opt
	case $opt in
	b)
		branch=$OPTARG
		;;
	*)
		echo "Usage: `basename $0` -b branch tag_to_merge_name"}
		exit 1
		;;
	esac
do
done
shift $(($OPTIND - 1))
if [ -z "$branch" -o -z "$1" ]
then
	echo "Usage: `basename $0` -b branch tag_to_merge_name"}
	exit 1
fi
#wfr-driver repo
echo -e "\n1.Clone wfr-driver"
git clone ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git
cd wfr-driver
echo -e "\n2.Determine driver version string"
mversion=`make version`
if [ -z "$mversion" ]
then
	echo "Unable to determine driver version"
	exit 1
fi
echo -e "\n3.Merge specified branch/tag into for-${branch}"
git checkout -b merge-$1 $1
git checkout for-${branch}
git merge -s ours merge-$1
echo -e "\n4.Update driver version string"
git checkout for-${branch}
sed -i  "/HFI_DRIVER_VERSION_BASE/s/1.11/${mversion}/" common.h
git commit -m"Update driver version string to ${mversion}" common.h
echo -e "\n5.Dry run push to wfr-driver's for-${branch} branch"
git push --dry-run origin for-${branch}
#wfr-linux-devel repo
echo -e "\n6.Clone wfr-linux-devel with the ${branch} branch"
cd ..
git clone --branch ${branch} ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-linux-devel wfr-linux-devel-merge
cd wfr-linux-devel-merge
echo -e "\n7.Add a new remote URL pointing to the wfr-driver repo"
git remote add -f wfr-driver ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git
echo -e "\n8.Synchronize new commits from for-${branch} with to ${branch}"
git subtree pull -P drivers/infiniband/hw/hfi1 wfr-driver for-${branch}
echo -e "\n9.Dry run push to wfr-linux-devel"
git push --dry-run origin ${branch}
echo -e "\nDone!"
