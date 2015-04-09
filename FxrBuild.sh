#!/bin/sh

KERNEL_BUILT_AGAINST=3.12.18-wfr+
RPM_PACKAGE_NAME=opa2_hfi
VERSION=0.0

# main start here
make rpm KVER=${KERNEL_BUILT_AGAINST} NAME=${RPM_PACKAGE_NAME} VERSION=${VERSION}
cd opa-headers.git
./build_rpm.sh

# copy rpm files to yum repository
scp -i ~/ssh-jenkins/id_rsa \
    ~/rpmbuild/RPMS/x86_64/opa2_hfi-0.0-*.x86_64.rpm \
    opa-headers-0.0-1.x86_64.rpm \
    cyokoyam@phlsvlogin02.ph.intel.com:/nfs/site/proj/ftp/fxr_yum/next
ssh -i ~/ssh-jenkins/id_rsa \
    jenkins@phlsdevlab.ph.intel.com \
    "/usr/bin/cobbler reposync --only=fxr-next"

exit 0
