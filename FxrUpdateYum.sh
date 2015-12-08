#!/bin/bash

COBBLER_CMD="/usr/bin/cobbler reposync --only=fxr-next"

# We copy all rpms to the drop directory, but base on version of driver rpm.
# We copy whichever opa-headers rpm was used, in case the tagging of headers
# was not done correctly.
function copy_drop_rpms {
  RPMS=$@
  DRIVER_RPM=$1
  RMAJOR=`rpm -qp ${DRIVER_RPM} --qf %{VERSION} | cut -d . -f 1`
  RDROP=`rpm -qp ${DRIVER_RPM} --qf %{VERSION} | cut -d . -f 2`
  RBUILD=`rpm -qp ${DRIVER_RPM} --qf %{RELEASE} | cut -d g -f 1`
  # bail if this is not first build of drop
  if [ ${RBUILD} != 0 ]; then
    return
  fi
  ssh -i ~/ssh-jenkins/id_rsa \
      cyokoyam@phlsvlogin02.ph.intel.com \
      mkdir -p -m 777 /nfs/site/proj/ftp/fxr_yum/latest/drop${RDROP}/
  scp -i ~/ssh-jenkins/id_rsa \
      ${RPMS} \
      cyokoyam@phlsvlogin02.ph.intel.com:/nfs/site/proj/ftp/fxr_yum/latest/drop${RDROP}/
  res=$?
  if [ ! ${res} ]; then
      echo fail on copying rpm files to yum repository
      exit 21
  fi
  # TODO use better command to update both fxr and fxr-next
  COBBLER_CMD="/usr/bin/cobbler reposync"
}

# main start here

DRV_RPM=~/rpmbuild/RPMS/x86_64/opa2_hfi-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm
HDR_RPM=opa-headers.git/opa-headers-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm
# copy rpm files to yum repository
scp -i ~/ssh-jenkins/id_rsa \
    $DRV_RPM $HDR_RPM \
    cyokoyam@phlsvlogin02.ph.intel.com:/nfs/site/proj/ftp/fxr_yum/next
res=$?
if [ ! ${res} ]; then
    echo fail on copying rpm files to yum repository
    exit 21
fi

# if first build of driver for next drop, this will update drop directory
copy_drop_rpms $DRV_RPM $HDR_RPM

# update yum database
ssh -i ~/ssh-jenkins/id_rsa \
    jenkins@phlsdevlab.ph.intel.com $COBBLER_CMD
res=$?
if [ ! ${res} ]; then
    echo fail on updating yum database
    exit 22
fi

exit 0
