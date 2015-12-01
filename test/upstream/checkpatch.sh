#!/bin/bash

test_repo=`dirname $0`
pushd $test_repo > /dev/null
test_repo=`pwd`
popd > /dev/null

if [ "$1" == "" ] || [ "$2" == "" ]; then
	echo "usage: $0 <kern_src> <base_commit>"
	echo "   Run check patch on commits from <base_commit>..HEAD"
	echo " Feed in kernel_build dir or use default."

	exit 1
fi

kern_src=$1
base_commit=$2

pushd $kern_src

kernel_basedir=`git rev-parse --show-toplevel`

for i in \
    $(git log $base_commit..$(git symbolic-ref HEAD) --pretty=format:'%H' ); do
	echo Checking patch $i
	patch=$(git format-patch -1 $i)
	$kernel_basedir/scripts/checkpatch.pl --strict --no-tree $patch
	rm -f $patch
done

popd

exit 0
