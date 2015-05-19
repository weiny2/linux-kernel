#!/bin/bash
set -e
#
# This script moves the driver into the wfr-linux-devel tree
#
set -e
while getopts "b:w:" opt; do
	case $opt in
	b)
		branch=$OPTARG
		;;
	w)
		driver_branch=$OPTARG
		;;
	*)
		echo "Usage: `basename $0` -b branch -w driver_branch patchfile configname"}
		exit 1
		;;
	esac
done
shift $(($OPTIND - 1))
if [ -z "$branch" -o -z "$1" -o -z "$2" ]
then
	echo "Usage: `basename $0` -b branch -w driver_branch patchfile configname"}
	exit 1
fi
echo -e "\n1. Clone wfr-linux-devel with the ${branch} branch"
git clone --branch ${branch} ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-linux-devel wfr-linux-devel-merge
cd wfr-linux-devel-merge
echo -e "\n2. Add a new remote URL pointing to the wfr-driver repo."
git remote add -f wfr-driver ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git
echo -e "\n3. Subtree merge the ${driver_branch} driver into the drivers/infiniband/hw/hfi1 directory"
git subtree add -P drivers/infiniband/hw/hfi1/ wfr-driver/${driver_branch}
echo -e "\n4. Apply patch $1"
patch -p1 < ../$1
git commit -a -m"Add driver to kbuild"
echo -e "\n5. Test Build kernel"
cp arch/x86/configs/$2 .config
make oldconfig
make -j 8 && make -j 8 modules
echo -e "\n6.Dry run push to wfr-linux-devel"
git push --dry-run origin ${branch}
echo -e "\nDone!"
