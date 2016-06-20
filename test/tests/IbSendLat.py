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

    RegLib.test_log(0, "Test: IbSendLat.py started")
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

    ext_lid = test_info.get_ext_lid()
    ################
    # body of test #
    ################
    test_fail = 0
    test_port = RegLib.get_test_port(host1, host2)

    # Start ib_send_lat on host1 (server)
    child_pid = os.fork()
    if child_pid == 0:
        if test_info.is_qib():
            dev = "qib0"
        else:
            dev = "hfi1_0"
	if ext_lid:
            cmd = "ib_send_lat -d %s -c UD -n 5 -p %d -x 1" % (dev, test_port)
        else:
            cmd = "ib_send_lat -d %s -c UD -n 5 -p %d" % (dev, test_port)
        (err, out) = do_ssh(host1, cmd)
        if err:
            RegLib.test_log(0, "Child SSH exit status bad")
        for line in out:
            print RegLib.chomp(line)
        sys.exit(err)

    RegLib.test_log(0, "Waiting for socket to be listening")
    if host1.wait_for_socket(test_port, "LISTEN") == False:
        RegLib.test_fail("Coudl not get socket listening")

    # Start ib_send_lat on host2 (client)
    server_name = host1.get_name()
    if test_info.is_qib():
        dev = "qib0"
    else:
        dev = "hfi1_0"
    if ext_lid:
        cmd = "ib_send_lat -d %s -c UD -n 5 -p %d -x 1 %s" % (dev, test_port, server_name)
    else:
        cmd = "ib_send_lat -d %s -c UD -n 5 -p %d %s" % (dev, test_port, server_name)
    (err, out) = do_ssh(host2, cmd)
    if err:
        RegLib.test_log(0, "Error on client")
        for x in out: print x.strip()
        test_fail = 1

    (pid, status) = os.waitpid(child_pid, 0)

    if status != 0:
        test_fail = 1
        RegLib.test_log(0, "Child proc reported bad exit status")

    for line in out:
        print RegLib.chomp(line)

    if test_fail:
        RegLib.test_fail("Failed!")
    else:
        RegLib.test_pass("Success!")


if __name__ == "__main__":
    main()

