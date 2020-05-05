#!/bin/bash
#Intel next setup script
#This script is to be ran in the kernel root before ./merge.py is ran
set -e
git fetch
git config rerere.enabled true
git checkout --detach
echo tracking; git branch -f master origin/master ; git branch -f eywa origin/eywa; git branch -f linus origin/linus
git reset --hard origin/eywa
cp -rf eywa/merge_ci/regen_configs.sh .
cp -rf eywa/merge_ci/merge.py .
cp -rf eywa/merge_ci/step3.sh .
cp -rf eywa/manifest_in.json .
#Remove from old merge
rm -rf patch-manifest
rm -rf intel-next-merge*.log
rm -rf .git/rr-cache
mkdir .git/rr-cache

#ADD freedesktop rr-cache to next rr cache
#requested by rodrigo vivi
echo "Adding drm rr-cache"
set +e
git remote | grep -q drm-cache
if [ $? != 0 ]; then

	git remote add drm-cache git://anongit.freedesktop.org/drm/drm-tip
fi
set -e
git fetch drm-cache
git checkout drm-cache/rerere-cache -- rr-cache 
cp -rf rr-cache .git
rm -rf rr-cache


echo "Adding internal rr-cache"
#Handle internal rr-cache
mkdir -p eywa/merge_ci/rr-cache/
if [ -d eywa/merge_ci/rr-cache/ ]; then
	#check if there are files inside folder and copy
	if [ "$(ls -A eywa/merge_ci/rr-cache)" ]; then
		cp -rf eywa/merge_ci/rr-cache/* .git/rr-cache
	fi
fi

