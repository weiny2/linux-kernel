#!/bin/bash
#Simple script to push changes if they have not already been pushed
DATE=$(date +%F)
DAYOFWEEK=$(date +%u)
HOUR=$(date +%k)

die() {
        echo "ERROR: $@"
        exit 1
}

if [ ! -f eywa/intel-next-merge-$DATE.log ]; then
	die "Merge not complete: missing log file in eywa dir"
fi

#Only push code Mon-Fri > 10AM
if [ $DAYOFWEEK -ge 6 ]  || [ $HOUR -lt 10 ]; then
	echo "Will only push code during monday-fri >10AM"
	exit 0
fi

LOGNAME=$(echo eywa/intel-next-merge-$DATE.log | sed "s/.log//g")
baseversion=$(grep "Resetting master to" $LOGNAME.log | uniq  | sed "s/Resetting master to //gi")
baseversion=$(echo $baseversion | sed "s/v//g")

#intel next tag
git ls-remote --tags | grep  -q "refs/tags/intel-$baseversion-$DATE"
if [ $? -ne 0 ]; then
        echo "Pushing intel Next tag"
	echo  "intel-$baseversion-$DATE" 
        git tag intel-$baseversion-$DATE || die "Unable to tag repo"
        echo "Pushing master branch upstream ..."
        git push origin -f master:master intel-$baseversion-$DATE || \
        die "Unable to push kernel master branch upstream"
else
        echo "Intel next tag: intel-$baseversion-$DATE already pushed" 
fi

#do linus tag
echo "Pushing new tag to linus branch"
echo "Switching to linus branch ..."
git checkout linus
git merge --ff-only v$baseversion
echo "Pushing linus branch upstream ..."
git push origin linus:linus
git push origin linus:linus v$baseversion
git checkout master
