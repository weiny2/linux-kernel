#!/bin/bash
#Automates run of merge.sh script
set -e
git checkout master -f
git reset --hard origin/master
make clean
rm -f *.log
rm -f *.err
set +e
echo starting merge
#delete master-old branch if it is there
git branch | grep master-old
if [ $? == 0 ]; then
	git branch -D master-old 
fi
./merge.sh
