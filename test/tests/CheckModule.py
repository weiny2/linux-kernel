#!/usr/bin/python

# Verify the driver loaded is the one you want.
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import subprocess

def do_ssh(host, cmd):
    RegLib.test_log(5, "[" + host.get_name() + "] Running " + cmd)
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

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()

    RegLib.test_log(0, "Test: CheckModule.py started")
    print test_info
    count = test_info.get_host_count()
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
    driver_path = ""
    vt_driver_name = "rdmavt"
    vt_driver_file = "rdmavt.ko"
    vt_driver_path = ""
    qib_driver_name = "ib_qib"
    qib_driver_file = "ib_qib.ko"
    sys_class_ib = "/sys/class/infiniband/hfi1_0"

    linux_src = test_info.get_linux_src()
    hfi_src = test_info.get_hfi_src()

    if linux_src != "None":
        RegLib.test_log(0, "Running with full kernel at: %s" % linux_src)

        if test_info.is_qib() == True:
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

    cmd = "modinfo -F srcversion %s" % driver_path
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    status = proc.wait()
    if status:
        RegLib.test_fail("Could not determine driver srcversion")

    src_vers = RegLib.chomp(proc.stdout.readline())
    RegLib.test_log(0, "Local Src version is " + src_vers)

    for host in hostlist:
        name = host.get_name()
        # if driver is loaded then rdmavt must also be loaded if applicable
        loaded = is_driver_loaded(host, driver_name)
        if loaded == False:
            RegLib.test_fail(name + " " + driver_name + " not loaded!")
        RegLib.test_log(0, driver_name + " is loaded")

        cmd = "cat /sys/module/%s/srcversion" % driver_name
        out = do_ssh(host, cmd)
        found = False
        for line in out:
            match = re.match(src_vers, line)
            if match:
                found = True

        if found == False:
            RegLib.test_fail("Did not find correct srcversion!")

        RegLib.test_log(0, "Found srcversion = " + src_vers)

        loaded = is_driver_loaded(host, "rdmavt")
        if loaded == True:
            RegLib.test_log(0, "Need to check rdmavt too")
            cmd = "modinfo -F srcversion %s" % vt_driver_path
            proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
            status = proc.wait()
            if status:
                RegLib.test_fail("Could not determine rdmavt srcversion")

            vt_src_vers = RegLib.chomp(proc.stdout.readline())
            RegLib.test_log(0, "rdmavt local Src version is " + vt_src_vers)

            cmd = "cat /sys/module/%s/srcversion" % vt_driver_name
            out = do_ssh(host, cmd)
            found = False
            for line in out:
                match = re.match(vt_src_vers, line)
                if match:
                    found = True

            if found == False:
                RegLib.test_fail("Did not find correct srcversion!")

            RegLib.test_log(0, "Found srcversion = " + vt_src_vers)

        else:
            RegLib.test_log(0, "No rdmavt required")

    RegLib.test_pass("Driver versions match source")

if __name__ == "__main__":
    main()

