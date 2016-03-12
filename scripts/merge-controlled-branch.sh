#!/bin/sh
#

getbranches()
{
	git branch -r | grep "$sourcebranch$" | grep wfr-ifs | grep -v ddn | xargs basename -a
}

sourcebranch=10_0-branch
while getopts "s:" opt
do
        case $opt in
        s)
		sourcebranch=$OPTARG
                ;;
        *)
                echo "Usage: `basename $0` [ -s sourcebranch ]"
                exit 1
                ;;
        esac
done


rm -rf wfr-driver-controlled-merge
git clone --branch $sourcebranch ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git wfr-driver-controlled-merge
cd wfr-driver-controlled-merge

# Automated driver version edit
mversion=`make version`
sed -i "/^#define HFI1_DRIVER_VERSION_BASE/s/\".*\"/\"$mversion\"/" common.h
git commit -s -m"Update driver version string to ${mversion}" common.h

# Do all branch merges, then breakout for inspection
branches=`getbranches`
for branch in $branches
do
	git checkout -b $branch origin/$branch
	if ! git merge --log -m"merge from $sourcebranch to $branch" $sourcebranch
	then
		echo "merge failed - resolve conflicts and push\n"
		/bin/sh || exit 1
	fi
done

echo -e "\nAll merges done. Check branches (git branch) and remove any unnecessary branches (git branch -D branchname)"
echo -e "Ready for: git push --all"
/bin/sh || exit 1
