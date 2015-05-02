#!/bin/sh

KERNEL_BUILT_AGAINST=3.12.18-wfr
RPM_PACKAGE_NAME=opa2_hfi
VERSION=0.0
TAG="v[0-9]*.[0-9]*"

# main start here
rm -rf ~/rpmbuild
git fetch # acquire newly created tag
if [ -d .git ]; then
	VERSION=`git describe --tags --long --match="$TAG" | cut -d'-' -f1 | tr -d 'v'`
fi
make rpm KVER=${KERNEL_BUILT_AGAINST} NAME=${RPM_PACKAGE_NAME} VERSION=${VERSION}
res=$?
if [ ! ${res} ]; then
    echo fail on building driver
    exit 1
fi

cd opa-headers.git
rm -f *.x86_64.rpm
[ -d config ] || mkdir config # eliminate warning
git fetch # acquire newly created tag
./build_rpm.sh
res=$?
if [ ! ${res} ]; then
    echo fail on building rpm
    exit 2
fi

exit 0
