#!/usr/bin/python

#
# Author: Ira Weiny (ira.weiny@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()


    RegLib.test_log(0, "Test: IbSendBwRC-badSL.py started")
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

    test_ports=[]
    for i in range(3):
        test_ports.append(RegLib.get_test_port(host1, host2))

    ################
    # body of test #
    ################

    test_fail = 1
    child_pid = os.fork()
    if child_pid == 0:
        # For some reason when running this command over ssh no output shows up
        # till after the proc ends. This also happens when running by hand and
        # not just in this script. So add stdbuf to the beginning and it seems
        # to help. We don't want to blindl do this in the SSH routine as it
        # seems to screw up the MPI tests.
        if test_info.is_qib():
            dev = "qib0"
        else:
            dev = "hfi1_0"
	for test_port in test_ports:
            cmd = "stdbuf -oL ib_send_bw -d %s -p %d -S 14 2>&1" % (dev, test_port)
            # This is the server let's display his output now
            err = host1.send_ssh(cmd, 0)
            RegLib.test_log(5, "Running cmd: " + cmd)
            if err:
                RegLib.test_log(0, "Child SSH exit status bad")
            sys.exit(err)

    RegLib.test_log(0, "Waiting for socket to be listening")
    port_acquired=0
    for test_port in test_ports:
        if host1.wait_for_socket(test_port, "LISTEN") == False:
            RegLib.test_log(0, "Could not get socket listening")
	else:
	    port_acquired=1
	    break

    if port_acquired != 1:
        RegLib.test_fail("Could not get any sockets listening")

    server_name = host1.get_name()
    if test_info.is_qib():
        dev = "qib0"
    else:
        dev = "hfi1_0"
    cmd = "ib_send_bw -d %s -p %d -S 14 %s 2>&1" % (dev, test_port, server_name)
    RegLib.test_log(0, "Running cmd: " + cmd)
    (err, out) = host2.send_ssh(cmd)
    if err:
        RegLib.test_log(0, "Error on client")
        test_fail = 0

    (pid, status) = os.waitpid(child_pid, 0)

    if status == 0:
        RegLib.test_fail("Failed!")

    # Now print out client's output that we saved off so it doesn't get all
    # jumbled up
    RegLib.test_log(0, "Client Output:>")
    for line in out:
        print RegLib.chomp(line)


    RegLib.test_pass("Invalid SL failed correctly")

if __name__ == "__main__":
    main()

