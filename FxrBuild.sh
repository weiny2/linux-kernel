#!/bin/sh

KERNEL_BUILT_AGAINST=3.12.18-wfr
RPM_PACKAGE_NAME=opa2_hfi
VERSION=0.0
TAG="v[0-9]*.[0-9]*"

# main start here
rm -rf ~/rpmbuild
git fetch # acquire newly created tag
if [ -d .git ]; then
	FULL_TAG=`git describe --tags --long --match="$TAG"`
	VERSION=`echo ${FULL_TAG} | cut -d'-' -f1 | tr -d 'v'`
	RELEASE=`echo ${FULL_TAG} | cut -d'-' -f2`
	RELEASE=${RELEASE}`echo ${FULL_TAG} | cut -d'-' -f3`
fi
make rpm KVER=${KERNEL_BUILT_AGAINST} NAME=${RPM_PACKAGE_NAME} VERSION=${VERSION} RELEASE=${RELEASE}
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
