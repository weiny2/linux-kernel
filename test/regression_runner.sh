#!/bin/bash

#-------------
# User defined
#-------------
#test_dir="/nfs/site/proj/fabric/mirror/scratch/wfr_reg_test"
test_dir=$PWD
update_driver=0
update_diags=0
update_psm=0
copy_driver=0
copy_diags=0
copy_psm=0
driver="/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-driver"
psm="/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-psm"
diags="/nfs/sc/disks/fabric_work/$USER/wfr/weekly-reg/wfr-diagtools-sw"
mode=$1
rpm_test=0
quick_test=1
default_test=1
mgmt_test=1
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
ssh_fpga_host1="ssh root@fpga2"
ssh_fpga_host2="ssh root@fpga3"
fpga_host1="fpga2"
fpga_host2="fpga3"
scp_fpga_host1="scp root@fpga2:"
scp_fpga_host2="scp root@fpga3:"
fpga_args="--nodelist \"fpga2,fpga3\" --modparm \"fw_8051_load=1\""

#------------------------
# Internal test variables
#------------------------
logdir=$test_dir/"full-reg-test"`date +%Y%m%d`
host1_ssh=""
host2_ssh=""
host1_scp=""
host2_scp=""
host1=""
host2=""
std_args=""
local_repo_dir="/root/wfr-reg-repos"
local_driver="$local_repo_dir/wfr-driver"
local_diags="$local_repo_dir/wfr-diags"
local_psm="$local_repo_dir/wfr-psm"

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

function update_repos {
	echo ""
	echo "--------------"
	echo "Updating Repos"
	echo "--------------"
	if [ $update_driver -eq 1 ]; then
		echo "Updating driver"
		update_repo $driver
	else
		echo "Skipping driver update"
	fi

	echo ""
	if [ $update_diags -eq 1 ]; then
		echo "Updating sw diags"
		update_repo $diags
	else
		echo "Skipping sw diags update"
	fi
	
	echo ""
	if [ $update_psm -eq 1 ]; then
		echo "Updating psm"
		update_repo $psm
	else
		echo "Skipping psm update"
	fi

	echo ""
}

function dump_change_levels {
	echo "---------------------"
	echo "Getting change levels"
	echo "---------------------"
	echo "Driver:"
	dump_change_level $driver
	echo ""
	echo "Diags:"
	dump_change_level $diags
	echo ""
	echo "PSM:"
	dump_change_level $psm
	echo ""
}

function dump_change_level {
	repo=$1
	run_cmd_fatal cd $repo
	echo "Checking repo: $repo"
	run_cmd_fatal "git describe --tags --long --match='v*' HEAD"
	run_cmd_fatal "git log -1 HEAD --pretty=\"%cd <%ae>%n- %s%n\""
	run_cmd_fatal cd $test_dir
}

function update_repo {
	repo=$1
	echo "Going to update $repo"
	run_cmd_fatal cd $repo
	run_cmd_fatal git remote update
	run_cmd_fatal git pull --rebase
	run_cmd_fatal cd $test_dir
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
	echo "Update flags"
	echo -e "\tDriver = $update_driver"
	echo -e "\tDiags = $update_diags"
	echo -e "\tPsm = $update_psm"
	echo "Copy flags"
	echo -e "\tDriver = $copy_driver"
	echo -e "\tDiags = $copy_diags"
	echo -e "\tPsm = $copy_psm"
	echo "Repo locations"
	echo -e "\tDriver = $driver"
	echo -e "\tDiags = $diags"
	echo -e "\tPsm = $psm"
	echo -e "\tLocal Driver = $local_driver"
	echo -e "\tLocal Diags = $local_diags"
	echo -e "\tLocal Psm = $local_psm"
	echo "Log directory = $logdir"
	echo "SSH/SCP"
	echo -e "\tHost 1 SSH = $host1_ssh"
	echo -e "\tHost 2 SSH = $host2_ssh"
	echo -e "\tHost 1 SCP = $host1_scp"
	echo -3 "\tHost 2 SCP = $host2_scp"

	if [[ $mode == "simics" ]]; then
		dump_simics_config
	elif [[ $mode == "fpga" ]]; then
		dump_fpga_config
	else
		echo "Unsupported mode"
		exit 1
	fi
	echo ""
}

function dump_modparms {
	echo ""
	echo "-------------------"
	echo "Mod Parms on $host1"
	echo "-------------------"
	run_cmd_fatal "$host1_ssh 'for i in \$(ls /sys/module/hfi/parameters); do echo \$i; cat /sys/module/hfi/parameters/\$i; done'"
	echo ""
	echo "-------------------"
	echo "Mod Parms on $host2"
	echo "-------------------"
	run_cmd_fatal "$host2_ssh 'for i in \$(ls /sys/module/hfi/parameters); do echo \$i; cat /sys/module/hfi/parameters/\$i; done'"
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

function copy_repos {
	echo ""
	echo "----------------------------------"
	echo "Copying needed repos to local disk"
	echo "----------------------------------"
	run_cmd_fatal "$host1_ssh \"mkdir -p $local_repo_dir\""
	run_cmd_fatal "$host2_ssh \"mkdir -p $local_repo_dir\""

	echo "Copying driver"
	copy_repo $driver $local_driver $copy_driver
	echo "Copying diags"
	copy_repo $diags $local_diags $copy_diags
	echo "Copying psm"
	copy_repo $psm $local_psm $copy_psm
}

function copy_repo {
	from_dir=$1
	to_dir=$2
	copy=$3

	if [[ $mode == "simics" ]]; then
		from_dir="/host"$from_dir
	fi

	if [ $copy -eq 0 ]; then
		echo "Skipping copy of $from_dir to $to_dir"
	else
		echo "Copying $from_dir to $to_dir"
		run_cmd_fatal "$host1_ssh \"rm -rf $to_dir\""
		run_cmd_fatal "$host2_ssh \"rm -rf $to_dir\""
		run_cmd_fatal "$host1_ssh \"cp -r $from_dir $to_dir\""
		run_cmd_fatal "$host2_ssh \"cp -r $from_dir $to_dir\""
	fi

}

function do_rpm_test {
	echo ""
	echo "----------------------"
	echo "Running RPM build test"
	echo "----------------------"
	if [ $rpm_test -eq 1 ]; then
		run_cmd_fatal "$host1_ssh \"cd $local_driver/test/tests && ./RpmTest.py --nodelist $host1 --psm $local_psm --sw-diags $local_diags\""
		run_cmd_fatal "$host2_ssh \"cd $local_driver/test/tests && ./RpmTest.py --nodelist $host2 --psm $local_psm --sw-diags $local_diags\""
	else
		echo "Skipping RPM Test"
	fi

	echo ""
	echo "Checking RPM verions for $host1:"
	run_cmd_fatal "$host1_ssh \"rpm -qa | grep hfi-\""

	echo ""
	echo "Checking RPM verions for $host2:"
	run_cmd_fatal "$host2_ssh \"rpm -qa | grep hfi-\""
	
}

function do_hipri_tests {
	echo ""
	echo "---------------------------------------------"
	echo "Running high priority tests. If failure abort."
	echo "---------------------------------------------"

	run_cmd_fatal cd $driver/test/tests
	run_cmd_fatal ./build.sh
	run_cmd_fatal ./LoadModule.py $std_args
	dump_modparms
	run_cmd_fatal cd $driver/test
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
	run_cmd_fatal cd $driver/test
	
	if [ $default_test -eq 1 ]; then
		run_cmd ./harness.py --type default $std_args
	else
		echo "Skipping default tests"
	fi

	if [ $mgmt_test -eq 1 ]; then
		run_cmd ./harness.py --type mgmt $std_args
	else
		echo "Skipping mgmt tetss"
	fi

	if [ $snoop_test -eq 1 ]; then
		run_cmd ./harness.py --type snoop $std_args
	else
		echo "Skipping snoop tests"
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

# The first thing we need to do is update the repos
update_repos
dump_change_levels

# Mode specific things like setting time and copying repos
do_mode_specific_pretest

# Dump our test knobs
dump_config

# Need some of the repos on the local host depending on the test type
copy_repos

# Now build and install PSM and sw diags fail if can't
do_rpm_test

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

