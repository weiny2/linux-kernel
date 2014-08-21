#!/usr/bin/python

# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time
import multiprocessing

def snoop_enabled(host):
    # First check and see if snoop is enabled.
    RegLib.test_log(0, "Checking snoop enablement for %s" % host.get_name())
    snoop_on = 0
    cmd = "echo snoop_enable = `cat /sys/module/hfi/parameters/snoop_enable`"
    (res,output) = host.send_ssh(cmd)
    if res:
        RegLib.test_log(0, "SSH failed")
        return False

    for line in output:
        matchObj = re.match(r"snoop_enable = 1", line)
        if matchObj:
            snoop_on = 1
            print RegLib.chomp(line)

    if snoop_on != 1:
        RegLib.test_log(0, "Snoop is not enabled")
        return False

    RegLib.test_log(0, "Snoop is enabled")
    return True

def enable_snoop(host1, host2, directory, mod_parms):
    host = host1.get_name() + "," + host2.get_name() 
    os.system(directory + "/LoadModule.py --nodelist " + hosts + " --modparm \"" + params + "\"")
    if (snoop_enabled(host1) == False or snoop_enabled(host2) == False):
        error("Could not enable snoop and snoop_drop_send on all hosts")

def start_pcap(host, cmd):
    RegLib.test_log(0, "Starting pcap process [%s] on %s" % (cmd, host.get_name())) 
    pid = os.fork()
    if pid == 0:
        sys.exit(host.send_ssh(cmd, False))  
    return pid

def kill_pcap(host1, host2):
    cmd = "killall -2 PcapLocal.py"
    ret = host1.send_ssh(cmd, False)
    if ret:
        RegLib.test_log(0, "WARNING: Coudld not stop PcapLocal")
    ret = host2.send_ssh(cmd, False)
    if ret:
        RegLib.test_log(0, "WARNING: Coudld not stop PcapLocal")

def is_sm_active(host, sm):
    cmd = "/sbin/service %s status" % sm
    ret = host.send_ssh(cmd, 0)
    if ret != 0:
        RegLib.test_log(0, "%s is not running" % sm)
    else:
        RegLib.test_log(0, "%s is running" % sm)
    
    return ret


def main():

    directory = os.path.dirname(os.path.realpath(__file__))
    pcap = directory + "/PcapLocal.py"

    test_info = RegLib.TestInfo()
    if test_info.is_simics():
        pcap = "/host" + pcap

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)

    default_parms = test_info.get_mod_parms()
    snoop_parms = default_parms + " snoop_enable=1"
    RegLib.test_log(0, "Default parms [%s] Snoop parms [%s]" % (default_parms, snoop_parms))

    if (snoop_enabled(host1) == False or snoop_enabled(host2) == False):
            enable_snoop(host1, host2, directory, snoop_parms)
  
    RegLib.test_log(0, "Trying to determine sm")

    ifs_fm_host = None
    for host in host1, host2:
        active = is_sm_active(host, "opensm")
        if active == 0:
            RegLib.test_fail("OpenSM not supported!")

        active = is_sm_active(host, "ifs_fm")
        if active == 0:
            if ifs_fm_host == None:
                ifs_fm_host = host
                RegLib.test_log(0, "Detected ifs_fm on %s" % host.get_name())
            else:
                RegLib.test_fail("ifs_fm detected on both nodes")

    if ifs_fm_host == None:
        RegLib.test_fail("ifs_fm not detected on any host")

    cmd = pcap + " --args verbose 2>&1 > /tmp/snoop_integ.log"

    pcap1 = start_pcap(host1, cmd)
    pcap2 = start_pcap(host2, cmd)
    
    RegLib.test_log(0, "Started pcap1 [%d] and pcap2 [%d]" % (pcap1, pcap2))
    
    RegLib.test_log(0, "Waiting 5 seconds for Pcap procs to get started")
    time.sleep(5)
    ret = ifs_fm_host.send_ssh("service ifs_fm sweep", False)
    if ret:
        kill_pcap(host1, host2)
        RegLib.test_fail("Could not sweep fabric")

    RegLib.test_log(0, "Ending Pcap procs")
    kill_pcap(host1, host2)

    (waited, status) = os.waitpid(pcap1, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Pcap1 process failed")

    (waited, status) = os.waitpid(pcap2, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Pcap2 process failed")

    # Now verify the packet MD5s match up or else we got a problem
    # Ok technically its possible that some packets make it into one
    # of the logs but not the other due to the fact that they can
    # not be started atomically but if that becomes a problem we
    # can deal with it later by inspecing and looking for only packets
    # that we care about

    (ret, checksums1) = host1.send_ssh("grep MD5 /tmp/snoop_integ.log", True)
    if ret:
        RegLib.test_fail("Could not retrieve checksums for host1")

    (ret, checksums2) = host1.send_ssh("grep MD5 /tmp/snoop_integ.log", True)
    if ret:
        RegLib.test_fail("Could not retrieve checksums for host2")

    for checksum in checksums1:
        RegLib.test_log(0, "Checking for %s" % checksum.strip())
        found = 0
        for target in checksums2:
            if checksum == target:
                RegLib.test_log(0, "Matched: %s" % target.strip())
                found = 1
        if found == 0:
            RegLib.test_fail("Could not find checksum %s" % checksum)

    RegLib.test_pass("Completed!")

if __name__ == "__main__":
    main()

