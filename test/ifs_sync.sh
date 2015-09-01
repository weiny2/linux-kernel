#!/bin/bash

# This script will sync an IFS build dir to the wfr yum repositories.

yum6=/nfs/ph/proj/ftp/wfr_yum/next
yum7=/nfs/ph/proj/ftp/wfr_yum7/next
ifs_root=/nfs/ph/proj/stlbuilds/Integration/GA10_0_0_991/OPENIB_INSTALL
ifs_base="$ifs_root/10_0_0_991_"
ifs_priv="/nfs/site/disks/ph_home_disk001/ddalessa/work/IFS/scratch"
cur_dir=$PWD

rm -rf $ifs_priv
if [ $? -ne 0 ]; then
	echo "Could not remove $ifs_priv"
	exit 1
fi

mkdir $ifs_priv
if [ $? -ne 0 ]; then
	echo "Could not create $ifs_priv"
	exit 1
fi

echo "Checking for IFS workspace"
if [ -d "$ifs_priv" ]; then
	echo "Found $ifs_priv"
else
	echo "Could not find $ifs_priv"
	exit 1
fi

echo "Checking yum repos"
if [ -d "$yum6" ]; then
	echo "Found $yum6"
else
	echo "$yum6 not found."
	exit 1
fi

if [ -d "$yum7" ]; then
	echo "Found $yum7"
else
	echo "$yum7 not found."
	exit 1
fi

echo "10 Latest Integration Builds:"
ls $ifs_root | awk 'BEGIN { FS = "_"} ; {print $5}' | sort -n | tail -n 10

echo "Latest Available DST Build:"
dst_build=`ls /nfs/ph/proj/stlbuilds/System_Test | grep OPENIB_INSTALL_10_0_0_991_* | awk 'BEGIN { FS = "_"} ; {print $7}' | sort -n | tail -n 1`
echo "$dst_build"

echo "Press ENTER to accept $dst_build or specify build #"
read integ_build
ifs_build=""
if [ "$integ_build" ]; then
	build_num="$integ_build"
	ifs_build="${ifs_base}$integ_build"
else
	build_num="$dst_build"
	ifs_build="${ifs_base}$dst_build"
fi

echo "Making sure $ifs_build exists"
if [ -d "$ifs_build" ]; then
	echo "Found $ifs_build"
else
	echo "Could not find $ifs_build"
	exit 1
fi

#copy tarballs
echo "Copying tarballs"
cp $ifs_build/X86_64_RHEL6/IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num.tgz $ifs_priv
if [ $? -ne 0 ]; then
	echo "Could not copy RHEL 6 tarball"
	exit 1
fi

cp $ifs_build/X86_64_RHEL7/IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num.tgz $ifs_priv
if [ $? -ne 0 ]; then
	echo "Could not copy RHEL 7 tarball"
	exit 1
fi

echo "Changing dir to $ifs_priv"
cd $ifs_priv
if [ $? -ne 0 ]; then
	echo "Could not change to $ifs_priv"
	cd $cur_dir
	exit 1
fi

echo "Extracting tarballs"
tar xzf $ifs_priv/IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num.tgz
if [ $? -ne 0 ]; then
	echo "Could not extract RHEL 6 tarball"
	cd $cur_dir
	exit 1
fi

tar xzf $ifs_priv/IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num.tgz 
if [ $? -ne 0 ]; then
	echo "Could not extract RHEL 7 tarball"
	cd $cur_dir
	exit 1
fi

exclude="--exclude=hfi1-devel-* --exclude=hfi1-0.7-* --exclude=hfi-* --exclude=hfi1-debuginfo-* --exclude=hfi1-0.9-*"

echo "Checking for RHEL 6 updates"
echo "---------------------------"
echo "OFED"
rsync -avh -n --ignore-existing $exclude IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num/IntelOPA-OFED.RHEL6-x86_64.10.0.0.991.*/RPMS/redhat-ES6/ $yum6
echo "FM"
rsync -avh -n --ignore-existing $exclude IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num/IntelOPA-FM.RHEL6-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum6
echo "FF"
rsync -avh -n --ignore-existing $exclude IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num/IntelOPA-Tools-FF.RHEL6-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum6

echo "Checking for RHEL 7 updates"
echo "---------------------------"
echo "OFED"
rsync -avh -n --ignore-existing $exclude IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num/IntelOPA-OFED_DELTA.RHEL7-x86_64.10.0.0.991.*/RPMS/redhat-ES7/ $yum7
echo "FM"
rsync -avh -n --ignore-existing $exclude IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num/IntelOPA-FM.RHEL7-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum7
echo "FF"
rsync -avh -n --ignore-existing $exclude IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num/IntelOPA-Tools-FF.RHEL7-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum7

echo "Press ENTER to update the repos or Ctrl+C to bail"
read resp
echo "Applying updates"

rsync -avh --ignore-existing $exclude IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num/IntelOPA-OFED.RHEL6-x86_64.10.0.0.991.*/RPMS/redhat-ES6/ $yum6
if [ $? -ne 0 ]; then
	echo "Could not apply RHEL 6 - OFED"
	exit 1
fi

rsync -avh --ignore-existing $exclude IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num/IntelOPA-FM.RHEL6-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum6
if [ $? -ne 0 ]; then
	echo "Could not apply RHEL 6 - FM"
	exit 1
fi

rsync -avh --ignore-existing $exclude IntelOPA-IFS.RHEL6-x86_64.10.0.0.991.$build_num/IntelOPA-Tools-FF.RHEL6-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum6
if [ $? -ne 0 ]; then
	echo "Could not apply RHEL 6 - FF"
	exit 1
fi

rsync -avh --ignore-existing $exclude IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num/IntelOPA-OFED_DELTA.RHEL7-x86_64.10.0.0.991.*/RPMS/redhat-ES7/ $yum7
if [ $? -ne 0 ]; then
	echo "Could not apply RHEL 7 - OFED"
	exit 1
fi

rsync -avh --ignore-existing $exclude IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num/IntelOPA-FM.RHEL7-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum7
if [ $? -ne 0 ]; then
	echo "Could not apply RHEL 7 - FM"
	exit 1
fi

rsync -avh --ignore-existing $exclude IntelOPA-IFS.RHEL7-x86_64.10.0.0.991.$build_num/IntelOPA-Tools-FF.RHEL7-x86_64.10.0.0.991.*/RPMS/x86_64/ $yum7
if [ $? -ne 0 ]; then
	echo "Could not apply RHEL 7 - FF"
	exit 1
fi

cd $cur_dir
if [ $? -ne 0 ]; then
	echo "Could not switch back to $cur_dir"
	exit 1
fi

chmod 755 $yum6/*.rpm
if [ $? -ne 0 ]; then
	echo "Could not chmod RHEL 6 files"
fi

chmod 755 $yum6
if [ $? -ne 0 ]; then
	echo "Could not chmod RHEL 6 repo"
fi

chmod 755 $yum7/*.rpm
if [ $? -ne 0 ]; then
	echo "Could not chmod RHEL 7 files"
fi

chmod 755 $yum7
if [ $? -ne 0 ]; then
	echo "Could not chmod RHEL 7 repo"
fi

echo "Complete. Go to Cobbler and use reposync option now"

