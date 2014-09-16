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

def snoop_enabled(hosts):
    # Firs check and see if snoop is enabled.
    for host in hosts:
        RegLib.test_log(0, "Checking snoop enablement for %s" % host.get_name())
        snoop_on = 0
        cmd = "echo snoop_enable = `cat /sys/module/hfi/parameters/snoop_enable`"
        (res,output) = host.send_ssh(cmd)
        if res:
            RegLib.test_fail("Could not get status for host %s" %
                             host.get_name())
        for line in output:
            matchObj = re.match(r"snoop_enable = 1", line)
            if matchObj:
                snoop_on = 1
                print RegLib.chomp(line)

        if snoop_on != 1:
            return False

    return True

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

    restart_fm = 0
    if snoop_enabled([host1, host2]):
        RegLib.test_log(0, "Making sure fm is stopped")
        cmd = "service ifs_fm stop"
        ret = host1.send_ssh(cmd, 0)
        restart_fm = 1
    else:
        RegLib.test_log(0, "Loading module with snoop on making sure no SM running")
        os.system(directory + "/LoadModule.py --nodelist=\"" + hosts + "\" --modparm \"" + snoop_parms + "\" --sm none")

    ret = host1.send_ssh(path, 0)
    if ret:
        RegLib.test_fail("IOCTL test failed")

    if restart_fm:
        cmd = "service ifs_fm start"
        RegLib.test_log(0, "Restarting ifs_fm")
        ret = host1.send_ssh(cmd, 0)

    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

