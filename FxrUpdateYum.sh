#!/bin/sh

# main start here
# copy rpm files to yum repository
scp -i ~/ssh-jenkins/id_rsa \
    ~/rpmbuild/RPMS/x86_64/opa2_hfi-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm \
    opa-headers.git/opa-headers-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm \
    cyokoyam@phlsvlogin02.ph.intel.com:/nfs/site/proj/ftp/fxr_yum/next
res=$?
if [ ! ${res} ]; then
    echo fail on copying rpm files to yum repository
    exit 21
fi
# update yum database
ssh -i ~/ssh-jenkins/id_rsa \
    jenkins@phlsdevlab.ph.intel.com \
    "/usr/bin/cobbler reposync --only=fxr-next"
res=$?
if [ ! ${res} ]; then
    echo fail on updating yum database
    exit 22
fi

exit 0
