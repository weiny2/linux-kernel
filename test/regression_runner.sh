#!/bin/bash

#-------------
# User defined
#-------------
test_dir=$PWD
mode=$1
host1=$2
host2=$3
nobuild=$4
rpm_test=0
quick_test=1
default_test=1
mgmt_test=1
perf_test=1
snoop_test=1

# Simics specific
ssh_viper0="ssh root@localhost -p 4022 -i /nfs/site/home/$USER/.ssh/simics_rsa -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"
ssh_viper1="ssh root@localhost -p 5022 -i /nfs/site/home/$USER/.ssh/simics_rsa -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"
scp_viper0="scp -P 4022 -i /nfs/site/home/$USER/.ssh/simics_rsa -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@localhost:"
scp_viper1="scp -P 5022 -i /nfs/site/home/$USER/.ssh/simics_rsa -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@localhost:"
craff_file="/scratch/$USER/simics/craff"
simics_file="/scratch/$USER/simics/workspace/targets/x86-x58-ich10/viper-rhel-wfr.simics"
viper0_console="/scratch/$USER/simics/workspace/viper0.console"
viper1_console="/scratch/$USER/simics/workspace/viper1.console"
simics_console="/scratch/$USER/simics/workspace/simics.console"
simics_host1="viper0"
simics_host2="viper1"
simics_args="--nodelist \"viper0,viper1\""

# FPGA specific
ssh_fpga_host1="ssh root@$host1"
ssh_fpga_host2="ssh root@$host2"
fpga_host1="$host1"
fpga_host2="$host2"
scp_fpga_host1="scp root@$host1:"
scp_fpga_host2="scp root@$host2:"
fpga_args="--nodelist \"$host1,$host2\""

#------------------------
# Internal test variables
#------------------------
logdir=$test_dir/"full-reg-test-${host1}-${host2}-"`date +%Y%m%d`
host1_ssh=""
host2_ssh=""
host1_scp=""
host2_scp=""
host1=""
host2=""
std_args=""

#-----------------
# Helper functions
#-----------------
function run_cmd_fatal {
	echo "Running:: $@"
	eval "$@"
	if [ $? -ne 0 ]; then
		echo "FULL-REG-STATUS :: FATAL ERR :: CMD FAIL: $@"
		exit 1
	fi
}

function run_cmd {
	echo "Running:: $@"
	eval "$@"
	if [ $? -ne 0 ]; then
		echo "FULL-REG-STATUS :: CMD FAIL: $@"
	fi
}

function drop_tag {
	repo=$1
	tag="regtest-"`date +%Y%m%d`
	run_cmd_fatal cd $repo
	echo "Going to tag $repo with $tag"
	run_cmd_fatal git tag $tag
	run_cmd_fatal git push origin $tag
	run_cmd_fatal cd $test_dir
}

function dump_config {
	echo ""
	echo "---------------"
	echo "Basic Settings:"
	echo "---------------"
	echo "Host 1 = $host1"
	echo "Host 2 = $host2"
	echo "Standard Args = $std_args"
	echo "Test Dir = $test_dir"
	echo "Test mode = $mode"
	echo "Rpm Test? = $rpm_test"
	echo "Quick Test? = $quick_test"
	echo "Default Test? = $default_test"
	echo "Mgmt Test? = $mgmt_test"
	echo "Snoop Test? = $snoop_test"
	echo "Log directory = $logdir"
	echo "SSH/SCP"
	echo -e "\tHost 1 SSH = $host1_ssh"
	echo -e "\tHost 2 SSH = $host2_ssh"
	echo -e "\tHost 1 SCP = $host1_scp"
	echo -e "\tHost 2 SCP = $host2_scp"

	if [[ $mode == "simics" ]]; then
		dump_simics_config
	elif [[ $mode == "fpga" ]]; then
		dump_fpga_config
	else
		echo "Unsupported mode"
		exit 1
	fi
	echo ""

	echo "Host 1 software versions":
	run_cmd_fatal $host1_ssh "rpm -qa | grep hfi1-psm"
	run_cmd_fatal $host1_ssh "rpm -qa | grep hfi1-diagtools"
	run_cmd_fatal $host1_ssh "rpm -qa | grep openmpi | grep \"2a1\|1.8.5\""

	echo ""

	echo "Host 2 software versions":
	run_cmd_fatal $host2_ssh "rpm -qa | grep hfi1-psm"
	run_cmd_fatal $host2_ssh "rpm -qa | grep hfi1-diagtools"
	run_cmd_fatal $host2_ssh "rpm -qa | grep openmpi | grep \"2a1\|1.8.5\""

	echo ""

	echo "Current driver software version:"
	run_cmd_fatal "git describe --tags --long --match='v*' HEAD"
	
	echo ""

	echo "Last 10 commits:"
	run_cmd_fatal "git log -10 HEAD --pretty=\"%cd <%ae>%n- %s%n\""	
	echo ""
}

function dump_modparms {
	echo ""
	echo "-------------------"
	echo "Mod Parms on $host1"
	echo "-------------------"
	run_cmd_fatal "$host1_ssh 'for i in \$(ls /sys/module/hfi1/parameters); do echo \$i; cat /sys/module/hfi1/parameters/\$i; done'"
	echo ""
	echo "-------------------"
	echo "Mod Parms on $host2"
	echo "-------------------"
	run_cmd_fatal "$host2_ssh 'for i in \$(ls /sys/module/hfi1/parameters); do echo \$i; cat /sys/module/hfi1/parameters/\$i; done'"
	echo ""
}

function dump_fpga_config {
	echo ""
	echo "-------------"
	echo "FPGA settings"
	echo "-------------"
	echo -e "\tFPGA host 1 SSH = $ssh_fpga_host1"
	echo -e "\tFPGA host 2 SSH = $ssh_fpga_host2"
	echo -e "\tFPGA host 1 SCP = $scp_fpga_host1"
	echo -e "\tFPGA host 2 SCP = $scp_fpga_host2"
}

function dump_simics_config {
	echo ""
	echo "---------------"
	echo "Simics settings"
	echo "---------------"
	echo -e "\tViper 0 SSH = $ssh_viper0"
	echo -e "\tViper 1 SSH = $ssh_viper1"
	echo -e "\tViper 0 SCP = $scp_viper0"
	echo -e "\tViper 1 SCP = $scp_viper1"
	echo -e "\tcraff_file = $craff_file"
	echo -e "\tsimcis_file = $simics_file"
	echo -e "\tViper 0 console = $viper0_console"
	echo -e "\tViper 1 console = $viper1_console"
	echo -e "\tSimics console = $simics_console"
	
	echo "Using craff:"
	run_cmd_fatal ls -l $craff_file

	echo "Simics settings:"
	run_cmd_fatal cat $simics_file
}

function do_mode_specific_pretest {
	echo ""
	echo "--------------------------------"
	echo "Doing mode specific pretest work"
	echo "--------------------------------"
	if [[ $mode == "simics" ]]; then
		do_simics_pretest
	elif [[ $mode == "fpga" ]]; then
		do_fpga_pretest
	else
		echo "Unsupported mode"
		exit 1
	fi
}

function do_mode_specific_posttest {
	echo ""
	echo "---------------------------------"
	echo "Doing mode specific posttest work"
	echo "---------------------------------"
	if [[ $mode == "simics" ]]; then
		do_simics_posttest
	elif [[ $mode == "fpga" ]]; then
		do_fpga_posttest
	else
		echo "Unsupported mode"
		exit 1
	fi
}

function do_simics_pretest {
	echo "Doing simics pretest"
	
	host1_ssh=$ssh_viper0
	host2_ssh=$ssh_viper1
	host1_scp=$scp_viper0
	host2_scp=$scp_viper1
	host1=$simics_host1
	host2=$simics_host2
	std_args=$simics_args
	# Set the time on the vipers
	echo "Setting date/time on vipers"
	cur_date=`date "+%Y-%m-%d %H:%M:%S %Z"`
	run_cmd_fatal "$host1_ssh \"date --set '$cur_date'\"" 
	run_cmd_fatal "$host2_ssh \"date --set '$cur_date'\""
}

function do_fpga_pretest {
	echo "Doing fpga pretest"

	host1_ssh=$ssh_fpga_host1
	host2_ssh=$ssh_fpga_host2
	host1_scp=$scp_fpga_host1
	host2_scp=$scp_fpga_host2
	host1=$fpga_host1
	host2=$fpga_host2
	std_args=$fpga_args
}

function do_simics_posttest {

	echo "Doing simics post test"

	run_cmd "$host1_ssh \"tar cvzf viper0_var_log.tgz /var/log/\""
	run_cmd "$host2_ssh \"tar cvzf viper1_var_log.tgz /var/log/\""
	run_cmd "$host1_scp\"/root/viper0_var_log.tgz\" $logdir"
	run_cmd "$host2_scp\"/root/viper1_var_log.tgz\" $logdir"

	run_cmd cp $viper0_console $logdir/
	run_cmd cp $viper1_console $logdir/
	run_cmd cp $simics_console $logdir/
}

function do_fpga_posttest {
	echo "FPGA - Nothing to do post test."
}

function do_hipri_tests {
	echo ""
	echo "---------------------------------------------"
	echo "Running high priority tests. If failure abort."
	echo "---------------------------------------------"

	run_cmd_fatal cd $test_dir/tests
	if [ $nobuild -eq 1 ]; then
		echo "Skipping build test due to argument"
	else
		run_cmd_fatal ./build.sh
	fi
	run_cmd_fatal ./LoadModule.py $std_args
	dump_modparms
	run_cmd_fatal cd $test_dir
	if [ $quick_test -eq 1 ]; then
		run_cmd_fatal ./harness.py --type quick $std_args
	else
		echo "Skipping quick tests"
	fi
	run_cmd_fatal cd $test_dir
}

function do_lopri_tests {

	echo ""
	echo "-----------------------------------------------"
	echo "Running low priority tests. Continue if failure"
	echo "-----------------------------------------------"
	run_cmd_fatal cd $test_dir
	
	if [ $default_test -eq 1 ]; then
		run_cmd ./harness.py --type default $std_args
	else
		echo "Skipping default tests"
	fi

	if [ $mgmt_test -eq 1 ]; then
		run_cmd ./harness.py --type mgmt $std_args
	else
		echo "Skipping mgmt tests"
	fi

	if [ $perf_test -eq 1 ]; then
		run_cmd ./harness.py --type perf $std_args
	else
		echo "Skipping perf tests"
	fi

	run_cmd_fatal cd $test_dir
}

#
# Main
#

if [[ $mode == "simics" ]]; then
	echo "Running in simics mode"
elif [[ $mode == "fpga" ]]; then
	echo "Running in FPGA mode"
else
	echo "Must pass 'simics' or 'fpga' as the mode"
	exit 0
fi

# Create the test directory, switch to it and start logging
run_cmd_fatal mkdir $logdir
exec > >(tee $logdir/test_output.txt)
exec 2>&1
echo "Starting test: in $logdir saving output in test_output.txt"
echo "Starting full regression test at " `date`

# Mode specific things like setting time and copying repos
do_mode_specific_pretest

# Dump our test knobs
dump_config

# Require a sub set of tests to pass before continuing
# If any of these fail the test aborts
do_hipri_tests

# If any of these tests fail just note the failure and continue
do_lopri_tests

do_mode_specific_posttest

echo "FULL-REG-STATUS :: Test Completed"
echo ""
echo "===================="
echo "Dumping Test Summary"
echo "===================="
echo ""
grep FULL-REG-STATUS $logdir/test_output.txt
echo ""
grep FAIL $logdir/test_output.txt
grep PASS $logdir/test_output.txt
echo ""
exit 0

