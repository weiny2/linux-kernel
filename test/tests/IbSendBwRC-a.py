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
    child_pid = os.fork()
    if child_pid == 0:
        # For some reason when running this command over ssh no output shows up
        # till after the proc ends. This also happens when running by hand and
        # not just in this script. So add stdbuf to the beginning and it seems
        # to help. We don't want to blindl do this in the SSH routine as it
        # seems to screw up the MPI tests.
        cmd = "stdbuf -oL ib_send_bw -d hfi0 -n 16 -u 21 -a 2>&1" 
        # This is the server let's display his output now
        err = host1.send_ssh(cmd, 0)
        RegLib.test_log(5, "Running cmd: " + cmd)
        if err:
            RegLib.test_log(0, "Child SSH exit status bad")
        sys.exit(err)

    time.sleep(5)

    server_name = host1.get_name()
    cmd = "ib_send_bw -d hfi0 -n 16 -u 21 -a " + server_name + " 2>&1"
    (err, out) = host2.send_ssh(cmd)
    if err:
        RegLib.test_log(0, "Error on client")
        test_fail = 1

    (pid, status) = os.waitpid(child_pid, 0)

    if status != 0:
        RegLib.test_fail("Failed!")

    # Now print out client's output that we saved off so it doesn't get all
    # jumbled up
    RegLib.test_log(0, "Client Output:>")
    for line in out:
        print RegLib.chomp(line)


    RegLib.test_pass("All sizes sent successfully")

if __name__ == "__main__":
    main()

