#!/bin/sh
set -x

myname=`basename $0 .sh`

. scripts/GlobalDefinition.sh

# Am I invoked by Jenkins?
ByJenkins=no
[ `id -un` == root ] && pwd | grep --quiet jenkins && ByJenkins=yes

. scripts/StartSimics.sh
. scripts/WaitSimics.sh

# show which version/commit of Simics, craff file and others I am using
pwd; git log -n1 | head -1
${fxr}/simics/workspace/bin/simics --version
( cd ${fxr}/simics/workspace/; git log -n1 | head -1 )
ls -l ${fxr}/simics/FxrRhel7.craff
ls -l ${fxr}/simics/SynopsisInstructionSetSimulator

ssh -p${viper0} root@localhost "service --skip-redirect opafm stop"

for viper in ${viper0} ${viper1}; do
    ssh_cmd="ssh -p${viper} root@localhost"
    scp_cmd="scp -P${viper}"

    ${ssh_cmd} "dmesg -c > /dev/null"
    # stop opa2_hfi daemon to release the driver
    ${ssh_cmd} "service --skip-redirect opa2_hfi stop"
    if [ ! $? ]; then
	echo fail on stoping opa2_hfi.
	exit 12
    fi
    # update the driver
    ${ssh_cmd} "rm -f /tmp/opa*.x86_64.rpm"
    ${scp_cmd} \
	~/rpmbuild/RPMS/x86_64/opa2_hfi-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm \
	opa-headers.git/opa-headers-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm \
	root@localhost:/tmp
    if [ ! $? ]; then
	echo fail on copying rpm files.
	exit 13
    fi
    ${ssh_cmd} "rpm -e opa2_hfi" 2>/dev/null
    ${ssh_cmd} "rpm -e opa-headers" 2>/dev/null
    ${ssh_cmd} "rpm -i /tmp/opa*.x86_64.rpm"
    if [ ! $? ]; then
	echo fail on the installation of rpm files.
	exit 14
    fi
done

# run default tests
cd opa-headers.git/test
./harness.py --nodelist=viper0,viper1 --type=default
res=$?
if [ ! ${res} ]; then
    echo fail on harness.
fi
./harness.py --nodelist=viper0,viper1 --testlist=ModuleLoad
res=$?
if [ ! ${res} ]; then
    echo fail on ModuleReload of harness.
fi

if [ ${ByJenkins} == yes ] ; then
	for viper in ${viper0} ${viper1}; do
		ssh_cmd="ssh -p${viper} root@localhost"

		echo dmesg on viper with port ${viper} start
		${ssh_cmd} "dmesg -c"
		echo dmesg on viper with port ${viper} end
	done
	# stop simics
	killall simics-common

	# display simics console logs
	echo simics console logs
	cat ${fxr}/simics/workspace/KnightsHill0.log

	cleanup_lock
fi

exit $res
