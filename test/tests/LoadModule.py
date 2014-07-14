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

def load_driver(host, driver_name, driver_path, driver_opts):
    cmd = "/sbin/insmod " + driver_path + " " + driver_opts
    out = do_ssh(host, cmd)
    loaded = is_driver_loaded(host, driver_name)
    return loaded

def is_sm_active(host, sm):
    cmd = "/sbin/service %s status" % sm
    ret = host.send_ssh(cmd, 0)
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

    # only supporting simics right now
    if test_info.is_simics:
        driver_name = "hfi"
        driver_file = "hfi.ko"
        driver_path = "/host" + test_info.get_hfi_src() + "/" + driver_file
        driver_parms = test_info.get_mod_parms()
        sm = test_info.which_sm()

        opensm_host = None
        ifs_fm_host = None
        if sm != "none":
            RegLib.test_log(0, "Trying to determine sm")

            for host in host1, host2:
                active = is_sm_active(host, "opensm")
                if active == 0:
                    if opensm_host == None:
                        opensm_host = host
                    else:
                        RegLib.test_fail("OpenSM detected on both nodes")

                active = is_sm_active(host, "ifs_fm")
                if active == 0:
                    if ifs_fm_host == None:
                        ifs_fm_host = host
                    else:
                        RegLib.test_fail("ifs_fm detected on both nodes")

            if opensm_host != None and ifs_fm_host != None:
                RegLib.test_fail("Both ifs_fm and open_sm detected")
            
            if opensm_host == None and ifs_fm_host == None:
                RegLib.test_log(0, "Neither ifs_fm nor open sm detected will try after loading driver")

            if opensm_host != None:
                RegLib.test_log(0, "Using %s as the open sm host" % opensm_host.get_name())

            if ifs_fm_host != None:
                RegLib.test_log(0, "Using %s as the ifs_fm host" % ifs_fm_host.get_name())

        # Make sure no fm/sm is running now
        RegLib.test_log(0, "Stopping all sm on all hosts")
        for host in host1, host2:
            for my_sm in "opensm", "ifs_fm":
                stop_sm(host, my_sm)
                active = is_sm_active(host, my_sm)
                if active == 0:
                    RegLib.test_fail("%s is still running on host" % my_sm)
                RegLib.test_log(0, "%s not found to be running" % my_sm)

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
            loaded = load_driver(host, driver_name, driver_path, driver_parms)
            if loaded == True:
                RegLib.test_log(0, name + " Driver loaded successfully")
            else:
                RegLib.test_fail(name + " Could not load driver")

        if sm == "none":
            RegLib.test_pass("Driver loaded sm/fm not running")

        if sm == "ifs_fm":
            if ifs_fm_host == None:
                ifs_fm_host = host1
            RegLib.test_log(0, "Starting ifs_fm on %s" % ifs_fm_host.get_name())
            start_sm(ifs_fm_host, "ifs_fm")
            active = is_sm_active(ifs_fm_host, "ifs_fm")
            if active != 0:
                RegLib.test_fail("Could not start ifs_fm")

        if sm == "opensm":
            if opensm_host == None:
                opensm_host = host1
            RegLib.test_log(0, "Starting opensm on %s" % opensm_host.get_name())
            start_sm(opensm_host, "opensm")
            active = is_sm_active(opensm_host, "opensm")
            if active != 0:
                RegLib.test_fail("Could not start opensm")

        if sm == "detect":
            RegLib.test_log(0, "Determining where to start SM")
            if opensm_host != None:
                RegLib.test_log(0, "Starting opensm on %s" % opensm_host.get_name())
                start_sm(opensm_host, "opensm")
                active = is_sm_active(opensm_host, "opensm")
                if active == 0:
                    RegLib.test_log(0, "opensm running on %s" % opensm_host.get_name())
                else:
                    RegLib.test_fail("Could not start opensm")
            elif ifs_fm_host != None:
                RegLib.test_log(0, "Starting ifs_fm on %s" % ifs_fm_host.get_name())
                start_sm(ifs_fm_host, "ifs_fm")
                active = is_sm_active(ifs_fm_host, "ifs_fm")
                if active == 0:
                    RegLib.test_log(0, "ifs_fm running on %s" % ifs_fm_host.get_name())
                else:
                    RegLib.test_fail("Could not start ifs_fm")
            else:
                ifs_fm_host = host1
                RegLib.test_log(0, "Starting ifs_fm on %s" % ifs_fm_host.get_name())
                start_sm(ifs_fm_host, "ifs_fm")
                active = is_sm_active(ifs_fm_host, "ifs_fm")
                if active == 0:
                    RegLib.test_log(0, "ifs_fm running on %s" % ifs_fm_host.get_name())
                else:
                    opensm_host = host1
                    ifs_fm_host = None
                    RegLib.test_log(0, "Starting opensm on %s" % opensm_host.get_name())
                    start_sm(opensm_host, "opensm")
                    active = is_sm_active(opensm_host, "opensm")
                    if active == 0:
                        RegLib.test_log(0, "opensm running on %s" % opensm_host.get_name())
                    else:
                        RegLib.test_fail("Could not start any sm")

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

        RegLib.test_pass("Driver loaded, adapters up SM running.")

    else:
        RegLib.test_fail("Only simics supported right now")

if __name__ == "__main__":
    main()

