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
    attempts = 3
    while (attempts > 0):
        cmd = "/sbin/rmmod " + driver_name
        do_ssh(host, cmd)
        # give it a couple seconds for the unload to take effect
        time.sleep(10)
        loaded = is_driver_loaded(host, driver_name)
        if loaded == False:
            RegLib.test_log(5, "Driver unloaded")
            return True
        else:
            attempts = attempts - 1
            RegLib.test_log(5, "Could not unload driver")
            RegLib.test_log(5, "Attempts left = %d" % attempts)
            if (attempts != 0):
                RegLib.test_log(5, "Trying again in 30 secs")
                time.sleep(30)
    return False

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
        cmd = "cat /sys/class/infiniband/hfi1_0/ports/1/phys_state"
        out = do_ssh(host, cmd)
        for line in out:
            matchObj = re.match(r"5: LinkUp", line)
            if matchObj:
                RegLib.test_log(0, "LinkUp")
                cmd = "cat /sys/class/infiniband/hfi1_0/ports/1/state"
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

    print "Found ", count, "hosts"
    hostlist = []
    for x in range(count):
        host = test_info.get_host_record(x)
        print host
        hostlist.append(host)

    ################
    # body of test #
    ################

    driver_name = "hfi1"
    driver_file = "hfi1.ko"
    driver_path = test_info.get_hfi_src() + "/" + driver_file
    if test_info.is_simics() == True:
        driver_path = "/host" + driver_path
    
    driver_parms = test_info.get_mod_parms()
    sm = test_info.which_sm()

    matchObj = re.match(r"(.*):(.*)", driver_parms)
    host_parms = ["", ""]
    if matchObj:
        RegLib.test_log(5, "Using new style modparms");
        host_parms[0] = matchObj.group(1)
        host_parms[1] = matchObj.group(2)
    else:
        RegLib.test_log(5, "Using old style modparms");
        host_parms[0] = driver_parms
        host_parms[1] = driver_parms


    host_parms = [x.strip() for x in host_parms]
    for x in range(len(host_parms)):
        RegLib.test_log(5, "host%d parms: %s" % (x, host_parms[x]))
      
    opafm_host = None
   
    if sm == "local":
        RegLib.test_log(0, "Trying to determine which node the fm is running on")
        for host in hostlist:
            active = is_sm_active(host, "opafm")
            if active == 0:
                if opafm_host == None:
                    opafm_host = host
                else:
                    RegLib.test_fail("opafm detected on both nodes")

        if opafm_host != None:
            RegLib.test_log(0, "Using %s as the opafm host" % opafm_host.get_name())

            # Make sure no fm is running now
            RegLib.test_log(0, "Stopping fm on all hosts")
            stop_sm(opafm_host, "opafm")
            active = is_sm_active(host, "opafm")
            if active == 0:
                RegLib.test_fail("fm is still running on host")
            RegLib.test_log(0, "fm not found to be running")

            #RegLib.test_log(0, "Waiting 15 seconds for opafm to quiesce")
            #time.sleep(15)

    # Go ahead and get the driver loaded or reloaded on both hosts
    for host in hostlist:
        name = host.get_name()
        loaded = is_driver_loaded(host, driver_name)
        if loaded == True:
            RegLib.test_log(0, name + " Need to remove driver first")
            removed = unload_driver(host, driver_name)
            if removed == False:
                RegLib.test_fail(name + " Could not remove HFI driver")
                # If we could not remove the driver it is a failure so bail.
                # We gave it 90 seconds if it takes longer something is wonky.
                RegLib.test_fail("Could not remove driver after 90 seconds!")
        
        # Make sure rdma service is running before loading the driver
        cmd = "service rdma start"
        do_ssh(host, cmd)

        RegLib.test_log(0, name + " Loading Driver")
        driver_parms = host_parms[hostlist.index(host)]

        loaded = load_driver(host, driver_name, driver_path, driver_parms)
        if loaded == True:
            RegLib.test_log(0, name + " Driver loaded successfully")
        else:
            RegLib.test_fail(name + " Could not load driver")

        #RegLib.test_log(0, "Sleeping 10 seconds after loading driver to let things settle")
        #time.sleep(10)

    if opafm_host == None: #choose host1 by default
        opafm_host = hostlist[0]

    if sm == "local":
        RegLib.test_log(0, "Starting opafm on %s" % opafm_host.get_name())
        start_sm(opafm_host, "opafm")
        active = is_sm_active(opafm_host, "opafm")
        if active != 0:
            RegLib.test_fail("Could not start opafm")
    elif sm =="remote":
        RegLib.test_log(0, "Not starting SM")  
    else:
        RegLib.test_pass("Driver loading. Ignoring SM")

    # Driver loaded and sm ready now wait till links are active on both
    # nodes.
    num_loaded = 0
    for host in hostlist:
        name = host.get_name()
        err = wait_for_active(host, 10, 20)
        if err:
            RegLib.test_log(0, name + " Could not reach active state")
        else:
            RegLib.test_log(0, name + " Adapter is up and running")
            num_loaded = num_loaded + 1

    if num_loaded != len(hostlist):
        RegLib.test_fail(name + " Unable to get active state on at least 1 node")

    # Before bailing out dump the kmod load address in case we need to do some
    # debugging later. We also need to restart irq balance for best performance.
    for host in hostlist:
        RegLib.test_log(0, "Restarting irq balacne on %s" % host.get_name())
        cmd = "service irqbalance restart"
        out = do_ssh(host, cmd)
        print out
        print host.get_name(), "module load address is:"
        cmd = "cat /sys/module/hfi1/sections/.init.text"
        out = do_ssh(host, cmd)
        print out

    RegLib.test_pass("Driver loaded, adapters up SM running in the fabric.")

if __name__ == "__main__":
    main()

