#!/usr/bin/python
#
# Author: Mitko Haralanov <mitko.haralanov@intel.com>

import RegLib
import time
import sys
import os
import subprocess
import fcntl

topdir = os.path.dirname(os.path.realpath(__file__))

def run_subtest(cmd):
    output = ""
    proc = subprocess.Popen(cmd.split(), shell=False, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, close_fds=True)
    flags = fcntl.fcntl(proc.stdout.fileno(), fcntl.F_GETFL)
    fcntl.fcntl(proc.stdout.fileno(), fcntl.F_SETFL, flags | os.O_NONBLOCK)
    status = proc.poll()
    while status == None:
        try:
            out = proc.stdout.read(128)
            while out:
                sys.stdout.write(out)
                output += out
                out = proc.stdout.read(128)
        except IOError: pass
        status = proc.poll()
    out = proc.stdout.read(128)
    while out:
        sys.stdout.write(out)
        output += out
        out = proc.stdout.read(128)
    return (status, output.split("\n"))

def reload_module(hostlist, src=None, opts={}):
    optstr = "--nodelist=%s" % ",".join([x.get_name() for x in hostlist])
    if src: optstr += " --hfisrc=%s" % src
    if opts:
        optstr += " --modparm"
        for opt in opts.items():
            optstr += " \"%s=%s\"" % opt
    cmd = mkpath(topdir, "LoadModule.py") + " " + optstr
    RegLib.test_log(0, "Executing %s..." % cmd)
    st, out = run_subtest(cmd)
    if st != 0: return False
    return True

def run_verbs_tests(hostlist):
    optstr = "--nodelist=%s" % ",".join([x.get_name() for x in hostlist])
    cmd = mkpath(topdir, "IbSendBwRC.py") + " " + optstr
    RegLib.test_log(0, "Executing %s..." % cmd)
    st, out = run_subtest(cmd)
    if st != 0: return False
    return True

def mkpath(*args):
    return os.sep.join(list(args))

def main():
    global topdir
    if not os.path.exists(mkpath(topdir, "LoadModule.py")) or \
       not os.path.exists(mkpath(topdir, "IbSendBwRC.py")):
        RegLib.test_fail("Could not find dependency test script")

    test_info = RegLib.TestInfo()
    module_src = test_info.get_hfi_src()

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)

    if not reload_module([host1, host2], module_src, {"max_mtu": 8192}):
        RegLib.test_fail("Failed to restart drivers with 8K MTU.")
    if not run_verbs_tests([host1, host2]):
        RegLib.test_fail("Failed to run verbs tests with 8K MTU.")
    if not reload_module([host1, host2], module_src, {"max_mtu": 4096}):
        RegLib.test_fail("Failed to restart drivers with 4K MTU.")
    if not run_verbs_tests([host1, host2]):
        RegLib.test_fail("Failed to run verbs tests with 4K MTU.")

    if not reload_module([host1, host2]):
        RegLib.test_fail("Failed to reset drivers")
    RegLib.test_pass("Test with 4K and 8K MTU successful.")
    return

if __name__ == "__main__":
    main()
