#!/bin/sh
#
# TODO - make these args
distros="3.12 rhel7.1 sles12.0"
sourcebranch=beta0
rm -rf wfr-driver-controlled-merge
git clone ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-driver.git wfr-driver-controlled-merge
cd wfr-driver-controlled-merge
dir=$PWD
for distro in $distros
do
	# special case
	if [ "$distro" != "3.12" ]
	then
		git checkout -b wfr-ifs-${distro}-${sourcebranch} origin/wfr-ifs-${distro}-${sourcebranch}
		if ! git merge --log -m"merge from $sourcebranch to wfr-ifs-${distro}-${sourcebranch}" origin/$sourcebranch
		then
			echo "merge failed\n"
			exit 1
		else
			echo "ready for git push origin wfr-ifs-${distro}-${sourcebranch}"
			/bin/sh || exit 1
		fi
	fi
	git checkout -b for-wfr-ifs-${distro}-${sourcebranch} origin/for-wfr-ifs-${distro}-${sourcebranch}
	if ! git merge --log -m"merge from $sourcebranch to for-wfr-ifs-${distro}-${sourcebranch}" origin/$sourcebranch
	then
		echo "merge failed - resolve conflicts and push\n"
		/bin/sh
	else
		
		echo "ready for git push origin for-wfr-ifs-${distro}-${sourcebranch}"
		/bin/sh || exit 1
	fi
done
