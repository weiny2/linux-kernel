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

    RegLib.test_log(0, "Loading module with snoop on making sure no SM running")
    os.system(directory + "/LoadModule.py --modparm \"snoop_enable=1\" --sm none")

    ret = host1.send_ssh(path, 0)

    if ret:
        RegLib.test_fail("IOCTL test failed")


    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

