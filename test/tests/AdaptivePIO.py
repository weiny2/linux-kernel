#!/usr/bin/python

# Test AdaptivePIO
# Author: Sebastian Sanchez (sebastian.sanchez@intel.com)
#

import RegLib
import signal
import time
import sys
import re
import os

def do_ssh(host, cmd, timeout=0):
    RegLib.test_log(5, "Running " + cmd)
    return (host.send_ssh(cmd, 0, timeout, run_as_root=True))

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()

    RegLib.test_log(0, "Test: AdaptivePIO.py started")
    RegLib.test_log(0, "Dumping test parameters")

    # Dump out the test and host info
    RegLib.test_log(0, test_info)

    count = test_info.get_host_count()

    RegLib.test_log(0, "Found %d hosts; only one is needed for this test." \
                    % count)
    client = test_info.get_host_record(0)
    server = test_info.get_host_record(1)
    RegLib.test_log(0, client)

    ################
    # body of test #
    ################
    test_fail = 0
    hca = "hfi1_0"
    qpn = 32
    num_ports = qpn
    test_port = RegLib.get_test_port(client, server, num_ports=num_ports)
    test_mode = 4
    iterations = 20
    pkt_size_min = 0
    pkt_size_max = 1024
    log_server_path = "/tmp/rc_int_chk_server_%d_%s_%d.log" \
		      % (test_port, server.get_name(), os.getuid())
    path_test = "/opt/opa/wfr-driver-test/verbs/"
    executable_name = "rc_test"
    exec_full_path = path_test + executable_name
    server_log_msg = "Server side log on %s is located at %s" \
                     % (server.get_name(), log_server_path)

    if test_port == 0:
        RegLib.test_fail("Could not get port number")

    RegLib.test_log(0, "Starting server on %s port=%d log=%s" \
                    % (server.get_name(), test_port, log_server_path))

    # Starting server
    child_pid = os.fork()
    if child_pid == 0:
        cmd = "%s -d=%s -p=%d -q=%d -a=%d -b=%d > %s" % (exec_full_path, hca,\
               test_port, qpn, pkt_size_min, pkt_size_max, log_server_path)
        err = do_ssh(server, cmd)
        if err:
            # If the process was terminated by a signal
            if err >= 128:
                RegLib.test_log(0, "Server was killed with a signal - %s" % err)
            else:
                RegLib.test_log(0, "ERROR starting server - %s" % err)
        sys.exit(err)

    RegLib.test_log(0, "Waiting for socket to be listening")
    if server.wait_for_socket(test_port, "LISTEN", num_ports=num_ports) == False:
        RegLib.test_log(0, server_log_msg)
        RegLib.test_log(0, "Could not get socket listening")
        test_fail = 1
    else:
        # Starting client
        cmd = "%s -d=%s -p=%d -q=%d -m=%d -n=%d -a=%d -b=%d %s 2>&1" \
               % (exec_full_path, hca, test_port, qpn, test_mode, iterations,
                  pkt_size_min, pkt_size_max, server.get_name())
        err = do_ssh(client, cmd)
        if err:
            # If the process was terminated by a signal
            if err >= 128:
                RegLib.test_log(0, "Server was killed with a signal - %s" % err)
            else:
                RegLib.test_log(0, "ERROR - Client detected error - %s", err)
                test_fail = 1

    # Checking if there's at least one server process running
    cmd = "/usr/bin/pgrep -f %s > /dev/null 2>&1" % executable_name
    err = do_ssh(server, cmd)
    if not err: # is server running
        RegLib.test_log(0, "Server still running - killing %s" \
            % executable_name)
        cmd = "killall %s > /dev/null 2>&1" % executable_name
        do_ssh(server, cmd)

    (pid, status) = os.waitpid(child_pid, 0)

    # There's an exit error, and the process wasn't terminated by a signal
    if status != 0 and status < 128:
        test_fail = 1
        RegLib.test_log(0, "Child proc reported bad exit status")

    if test_fail:
        RegLib.test_log(0, server_log_msg)
        RegLib.test_fail("Failed!")
    else:
        # Since test was successful remove server log file
        cmd = "rm %s" % log_server_path
        do_ssh(server, cmd)
        RegLib.test_pass("Adaptive PIO test passed")

if __name__ == "__main__":
    main()
