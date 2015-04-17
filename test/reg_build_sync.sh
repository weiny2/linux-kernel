#!/bin/bash

# Update git repos and do builds. Then update yum repos in KoP

driver=/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-driver
psm=/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-psm
diags=/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-diagtools-sw

yum_server=phlsvlogin02.ph.intel.com
yum_dir=/nfs/site/proj/ftp/wfr_yum/next
yum_repo="$yum_server:$yum_dir"
cur_dir=$PWD

echo "Checking for yum updates for ifs-kernel-devel"
sudo yum update ifs-kernel-updates-devel

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

echo "Sycning PSM"
echo "-----------"
cd $psm
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

echo "Sycning Diags"
echo "-------------"
cd $diags
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

#echo "Building PSM"
echo "------------"
cd $psm
rm -f *.tar.gz
make dist
psm_drop=`ls *.tar.gz  | awk 'BEGIN { FS="-"} ; { print $3 "-" $4 }' | awk 'BEGIN { FS="." } ; {print $1 "." $2}'`
rpmbuild -ta hfi1-psm-$psm_drop.tar.gz | tee .psm.build
if [ $? -ne 0 ]; then
	echo "Build failed see .psm.build"
	exit 1
fi
for i in $(grep x86_64.rpm .psm.build | awk '{print $2}'); do 
	echo Found RPM $i
	path=`readlink -f $i`
	echo "Actual path is $path"
	echo "Installing $path"
	sudo rpm --nodeps -Uvh $path
	echo "Copying to yum repo"
	scp $path $yum_repo
done
echo "Building Diagtools"
echo "------------"
cd $diags
rm -f *.tar.gz
make dist
diags_drop=`ls hfi1-utils*.tar.gz  | awk 'BEGIN { FS="-"} ; { print $3 "-" $4 }' | awk 'BEGIN { FS="." } ; {print $1 "." $2}'`
rpmbuild --define 'require_kver 3.12.18-wfr+' -ta hfi1-diagtools-sw-$diags_drop.tar.gz | tee .diags.ship
if [ $? -ne 0 ]; then
	echo "Build failed see .diags.ship"
	exit 1
fi

rpmbuild --define 'require_kver 3.12.18-wfr+' -ta hfi1-diagtools-sw-noship-$diags_drop.tar.gz | tee .diags.noship
if [ $? -ne 0 ]; then
	echo "Build failed see .diags.noship"
	exit 1
fi

rpmbuild --define 'require_kver 3.12.18-wfr+' -ta hfi1-utils-$diags_drop.tar.gz | tee .diags.utils
if [ $? -ne 0 ]; then
	echo "Build failed see .diags.utils"
	exit 1
fi

for i in $(grep x86_64.rpm .diags.ship | awk '{print $2}'); do 
	echo Found RPM $i
	path=`readlink -f $i`
	echo "Actual path is $path"
	echo "Copying to yum repo"
	scp $path $yum_repo
done

for i in $(grep x86_64.rpm .diags.noship | awk '{print $2}'); do 
	echo Found RPM $i
	path=`readlink -f $i`
	echo "Actual path is $path"
	echo "Copying to yum repo"
	scp $path $yum_repo
done

for i in $(grep x86_64.rpm .diags.utils | awk '{print $2}'); do 
	echo Found RPM $i
	path=`readlink -f $i`
	echo "Actual path is $path"
	echo "Copying to yum repo"
	scp $path $yum_repo
done

echo "Changing repo perms"
ssh $yum_server "cd $yum_dir && chmod 755 *" 
cd $cur_dir

echo "Versions Updated:"
echo "Driver: $driver_drop"
echo "PSM: $psm_drop"
echo "Diags: $diags_drop"

