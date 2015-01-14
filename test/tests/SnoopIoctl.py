#!/usr/bin/python

# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time
import subprocess

def main():

    directory = os.path.dirname(os.path.realpath(__file__))
    path = directory + "/SnoopIoctlLocal.py"

    test_info = RegLib.TestInfo()
    if test_info.is_simics():
        path = "/host" + path

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)
    default_parms = test_info.get_mod_parms()
    snoop_parms = default_parms + " snoop_enable=1"
    hosts = host1.get_name() + "," + host2.get_name()

    RegLib.test_log(0, "Making sure fm is stopped")
    cmd = "service opafm stop"
    ret = host1.send_ssh(cmd, 0, run_as_root=True)

    ret = host1.send_ssh(path, 0, run_as_root=True)
    if ret:
        RegLib.test_fail("IOCTL test failed")

    cmd = "service opafm start"
    RegLib.test_log(0, "Restarting opafm")
    ret = host1.send_ssh(cmd, 0, run_as_root=True)

    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

