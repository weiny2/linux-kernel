#!/bin/bash

# Run check patch on current git commits
# Feed in kernel_build dir or use default.

kernel_build=$1
if [ -z $1 ]; then
	kernel_build=`git rev-parse --show-toplevel`
fi

for i in \
    $(git log origin/master..$(git symbolic-ref HEAD) --pretty=format:'%H' ); do
	echo Checking patch $i
	patch=$(git format-patch -1 $i)
	$kernel_build/scripts/checkpatch.pl --strict --no-tree $patch
	rm -f $patch
done
