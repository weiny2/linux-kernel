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

def do_pkt_send(host, packet_file, dest_lid = None, dest_ctxt = 1, count = 1, use_diagpkt = False, psm_libs = None):
    RegLib.test_log(0, "Starting hfi_pkt_send test with packet file %s" % packet_file)

    if psm_libs is not None:
        cmd_libs = "export LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH; " % psm_libs
    else:
        cmd_libs = ""

    if dest_lid is None:
        # Query the host for its LID to use as a destination LID.
        dest_lid = host.get_lid()
        if dest_lid is None:
                RegLib.test_log(0, "Unable to query host (%s) LID" % host.get_name())
                RegLib.test_fail("Failed!")
                return False
        dest_lid = int(dest_lid)

    cmd_diagpkt_arg = ""
    if use_diagpkt:
        cmd_diagpkt_arg = "-k "

    cmd_pattern = "%s hfi_pkt_send -L %d -C %d -c %d %s%s"
    cmd = cmd_pattern % (cmd_libs, dest_lid, dest_ctxt,
                         count, cmd_diagpkt_arg, packet_file)

    err = do_ssh(host, cmd)
    if err:
        RegLib.test_log(0, "Error on client")
        RegLib.test_fail("Failed!")
        return False

    return True

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()

    RegLib.test_log(0, "Test: HfiPktSend.py started")
    RegLib.test_log(0, "Dumping test parameters")

    # Dump out the test and host info
    print test_info

    count = test_info.get_host_count()

    print "Found ", count, "hosts; only one is needed for this test."
    host = test_info.get_host_record(0)
    print host

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

    pkt_dir = test_info.get_test_pkt_dir() + "/"

    # Send packets via user-level HFI context.
    do_pkt_send(host, pkt_dir + "normal", psm_libs = psm_libs)
    do_pkt_send(host, pkt_dir + "psm_min", psm_libs = psm_libs)
    do_pkt_send(host, pkt_dir + "psm_min_psn31", count = 10, psm_libs = psm_libs)

    # Send packets via kernel diagpkt interface
    do_pkt_send(host, pkt_dir + "bypass8", use_diagpkt = True, psm_libs = psm_libs)
    do_pkt_send(host, pkt_dir + "bypass10", use_diagpkt = True, psm_libs = psm_libs)
    do_pkt_send(host, pkt_dir + "bypass16", use_diagpkt = True, psm_libs = psm_libs)
    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

