#!/usr/bin/python
#
# Author: Mitko Haralanov <mitko.haralanov@intel.com>

import RegLib
import time
import sys
import os
import subprocess
import fcntl
import shlex

topdir = os.path.dirname(os.path.realpath(__file__))

SUBTEST_LIST = ["MpiStress"]

def run_subtest(cmd):
    output = ""
    args = shlex.split(cmd)
    print "Going to run:", args
    proc = subprocess.Popen(args, shell=False, stdout=subprocess.PIPE,
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

def reload_module(hostlist, src=None, opts=[]):
    optstr = "--nodelist=%s" % ",".join([x.get_name() for x in hostlist])
    if src: optstr += (" " + src)
    if opts:
        o = []
        for x in range(len(hostlist)):
            if opts[x]:
                o.append(" ".join(["%s=%s" % (k, v) for k, v in opts[x].items()]))
        if o: optstr += " --modparm=\"%s\"" % " : ".join(o)
    test_info = RegLib.TestInfo()
    ext_lid = test_info.get_ext_lid;
    if ext_lid:
	optstr += " --extlid"
    cmd = os.path.join(topdir, "LoadModule.py") + " " + optstr
    RegLib.test_log(0, "Executing %s..." % cmd)

    st, out = run_subtest(cmd)
    if st != 0: return False
    return True

def run_loopback_tests(test, hostlist, psm="DEFAULT", opts="", args=None):
    optstr = ["--nodelist=%s" % ",".join([x.get_name() for x in hostlist])]
    if psm != "DEFAULT": optstr.append("--psm=%s" % psm)
    if opts: optstr.append("--psmopts=\"%s\"" % opts)
    if args: optstr.append("--args=\"%s\"" % " ".join(args))
    cmd = os.path.join(topdir, test+".py") + " ".join([""] + optstr)
    RegLib.test_log(0, "Executing %s..." % cmd)
    st, out = run_subtest(cmd)
    if st != 0: return False
    return True

def main():
    global topdir

    for test in SUBTEST_LIST + ["LoadModule"]:
        if not os.path.exists(os.path.join(topdir, test+".py")):
            RegLib.test_fail("Could not find dependency test script: %s" % test)

    test_info = RegLib.TestInfo()
    linux_src = test_info.get_linux_src()
    module_src = ""
    if linux_src != "None":
        module_src = "--linuxsrc=%s" % linux_src
    else:
        module_src = "--hfisrc=%s" % test_info.get_hfi_src()

    host1 = test_info.get_host_record(0)

    opts = test_info.get_mod_params_dict(host1.get_name())
    # get PSM opts and strip the encosing quotes
    psm_opts = test_info.get_psm_opts()[1:-1]
    psm_opts += " -x PSM_DEVICES=self,hfi -x PSM_HFI_LOOPBACK=1"
    psm_lib = test_info.get_psm_lib()

    args = test_info.parse_extra_args()
    if not args or (len(args) == 1 and not args[0]):
        args = ["-L 1", "-M 1", "-w 2", "-z"]
    args.append("-i")

    if not test_info.is_fpga():
        # LCB loopback
        loopback_opts = {"loopback" : 2}
        loopback_opts.update(opts)

        if not reload_module([host1], module_src, [loopback_opts]):
            RegLib.test_fail("Failed to restart driver with LCB loopback")

        for test in SUBTEST_LIST:
            if not run_loopback_tests(test, [host1], psm_lib, psm_opts, args):
                RegLib.test_fail("Failed to execute subtest: %s" % test)

    if not test_info.is_simics():
        loopback_opts = {"loopback" : 1}
        loopback_opts.update(opts)

        if not reload_module([host1], module_src, [loopback_opts]):
            RegLib.test_fail("Failed to restart the driver with SerDes loopback")

        for test in SUBTEST_LIST:
            if not run_loopback_tests(test, [host1], psm_lib, psm_opts, args):
                RegLib.test_fail("Failed to execute subtest: %s" % test)


    RegLib.test_pass("Test with LCB and SerDes loopback passed.")
    return

if __name__ == "__main__":
    main()
