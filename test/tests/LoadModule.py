#!/usr/bin/python

# Test loading the hfi driver
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os

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
    out = do_ssh(host, "modprobe i2c_algo_bit")
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

def set_ext_lid(host):
    cmd = "/usr/sbin/opaportconfig -L 0x10001"
    out = do_ssh(host, cmd)
    return

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

def wait_for_active(host, timeout, attempts, sys_class_ib):
    for iter in range(attempts):
        RegLib.test_log(0, "Checking for LinkUp")
        cmd = "cat %s/ports/1/phys_state" % sys_class_ib
        out = do_ssh(host, cmd)
        for line in out:
            matchObj = re.match(r"5: LinkUp", line)
            if matchObj:
                RegLib.test_log(0, "LinkUp")
                cmd = "cat %s/ports/1/state" % sys_class_ib
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

    topdir = os.path.dirname(os.path.realpath(__file__))
    
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

    # TODO: Add a flag to RegLib to differentiate between qib and hfi tests
    driver_name = "hfi1"
    driver_file = "hfi1.ko"
    driver_path = ""
    vt_driver_name = "rdmavt"
    vt_driver_file = "rdmavt.ko"
    vt_driver_path = ""
    qib_driver_name = "ib_qib"
    qib_driver_file = "ib_qib.ko"
    sys_class_ib = "/sys/class/infiniband/hfi1_0"
    qib_flag = ""
    linux_src = test_info.get_linux_src()
    hfi_src = test_info.get_hfi_src()
    ext_lid = test_info.get_ext_lid()

    if linux_src != "None":
        RegLib.test_log(0, "Running with full kernel at: %s" % linux_src)

        if test_info.is_qib() == True:
            qib_flag = "--qib"
            driver_path = linux_src + "/drivers/infiniband/hw/qib/" + qib_driver_file
            driver_name = qib_driver_name
            sys_class_ib = "/sys/class/infiniband/qib0"
        else:
            if os.path.exists(linux_src + "/drivers/staging/rdma/hfi1"):
                driver_path = linux_src + "/drivers/staging/rdma/hfi1/" + driver_file
            else:
                driver_path = linux_src + "/drivers/infiniband/hw/hfi1/" + driver_file

        RegLib.test_log(0, "Using %s for driver" % driver_path)
        # If rdmavt dir exists use it
        vt_driver_path = linux_src + "/drivers/infiniband/sw/rdmavt/" + vt_driver_file
        if os.path.exists(vt_driver_path):
            RegLib.test_log(0, "Using %s for rdmvat driver" % vt_driver_path)
            if test_info.is_simics() == True:
                vt_driver_path = "/host" + vt_driver_path
        else:
            RegLib.test_log(0, "No rdmavt driver found, not using one")
            vt_driver_path = None
    else:
        RegLib.test_log(0, "Running with out-of-tree driver in: %s" % hfi_src)
        driver_path = hfi_src + "/" + driver_file
        vt_driver_file = None  # rdmavt not supported for out of tree testing
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
            RegLib.test_log(0, name + " Need to remove HFI driver first")
            removed = unload_driver(host, driver_name)
            if removed == False:
                RegLib.test_fail(name + " Could not remove HFI driver")

        loaded = is_driver_loaded(host, vt_driver_name)
        if loaded == True:
            RegLib.test_log(0, name + " Need to remove rdmavt driver")
            removed = unload_driver(host, vt_driver_name)
            if removed == False:
                RegLib.test_fail(name + " Could not remove rdmavt driver")

        # Make sure rdma service is running before loading the driver
        cmd = "service rdma start"
        do_ssh(host, cmd)

        if vt_driver_file:
            RegLib.test_log(0, name + " Loading rdmavt driver")
            loaded = load_driver(host, vt_driver_name, vt_driver_path, "")
            if loaded == True:
                RegLib.test_log(0, name + " rdmavt loaded successfully")
            else:
                RegLib.test_fail(name + " could not load rdmavt driver")
        else:
            RegLib.test_log(0, "Using legacy driver no rdmavt")

        RegLib.test_log(0, name + " Loading Hfi Driver")
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
	if ext_lid:
	    set_ext_lid(opafm_host)
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

        err = wait_for_active(host, 10, 20, sys_class_ib)
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
        if test_info.is_simics() == False:
            RegLib.test_log(0, "Restarting irq balance on %s" % host.get_name())
            cmd = "service irqbalance restart"
            out = do_ssh(host, cmd)
            print out
            print host.get_name(), "module load address is:"
            cmd = "cat /sys/module/%s/sections/.init.text" % driver_name
            out = do_ssh(host, cmd)
            print out

            path = os.path.join(topdir, "CheckModule.py")
            if linux_src != "None":
                res = os.system("%s --linuxsrc %s --nodelist %s %s" % (path, linux_src, host.get_name(), qib_flag))
            else:
                res = os.system("%s --hfisrc %s --nodelist %s %s" % (path, hfi_src, host.get_name(), qib_flag))
    
            if res != 0:
                RegLib.test_fail("Incorrect driver loaded!")

    RegLib.test_pass("Driver loaded, adapters up SM running in the fabric.")

if __name__ == "__main__":
    main()

