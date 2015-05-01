#!/bin/sh

KERNEL_BUILT_AGAINST=3.12.18-wfr
RPM_PACKAGE_NAME=opa2_hfi
VERSION=0.0

# main start here
rm -rf ~/rpmbuild
make rpm KVER=${KERNEL_BUILT_AGAINST} NAME=${RPM_PACKAGE_NAME} VERSION=${VERSION}
res=$?
if [ ! ${res} ]; then
    echo fail on building driver
    exit 1
fi

cd opa-headers.git
rm -f *.x86_64.rpm
./build_rpm.sh
res=$?
if [ ! ${res} ]; then
    echo fail on building rpm
    exit 2
fi

exit 0
