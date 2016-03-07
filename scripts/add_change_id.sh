#!/bin/sh
# add a change id using the google developed script
# to the patches in an stg patch stack
tag=$1
if [ -z "$tag" ]
then
	echo "`basename $0`: commitish arg required"
	exit 1
fi
stg series | sed 's/..//' | tac > names$$
# get the commitids associated with each
git log `git merge-base $tag HEAD`.. HEAD | grep '^commit' | sed 's/^commit //' > commits$$
# paste them together
paste names$$ commits$$ | while read name commit
do
	echo processing $name $commit
	git log --format=%B -n 1 $commit > ${commit}.txt
	# add the commit id
	commit-msg ${commit}.txt
	# add the change id with stg edit
	echo stg edit --file ${commit}.txt $name
	stg edit --file ${commit}.txt $name
	rm -f ${commit}.txt
done
rm -f names$$ commits$$
