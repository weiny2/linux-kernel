#!/bin/bash
#
# Print WFR driver patch versions, from tag v0.2 onward.
#
# Issues:
# o The last 8 commits found by "git log" cannot find which version
#   tag they belong to.  --always was aded git describe so it would
#   not print to stderr, but it does make a mess
# o The v0.2 commits are not sequential: 27,24,23,9-1.
#
verbose=

while getopts ":v" opt; do
	case $opt in
	v)
		verbose=1
		;;
	\?)
		echo "usage: changelog.sh [-v]"
		exit 1
		;;
	esac
done

last_base=""
for commit in $(git log --no-merges v0.2..HEAD --pretty="%H"); do
	vers=`git describe --always --tags --long --match='v*' $commit`
	base=$(echo $vers | tr '-' ' ' | awk '{ print $1 }')
	patch=$(echo $vers | tr '-' ' ' | awk '{ print $2 }')
	desc=`git log -1 $commit --pretty=format:%s`

	if [ "$last_base" != "$base" ]; then
		echo "===== $base ======"
		last_base=$base
	fi
	printf "%3d - $desc\n" $patch
	if [ -n "$verbose" ]; then
		abbrev=$(echo $vers | tr '-' ' ' | awk '{ print $3 }' | sed -e 's/^g//')
		commiter=`git log -1 $commit --pretty=format:%ce`
		echo "      $abbrev $commiter"
	fi
done
