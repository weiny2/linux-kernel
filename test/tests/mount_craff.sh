#!/bin/bash

# Mount a craff file for simics. Requires to be run as root or
# with sudo. Takes a single argument and that is the craff to mount.
# The craff utility must be in your path. If its not do something like:
# PATH=/path/to/simics/bin:$PATH ./mount_craff.sh /path/to/craff

craff=$1
out_craff=$2

echo "Checking for existence of $1"
if [[ -f $craff ]]; then
	echo "Craff found"
else
	echo "Could not find craff!"
	exit 1
fi

# Explode the craff
uncomp=$craff.img

echo "Uncompressing craff file $craff to $uncomp. May take a couple minutes."
craff -d -o $uncomp $craff
echo "Craff uncompressed. Mounting"

if ! losetup /dev/loop0 $uncomp; then
	echo "Could not run losetup"
	exit 1
fi

if ! kpartx -a /dev/loop0; then
	echo "Could not run kpartx"
	exit 1
fi

if ! vgscan; then
	echo "Could not scan vol groups"
	exit 1
fi

if ! vgchange -ay VolGroup; then
	echo "Could not make VolGroup active"
	exit 1
fi

if ! mkdir -p /mnt/simics; then
	echo "Could not mkdir /mnt/simics"
	exit 1
fi

if ! mount /dev/VolGroup/lv_root /mnt/simics; then
	echo "Could not mont lv_root in /mnt/simics"
	exit 1
fi

if ! mount /dev/mapper/loop0p1 /mnt/simics/boot; then
	echo "could not mouint loop0p1 in /mnt/simics/boot"
	exit 1
fi

if ! mount -o bind /proc /mnt/simics/proc; then
	echo "could not bind /proc"
	exit 1;
fi

if ! mount -o bind /dev /mnt/simics/dev; then
	echo "Could not bind /dev"
	exit 1
fi

if ! mount -o bind /scratch /mnt/simics/scratch; then
	echo "Could not bind /scratch"
	exit 1
fi

echo "Craff is now mounted at /mnt/simics"

echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
echo "You will next be placed inside of the craff running as root."
echo "When you are done doing your work use the [exit] command to leave the"
echo "change rooted envrionment. The craff will then be saved and the"
echo "temporary image file removed."
echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
echo "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

echo "Changing root into the craff"
chroot /mnt/simics /bin/bash

echo "Back to normal root"
echo "Unmounting craff"

if ! umount /mnt/simics/dev; then
	echo "Could not unmount /dev from simics"
	exit 1
fi

if ! umount /mnt/simics/scratch; then
	echo "Could not unmount /scratch from simics"
	exit 1
fi

if ! umount /mnt/simics/proc; then
	echo "Could not unmount /proc from simics"
	exit 1
fi

if ! umount /mnt/simics/boot; then
	echo "Could not unmount /boot from simics"
	exit 1
fi

if ! umount /mnt/simics; then
	echo "could not umount simics"
	exit 1
fi

if ! vgchange -an VolGroup; then
	echo "Could not make volGroup inactive"
	exit 1
fi

if ! kpartx -d /dev/loop0; then
	echo "Failed to run kpartx"
	exit 1;
fi

if ! losetup -d /dev/loop0; then
	echo "Failed to run losetup"
	exit 1;
fi

if [[ -z $out_craff ]]; then
	echo "Warning: Did not specify a craff to write."
	echo "preserving $craff.img."
	echo "Remove manually after saving with the craff -o command"
else
	echo "Saving craff file to $out_craff"
	craff -o $out_craff $uncomp

	echo "Removing image file"
	rm $uncomp
fi

exit 0
