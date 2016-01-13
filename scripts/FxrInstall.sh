#!/bin/sh
set -x

. scripts/GlobalDefinition.sh
. scripts/StartSimics.sh
. scripts/WaitSimics.sh

# make sure to stop opafm
ssh -p${viper0} root@localhost "service --skip-redirect opafm stop"

# install driver and unit tests
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
