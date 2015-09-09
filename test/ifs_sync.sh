#!/bin/bash

# This script will sync an IFS build dir to the wfr yum repositories.

ifs_number="991"
ifs_priv="/tmp/ifs_scratch.$USER"
exclude="--exclude=hfi1-devel-* --exclude=hfi1-0.7-* --exclude=hfi-* --exclude=hfi1-debuginfo-* --exclude=hfi1-0.9-*"

cobbler="10.228.211.254"

ifs_root=/nfs/sc/disks/fabric_mirror/KOP/Integration/GA10_0_0_$ifs_number/OPENIB_INSTALL

yum6=phlsvlogin02.ph.intel.com:/nfs/ph/proj/ftp/wfr_yum/next
yum7=phlsvlogin02.ph.intel.com:/nfs/ph/proj/ftp/wfr_yum7/next
yum71=phlsvlogin02.ph.intel.com:/nfs/ph/proj/ftp/wfr_yum7-1/next

ifs_base="$ifs_root/10_0_0_${ifs_number}_"
cur_dir=$PWD

function extract {
	rel=$1

	tarball_rel=`echo $rel | sed 's/_//'`

	echo "Copying tarball for $rel to $ifs_priv"
	cp $ifs_build/X86_64_$rel/IntelOPA-IFS.$tarball_rel-x86_64.10.0.0.$ifs_number.$build_num.tgz $ifs_priv
	if [ $? -ne 0 ]; then
		echo "Could not copy RHEL 6 tarball"
		exit 1
	fi


	echo "Changing dir to $ifs_priv"
	cd $ifs_priv
	if [ $? -ne 0 ]; then
		echo "Could not change to $ifs_priv"
		cd $cur_dir
		exit 1
	fi

	echo "Extracting tarball for $rel in $ifs_priv"
	tar xzf $ifs_priv/IntelOPA-IFS.$tarball_rel-x86_64.10.0.0.$ifs_number.$build_num.tgz
	if [ $? -ne 0 ]; then
		echo "Could not extract $rel tarball"
		cd $cur_dir
		exit 1
	fi

	echo "Changing pack to $cur_dir"
	cd $cur_dir
}

function do_updates {
	rel=$1
	yum=$2
	vers=$3
	ofed=$4
	dryrun=$5

	cd $ifs_priv
	echo ""
	echo "---------------------------"
	echo "Checking for $rel updates"
	echo "---------------------------"
	echo ""
	echo "OFED"
	rsync -avh $dryrun -e ssh --ignore-existing $exclude IntelOPA-IFS.$rel-x86_64.10.0.0.$ifs_number.$build_num/IntelOPA-$ofed.$rel-x86_64.10.0.0.$ifs_number.*/RPMS/$vers/ $yum
	if [ $? -ne 0 ]; then
		echo "Could not do rsync"
		exit 1
	fi
	echo ""
	echo "FM"
	rsync -avh $dryrun -e ssh --ignore-existing $exclude IntelOPA-IFS.$rel-x86_64.10.0.0.$ifs_number.$build_num/IntelOPA-FM.$rel-x86_64.10.0.0.$ifs_number.*/RPMS/x86_64/ $yum
	if [ $? -ne 0 ]; then
		echo "Could not do rsync"
		exit 1
	fi
	echo ""
	echo "FF"
	rsync -avh $dryrun -e ssh --ignore-existing $exclude IntelOPA-IFS.$rel-x86_64.10.0.0.$ifs_number.$build_num/IntelOPA-Tools-FF.$rel-x86_64.10.0.0.$ifs_number.*/RPMS/x86_64/ $yum
	if [ $? -ne 0 ]; then
		echo "Could not do rsync"
		exit 1
	fi
	echo ""
	cd $cur_dir
}

function repo_sync {
	repo=$1
	echo "Running repo sync for $repo on $cobbler"
	ssh -t $cobbler "sudo cobbler reposync --only=$repo"
	if [ $? -ne 0 ]; then
		echo "Could not sync $repo"
		exit 1
	fi
}

echo "Removing contents of $ifs_priv"
rm -rf $ifs_priv
if [ $? -ne 0 ]; then
	echo "Could not remove $ifs_priv"
	exit 1
fi

echo "Re-creating $ifs_priv"
mkdir $ifs_priv
if [ $? -ne 0 ]; then
	echo "Could not create $ifs_priv"
	exit 1
fi

if [ -d "$ifs_priv" ]; then
	echo "Found $ifs_priv"
else
	echo "Could not find $ifs_priv"
	exit 1
fi

echo "10 Latest Integration Builds:"
ls $ifs_root | awk 'BEGIN { FS = "_"} ; {print $5}' | sort -n | tail -n 10

echo "Latest Available DST Build:"
dst_build=`readlink /nfs/sc/disks/fabric_mirror/KOP/Integration/GA10_0_0_$ifs_number/OPENIB_INSTALL/latest_DST | awk 'BEGIN {FS = "_"}; {print $5}'`
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

#copy and extract tarballs
extract RHEL6
extract RHEL7
extract RHEL7_1

# The -n here means do a dry run so we don't do the actual rsync
do_updates RHEL6 $yum6 redhat-ES6 OFED -n
do_updates RHEL7 $yum7 redhat-ES7 OFED_DELTA -n
do_updates RHEL71 $yum71 redhat-ES71 OFED_DELTA -n

echo "Press ENTER to update the repos or Ctrl+C to bail"
read resp
echo "Applying updates"

# Do not include the -n here to actually do the rsync
do_updates RHEL6 $yum6 redhat-ES6 OFED
do_updates RHEL7 $yum7 redhat-ES7 OFED_DELTA
do_updates RHEL71 $yum71 redhat-ES71 OFED_DELTA

# Run reposync on the cobbler server
repo_sync wfr-next
repo_sync wfr-next-rhel7
repo_sync wfr-next-rhel7-1

