#!/usr/bin/python

# Test IbSendLat
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
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
    # Run ping from host 1
    cmd = "ping -c 5 -W 10 " + host2.get_name() + "-ib"
    (err, out) = do_ssh(host1, cmd)
    if err:
        test_fail = 1
        RegLib.test_log(0, "Child SSH exit status bad")
    for line in out:
        print RegLib.chomp(line)
    if test_fail:
        RegLib.test_fail("Failed!")
    else:
        RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

