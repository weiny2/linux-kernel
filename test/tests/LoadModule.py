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
    (err, out) = host.send_ssh(cmd, run_as_root=True)
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

def load_driver(host, driver_name, driver_path, driver_opts):
    cmd = "/sbin/insmod " + driver_path + " " + driver_opts
    out = do_ssh(host, cmd)
    loaded = is_driver_loaded(host, driver_name)
    return loaded

def is_sm_active(host, sm):
    cmd = "/sbin/service %s status" % sm
    ret = host.send_ssh(cmd, 0, run_as_root=True)
    if ret != 0:
        RegLib.test_log(0, "%s is not running" % sm)
    else:
        RegLib.test_log(0, "%s is running" % sm)
    
    return ret

def start_sm(host, sm):
    cmd = "/sbin/service %s start" % sm
    out = do_ssh(host, cmd)
    return

def restart_sm(host, sm):
    cmd = "/sbin/service %s restart" % sm
    out = do_ssh(host, cmd)
    return

def stop_sm(host, sm):
    cmd = "/sbin/service %s stop" % sm
    out = do_ssh(host, cmd)
    return

def wait_for_active(host, timeout, attempts):
    for iter in range(attempts):
        RegLib.test_log(0, "Checking for LinkUp")
        cmd = "cat /sys/class/infiniband/hfi0/ports/1/phys_state"
        out = do_ssh(host, cmd)
        for line in out:
            matchObj = re.match(r"5: LinkUp", line)
            if matchObj:
                RegLib.test_log(0, "LinkUp")
                cmd = "cat /sys/class/infiniband/hfi0/ports/1/state"
                out = do_ssh(host, cmd)
                for line in out:
                    matchObj = re.match(r"4: ACTIVE", line)
                    if matchObj:
                        RegLib.test_log(0, "Active")
                        return 0
        RegLib.test_log(0, "Not active yet trying again in a few seconds")
        time.sleep(timeout)

    return 1 #error if we got to here

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

    driver_name = "hfi"
    driver_file = "hfi.ko"
    driver_path = test_info.get_hfi_src() + "/" + driver_file
    if test_info.is_simics() == True:
        driver_path = "/host" + driver_path
    
    driver_parms = test_info.get_mod_parms()
    sm = test_info.which_sm()

    matchObj = re.match(r"(.*):(.*)", driver_parms)
    host1_parms = ""
    host2_parms = ""
    if matchObj:
        RegLib.test_log(5, "Using new style modparms");
        host1_parms = matchObj.group(1)
        host2_parms = matchObj.group(2)
    else:
        RegLib.test_log(5, "Using old style modparms");
        host1_parms = driver_parms
        host2_parms = driver_parms

    host1_parms = host1_parms.strip()
    host2_parms = host2_parms.strip()
    RegLib.test_log(5, "host1 parms: %s" %host1_parms)
    RegLib.test_log(5, "host2 parms: %s" %host2_parms)
      
    ifs_fm_host = None
    
    if sm != "none":
        RegLib.test_log(0, "Trying to determine which node the fm is running on")
        for host in host1, host2:
            active = is_sm_active(host, "ifs_fm")
            if active == 0:
                if ifs_fm_host == None:
                    ifs_fm_host = host
                else:
                    RegLib.test_fail("ifs_fm detected on both nodes")

        if ifs_fm_host != None:
            RegLib.test_log(0, "Using %s as the ifs_fm host" % ifs_fm_host.get_name())

            # Make sure no fm is running now
            RegLib.test_log(0, "Stopping fm on all hosts")
            stop_sm(ifs_fm_host, "ifs_fm")
            active = is_sm_active(host, "ifs_fm")
            if active == 0:
                RegLib.test_fail("fm is still running on host")
            RegLib.test_log(0, "fm not found to be running")

            #RegLib.test_log(0, "Waiting 15 seconds for ifs_fm to quiesce")
            #time.sleep(15)

    # Go ahead and get the driver loaded or reloaded on both hosts
    for host in host1,host2:
        name = host.get_name()
        loaded = is_driver_loaded(host, driver_name)
        if loaded == True:
            RegLib.test_log(0, name + " Need to remove driver first")
            removed = unload_driver(host, driver_name)
            if removed == False:
                RegLib.test_fail(name + " Could not remove HFI driver")
        
        #RegLib.test_log(0, "Sleeping 10 seconds after removing driver to let things settle")
        #time.sleep(10)

        RegLib.test_log(0, name + " Loading Driver")
        if host == host1:
            driver_parms = host1_parms
        else:
            driver_parms = host2_parms

        loaded = load_driver(host, driver_name, driver_path, driver_parms)
        if loaded == True:
            RegLib.test_log(0, name + " Driver loaded successfully")
        else:
            RegLib.test_fail(name + " Could not load driver")

        #RegLib.test_log(0, "Sleeping 10 seconds after loading driver to let things settle")
        #time.sleep(10)

    if sm == "none":
        RegLib.test_pass("Driver loaded sm/fm not running")

    if ifs_fm_host == None: #chose host1 by default
        ifs_fm_host = host1
    RegLib.test_log(0, "Starting ifs_fm on %s" % ifs_fm_host.get_name())
    start_sm(ifs_fm_host, "ifs_fm")
    active = is_sm_active(ifs_fm_host, "ifs_fm")
    if active != 0:
        RegLib.test_fail("Could not start ifs_fm")

    # Driver loaded and sm ready now wait till links are active on both
    # nodes.
    num_loaded = 0
    for host in host1,host2:
        name = host.get_name()
        err = wait_for_active(host, 10, 6)
        if err:
            RegLib.test_log(0, name + " Could not reach active state")
        else:
            RegLib.test_log(0, name + " Adapter is up and running")
            num_loaded = num_loaded + 1

    if num_loaded != 2:
        RegLib.test_fail(name + " Unable to get active state on at least 1 node")

    # Before bailing out dump the kmod load address in case we need to do some
    # debugging later.
    for host in host1,host2:
        print host.get_name(), "module load address is:"
        cmd = "cat /sys/module/hfi/sections/.init.text"
        (err, out) = do_ssh(host, cmd)
        print out

    RegLib.test_pass("Driver loaded, adapters up SM running.")

if __name__ == "__main__":
    main()

