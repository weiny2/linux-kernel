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
import struct
import hashlib
import fcntl
import socket
from ctypes import *
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

def _get_test_port():
    port = random.randrange(1025, 65535)
    return port

def _verify_test_port_available(host, port, num_ports=1):
    cmd = "/bin/netstat -vatn"
    test_log(0, "Running %s" % cmd)
    (err, out) = host.send_ssh(cmd)
    if (err != 0) or \
        any(str(port_x) in out for port_x in range(port, port+num_ports)):
        if not num_ports > 1:
            # Dump the output of netstat for later triage
            for line in out:
                test_log(0, line)
        return False

    return True

def get_test_port(host1, host2, num_ports=1):
    attempts = 100
    while attempts > 0:
        attempts -= 1
        port = _get_test_port()

        test_log(0, "Checking if port(s) %d is available on host1." % port)
        if _verify_test_port_available(host1, port, num_ports):
            test_log(0, "Port(s) %d should be free checking host2." % port)
            if _verify_test_port_available(host2, port, num_ports):
                return port
            else:
                port = 0
        else:
            port = 0
    return port

def md5_from_packet(packet):

    # Build the md5 of the packet
    m = hashlib.md5()
    m.update(packet)
    return m.hexdigest()

# Parse a string representation of a packet
def parse_packet(packet, has_metadata=False):

    offset = 0

    # First thing we do is strip off the last 4 bytes because that is CRC
    # which is meaningless here.
    packet = packet[:-4]

    # ---------
    # Meta Data
    # ---------
    if has_metadata == True:
        port = struct.unpack('B', packet[offset:offset+1])[0]
        offset += 1

        dir_val = struct.unpack('B', packet[offset:offset+1])[0]
        offset = offset + 1
        dir = ""
        if dir_val == 0:
            dir = "EGRESS"
        else:
            dir = "INGRESS"

        offset += 6
        pbc_rhf = struct.unpack('Q', packet[offset:offset+8])[0]
        offset += 8
        md_offset = 16
        print "MD5:", md5_from_packet(packet[16:]), "Len:", len(packet[16:]), "bytes"
        print "MetaData:\n\tPort:", port, "\n\tDir:", dir, "\n\tPBC/RHF:", hex(pbc_rhf)
    else:
        md_offset = 0
        print "MD5:", md5_from_packet(packet), "Len:", len(packet), "bytes"

    # ----------------
    # LRH - big endian
    # ----------------
    vl_lver = struct.unpack('B', packet[offset:offset+1])[0]
    offset += 1
    vl = (vl_lver >> 4) & 0xF
    lver = vl_lver & 0xF

    sl_lnh = struct.unpack('B', packet[offset:offset+1])[0]
    offset += 1
    sl = (sl_lnh >> 4) & 0xF
    lnh = sl_lnh & 0x3

    dlid = struct.unpack('>H', packet[offset:offset+2])[0]
    offset += 2

    pktlen = struct.unpack('>H', packet[offset:offset+2])[0]
    offset += 2
    pktlen = pktlen & 0xFFF

    slid = struct.unpack('>H', packet[offset:offset+2])[0]
    offset += 2

    print "LRH:\n\tVL:", vl, "LVer:", lver, "SL:", sl, "LNH:", lnh
    print "\tDLID:", hex(dlid), "SLID:", hex(slid), "PktLen:", pktlen

    if lnh == 0:
        test_fail("Error: Detected Raw packet, unsupported")
    elif lnh == 1:
        test_fail("Error: Detected Ipv6 packet, unsupported")
    elif lnh == 3:
        print "GRH: Detected. Skipping though"
        offset += 40
    elif lnh != 2:
        test_fail("Error: Undetectable packet")

    # ----------------
    # BTH - big endian
    # ----------------
    opcode = struct.unpack('B', packet[offset:offset+1])[0]
    offset += 1

    # skip solicited event, migration state, padding, and tversion
    offset += 1

    pkey = struct.unpack('>H', packet[offset:offset+2])[0]
    offset += 2

    f_b_qp = struct.unpack('>I', packet[offset:offset+4])[0]
    offset += 4
    dest_qp = f_b_qp & 0xFFFFFF00

    psn_resv = struct.unpack('>I', packet[offset:offset+4])[0]
    offset += 4
    psn = psn_resv & 0x7FFFFFFF

    print "BTH:"
    print "\tOpcode:", hex(opcode), "Pkey:", hex(pkey)
    print "\tDestQP:", hex(dest_qp), "PSN:", hex(psn)

    # Now go on to the payload
    bytes = len(packet)
    line_count = 0
    line_offset = 0
    offset = md_offset # show the full packet starting at end of metadata
    while offset < bytes:
        byte = struct.unpack('B', packet[offset:offset+1])[0]
        if line_count == 0:
            print line_offset, ":: ",
            line_offset += 16

        line_count += 1
        print format(byte,'X').zfill(2),

        if line_count == 16:
            print ""
            line_count = 0
        offset += 1

    # Packet end
    print ""

class HostInfo:
    """Holds information on a host under test"""

    def __init__(self, name, dns_name, port, opts, force_root):
        self.name = name
        self.dns_name = dns_name
        self.port = port
        self.opts = opts
        self.force_root = force_root

    def __repr__(self):
        return "Name: %s DNS: %s Port %s Opts: %s" % (self.name,
                self.dns_name, self.port, self.opts)

    def get_name(self):
        return self.name

    def get_lid(self):
        # Not supported for qib. The only tests which need this are for HFI
        # we should probably fix this at some point.
        cmd = "cat /sys/class/infiniband/hfi1_0/ports/1/lid"
        (err, out) = self.send_ssh(cmd)
        if err:
            return None

        for line in out:
            matchObj = re.match(r"0x\S+", line)
            if matchObj:
                return line.strip()

        return None #error if we got to here

    def file_exists(self, path):
        cmd = "test -e " + path
        (err, out) = self.send_ssh(cmd)
        if err:
            return False
        return True

    def _wait_for_single_socket(self, port, state, timeout=1, attempts=10):
        """ Waits for the socket at the given port to reach a specific state"""

        while attempts > 0:
            attempts -= 1
            cmd = "/bin/netstat -vatn | /bin/grep %s" % port
            test_log(0, "Running %s" %cmd)
            (err, out) = self.send_ssh(cmd)
            for line in out:
                if state in line:
                    return True
            test_log(5, "Socket was not listening, will try %d more times" % attempts)
            test_log(5, "Sleeping for %d seconds" % timeout)
            time.sleep(timeout)
        return False #if we get to here it is an error

    def wait_for_socket(self, port, state, timeout=1, attempts=10, num_ports=1):
        """ Waits for the socket at the given port(s) to reach a specific state"""

        return all(self._wait_for_single_socket(port_x, state, timeout, attempts)\
                    for port_x in range(port, port+num_ports))

    def send_ssh(self, cmd, buffered=1, timeout=0, run_as_root=False, use_tty=False):
        """ Send an SSH command. We may need to add a timeout mechanism
            buffered mode will save output and return it. Non buffered mode
            will display the output on the screen and only return a status.
            Passing a timeout value will cause the command to run then return
            after a set amount of time"""

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
        if self.force_root == True:
            run_as_root = True

        cmdline = ""
        if run_as_root == True:
            cmdline = "ssh -l root " + self.dns_name + " "
        elif use_tty == True:
            cmdline = "ssh -t " + self.dns_name + " "
        else:
            cmdline = "ssh " + self.dns_name + " "

        cmdline = cmdline + " -p " + self.port
        cmdline = cmdline + " " + opts + " " + cmd;
        test_log(10, "Command line is " + cmdline)
        args = shlex.split(cmdline)

        # Fork a process and wait for it to complete. This is where we may need
        # to add a timeout at some point. We could use Popen.poll in a loop.
        ssh = subprocess.Popen(args, shell=False, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)

        if timeout:
            # Note that this leaves the command running it is up to the caller
            # to send another ssh command to kill the proc if needed.
            start = time.time()
            test_log(0, "Waiting for %d seconds for cmd to run" % timeout)
            output = ""
            fd = ssh.stdout.fileno()
            fl = fcntl.fcntl(fd, fcntl.F_GETFL)
            fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
            while True:
                # Read a character
                ch = ''
                try:    ch = ssh.stdout.read(1)
                except: pass
                if ch != '':
                    output += ch

                # Make sure we haven't timed out yet
                now = time.time()
                duration = int(now - start)
                if duration >= timeout:
                    break

            test_log(0, "Done waiting. Returning.")
            lines = output.split('/n')
            for line in lines:
                line += "\n"
            # Now go ahead and return the output
            return (0, lines)

        elif buffered:
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

    def is_switched_config(self):
        """ Determines what the neighbor is """
        cmd =  "opasmaquery -o portinfo"
        (result, out) = self.send_ssh(cmd)
        for line in out:
            if "Switch" in line:
                return True
        return False

class TestInfo:
    """A simple regression test info class"""

    def __init__(self):
        self.get_parse_standard_args()
        if self.get_host_count() > 0:
            self.get_mpi_dir()

    global simics_key

    # Standard variables
    nodelist = []
    hfi_src = None
    linux_src = None
    kbuild_dir = None
    simics = False
    qib = False
    fpga = False
    mpiverbs = False
    paramdict = None
    np = "6"
    mpiverbs_path = ""
    mpipsm_path = ""

    def get_hfi_src(self):
        return self.hfi_src

    def get_linux_src(self):
        return self.linux_src

    def is_simics(self):
        return self.simics

    def is_qib(self):
        return self.qib

    def is_fpga(self):
        return self.fpga

    def get_mpi_dir(self):
        host = self.get_host_record(0)
        if host.file_exists("/usr/mpi/gcc/openmpi-1.10.2/bin/mpirun"):
            self.mpiverbs_path = "/usr/mpi/gcc/openmpi-1.10.2"
        elif host.file_exists("/usr/mpi/gcc/openmpi-1.8.5/bin/mpirun"):
            self.mpiverbs_path = "/usr/mpi/gcc/openmpi-1.8.5"
        elif host.file_exists("/usr/mpi/gcc/openmpi-1.8.2a1/bin/mpirun"):
            self.mpiverbs_path = "/usr/mpi/gcc/openmpi-1.8.2a1"
        elif host.file_exists("/usr/mpi/gcc/openmpi-1.10.0/bin/mpirun"):
            self.mpiverbs_path = "/usr/mpi/gcc/openmpi-1.10.0"
        else:
            test_log(0, "could not find verbs MPI path")

        if host.file_exists("/usr/mpi/gcc/openmpi-1.10.2-hfi/bin/mpirun"):
            self.mpipsm_path = "/usr/mpi/gcc/openmpi-1.10.2-hfi"
        elif host.file_exists("/usr/mpi/gcc/openmpi-1.8.5-hfi/bin/mpirun"):
            self.mpipsm_path = "/usr/mpi/gcc/openmpi-1.8.5-hfi"
        elif host.file_exists("/usr/mpi/gcc/openmpi-1.8.2a1-hfi/bin/mpirun"):
            self.mpipsm_path = "/usr/mpi/gcc/openmpi-1.8.2a1-hfi"
        elif host.file_exists("/usr/mpi/gcc/openmpi-1.10.0-hfi/bin/mpirun"):
            self.mpipsm_path = "/usr/mpi/gcc/openmpi-1.10.0-hfi"
        elif host.file_exists("/usr/mpi/gcc/openmpi-1.8.4-qlc/bin/mpirun"):
            self.mpipsm_path = "/usr/mpi/gcc/openmpi-1.8.4-qlc"
        else:
            test_log(0, "could not find PSM MPI path")

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

        parser.add_option("--linuxsrc", dest="linuxsrc",
                          help="Path to full kernel source. Default: None",
                          metavar="PATH",
                          default=None)

        parser.add_option("--kbuild", dest="kbuild",
                          help="Path to kbuild dir",
                          metavar="PATH")

        parser.add_option("--simics", action="store_true", dest="simics",
                          help="Run on simics environment. Optional if using "
                               + "viper0,viper1 as nodelist")

        parser.add_option("--qib", action="store_true", dest="qib",
                          help="Run on Truescale using qib driver ")

        parser.add_option("--fpga", action="store_true", dest="fpga",
                          help="Run on FPGA environment. Optional if using "
                          + "fpga* as nodelist")

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

        parser.add_option("--perfpath", dest="perf_path",
                          help="Optional location of perftest. Default: /bin",
                          metavar="PATH",
                          default="/bin")

        parser.add_option("--perfdir", dest="perf_dir",
                          help = "Retain perfomance data in this parent directory",
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
                               + "parm1=X param2=Y : parm1=Z")
        parser.add_option("--args", dest="extra_args",
                          help="Optional params to pass to tests as a comma sep list",
                          metavar="LIST",
                          default="")
        parser.add_option("--sm", dest="sm",
                          help="Which SM to use. Valid values are none, remote, or " +
                          "local. Default: local",
                          metavar="SM",
                          default="local")
        parser.add_option("--basedir", dest="base_dir",
                          help="Optional base directory to pass to a test. Used to source executables and such.",
                          metavar="PATH",
                          default="")
        parser.add_option("--testlist", dest="test_list",
                          help="List of tests to execute. Can be used with '--type' to further filter the test list.")
        parser.add_option("--psmopts", dest="psm_opts",
                          help="String of options to pass to PSM tests.",
                          default="")
        parser.add_option("--forceroot", action="store_true", dest="force_root",
                          help="Force all commands to run as root.")
        parser.add_option("--np", dest="np",
                          help="Number of processes.",
                          default="6")

        (options, args) = parser.parse_args()

        # Simics or not?
        self.simics = options.simics
        if self.simics == None:
            self.simics = False
        else:
            self.force_root = True

        self.qib = options.qib
        if self.qib == None:
            self.qib = False

        self.fpga = options.fpga
        if self.fpga == None:
            self.fpga = False

        if options.mpiverbs:
            self.mpiverbs = True

        self.force_root = options.force_root
        if self.force_root == None:
            self.force_root = False;

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
                    break
                elif "fpga" in node:
                    self.fpga = True
                    break

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
                                        iopts, True)
                    elif node == "viper1":
                        host = HostInfo(node, "localhost", "5022", oopts + " " +
                                        iopts, True)
                    elif node == "localhost":
                        host = HostInfo(node, node, "22", oopts, True)
                    else:
                        test_fail("Simics specified but did not find vipers")
                else:
                    host = HostInfo(node, node, "22", oopts, self.force_root)

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

        if options.linuxsrc:
            if options.linuxsrc != "None":
                self.linux_src = os.path.abspath(options.linuxsrc)
                if not os.path.exists(self.linux_src):
                    test_fail("linux_src is not a valid path [" + self.linux_src + "]")
                if not os.path.isdir(self.linux_src):
                    test_fail("linux_src is not a directory")
            else:
                self.linux_src = "None"
        else:
            self.linux_src = "None"

        # Location of the kbuild dir.
        if options.kbuild:
            self.kbuild_dir= os.path.abspath(options.kbuild)
            if not os.path.exists(self.kbuild_dir):
                test_fail("kbuild is not a valid path")

        self.perf_dir = options.perf_dir

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

        self.psm_opts = options.psm_opts

        if options.np:
            self.np = options.np

        self.perf_path = options.perf_path

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

    def is_mpiverbs(self):
        return self.mpiverbs

    def get_psm_opts(self):
        return "\"%s\"" % self.psm_opts

    def get_mpi_lib_path(self):
        if self.mpiverbs:
            return self.mpiverbs_path + "/lib64"
        else:
            return self.mpipsm_path + "/lib64"

    def get_mpi_bin_path(self):
        if self.mpiverbs:
            return self.mpiverbs_path + "/bin"
        else:
            return self.mpipsm_path + "/bin"

    def get_mpi_opts(self):
        if not self.mpiverbs:
	     if self.qib == False:
                  ret = " --mca pml cm --mca mtl psm2 -x HFI_UNIT=0 -x HFI_PORT=1 -x xxxPSM_CHECKSUM=1 -x PSM_PKEY=0x8001 -x PSM_SDMA=1 -x PSM_TID=1 %s --allow-run-as-root -x" % self.psm_opts
	     else:
                  ret = " --mca pml cm --mca mtl psm -x IPATH_UNIT=0 -x IPATH_PORT=1 -x PSM_PKEY=0x7FFF %s --allow-run-as-root -x" % self.psm_opts
        else:
	     if self.qib == False:
                  ret = " --mca btl sm,openib,self --mca mtl ^psm,psm2 -mca btl_openib_max_inline_data 0 --mca btl_openib_warn_no_device_params_found 0 --allow-run-as-root -x"
	     else:
                  ret = " --mca btl sm,openib,self --mca mtl ^psm -mca btl_openib_max_inline_data 0 --mca btl_openib_warn_no_device_params_found 0 --allow-run-as-root -x"
        return ret

    def get_osu_benchmark_dir(self):
        if self.mpiverbs:
            return self.mpiverbs_path + "/tests/osu_benchmarks-3.1.1/"
        else:
            return self.mpipsm_path + "/tests/osu_benchmarks-3.1.1/"

    def get_mod_parms(self):
        return self.module_params

    def get_mod_params_dict(self, host=None):
        if not self.paramdict:
            hostnames = [x.get_name() for x in self.nodelist]
            self.paramdict = dict.fromkeys(hostnames, None)
            if ':' in self.module_params:
                per_host = [x.strip() for x in self.module_params.split(':') if x]
            else:
                per_host = [self.module_params] * len(hostnames) if self.module_params else []
            for idx in range(len(hostnames)):
                self.paramdict[hostnames[idx]] = {}
                if len(per_host) and idx < len(per_host):
                    for opt in per_host[idx].split():
                        try:
                            k, v = opt.strip().split('=')
                            self.paramdict[hostnames[idx]][k] = v
                        except ValueError:
                            continue
        if host:
            if host in self.paramdict :
                return self.paramdict[host]
            else: return {}
        return self.paramdict

    def parse_extra_args(self):
        list_args = self.extra_args.split(",")
        return list_args

    def get_base_dir(self):
        return self.base_dir

    def which_sm(self):
        return self.sm

    def get_np(self):
        return self.np

    def get_perf_path(self):
        return self.perf_path

    def get_perf_dir(self):
        return self.perf_dir

