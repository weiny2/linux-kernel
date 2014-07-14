#!/usr/bin/python

# Test IpoibQperf
# Author: Mike Marciniszyn (mike.marciniszyn@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time

def do_ssh(host, cmd):
    RegLib.test_log(5, "Running " + cmd)
    return (host.send_ssh(cmd))

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
    test_fail = 0

    (err, out) = do_ssh(host1, "modprobe ib_ipoib")
    if err:
        RegLib.test_fail("Could not load ipoib on host1")

    (err, out) = do_ssh(host2, "modprobe ib_ipoib")
    if err:
        RegLib.test_fail("Could not load ipoib on host2")

    RegLib.test_log(0,"Waiting 10 seconds for Ipoib to get running")
    time.sleep(10)

    # insure not running on either host
    cmd = "pkill qperf"
    err = do_ssh(host1, "pkill qperf")
    cmd = "pkill qperf"
    err = do_ssh(host2, "pkill qperf")

    # Start qperf on host1 (server)
    child_pid = os.fork()
    if child_pid == 0:
        cmd = "qperf"
        (err, out) = do_ssh(host2, cmd)
        if err:
            test_fail = 1
            RegLib.test_log(0, "Child SSH exit status bad")
        for line in out:
            print RegLib.chomp(line)
        sys.exit(err)

    # Start ib_send_lat on host2 (client)
    server_name = host2.get_name()
    cmd = "qperf " + server_name + "-ib --wait_server 5 -oo msg_size:8:64:*2 -vu tcp_bw"
    (err, out) = do_ssh(host1, cmd)
    if err:
        RegLib.test_log(0, "Error on client")
        test_fail = 1

    cmd = "pkill qperf"
    err = do_ssh(host2, "pkill qperf")
    (pid, status) = os.waitpid(child_pid, 0)

    for line in out:
        print RegLib.chomp(line)

    (err, out) = do_ssh(host1, "rmmod ib_ipoib")
    if err:
        RegLib.test_fail("Could not unload ipoib on host1")

    (err, out) = do_ssh(host2, "rmmod ib_ipoib")
    if err:
        RegLib.test_fail("Could not unload ipoib on host2")

    RegLib.test_log(0, "Waiting 10 seconds for Ipoib to stop running")
    time.sleep(10)

    if test_fail:
        RegLib.test_fail("Failed!")
    else:
        RegLib.test_pass("Success!")


if __name__ == "__main__":
    main()

