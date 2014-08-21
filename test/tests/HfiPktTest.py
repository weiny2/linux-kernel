#!/usr/bin/python

# Test HfiPktTest
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
# Author: Andrew Friedley (andrew.friedley@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time

def do_ssh(host, cmd):
    RegLib.test_log(5, "Running " + cmd)
    return (host.send_ssh(cmd, 0))

def do_pingpong(diag_path, host1, host2, psm_libs = None,
	        payload = 1024, count = 500):
    RegLib.test_log(0, "Starting hfi_pkt_test ping-pong benchmark")
    test_pass = True

    if psm_libs is not None:
        cmd_libs = "export LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH; " % psm_libs
    else:
        cmd_libs = ""

    # First run a command on host1 to get its LID.
    host1_lid = host1.get_lid()
    if host1_lid is None:
        RegLib.test_log(0, "Unable to query host1 (%s) LID" % host1.get_name())
        RegLib.test_fail("Failed!")
        return False

    # Start the receiver on host1; host2 will initiate ping-pongs.
    cmd = cmd_libs + diag_path + "hfi_pkt_test -r"
    (err, out) = host1.send_ssh(cmd, 0, 10)

    # Now figure out what context is being listend on look for:
    #         Receiving on LID 1 context 1
    context = ""
    for line in out:
        print line
        matchObj = re.search(r"Receiving on LID \d+ context (\d+)", line)
        if matchObj:
            context = matchObj.group(1)

    if context == "":
        RegLib.test_fail("Could not determine context of receiver")

    RegLib.test_log(0, "Using context %s" % context)

    cmd_pattern = "%s %shfi_pkt_test -C %s -L %s -s %d -p -c %d"
    cmd = cmd_pattern % (cmd_libs, diag_path, context, host1_lid, payload, count)

    err = do_ssh(host2, cmd)
    if err:
        RegLib.test_log(0, "Error on client")
        for x in out: print x.strip()
        test_pass = False

    # Receiver side of hfi_pkt_test never exits on its own, it must be killed.
    do_ssh(host1, "pkill hfi_pkt_test")

    if test_pass:
        RegLib.test_pass("Success!")
    else:
        RegLib.test_fail("Failed!")
    return test_pass

def do_piotest(diag_path, host, psm_libs):
    RegLib.test_log(0, "Starting hfi_pkt_test PIO buffer copy benchmark")

    if psm_libs is not None:
        cmd_libs = "export LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH; " % psm_libs
    else:
        cmd_libs = ""

    cmd = cmd_libs + diag_path + "hfi_pkt_test -B"
    err = do_ssh(host, cmd)

    if err:
        RegLib.test_fail("Failed!")
        return False
    else:
        RegLib.test_pass("Success!")
        return True

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()

    RegLib.test_log(0, "Test: HfiPktTest.py started")
    RegLib.test_log(0, "Dumping test parameters")

    # Dump out the test and host info. We need 2 hosts for this test
    print test_info

    count = test_info.get_host_count()

    print "Found ", count, "hosts"
    host1 = test_info.get_host_record(0)
    print host1

    if count >= 2:
        host2 = test_info.get_host_record(1)
        print host2

    ################
    # body of test #
    ################

    psm_libs = test_info.get_psm_lib()
    if psm_libs == "DEFAULT":
        psm_libs = None
        print "Using default PSM lib"
    else:
        if test_info.is_simics():
            psm_libs = "/host" + psm_libs
        print "We have PSM path set to", psm_libs

    diag_path = test_info.get_diag_lib()
    if diag_path == "DEFAULT":
	    # Assume the diagtools are already installed somewhere in $PATH.
    	diag_path = ""
    else:
	    diag_path += "/build/targ-x86_64/utils/"

    if count == 1:
        do_piotest(diag_path, host1, psm_libs)
    elif count >= 2:
        do_pingpong(diag_path, host1, host2, psm_libs)

if __name__ == "__main__":
    main()

