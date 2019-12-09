#!/bin/bash
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

rm -rfv .git/rr-cache
mkdir .git/rr-cache
set +e
git remote add drm-cache git://anongit.freedesktop.org/drm/drm-tip
set -e
git fetch drm-cache
git checkout drm_cache/origin/rerere-cache -- rr-cache 

mkdir -p eywa/merge_ci/rr-cache/
if [ -d eywa/merge_ci/rr-cache/ ]; then
	#check if there are files inside folder and copy
	if [ "$(ls -A eywa/merge_ci/rr-cache)" ]; then
		cp -rfv eywa/merge_ci/rr-cache/* .git/rr-cache
	fi
fi
cp -rfv rr-cache .git
rm -rf rr-cache

