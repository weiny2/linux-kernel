#!/bin/bash

# This is the script customized to merge latest development of FortPark
# driver, Network Adapter Driver for PCI-E* 40 Gigabit Network Connections,
# into Eywa. We obtain this driver from an archive released every night and
# made available via windows share drive. Here we mount that drive, find
# the latest release, and extract latest into Eywa kernel.
#
# For this script to work there has to be a mountpoint on this system,
# /mnt/eywa/kernel/fortpark, that mounts the windows share:
# //elfin/COMP_LIB/Linux_Kernel_i40e_i40evf
#
# This can be accomplished by adding the following line to the fstab:
# //elfin/COMP_LIB/Linux_Kernel_i40e_i40evf /mnt/eywa/kernel/fortpark cifs
# credentials=/PATH/TO/.smbcredentials,users,noauto,noexec,ro 0 0
#
# Assume script is run from within kernel source top level dir
#set -x

RELEASES_MNT="/mnt/eywa/kernel/fortpark"

function die () {
	echo "ERROR: $@"
	exit 1
}

function do_apply () {
	ethdevice=$1
	echo "Applying new changes for $ethdevice"
	release="$RELEASES_MNT/$latest_release/${ethdevice}-${latest_release}_k.tar.gz"

	if [ -e $release ]; then
		echo "Found $ethdevice release $release"
		tar xzvf $release -C $tmpdir || \
			die "Unable to extract $ethdevice driver"
	else
		die "Unable to find release $release"
	fi

	dest="drivers/net/ethernet/intel/$ethdevice"
	cp $tmpdir/${ethdevice}-${latest_release}_k/src/* $dest ||\
		die "Unable to copy new release of $ethdevice into kernel source"
}

tmpdir=$(mktemp -d) || die "Unable to create temporary directory"

is_mounted=$(mount | grep -q $RELEASES_MNT && echo -n 1)
if [ -z $is_mounted ]; then
	mount $RELEASES_MNT || die "Unable to mount $RELEASES_MNT"
else
	# If windows share has been mounted for a while then trying
	# to access it may stall. Ensure we are dealing with a "fresh"
	# mount
	umount $RELEASES_MNT || die "Unable to unmount $RELEASES_MNT"
	mount $RELEASES_MNT || die "Unable to mount $RELEASES_MNT"
fi

latest_release=$(ls $RELEASES_MNT | sort -Vr | head -1)
[ -z $latest_release ] && die "Unable to determine latest release"
echo "Adding FortPark release $latest_release"

do_apply i40e
do_apply i40evf

dest="drivers/net/ethernet/intel"
isclean=$(git status --porcelain $dest)
if [ "z$isclean" != "z" ]; then
	echo "Received new changes to $dest"
	tmpfile=$(mktemp) || die "Unable to create temporary file"
	cat > $tmpfile << EOF
i40e/i40evf: Apply changes from release $latest_release

EOF
	cat $RELEASES_MNT/$latest_release/changes.log >> $tmpfile || \
		die "Unable to obtain changelog"
	git add $dest || die "Unable to add changes to index"
	git commit --cleanup=verbatim -s -F $tmpfile || \
		die "Unable to commit the $dest changes"
	echo
	rm $tmpfile || die "Unable to remove temporary file $tmpfile"
fi

umount $RELEASES_MNT 2>/dev/null
if [ -d $tmpdir ]; then
	echo "Cleaning up $tmpdir"
	rm -rf $tmpdir
fi

exit 0
