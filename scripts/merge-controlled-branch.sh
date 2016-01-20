#!/bin/sh
#

getbranches()
{
	git branch -r | grep "$sourcebranch$" | grep wfr-ifs | xargs basename -a
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
git clone ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git wfr-driver-controlled-merge
cd wfr-driver-controlled-merge
branches=`getbranches`
for branch in $branches
do
	git checkout -b $branch origin/$branch
	if ! git merge --log -m"merge from $sourcebranch to $branch" origin/$sourcebranch
	then
		echo "merge failed - resolve conflicts and push\n"
		/bin/sh || exit 1
	else
		
		echo "ready for git push to origin $branch"
		/bin/sh || exit 1
	fi
done
