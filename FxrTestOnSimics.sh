#!/bin/sh

fxr=~caz/fabric/fxr
ssh_cmd="ssh -i /home/caz/.ssh/id_rsa -p4022 root@localhost"
scp_cmd="scp -i /home/caz/.ssh/id_rsa -P4022"

# start simics if not yet
if [ -z `pidof simics-common` ]; then
    pushd ${fxr}/simics/workspace
    ./simics -no-win -e '$disk_image=../FxrRhel7.craff' knl.simics -e c \
        >../simics.log &
    popd
    sleep 45
    ${ssh_cmd} "service opa2_hfi start"
    if [ ! $? ]; then
	echo fail on starting opa2_hfi.
	exit 11
    fi
fi

# stop opa2_hfi daemon to release the driver
${ssh_cmd} "service opa2_hfi stop"
if [ ! $? ]; then
    echo fail on stoping opa2_hfi.
    exit 12
fi
# update the driver
${ssh_cmd} "rm -f /tmp/opa*.x86_64.rpm"
${scp_cmd} \
    /home/caz/rpmbuild/RPMS/x86_64/opa2_hfi-0.0-*.x86_64.rpm \
    opa-headers.git/opa-headers-0.0-*.x86_64.rpm \
    root@localhost:/tmp
if [ ! $? ]; then
    echo fail on copying rpm files.
    exit 13
fi
${ssh_cmd} "rpm -e opa2_hfi"
${ssh_cmd} "rpm -e opa-headers"
${ssh_cmd} "rpm -i /tmp/opa*.x86_64.rpm"
if [ ! $? ]; then
    echo fail on the installation of rpm files.
    exit 14
fi
# start opa2_hfi daemon
${ssh_cmd} "service opa2_hfi start"
if [ ! $? ]; then
    echo fail on starting opa2_hfi.
    exit 15
fi
# run quick test.
cd ${fxr}/fxr-driver/test
./harness.py --nodelist=viper0 --type=quick
res=$?
if [ ! ${res} ]; then
    echo fail on harness.
fi
exit $res
