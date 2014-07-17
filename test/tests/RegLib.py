# RegLib.py - Regression test module for WFR HFI development.
#
# Author: Dennis Dalessandro (dennis.dalessandro@intel.com)
#

import sys
import re
import os
import subprocess
import shlex
import tempfile
import time
import random
from optparse import OptionParser
from datetime import datetime

verbosity_level = 5

simics_key="""
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA1vn8pQbZL8uf6mYC6tb7HWGbMtNKGMXkUWYKpbX/YJGKlOYi
6Lk9tC+lQysVocUBQZ44QQw6CsEz641cuhxG333EUfHJyMgZ8zD2IWso5CNFbAnX
SqSUph/8Ef3HCmCJwT9KbYmQfUqWrnRzYqKoC4gFSmz36xn1Ckbp58gebb+Y3peF
7VVfqlEcnE7J5Q2YMASaZz02yRmpJ7yfXoSVGVYv+u6xanbg4uRQiuuNADtg19tr
13m9AhOhjA2nAwXksoY+KRDckOssD+rpVxq70cGINqc60fb/QVzzBARjfAz3bkRb
pkgi8yNT1W3pqxl7C6n1vdRhU0Oh5/B7UJVhawIBIwKCAQEAsh+W0eEaWs1J/Lrs
eXBMaM03c0Faps/ab1SMeqy9p8kMbL6vNdtQYhjZY4oZPOUPq2XW1s+dzmWRa2aA
B+uLLjUBwD1eDMMOL+a9TuPCy6gyNPI2CqWfvNisSWvfbv+IFSXP9FwKAWmwDOQl
FzZQuRjucN3x/VAFjCwg5J5/nMHNDTu14/OHfvroaMsaYSB0SfDrma7OoCyH3X6v
JSE9jM7foCKtXWix4FYZMvbth+oE/ZSpQq6v+F3a3HTui8xVrQoW7P9ZkqI5Ze9e
3su9t2P6gxiM4jzkvWGn5jtYK4xU0BB4Zmo3FvUERejH4KNLUlTEh9Jq8D92J50L
K2M7SwKBgQD7xhR4miEFAIDE4M6s8F2pEVreIAqUz/aE/ufYNQ6nb2j/txKw3x4T
gZho8K2Bx1KLPRO+odxjms3a6O2CSBEboxlO2e9zEVB1u+oh2El7UyR5I4+Gde56
LN4Hewr2LBe61mGtCvBHckt+5W9Y/3QtSLSb54QZts2pemkaZDIroQKBgQDalcg6
oTG/9hRIEONDvUIk+mOowq8bWyJc37d4MZ1CRps82t5JyynlC+zSBHUhgzhT/s2j
Iu3UeVAWxTHqWNrdPpW5KEWBzqH1d9/iPPXXHN3GP+hrbkxE4HvRcxu8R4mqGFwL
pCcdbfa+IAju4ybYX5ffZ1snpqT3EZTgrzIRiwKBgQDtYv1bxIWIX49aitF+fDsN
Hv3nYAn6BehuxHQyQKdqqt9XgLnZ9vB4yqWkxaroE7Q6I6Tm8GIUxSh98Y961jwE
HW+iHfBlLZUBSsbPdCgGkDhU9aSUuFXg6HmSBkwp7w8PPyjk+69ZToGyKMgSFW2J
yDVCidRSwkzhn00ngwq7awKBgQDH2WacAReK752Dt6s2nmhcUqRChhxwyFnug2Xq
O/1+bHCsqtndEYVjsyGqErQ75a/XxGQ9YcOAbuojcnbHobIpXcq36k4tibil6fFD
3/a2C8NzbaFMR5YwWDaw3kU+bUqqJOZxCxxyra5kr5MjjdpfbVenrvRBgmrwhRpn
BpQtSwKBgQDBq3+Bm6KqkcXxt42QeCIBipeyj0UtcQT1M68xyZHaxNGQhOKRldHs
a52nCvCVuw0n6V3n/ABAM9+TtkW1R0+uzjZ2CIO9+T48vi966nTIQdeUwW81vmLD
LpBUwtjxAlLUTn3VxojSKiEt24R3G2i4G0B8klvauXKS5PkzB9efhA==
-----END RSA PRIVATE KEY-----
"""

def test_fail(msg):
    test_log(0,"Test Failure: " + msg)
    sys.exit(1)

def test_pass(msg):
    test_log(0,"Test Success: " + msg)
    sys.exit(0)

def test_log(lvl, msg):
    if (lvl <= verbosity_level):
        timestr = str(datetime.now())
        print "[%s] %s" % (timestr, msg)

def chomp(str):
    # How can python not have chomp, guess we need to make one. The built in
    # strip function will remove all newlines we just want to cut off the last
    # one.
    matchObj = re.match(r"(.*)\n$", str)
    if matchObj:
        return matchObj.group(1)
    else:
        return str

def chomp_comma(str):
    matchObj = re.match(r"(.*),$", str)
    if matchObj:
        return matchObj.group(1)
    else:
        return str

def get_test_port():
    return random.randrange(1025, 65535)

class HostInfo:
    """Holds information on a host under test"""

    def __init__(self, name, dns_name, port, opts):
        self.name = name
        self.dns_name = dns_name
        self.port = port
        self.opts = opts

    def __repr__(self):
        return "Name: %s DNS: %s Port %s Opts: %s" % (self.name,
                self.dns_name, self.port, self.opts)

    def get_name(self):
        return self.name

    def get_lid(self):
        cmd = "cat /sys/class/infiniband/hfi0/ports/1/lid"
        (err, out) = self.send_ssh(cmd)
        if err:
            return None

        for line in out:
            matchObj = re.match(r"0x\d+", line)
            if matchObj:
                return line.strip()

        return None #error if we got to here

    def send_ssh(self, cmd, buffered=1):
        """ Send an SSH command. We may need to add a timeout mechanism
            buffered mode will save output and return it. Non buffered mode
            will display the output on the screen and only return a status"""

        cmd = "'" + cmd + "'"
        test_log(10, "Sending CMD to [" + self.name + "|" +
                 self.dns_name + "]: " + cmd)
        opts = self.opts
        if "SIMICS_KEY_FILE" in self.opts:
            # We need to create a keyfile. The reason we did not create one
            # when we created the host or test object and just saved the name is
            # because if a user program forks when the child proc ends it will
            # close the named temp file and delete it. This is very annoying. So
            # to get around this we need to either manually try to tear down a
            # file and deal with ctrl+c in them middle of the program and/or
            # other complications, lets just create the file real quick before
            # sending an SSH command. It will get deleted when it goes out of
            # scope.
            test_log(10, "Simics hosts detected creating ssh key file in /tmp")
            key_file = tempfile.NamedTemporaryFile(mode='w', dir='/tmp')
            if not key_file:
                return(1, "Could not create key file in /tmp")
            else:
                test_log(10, "temp file name is " + key_file.name)
                key_file.write(simics_key)
                # must flush the file or else it may still be empty by the
                # time we send the first SSH command it is not a lot of data
                # being written so it will likely just be buffered otherwise
                key_file.flush()
            opts = opts.replace("SIMICS_KEY_FILE", key_file.name)

        # Build up the command line and parse it into tokens using shlex
        cmdline = "ssh -l root " + self.dns_name + " "
        cmdline = cmdline + " -p " + self.port
        cmdline = cmdline + " " + opts + " " + cmd;
        test_log(10, "Command line is " + cmdline)
        args = shlex.split(cmdline)

        # Fork a process and wait for it to complete. This is where we may need
        # to add a timeout at some point. We could use Popen.poll in a loop.
        ssh = subprocess.Popen(args, shell=False, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)

        if buffered:
            result = ssh.wait()
            # Now that we have the data go ahead and dump it on the screen before
            # returning the list of lines. Call off to our version of chomp to strip
            # the newline off of the lines. The lines in result are not altered and
            # are returned as is.
            out = ssh.stdout.readlines()
            return (result, out)
        else:
            while True:
                out = ssh.stdout.read(1)
                if out == '' and ssh.poll() != None:
                    break
                if out != '':
                    sys.stdout.write(out)
                    sys.stdout.flush()
            return ssh.returncode


class TestInfo:
    """A simple regression test info class"""

    def __init__(self):
        self.get_parse_standard_args()

    global simics_key

    # Standard variables
    nodelist = []
    hfi_src = None
    kbuild_dir = None
    simics = False
    mpiverbs = False

    def get_hfi_src(self):
        return self.hfi_src

    def is_simics(self):
        return self.simics

    def get_parse_standard_args(self):
        parser = OptionParser()

        git_root = os.popen('git rev-parse --show-toplevel').read()
        git_root = chomp(git_root)

        # Some ancient disros do not support the above git command. Or git may
        # not be in the users path. Rather than getting garbage and accepting
        # it, if the path is not valid, just clear it out.
        if not os.path.exists(git_root):
            git_root = ""

        parser.add_option("--nodelist", dest="nodelist",
                          help="Nodes to run on. Default: \"viper0,viper1\"",
                          metavar="NODES",
                          default="viper0,viper1")

        parser.add_option("--hfisrc", dest="hfisrc",
                          help="Path to hfi driver source. Default: " + git_root,
                          metavar="PATH",
                          default=git_root)

        parser.add_option("--kbuild", dest="kbuild",
                          help="Path to kbuild dir",
                          metavar="PATH")

        parser.add_option("--simics", action="store_true", dest="simics",
                          help="Run on simics environment. Optional if using "
                               + "viper0,viper1 as nodelist")

        parser.add_option("--type", dest="test_types",
                          help="Test type(s) to run. Use 'all' for everything."
                          + " Default: 'default'")

        parser.add_option("--list", action="store_true", dest="list_only",
                          help="List available tests")

        parser.add_option("--psm", dest="psm_lib",
                          help="Optional PSM Location. Default: <none>",
                          metavar="PATH")

        parser.add_option("--sw-diags", dest="diag_lib",
                          help="Optional location to diag libs. Default: <none>",
                          metavar="PATH")

        parser.add_option("--mpiverbs", action="store_true", dest="mpiverbs",
                          help="Run MPI over verbs instead of the default PSM")

        test_pkt_dir_default = "/usr/share/hfi-diagtools-sw/test_packets"
        parser.add_option("--test-pkt-dir", dest="test_pkt_dir",
                          help="Optional location of test packets. Default: "
                          + test_pkt_dir_default,
                          metavar="PATH", default=test_pkt_dir_default)

        parser.add_option("--modparm", dest="module_params",
                          help="Optional module paramters to pass "
                               + "Like: param1=X param2=Y "
                               + "or colon separated for per host like: "
                               + "parm1 = X param2=Y : parm1 = Z")
        parser.add_option("--args", dest="extra_args",
                          help="Optional params to pass to tests as a comma sep list",
                          metavar="LIST",
                          default="")
        parser.add_option("--sm", dest="sm",
                          help="Which SM to use. Valid values are opensm, " +
                          "ifs_fm, detect, or none. Default: detect",
                          metavar="SM",
                          default="detect")
        parser.add_option("--basedir", dest="base_dir",
                          help="Optional base directory to pass to a test. Used to source executables and such.",
                          metavar="PATH",
                          default="")
        parser.add_option("--testlist", dest="test_list",
                          help="List of tests to execute. Can be used with '--type' to further filter the test list.")

        (options, args) = parser.parse_args()

        # Simics or not?
        self.simics = options.simics
        if self.simics == None:
            self.simics = False

        if options.mpiverbs:
            self.mpiverbs = True

        # Listing?
        self.list_only = options.list_only
        if self.list_only == None:
            self.list_only = False
        else:
            return

        # Save our list of nodes
        if options.nodelist:
            nodelist = re.split(r',', options.nodelist)

            # Try to auto detect simics
            for node in nodelist:
                if node == "viper0" or node == "viper1":
                    self.simics = True

            for node in nodelist:
                found_localhost = 0
                oopts = "-o StrictHostKeyChecking=no "
                oopts += "-o UserKnownHostsFile=/dev/null"

                if self.simics:
                    # For simics we will accept only viper0, viper1 and
                    # localhost for running directly on one of the vipers. This
                    # is not bullet proof so user has to do the right thing.
                    iopts = "-i SIMICS_KEY_FILE"
                    if node == "viper0":
                        host = HostInfo(node, "localhost", "4022", oopts + " " +
                                        iopts)
                    elif node == "viper1":
                        host = HostInfo(node, "localhost", "5022", oopts + " " +
                                        iopts)
                    elif node == "localhost":
                        host = HostInfo(node, node, "22", oopts)
                    else:
                        test_fail("Simics specified but did not find vipers")
                else:
                    host = HostInfo(node, node, "22", oopts)

                self.nodelist.append(host)

        # Diagtools test packet directory
        # test_pkt_dir specifies a directory on the test host (e.g.,
        # viper0).  Since that direct may not exist (in the same place) on
        # this host, we cannot do sanity checks here for presence/type.
        if options.test_pkt_dir:
            self.test_pkt_dir = os.path.abspath(options.test_pkt_dir)
        else:
            self.test_pkt_dir = test_pkt_dir_default

        # What is the location of the driver source tree?
        if options.hfisrc:
            self.hfi_src = os.path.abspath(options.hfisrc)
            if not os.path.exists(self.hfi_src):
                test_fail("hfi_src is not a valid path [" + self.hfi_src + "]")
            if not os.path.isdir(self.hfi_src):
                test_fail("hfi_src is not a directory")

        # Location of the kbuild dir.
        if options.kbuild:
            self.kbuild_dir= os.path.abspath(options.kbuild)
            if not os.path.exists(self.kbuild_dir):
                test_fail("kbuild is not a valid path")

        # Optional PSM libs
        if options.psm_lib:
            if options.psm_lib  == "DEFAULT":
                self.psm_lib = options.psm_lib
            else:
                self.psm_lib = os.path.abspath(options.psm_lib)
                if not os.path.exists(self.psm_lib):
                    test_fail("psm lib is not a valid path")
        else:
            self.psm_lib = None

        if options.diag_lib:
            if options.diag_lib  == "DEFAULT":
                self.diag_lib = options.diag_lib
                return

            self.diag_lib = os.path.abspath(options.diag_lib)
            if not os.path.exists(self.diag_lib):
                test_fail("diag lib is not a valid path")
        else:
            self.diag_lib = None

        if options.test_types:
            self.test_types = options.test_types
        else:
            self.test_types = "default"

        if options.test_list:
            self.test_list = options.test_list
        else:
            self.test_list = None

        if options.module_params:
            self.module_params = options.module_params
        else:
            self.module_params = ""

        if options.extra_args:
            self.extra_args = options.extra_args
        else:
            self.extra_args = ""

        if options.base_dir:
            self.base_dir = options.base_dir
        else:
            self.base_dir = ""

        if options.sm:
            self.sm = options.sm

    def get_host_list(self):
        host_list = []
        for host in self.nodelist:
            host_list.append(host.get_name())
        print "returning ", host_list
        return host_list

    def get_host_count(self):
        return len(self.nodelist)

    def get_host_record(self, index):
        return self.nodelist[index]

    def get_host_name_by_index(self, index):
        host = self.nodelist[index]
        return host.get_name()

    def get_test_types(self):
        return self.test_types

    def get_test_list(self):
        return self.test_list

    def get_kbuild_dir(self):
        return self.kbuild_dir

    def list_only(self):
        return self.list_only

    def get_psm_lib(self):
        if self.psm_lib:
            return self.psm_lib
        else:
            return "DEFAULT"

    def get_diag_lib(self):
        if self.diag_lib:
            return self.diag_lib
        else:
            return "DEFAULT"

    def get_test_pkt_dir(self):
        return self.test_pkt_dir

    def __str__(self):
        ret = ""
        for node in self.nodelist:
            info = repr(node)
            ret = ret + "\n\t\t" + info
        return "\tNodeList: %s\n\tHFI: %s\n\tkbuild: %s\n\tsimics: %s\n\tmpiverbs: %s" % (
               ret, self.hfi_src, self.kbuild_dir, self.simics, self.mpiverbs)

    # For now just hard code the MPI paths since MPI is installed in the craff
    # file for simics. If we need to pass a custom MPI path we can add an option
    # later.
    def get_mpi_lib_path(self):
        return "/usr/mpi/gcc/openmpi-1.6.5-qlc/lib64"

    def get_mpi_bin_path(self):
        return "/usr/mpi/gcc/openmpi-1.6.5-qlc/bin"

    def get_mpi_opts(self):
        # This part is different depending on if we use verbs or PSM
	if not self.mpiverbs:
             ret = " --mca mtl psm -x HFI_UNIT=0 -x HFI_PORT=1 -x xxxPSM_CHECKSUM=1 -x PSM_TID=0 -x PSM_SDMA=0 -x"
	else:
             ret = " --mca btl sm,openib,self --mca mtl ^psm -mca btl_openib_max_inline_data 0 --mca btl_openib_warn_no_device_params_found 0 -x"
        return ret

    def get_osu_benchmark_dir(self):
        # Look on one of the target nodes for the benchmark location.
        # Use buffered send_ssh() so we don't see the annoying
        # "Warning: Permanently added ..." message on the output.
        node = self.nodelist[0]
        # current simualtion disk images
        (err, out) = node.send_ssh(
                          "test -d /usr/local/libexec/osu-micro-benchmarks")
        if err == 0:
            return "/usr/local/libexec/osu-micro-benchmarks/mpi/pt2pt/"
        # FPGA machiines
        (err, out) = node.send_ssh("test -d /usr/lib64/openmpi/bin")
        if err == 0:
            return "/usr/lib64/openmpi/bin/mpitests-"
        # no identifiable path, return no prefix
        return ""

    def get_mod_parms(self):
        return self.module_params

    def parse_extra_args(self):
        list_args = self.extra_args.split(",")
        return list_args

    def get_base_dir(self):
        return self.base_dir

    def which_sm(self):
        return self.sm

