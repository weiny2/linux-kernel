#!/usr/bin/python

import RegLib
import time
import sys
import os
import subprocess
import fcntl
import shlex

topdir = os.path.dirname(os.path.realpath(__file__))

def do_ssh(host, cmd):
    RegLib.test_log(5, "Running " + cmd)
    return (host.send_ssh(cmd))

def main():
    global topdir
    curr_file = os.path.basename(__file__)

    test_info = RegLib.TestInfo()
    host = test_info.get_host_record(0)

    RegLib.test_log(0, "Test: " + curr_file + " started")
    cmd = "hfi_tx_auth"
    (err, out) = do_ssh(host, cmd)
    for line in out:
        print RegLib.chomp(line)
    if err:
        RegLib.test_fail("hfi_tx_auth failed")

    RegLib.test_pass("hfi_tx_auth passed")


if __name__ == "__main__":
    main()
