#
# Copyright (c) 2014, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

. ../testconfig.sh

#
# create_file -- create zeroed out files of a given length in megs
#
# example, to create two files, each 1GB in size:
#	create_file 1024 testfile1 testfile2
#
function create_file() {
	size=$1
	shift
	for file in $*
	do
		dd if=/dev/zero of=$file bs=1M count=$size >> prep$UNITTEST_NUM.log
	done
}

#
# create_holey_file -- create holey files of a given length in megs
#
# example, to create two files, each 1GB in size:
#	create_holey_file 1024 testfile1 testfile2
#
function create_holey_file() {
	size=$1
	shift
	for file in $*
	do
		truncate -s ${size}M $file >> prep$UNITTEST_NUM.log
	done
}

#
# expect_normal_exit -- run a given command, expect it to exit 0
#
function expect_normal_exit() {
	eval $*
}

#
# expect_abnormal_exit -- run a given command, expect it to exit non-zero
#
function expect_abnormal_exit() {
	set +e
	eval $*
	[ $? == 0 ] && return 1
	set -e
}

#
# _is_mounted - check of a device is mounted
#
function _is_mounted() {
	grep -q $1 /proc/mounts
}

#
# mount_dax - mount a dax enabled fs on a pmem device
#
function mount_dax() {
	local num=$1

	mkdir -p /mnt/mem$num
	mke2fs -q -t ext4 /dev/pmem$num
	mount -t ext4 -o dax /dev/pmem$num /mnt/mem$num
}



#
# un_mount - umount a pmem device
#
function un_mount() {
	local dev=/dev/pmem$1

	_is_mounted $dev && umount $dev > /dev/null 2>&1
	return 0
}

#
# check_pmem - verify the existance of a pmem device
#
function check_pmem() {
	test -b /dev/pmem$1
}


#
# setup_btt -- setup a btt backed by the corresponding pmem device
#
function setup_btt() {
	local num=$1
	local bs=$2

	pushd /sys/class/nd_bus/ndctl0/device > /dev/null

	echo /dev/pmem$num > btt$num/backing_dev
	uuidgen -r > btt$num/uuid
	echo $bs >btt$num/sector_size
	echo btt$num > /sys/module/nd_btt/drivers/nd\:nd_btt/bind

	popd > /dev/null
}

#
# check_btt -- verify creation of a btt
#
function check_btt() {
	test -b /dev/btt$1
}

#
# disable_btt -- disable a btt
#
function disable_btt() {
	local num=$1

	pushd /sys/class/nd_bus/ndctl0/device > /dev/null
	echo btt$num > /sys/module/nd_btt/drivers/nd\:nd_btt/unbind
	echo "" > btt$num/backing_dev
	popd > /dev/null
}

#
# redir_start -- redirect all future output
#
saved_fd=""
function redir_start() {
	local logfile=$1
	[ -n "$logfile" ] || return
	exec {saved_fd}>&1
	exec > $logfile
}

#
# redir_end -- stop redirecting
#
function redir_end() {
	[ -n "$saved_fd" ] || return
	exec 1>&$saved_fd-
}

#
# setup -- print message that test setup is commencing
#
function setup() {
	# currently a no-op, keep if needed
	return
}

#
# check -- check test results (using .match files)
#
function check() {
	../match $(find . -regex "[^0-9]*${UNITTEST_NUM}\.log\.match" | xargs)
}

#
# pass -- print message that the test has passed
#
function pass() {
	echo $UNITTEST_NAME: PASS
}
