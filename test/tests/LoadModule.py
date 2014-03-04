#!/usr/bin/python

# Test loading the hfi driver
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re

def do_ssh(host, cmd):
    RegLib.test_log(5, "Running " + cmd)
    (err, out) = host.send_ssh(cmd)
    if err:
        RegLib.test_log(10, "SSH ERROR (may be expected!)")
    return out

def is_driver_loaded(host, driver_name):
    cmd = "/sbin/lsmod"
    out = do_ssh(host, cmd)
    loaded = 0
    for line in out:
        if driver_name in line:
            return True
    return False

def unload_driver(host, driver_name):
    cmd = "/sbin/rmmod " + driver_name
    do_ssh(host, cmd)
    loaded = is_driver_loaded(host, driver_name)
    return not loaded

def load_driver(host, driver_name, driver_path):
    cmd = "/sbin/insmod " + driver_path
    out = do_ssh(host, cmd)
    loaded = is_driver_loaded(host, driver_name)
    return loaded

def is_opensm_active(host):
    cmd = "/sbin/service opensm status"
    out = do_ssh(host, cmd)
    for line in out:
        if "is running" in line:
            return True
    return False

def start_opensm(host):
    cmd = "/sbin/service opensm start"
    out = do_ssh(host, cmd)
    return

def restart_opensm(host):
    cmd = "/sbin/service opensm restart"
    out = do_ssh(host, cmd)
    return

def stop_opensm(host):
    cmd = "/sbin/service opensm stop"
    out = do_ssh(host, cmd)
    return

def wait_for_active(host, timeout, attempts):
    for iter in range(attempts):
        RegLib.test_log(0, "Checking for active")
        cmd = "/usr/sbin/ibportstate -D 0 query"
        out = do_ssh(host, cmd)
        for line in out:
            #print line
            matchObj = re.match(r"LinkState.+Active", line)
            if matchObj:
               return 0 #no error

        RegLib.test_log(0, "Not active yet trying again in a few seconds")
        time.sleep(timeout)

    return 1 #error if we got to here

        # Sample output if we want to check for more details later:
        #[root@viper0 ~]# ibportstate -D 0 query
        #CA PortInfo:
        #        Port info: DR path slid 65535; dlid 65535; 0 port 0
        #LinkState:.......................Active
        #PhysLinkState:...................LinkUp
        #Lid:.............................1
        #SMLid:...........................1
        #LMC:.............................0
        #LinkWidthSupported:..............1X or 4X
        #LinkWidthEnabled:................1X or 4X
        #LinkWidthActive:.................4X
        #LinkSpeedSupported:..............undefined (0) (IBA extension)
        #LinkSpeedEnabled:................undefined (0) (IBA extension)
        #LinkSpeedActive:.................5.0 Gbps

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()

    RegLib.test_log(0, "Test: LoadModule.py started")
    RegLib.test_log(0, "Dumping test parameters")

    # Dump out the test and host info. We need 2 hosts for this test
    print test_info

    count = test_info.get_host_count()
    if count < 2:
        RegLib.test_fail("Not enough hosts")

    print "Found ", count, "hosts"
    host1 = test_info.get_host_record(0)
    print host1

    host2 = test_info.get_host_record(1)
    print host2

    ################
    # body of test #
    ################

    # only supporting simics right now
    if test_info.is_simics:
        driver_name = "hfi"
        driver_file = "hfi.ko"
        driver_path = "/host" + test_info.get_hfi_src() + "/" + driver_file

        # Go ahead and get the driver loaded or reloaded on both hosts
        for host in host1,host2:
            name = host.get_name()
            loaded = is_driver_loaded(host, driver_name)
            if loaded == True:
                RegLib.test_log(0, name + " Need to remove driver first")
                removed = unload_driver(host, driver_name)
                if removed == False:
                    RegLib.test_fail(name + " Could not remove HFI driver")
            RegLib.test_log(0, name + " Loading Driver")
            loaded = load_driver(host, driver_name, driver_path)
            if loaded == True:
                RegLib.test_log(0, name + " Driver loaded successfully")
            else:
                RegLib.test_fail(name + " Could not load driver")

        # Now that the driver is loaded make sure one of the hosts is running
        # opensm if none are then start it on host1.
        opensmhost = None
        for host in host1,host2:
            name = host.get_name()
            opensm = is_opensm_active(host)
            if opensm == True:
                if opensmhost:
                    RegLib.test_fail(name + " OpenSM detected on both nodes!")
                else:
                    opensmhost = host

        if opensmhost == None:
            opensmhost = host1
            start_opensm(host1)
            if is_opensm_active(host1) == True:
                RegLib.test_log(0, "Driver loaded and opensm started")
            else:
                RegLib.test_fail("Could not start opensm")
        else:
            # if we do not restart opensm it can take a long time for it to
            # notice the driver has been reloaded and bring it up so don't wait
            # restarting the service forces a rescan
            RegLib.test_log(0, "Driver loaded and opensm running restarting")
            restart_opensm(opensmhost)
            if is_opensm_active(opensmhost) == True:
                RegLib.test_log(0, "Open SM reloaded")
            else:
                RegLib.test_fail("Could not reload opensm")

        # Driver loaded and opensm ready now wait till links are active on both
        # nodes.
        num_loaded = 0
        for host in host1,host2:
            name = host.get_name()
            err = wait_for_active(host, 10, 2)
            if err:
                RegLib.test_log(0, name + " Could not reach active state")
                RegLib.test_log(0, name + " Attempting to restart opensm")
                restart_opensm(opensmhost)
                if is_opensm_active(opensmhost) == True:
                    RegLib.test_log(0, name + " Open SM reloaded")
                else:
                    RegLib.test_fail(name + " Could not reload opensm")
            else:
                RegLib.test_log(0, name + " Adapter is up and running")
                num_loaded = num_loaded + 1

        if num_loaded != 2:
            RegLib.test_fail(name + " Unable to get active state on at least 1 node")

        RegLib.test_pass("Driver loaded, adapters up, openSM running.")

    else:
        RegLib.test_fail("Only simics supported right now")

if __name__ == "__main__":
    main()

