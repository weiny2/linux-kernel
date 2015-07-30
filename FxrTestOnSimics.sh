#!/bin/sh
set -x

fxr=/mnt/fabric/fxr
viper0=4022
viper1=5022
export LD_LIBRARY_PATH=${fxr}/simics/SynopsisInstructionSetSimulator/lib
export SNPSLMD_LICENSE_FILE="26586@synopsys03p.elic.intel.com"
export LM_PROJECT=”FDO”

# Am I invoked by Jenkins?
ByJenkins=no
[ `id -un` == root ] && pwd | grep --quiet jenkins && ByJenkins=yes

if [ ${ByJenkins} == yes ] ; then
    # make sure no simics process running
    killall simics-common

    # start simics
    pushd ${fxr}/simics/workspace
    ./simics -no-win -e '$disk_image=../FxrRhel7.craff' \
	FXR.simics >../simics.log &
    popd
    sleep 120
else # manual invocation
    # start simics if not yet
    if [ -z `pidof simics-common` ]; then
	pushd ${fxr}/simics/workspace
	./simics -no-win -e '$disk_image=../FxrRhel7.craff' \
	    FXR.simics >../simics.log &
	popd
	sleep 120
    fi
fi

# show which version/commit of Simics, craff file and others I am using
pwd; git log -n1 | head -1
${fxr}/simics/workspace/bin/simics --version
( cd ${fxr}/simics/workspace/; git log -n1 | head -1 )
ls -l ${fxr}/simics/FxrRhel7.craff
ls -l ${fxr}/simics/SynopsisInstructionSetSimulator

for viper in ${viper0} ${viper1}; do
    ssh_cmd="ssh -p${viper} root@localhost"
    scp_cmd="scp -P${viper}"

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
    # start opa2_hfi daemon
    ${ssh_cmd} "service --skip-redirect opa2_hfi start"
    if [ ! $? ]; then
	echo fail on starting opa2_hfi.
	exit 15
    fi
done

# run quick test.
cd opa-headers.git/test
./harness.py --nodelist=viper0,viper1 --type=quick
res=$?
if [ ! ${res} ]; then
    echo fail on harness.
fi

if [ ${ByJenkins} == yes ] ; then
    # stop simics
    killall simics-common
fi

exit $res
