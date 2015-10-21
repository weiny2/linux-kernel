#!/bin/bash

# Run weekly regression tests and updates

#################
# Configuration #
#################

# Set of tests to run
test1="sperf-05 sperf-06"
test1_tag="regtest-A0"
test2="fpga2 fpga3"
test2_tag="regtest-FPGA-B1-PRR"
test3="fpga1 fpga4"
test3_tag="regtest-FPGA-B1-B2B"

# Host to use for opa-fm configuration
fmhost="sperf-05"

# PRR host
prrhost="fpga6"



ifs_vers=0

function update_ifs {
	./ifs_sync.sh
	ifs_vers=$?
}

function update_driver_rpm {
	./yum_update_driver.sh
	if [ $? -ne 0 ]; then
		echo "Could not update driver RPM"
		exit 1
	fi
}

function check_update_host {
	host=$1
	ssh root@$host "yum clean all && yum check-update"
}

function update_host {
	host=$1
	ssh root@$host "yum update -y"
	if [ $? -ne 0 ]; then
		echo "Could not update $host"
		exit 1
	fi
}

function do_updates {
	hosts=$@
	for h in $hosts; do
		echo "---------------------------"
		echo $h
		echo "---------------------------"
		check_update_host $h
	done
	echo ""
	echo "Are these updates OK to apply? Press ENTER if so, else Ctrl+C"
	read ok

	for h in $hosts; do
		echo "---------------------------"
		echo $h
		echo "---------------------------"
		update_host $h
	done
}

get_uptime () {
	local host=$1
	ssh root@$host "cat /proc/uptime" | awk '{print $1}' | awk 'BEGIN { FS="." } {print $1}'
}

power_cycle() {
	local host=$1
	echo "Power cycling $host"
	ssh fabserv2 "/nfs/site/proj/fabric/apps/power/bin/power -r $host"
}

check_up() {
	local host=$1
	local uptime=$2

	attempts=0
	while [ $attempts -lt 10 ]; do
		# Try a ping
		ping -c 1 -t 3 $host
		if [ $? -eq 0 ]; then
			echo "$host pings"
			cur_uptime=$(get_uptime $host)
			if [ $cur_uptime -eq 0 ]; then
				echo "Cur uptime is 0 host may be booting still"
			fi

			if [ $cur_uptime -gt $uptime ]; then
				echo "Host never rebooted after 120 seconds ($cur_uptime $uptime)"
				return 1
			fi

			if [ $cur_uptime -lt $uptime ]; then
				echo "Host has rebooted"
				return 0
			fi
		else
			echo "$host does not ping"
		fi
		echo "Checking again in 30 seconds"
		sleep 30
	done

	return 1
}

do_reboot() {
	local host=$1
	echo "Rebooting $host"
	uptime=$(get_uptime $host)
	echo "$host has been up for $uptime"
	ssh root@$host 'reboot'&
	echo "Waiting 2 minutes to give host time to shutdown and start booting"
	sleep 120
	check_up $host $uptime
	if [ $? -eq 0 ]; then
		return 0
	else
		echo "$host never came up"
		power_cycle $host1

	fi
	check_up $host $uptime
	if [ $? -eq 0]; then
		return 0
	else
		echo "$host never came back after power cycle"
		return 1
	fi
}


function reboot_hosts {
	local hosts=$@
	for h in $hosts; do
		do_reboot $h &
	done

	wait #wait for all reboots to finish
}

function handle_opafm {
	hosts=$@
	echo "Using $fmhost..."
	echo "IFS version is $ifs_vers"
	golden_file="/nfs/sc/disks/fabric_files/scratch/fdosv/wfr/opafm.xml_992.$ifs_vers"

	ssh root@$fmhost "[ -e /etc/sysconfig/opafm.xml.rpmnew ]"
	if [ $? -eq 0 ]; then
		echo "Manually patch up opafm.xml file and press ENTER when done"
		echo "Make the following changes:"
		echo "First move opafm.xml.rpmnew to opafm.xml, then edit:"
		echo "        disable FE"
		echo "        enable loopback mode"
		echo "        change min supported VLS to 1"
		echo "Then copy file to: $golden_file"
		echo "Remove old symlink: rm /nfs/sc/disks/fabric_files/scratch/fdosv/wfr/opafm.xml"
		echo "Finaly make symlink:  ln -s $golden_file /nfs/sc/disks/fabric_files/scratch/fdosv/wfr/opafm.xml"
		echo "Press ENTER when done"
		read ok
	fi

	echo "Remove RPM save files on hosts"
	for h in $hosts; do
		ssh root@$h "rm -f /etc/sysconfig/opafm.xml.rpmnew"
	done

	echo "Copying golden XML file"
	for h in $hosts; do
		ssh root@$h "cp $golden_file /etc/sysconfig/opafm.xml"
		if [ $? -ne 0 ]; then
			echo "Could not update $h opafm.xml"
			exit 1
		fi
	done

}

function update_prr {
	#echo "Updating PRR use $ifs_vers as version when prompted"
	#echo "First password asked for is root: P@ssw0rd"
	#echo "Then enter your normal login/password at the prompts"
	#ssh root@$prrhost "cd /root/prr && ./get_and_run_prr.sh"
	echo "Log into $prrhost and run get_and_run_prr.sh"
	echo "Press ENTER to continue when done"
	read ok
}

function fun_reg {
	local hosts=$@
	./regression_runner hw $hosts 1 > $
}

hostlist="$test1 $test2 $test3"

echo "Running updates and tests on: $hostlist"
echo "Press ENTER to continue"
read ok

update_ifs
update_driver_rpm
do_updates $hostlist
reboot_hosts $hostlist
handle_opafm $hostlist
update_prr 

cd tests && ./build.sh
cd ..

./regression_runner.sh hw $test1 1 1 > test1.out &
./regression_runner.sh hw $test2 1 0 > test2.out &
./regression_runner.sh hw $test3 1 0 > test3.out &

echo "Waiting for tests"
wait

echo "Moving test logs"
mv full-reg-test-* /nfs/sc/disks/fabric_mirror/scratch/wfr_reg_test/

TID=`date "+%Y%m%d"`
DST="992_$ifs_vers"
test1_tag="$test1_tag-$DST-$TID"
test2_tag="$test2_tag-$DST-$TID"
test3_tag="$test3_tag-$DST-$TID"
echo "Press Enter to tag repo with:"
echo $test1_tag
echo $test2_tag
echo $test3_tag
read ok
git tag $test1_tag && git push origin $test1_tag
git tag $test2_tag && git push origin $test2_tag
git tag $test3_tag && git push origin $test3_tag
