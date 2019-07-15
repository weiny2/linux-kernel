#!/bin/bash
set +e
git config rerere.enabled true
git reset master --hard
git checkout --detach
echo tracking; git branch -f master origin/master ; git branch -f eywa origin/eywa; git branch -f linus origin/linus
git fetch
git checkout eywa -f
git reset --hard origin/eywa
rm -rfv .git/rr-cache
mkdir .git/rr-cache
cp -rf eywa/merge_ci/blacklist .
cp -rf eywa/merge_ci/blacklist.py .
cp -rf eywa/merge_ci/replace .
cp -rf eywa/merge_ci/replace.py .
cp -rf eywa/merge_ci/step1.sh .
cp -rf eywa/merge_ci/step2.sh .
cp -rf eywa/merge_ci/step3.sh .
cp -rf eywa/merge_ci/merge.sh .
mkdir -p eywa/merge_ci/rr-cache/
if [ -d eywa/merge_ci/rr-cache/ ]; then
	#check if there are files inside folder and copy
	if [ "$(ls -A eywa/merge_ci/rr-cache)" ]; then
		cp -rfv eywa/merge_ci/rr-cache/* .git/rr-cache
	fi
fi
rm -f kernelcommit

