#!/bin/bash

# Run check patch on current git commits
# Feed in kernel_build dir ot use default.

kernel_build=$1
if [ -z $1 ]; then
	kernel_build="/lib/modules/3.12.18-wfr+/build"
fi

for i in \
    $(git log origin/master..$(git symbolic-ref HEAD) --pretty=format:'%H' ); do
	echo Checking patch $i
	patch=$(git format-patch -1 $i)
	$kernel_build/scripts/checkpatch.pl --no-tree $patch
	rm -f $patch
done
