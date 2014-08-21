#!/usr/bin/python

# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time
import subprocess


directory = os.path.dirname(os.path.realpath(__file__))
def enable_snoop(hosts, params):
    os.system(directory + "/LoadModule.py --nodelist " + hosts + " --modparm \"" + params + "\"")

def restore_default_params(hosts, params):
    os.system(directory + "/LoadModule.py --nodelist " + hosts + " --modparm \"" + params + "\"")

def snoop_enabled(hosts):
    # Firs check and see if snoop is enabled.
    for host in hosts:
        RegLib.test_log(0, "Checking snoop enablement for %s" % host.get_name())
        snoop_on = 0
        cmd = "echo snoop_enable = `cat /sys/module/hfi/parameters/snoop_enable`"
        (res,output) = host.send_ssh(cmd)
        if res:
            RegLib.test_fail("Could not get status for host %s" %
                             host.get_name())
        for line in output:
            matchObj = re.match(r"snoop_enable = 1", line)
            if matchObj:
                snoop_on = 1
                print RegLib.chomp(line)

        if snoop_on != 1:
            return False

    return True

def run_filter_test(host1, host2, path, filter):

    pid = os.fork()
    cmd = path + " --args \"" + filter + ","
    if pid == 0:
        # Run the client
        cmd = cmd + "client\""
        RegLib.test_log(0,"Running: %s" % cmd)
        (ret, output) = host2.send_ssh(cmd)
        for line in output:
            print RegLib.chomp(line)
        sys.exit(ret)

    RegLib.test_log(0,"Starting server in 5 seconds")
    time.sleep(5)

    # Run the server
    cmd = cmd + "server\""
    RegLib.test_log(0, "Running: %s" % cmd)
    (ret, output) = host1.send_ssh(cmd)

    # Now wait for client to get done. It may hang and if so that is a test
    # failure.
    attempts = 1
    test_failed = 0
    while attempts <= 30:
        (waited, status) = os.waitpid(pid, os.WNOHANG)
        if waited == 0 and status == 0:
            RegLib.test_log(0, "client still running check again in 1 second")
            time.sleep(1)
            attempts += 1
            continue
        if status != 0:
            test_failed = 1
            RegLib.test_log(0, "Client returned bad status")
        RegLib.test_log(0, "Client done and returned success")
        break

    if attempts == 30:
        RegLib.test_fail("Client proc never finished")

    if ret:
        RegLib.test_fail("Server test failed")

    if test_failed:
        RegLib.test_fail("Client test failed")

    for line in output:
        print RegLib.chomp(line)

def main():
    global directory
    path = directory + "/SnoopFilterLocal.py"

    test_info = RegLib.TestInfo()
    if test_info.is_simics() == True:
        path = "/host" + path

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)
    nodes = host1.get_name() + "," + host2.get_name()
    
    disable_snoop = 0

    default_parms = test_info.get_mod_parms()
    snoop_parms = default_parms + " snoop_enable=1"
    RegLib.test_log(0, "Default parms [%s] Snoop parms [%s]" % (default_parms, snoop_parms))

    if not snoop_enabled([host1,host2]):
        enable_snoop(nodes, snoop_parms)
        disable_snoop = 1
        if not snoop_enabled([host1,host2]):
            RegLib.test_fail("Could not enable snoop")


    for filter in ["0x0", "0x1", "0x2", "0x3", "0x4", "0x5", "0x6"]:
        RegLib.test_log(0, "Running filter test %s" % filter)
        run_filter_test(host1, host2, path, filter)

    if disable_snoop:
        restore_default_params(nodes, default_parms)

    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

