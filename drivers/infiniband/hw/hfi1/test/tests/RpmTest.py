#!/usr/bin/python

# Test building of RPMs
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import subprocess
import shlex
import glob
import os

def make_dist(path, tarball, extra_args = ""):

    RegLib.test_log(5, "Running make dist in " + path)

    cmd = "cd %s && make dist %s" % (path, extra_args)
    ret = subprocess.call(cmd, shell=True)

    if ret:
        RegLib.test_fail("Could not do make dist")
    else:
        RegLib.test_log(5, "Make dist success")

    RegLib.test_log(5, "Looking for tarball matching: %s" % tarball)
    tarball = glob.glob(tarball)
    if len(tarball) > 1:
        RegLib.tet_fail("Found more than 1 tarball in %s, baling" % path)
    elif len(tarball) != 1:
        RegLib.test_fail("Did not find tarball in %s" % path)

    return tarball[0]

def build_rpm(path, tarball):

    cmd = "cd %s && rpmbuild -ta %s" % (path, tarball)
    args = shlex.split(cmd)
    print "Running rpmbuild command:", cmd
    proc = subprocess.Popen(cmd, shell=True,  stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)

    status = proc.wait()
    if status:
        RegLib.test_fail("Rpmbuild command failed")

    output = proc.stdout.readlines()
    rpms = []
    for line in output:
        matchObj = re.match(r"Wrote:\s+(.+RPMS/x86_64/.+.x86_64.rpm)", line)
        if matchObj:
            rpm_path = os.path.abspath(matchObj.group(1))
            #RegLib.test_log(5, "Found rpm: %s" % rpm_path)
            rpms.append(rpm_path)

    return rpms

def install_rpm(test_info, host, rpms):

    for rpm in rpms:
        if test_info.is_simics():
            rpm = "/host/%s" % rpm
        RegLib.test_log(0,"Installing RPM: %s" % rpm)
        cmd = "rpm -Uvh --replacefiles --replacepkgs %s" % rpm
        err = host.send_ssh(cmd, 0)
        if err:
            RegLib.test_fail("Could not install RPM %s" % rpm)
        else:
            RegLib.test_log(0, "Installed %s" % rpm)

def main():

    test_info = RegLib.TestInfo()
    RegLib.test_log(0, "Test: RpmTest.py started")
    host = test_info.get_host_record(0)  #single node test

    psm_path= test_info.get_psm_lib()
    tarball = make_dist (psm_path, "%s/hfi-psm-*.tar.gz" % psm_path)
    rpms = build_rpm(psm_path, tarball)
    install_rpm(test_info, host, rpms)

    diag_path = test_info.get_diag_lib()
    tarball = make_dist (diag_path, "%s/hfi-diagtools-sw-noship-*.tar.gz" % diag_path)
    rpms = build_rpm(diag_path, tarball)
    install_rpm(test_info, host, rpms)

    RegLib.test_pass("All RPMs built and installed")

if __name__ == "__main__":
    main()

