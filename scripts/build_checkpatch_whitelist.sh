#!/bin/sh
#
# This script runs checkpatch to generate a summary
# of the following format:
#     count errorype message
#
# The script is suitable for creating a new whitelist
# and for testing against an existing whitelist.
#
if [ -z $1 ]
then
        topdir=`git rev-parse --show-toplevel`
else
	topdir=$1
fi
cd $topdir
declare -a filelist
filelist=$(echo *.[ch])
count=0
lfn=""
lerr=""
lm=""
for f in $filelist
do
	$topdir/scripts/checkpatch.pl --no-tree -F $f | egrep '^ERROR|^WARNING' | sort | sed "s/^/$f /"
done | while read fn err m
do
	count=$(( $count + 1 ))
	if [ -n "$lfn" -a "$fn" != "$lfn" ]
	then
		# filename changed - output
		echo $count $lfn $lerr $lm
		count=0
	elif [ -n "$lm" -a "$lm" != "$m" ]
	then
		# message changed
		test -n "$lfn" && echo $count $lfn $lerr $lm
		count=0
	fi
	lfn=$fn
	lerr=$err
	lm="$m"
done
