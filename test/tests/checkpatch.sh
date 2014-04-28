#!/bin/bash

# Run check patch on current git commits
# Feed in kernel_build dir ot use default.

kernel_build=$1
if [ -z $1 ]; then
	kernel_build="/lib/modules/3.9.2-wfr+/build"
fi

for i in $(git log origin/master..$(git symbolic-ref HEAD) | head -n 1 | awk '{print $2}'); do
	echo Checking patch $i
	git show $i > check_patch.tmp
	$kernel_build/scripts/checkpatch.pl --no-tree check_patch.tmp
	rm -f check_patch.tmp
done
