#!/bin/bash

# Update git repos and do builds. Then update yum repos in KoP
# We no longer build PSM and DIAGs from repo, get them from IFS build

vers=`cat /etc/redhat-release`
if [[ $vers = "Red Hat Enterprise Linux Server release 7.0 (Maipo)" ]]; then
	RHEL=7
	driver=/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-driver7
	yum_dir=/nfs/site/proj/ftp/wfr_yum7/next
elif [[ $vers = "Red Hat Enterprise Linux Server release 6.3 (Santiago)" ]] ; then
	RHEL=6
	driver=/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-driver
	yum_dir=/nfs/site/proj/ftp/wfr_yum/next
else
	echo "Could not determine RHEL version"
	exit 1
fi

echo "Building driver and updating yum for RHEL $RHEL"

yum_server=phlsvlogin02.ph.intel.com
yum_repo="$yum_server:$yum_dir"
cur_dir=$PWD

echo "Checking for yum updates for ifs-kernel-devel"
sudo yum update ifs-kernel-updates-devel

rm -f /nfs/sc/disks/fabric_work/$USER/rpmbuild/RPMS/x86_64/hfi1*0.9*

echo "Syncing Driver"
echo "--------------"
cd $driver
git remote update
if [ $? -ne 0 ]; then
	echo "Failed to do git update"
	exit 1
fi
git pull --rebase
if [ $? -ne 0 ]; then
	echo "Failed to pull and rebase"
	exit 1
fi

echo "Building Driver"
echo "---------------"
cd $driver
rm -f *.tgz
make dist
driver_drop=`ls *.tgz | awk 'BEGIN { FS="-"} ; { print $2 }' | awk 'BEGIN { FS="." } ; {print $1 "." $2}'`
rpmbuild --define 'require_kver 3.12.18-wfr+' -ta hfi1-$driver_drop.tgz | tee .driver.build
if [ $? -ne 0 ]; then
	echo "Build failed see .driver.build"
	exit 1
fi
for i in $(grep x86_64.rpm .driver.build | awk '{print $2}'); do 
	echo Found RPM $i
	path=`readlink -f $i`
	echo "Actual path is $path"
	echo "Installing $path"
	sudo rpm -Uvh $path
	echo "Copying to yum repo"
	scp $path $yum_repo
done

# Need to patch up driver version
driver_drop=`grep Wrote .driver.build | grep hfi1-devel | awk 'BEGIN { FS="x86_64/" } ; {print $2}' | awk 'BEGIN { FS="-" } ; { print $3 "-" $4}' | awk 'BEGIN {FS="."} ; {print $1 "." $2}'`

echo "Changing repo perms"
ssh $yum_server "cd $yum_dir && chmod 755 *" 
cd $cur_dir

echo "Versions Updated:"
echo "Driver: $driver_drop"

