#!/bin/sh
set -x

fxr=~caz/fabric/fxr
ssh_cmd="ssh -p4022 root@localhost"
scp_cmd="scp -P4022"

# start simics if not yet
if [ -z `pidof simics-common` ]; then
    pushd ${fxr}/simics/workspace
    ./simics -no-win -e '$disk_image=../FxrRhel7-201505011038.craff' \
	FXR.simics >../simics.log &
    popd
    sleep 45
fi

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
# run quick test.
cd opa-headers.git/test
./harness.py --nodelist=viper0 --testlist=HFI-RING,HFI-APPEND,HFI-CMD,HFI-EQ,HFI-JOB,HFI-ME,HFI-PUT,HFI-SEND-SELF,HFI-TX-AUTH
res=$?
if [ ! ${res} ]; then
    echo fail on harness.
fi
exit $res
