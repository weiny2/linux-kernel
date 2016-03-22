#!/bin/bash

cur_build=`git rev-parse --short HEAD`

if [[ ! -e wfr-driver-master.last ]]; then
	echo $cur_build > wfr-driver-master.last
fi

prev_build=`cat wfr-driver-master.last`

echo "Most recent commits:" > mail.tmp
git --no-pager log --oneline ${prev_build}..HEAD >> mail.tmp
echo "" >> mail.tmp
echo "Last 25 commits:" >> mail.tmp
git --no-pager log --oneline -n 25 >> mail.tmp

echo $cur_build > wfr-driver-master.last
to="`cat passing_reg.subscribers`"
mailx -S smtp=smtp://smtp.intel.com -s "[CRON_REG] Commit Summary ${prev_build}..${cur_build}" -r dennis.dalessandro@intel.com $to < mail.tmp
