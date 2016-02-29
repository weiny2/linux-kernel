#!/usr/bin/python

# Test IbReadBwRC
# Author: Mike Marciniszyn (mike.mmarciniszyn@intel.com)
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

    RegLib.test_log(0, "Test: IbReadBwRC.py started")
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

    sizes = ['1', '256', '512', '1024', '2048', '4096', '8192', '16384']

    for size in sizes:
        test_port = RegLib.get_test_port(host1, host2)
        test_fail = 0
        # Start ib_read_bw on host1 (server)
        child_pid = os.fork()
        if child_pid == 0:
            if test_info.is_qib():
                dev = "qib0"
            else:
                dev = "hfi1_0"
            cmd = "ib_read_bw -m 4096 -d %s -n 5 -u 21 -p %d -s %s" % (dev, test_port, size)
            (err, out) = do_ssh(host1, cmd)
            if err:
                RegLib.test_log(0, "Child SSH exit status bad")
            for line in out:
                print RegLib.chomp(line)
            sys.exit(err)

        RegLib.test_log(0, "Waiting for socket to be listening")
        if host1.wait_for_socket(test_port, "LISTEN") == False:
            RegLib.test_fail("Coudl not get socket listening")

        # Start ib_read_bw on host2 (client)
        server_name = host1.get_name()
        if test_info.is_qib():
            dev = "qib0"
        else:
            dev = "hfi1_0"
        cmd = "ib_read_bw -m 4096 -d %s -n 5 -u 21 -p %d -s %s %s" % (dev, test_port, size,
                                                                server_name)
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

if __name__ == "__main__":
    main()

