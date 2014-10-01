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

def run_filter_test(host1, host2, path, filter):

    pid = os.fork()
    cmd = path + " --args \"" + filter + ","
    if pid == 0:
        # Run the client
        cmd = cmd + "client\""
        RegLib.test_log(0,"Running: %s" % cmd)
        (ret, output) = host2.send_ssh(cmd, run_as_root=True)
        for line in output:
            print RegLib.chomp(line)
        sys.exit(ret)

    RegLib.test_log(0,"Starting server in 5 seconds")
    time.sleep(5)

    # Run the server
    cmd = cmd + "server\""
    RegLib.test_log(0, "Running: %s" % cmd)
    (ret, output) = host1.send_ssh(cmd, run_as_root=True)

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

    if attempts > 30:
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
    
    for filter in ["0x0", "0x1", "0x2", "0x3", "0x4", "0x5", "0x6"]:
        RegLib.test_log(0, "Running filter test %s" % filter)
        run_filter_test(host1, host2, path, filter)

    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

