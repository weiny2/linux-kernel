#!/bin/bash
#
# this script moves patches from the master brach
# to the specific <branch>.
#
# A version string is created for commits
# get branch
set -e
while getopts "b:" opt
do
	case $opt in
	b)
		branch=$OPTARG
		;;
	*)
		echo "Usage: `basename $0` -b branch"}
		exit 1
		;;
	esac
done
shift $(($OPTIND - 1))
if [ -z "$branch" ]
then
	echo "Usage: `basename $0` -b branch"}
	exit 1
fi
#wfr-linux-devel repo
echo -e "\n6.Clone wfr-linux-devel with the ${branch} branch"
rm -rf wfr-linux-devel-merge
git clone --branch ${branch} ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-linux-devel wfr-linux-devel-merge
cd wfr-linux-devel-merge
echo -e "\n7.Add a new remote URL pointing to the wfr-driver repo"
git remote add -f wfr-driver ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git
echo -e "\n8.Synchronize new commits from ${branch} with to ${branch}"
git subtree pull -P drivers/infiniband/hw/hfi1 wfr-driver ${branch}
echo -e "\n9.Dry run push to wfr-linux-devel"
git push --dry-run origin ${branch}
echo -e "\nDone!"
