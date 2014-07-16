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

host1 = None
host2 = None
directory = os.path.dirname(os.path.realpath(__file__))
test_path = directory + "/PcapLocal.py"

def enable_snoop():
    os.system(directory + "/LoadModule.py --modparm \"snoop_enable=1\"")

def restore_default_params():
    os.system(directory + "/LoadModule.py")

def snoop_enabled():
    # Firs check and see if snoop is enabled.
    global host1
    global host2
    for host in [host1, host2]:
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

def start_pcap(host):
    # Start packet capture and when done check its output. This is done in a
    # child process.
    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting Pcap.py on %s" % host.get_name())
        (res, output) = host.send_ssh(test_path, 1)
        ping = 0;
        pong = 0;
        for line in output:
            #BytesRead: 304 PacketBytes: 288 Port: 0 Dir: INGRESS  PBC|RHF: 0 SLID: 2 DLID: 65535 VL: 15 PktWords: 72
            matchObj = re.search(r"PacketBytes: (\d+) .+ SLID: (\d+) DLID: (\d+)",
                                 line)
            if matchObj:
                #print RegLib.chomp(line)
                size = matchObj.group(1)
                slid = matchObj.group(2)
                dlid = matchObj.group(3)
                if size == "36":
                    # Why 36? This is because it is a 2 byte write which gets
                    # padded to 4 bytes. So 4 byte payload + 28 byte header + 4
                    # byte ICRC = 36 or 9 Dwords. The test does a 5x ping pong
                    if (dlid == "1") and (slid == "2"):
                        ping = ping+1
                    elif (dlid == "2") and (slid == "1"):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Ping:", ping, "Pong", pong
        if ping != 5 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)
    return pid

def main():

    global host1
    global host2
    global snoop_path
    global test_path

    test_info = RegLib.TestInfo()
    if test_info.is_simics():
        test_path = "/host" + test_path

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)

    disable_snoop = 1

    if snoop_enabled():
        disable_snoop = 0
    else:
        enable_snoop()
        if not snoop_enabled():
            RegLib.test_fail("Couild not enable capture")

    pid1 = start_pcap(host1)
    pid2 = start_pcap(host2)

    RegLib.test_log(0, "Waiting for 5 seconds for Pcap pid %d, %d to get ready" % (pid1, pid2))
    time.sleep(5)
    RegLib.test_log(0, "Running IbSendLat, be patient")

    cmd = directory + "/IbSendLat.py"
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    status = proc.wait()
    if status:
        RegLib.test_fail("IbSendLat failed")

    #output = proc.stdout.readlines()
    #for line in output:
    #    print RegLib.chomp(line)

    RegLib.test_log(0, "Killing off Pcap")
    cmd = "killall -2 PcapLocal.py"
    status = host1.send_ssh(cmd, 0)
    if status:
        RegLib.test_fail("Could not stop Pcap on remote")

    status = host2.send_ssh(cmd, 0)
    if status:
        RegLib.test_fail("Could not stop Pcap on remote")

    RegLib.test_log(0, "Waiting for child pid to stop and validate")
    (waited, status) = os.waitpid(pid1, 0)
    if status:
        RegLib.test_fail("Test Failed")
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))

    RegLib.test_log(0, "Waiting for child pid to stop and validate")
    (waited, status) = os.waitpid(pid2, 0)
    if status:
        RegLib.test_fail("Test Failed")
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))

    if disable_snoop:
        restore_default_params()

    if status:
        RegLib.test_fail("Failed!")
    else:
        RegLib.test_pass("Success!")


if __name__ == "__main__":
    main()

