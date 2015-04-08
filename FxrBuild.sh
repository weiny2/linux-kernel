#!/bin/sh

KERNEL_BUILT_AGAINST=3.12.18-wfr+
RPM_PACKAGE_NAME=opa2_hfi
VERSION=0.0

# main start here
myname=$0
PARALLEL=`grep processor /proc/cpuinfo | wc -l`

# clean up and update to the latest
git clean -xfd
git pull
make rpm KVER=${KERNEL_BUILT_AGAINST} NAME=${RPM_PACKAGE_NAME} VERSION=${VERSION}
cd opa-headers.git
./build_rpm.sh

exit 0
