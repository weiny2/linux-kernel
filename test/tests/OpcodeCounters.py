#!/usr/bin/python

# Test OpcodeCounters
# Author: Mike Marciniszyn <mike.marciniszyn@intel.com>
#

import RegLib

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
    # file must be present
    (err, out) = do_ssh(host1, "cat /sys/kernel/debug/hfi/hfi0/opcode_stats > /tmp/opcode_stats");
    if err:
       RegLib.test_log(0, "ssh host1 opcode_stats read failed")
       test_fail=1
    (err, out) = do_ssh(host2, "cat /sys/kernel/debug/hfi/hfi0/opcode_stats > /tmp/opcode_stats");
    if err:
       RegLib.test_log(0, "ssh host2 opcode_stats read failed")
       test_fail=1
    (err, out) = do_ssh(host1, "test -s /tmp/opcode_stats");
    if err:
       RegLib.test_log(0, "ssh host1 opcode_stats size failed")
       test_fail=1
    (err, out)  = do_ssh(host2, "test -s /tmp/opcode_stats");
    if err:
       RegLib.test_log(0, "ssh host2 opcode_stats size failed")
       test_fail=1

    if test_fail:
        RegLib.test_fail("Failed!")
    else:
        RegLib.test_pass("Passed!")
if __name__ == "__main__":
    main()

