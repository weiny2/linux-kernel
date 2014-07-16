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
snoop_path = directory + "/SnoopLocal.py"
capture_path = directory + "/PcapLocal.py"

def enable_snoop():
    os.system(directory + "/LoadModule.py --modparm \"snoop_enable=1\"")

def enable_snoop_drop_send():
    os.system(directory + "/LoadModule.py --modparm \"snoop_enable=1 snoop_drop_send=1\"")

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

def snoop_drop_send_enabled():
    # Firs check and see if snoop is enabled.
    global host1
    global host2
    for host in [host1, host2]:
        RegLib.test_log(0, "Checking snoop drop_send enablement for %s" % host.get_name())
        snoop_on = 0
        cmd = "echo snoop_drop_send = `cat /sys/module/hfi/parameters/snoop_drop_send`"
        (res,output) = host.send_ssh(cmd)
        if res:
            RegLib.test_fail("Could not get status for host %s" %
                             host.get_name())
        for line in output:
            matchObj = re.match(r"snoop_drop_send = 1", line)
            if matchObj:
                snoop_on = 1
                print RegLib.chomp(line)

        if snoop_on != 1:
            return False

    return True

def snoop_do_not_intercept_outgoing(host):
    global snoop_path

    cmd = snoop_path + " --args=\"0x1,0x2,0,1,0\""

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        print "Cmd", cmd
        (res, output) = host.send_ssh(cmd, 1)
        ping = 0;
        pong = 0;
        for line in output:
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 1 slid 2
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 2 slid 1
            matchObj = re.search(r"logged (\d+) byte pkt .+ dlid (\d+) slid (\d+)",
                                 line)
            if matchObj:
                #print RegLib.chomp(line)
                size = matchObj.group(1)
                dlid = matchObj.group(2)
                slid = matchObj.group(3)
                if size == "36":
                    # Why 36? This is because it is a 2 byte write which gets
                    # padded to 4 bytes. So 4 byte payload + 28 byte header + 4
                    # byte ICRC = 36 or 9 Dwords. The test does a 5x ping pong
                    if (dlid == "1") and (slid == "2"):
                        ping = ping+1
                    elif (dlid == "2") and (slid == "1"):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 0 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def snoop_intercept_outgoing(host):
    global snoop_path

    cmd = snoop_path + " --args=\"0x1,0x2,0,0,0\""

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(cmd, 1)
        ping = 0;
        pong = 0;
        for line in output:
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 1 slid 2
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 2 slid 1
            matchObj = re.search(r"logged (\d+) byte pkt .+ dlid (\d+) slid (\d+)",
                                 line)
            if matchObj:
                #print RegLib.chomp(line)
                size = matchObj.group(1)
                dlid = matchObj.group(2)
                slid = matchObj.group(3)
                if size == "36":
                    # Why 36? This is because it is a 2 byte write which gets
                    # padded to 4 bytes. So 4 byte payload + 28 byte header + 4
                    # byte ICRC = 36 or 9 Dwords. The test does a 5x ping pong
                    if (dlid == "1") and (slid == "2"):
                        ping = ping+1
                    elif (dlid == "2") and (slid == "1"):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 0 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def snoop_intercept_outgoing_2(host):
    global snoop_path

    cmd = snoop_path + " --args=\"0x1,0x1,0,0,1\""

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(cmd, 1)
        ping = 0;
        pong = 0;
        for line in output:
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 1 slid 2
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 2 slid 1
            matchObj = re.search(r"logged (\d+) byte pkt .+ dlid (\d+) slid (\d+)",
                                 line)
            if matchObj:
                #print RegLib.chomp(line)
                size = matchObj.group(1)
                dlid = matchObj.group(2)
                slid = matchObj.group(3)
                if size == "36":
                    # Why 36? This is because it is a 2 byte write which gets
                    # padded to 4 bytes. So 4 byte payload + 28 byte header + 4
                    # byte ICRC = 36 or 9 Dwords. The test does a 5x ping pong
                    if (dlid == "1") and (slid == "2"):
                        ping = ping+1
                    elif (dlid == "2") and (slid == "1"):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 5 or pong != 0:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid


def snoop_intercept_outgoing_3(host):
    global snoop_path

    cmd = snoop_path + " --args=\"0x1,0x2,1,0,0\""

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(cmd, 1)
        ping = 0;
        pong = 0;
        for line in output:
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 1 slid 2
            #.....logged 36 byte pkt vl 0 pktlen 9 Dwords dlid 2 slid 1
            matchObj = re.search(r"logged (\d+) byte pkt .+ dlid (\d+) slid (\d+)",
                                 line)
            if matchObj:
                #print RegLib.chomp(line)
                size = matchObj.group(1)
                dlid = matchObj.group(2)
                slid = matchObj.group(3)
                if size == "36":
                    # Why 36? This is because it is a 2 byte write which gets
                    # padded to 4 bytes. So 4 byte payload + 28 byte header + 4
                    # byte ICRC = 36 or 9 Dwords. The test does a 5x ping pong
                    if (dlid == "1") and (slid == "2"):
                        ping = ping+1
                    elif (dlid == "2") and (slid == "1"):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 0 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def capture_do_not_intercept_outgoing(host):
    global capture_path;

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting PcapLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(capture_path, 1)
        ping = 0;
        pong = 0;
        for line in output:
            matchObj = re.search(r"PacketBytes: (\d+) .+ SLID: (\d+) DLID: (\d+)",
                                 line)
            if matchObj:
                #print RegLib.chomp(line)
                size = matchObj.group(1)
                slid = matchObj.group(2)
                dlid = matchObj.group(3)
                if size == "36":
                    # Why 36 This is because it is a 2 byte write which gets
                    # padded to 4 bytes. So 4 byte payload + 28 byte header + 4
                    # byte ICRC = 36 or 9 Dwords. There is a 16 byte header
                    # for Pcap metadata but it is not included in PacketBytes.
                    # The test does a 5x ping pong
                    if (dlid == "1") and (slid == "2"):
                        ping = ping+1
                    elif (dlid == "2") and (slid == "1"):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Capture: Ping:", ping, "Pong", pong
        if ping != 5 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def capture_do_not_intercept_outgoing_2(host):
    global capture_path;

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting PcapLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(capture_path, 1)
        ping = 0
        pong = 0
        foreign_lid = 0
        for line in output:
            matchObj = re.search(r"PacketBytes: (\d+) .+ SLID: (\d+) DLID: (\d+)",
                                 line)
            if matchObj:
                #print RegLib.chomp(line)
                size = matchObj.group(1)
                slid = matchObj.group(2)
                dlid = matchObj.group(3)
                if size == "36":
                    # Why 36? See previous function
                    if (dlid == "1") and (slid == "2"):
                        ping = ping+1
                    elif (dlid == "2") and (slid == "1"):
                        pong = pong + 1
                    elif (slid == "1"):
                        foreign_lid = foreign_lid + 1
                        print "Landed foreign lid"
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Capture: Ping:", ping, "Pong", pong
        if ping != 5 or pong != 5 or foreign_lid != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs %d foreign lidsonly" % (ping, pong, foreign_lid))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def run_ib_send_lat():
    global host1
    global host2
    global directory

    RegLib.test_log(0, "Waiting for 5 seconds for Snoop/Pcap pids to get ready")
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

    RegLib.test_log(0, "Killing off Snoop")
    cmd = "killall -2 SnoopLocal.py"
    status = host1.send_ssh(cmd, 0)
    if status:
        RegLib.test_fail("Could not stop Snoop on remote")

    RegLib.test_log(0, "Killing off Pcap")
    cmd = "killall -2 PcapLocal.py"
    status = host2.send_ssh(cmd, 0)
    if status:
        RegLib.test_fail("Could not stop Pcap on remote")

def run_ib_send_lat_2():
    global host1
    global host2
    global directory

    RegLib.test_log(0, "Waiting for 5 seconds for Snoop/Pcap pids to get ready")
    time.sleep(5)

    RegLib.test_log(0, "Running IbSendLat, be patient")

    cmd = directory + "/IbSendLat.py"
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)


    # IbSendLat will not finish this is by design so give it 30 seconds
    # before killing it off
    RegLib.test_log(0, "Sleeping 30 seconds for packets to move")
    time.sleep(10)
    RegLib.test_log(0, "...20 more")
    time.sleep(10)
    RegLib.test_log(0, "...10 more")
    time.sleep(10)

    RegLib.test_log(0, "Killing off IbSendLat")
    cmd = "killall -9 ib_send_lat"
    status = host1.send_ssh(cmd, 0)
    if status:
        RegLib.test_log(5,"Could not stop ib_send_lat on 1")

    cmd = "killall -9 ib_send_lat"
    status = host2.send_ssh(cmd, 0)
    if status:
        RegLib.test_log(5,"Could not stop ib_send_lat on 2")

    RegLib.test_log(0, "Killing off Snoop")
    cmd = "killall -2 SnoopLocal.py"
    status = host1.send_ssh(cmd, 0)
    if status:
        RegLib.test_fail("Could not stop Snoop on remote")

    RegLib.test_log(0, "Killing off Pcap")
    cmd = "killall -2 PcapLocal.py"
    status = host2.send_ssh(cmd, 0)
    if status:
        RegLib.test_fail("Could not stop Pcap on remote")

    proc.wait()
    #output = proc.stdout.readlines()
    #for line in output:
    #    print RegLib.chomp(line)

def main():
    global host1
    global host2
    global snoop_path
    global capture_path

    test_info = RegLib.TestInfo()
    if test_info.is_simics():
        snoop_path = "/host" + snoop_path
        capture_path = "/host" + capture_path


    print "snoop path is", snoop_path
    print "capture path is", capture_path

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)

    print "XXXXXXXXXXXXXXX"
    print "Starting Test 1"
    print "XXXXXXXXXXXXXXX"

    if not snoop_enabled():
        enable_snoop()
        if not snoop_enabled():
            RegLib.test_fail("Could not enable snoop")

    if snoop_drop_send_enabled():
        enable_snoop()
        if not snoop_enabled():
            RegLib.test_fail("Could not enable snoop")
        if snoop_drop_send_enabled():
            RegLib.test_fail("Snoop drop send is enabled and it should be off")

    snoop_pid = snoop_do_not_intercept_outgoing(host1)
    capture_pid = capture_do_not_intercept_outgoing(host2)

    run_ib_send_lat()

    RegLib.test_log(0, "Waiting for snoop pid to stop and validate")
    (waited, status) = os.waitpid(snoop_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Snoop Proc Test 1 failed")

    RegLib.test_log(0, "Waiting for capture pid to stop and validate")
    (waited, status) = os.waitpid(capture_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Cpature Proc Test 1 failed")

    print "XXXXXXXXXXXXXXX"
    print "Starting Test 2"
    print "XXXXXXXXXXXXXXX"
    if not snoop_drop_send_enabled():
        enable_snoop_drop_send()
        if not snoop_drop_send_enabled():
            RegLib.test_fail("Could not enable snoop drop send")

    if not snoop_enabled():
        RegLib.test_fail("Snoop got disabled somehow")

    snoop_pid = snoop_intercept_outgoing(host1)
    capture_pid = capture_do_not_intercept_outgoing(host2)

    run_ib_send_lat()

    RegLib.test_log(0, "Waiting for snoop pid to stop and validate")
    (waited, status) = os.waitpid(snoop_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Snoop Proc Test 2 failed")

    RegLib.test_log(0, "Waiting for capture pid to stop and validate")
    (waited, status) = os.waitpid(capture_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Cpature Proc Test 2 failed")

    print "XXXXXXXXXXXXXXX"
    print "Starting Test 3"
    print "XXXXXXXXXXXXXXX"

    if not snoop_enabled():
        enable_snoop()
        if not snoop_enabled():
            RegLib.test_fail("Could not enable snoop")

    if snoop_drop_send_enabled():
        enable_snoop()
        if not snoop_enabled():
            RegLib.test_fail("Could not enable snoop")
        if snoop_drop_send_enabled():
            RegLib.test_fail("Snoop drop send is enabled and it should be off")

    snoop_pid = snoop_intercept_outgoing_2(host1)
    capture_pid = capture_do_not_intercept_outgoing(host2)

    run_ib_send_lat_2()

    RegLib.test_log(0, "Waiting for snoop pid to stop and validate")
    (waited, status) = os.waitpid(snoop_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Snoop Proc Test 2 failed")

    RegLib.test_log(0, "Waiting for capture pid to stop and validate")
    (waited, status) = os.waitpid(capture_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Cpature Proc Test 2 failed")

    print "XXXXXXXXXXXXXXX"
    print "Starting Test 4"
    print "XXXXXXXXXXXXXXX"

    if not snoop_enabled():
        enable_snoop()
        if not snoop_enabled():
            RegLib.test_fail("Could not enable snoop")

    if snoop_drop_send_enabled():
        enable_snoop()
        if not snoop_enabled():
            RegLib.test_fail("Could not enable snoop")
        if snoop_drop_send_enabled():
            RegLib.test_fail("Snoop drop send is enabled and it should be off")

    snoop_pid = snoop_intercept_outgoing_3(host1)
    capture_pid = capture_do_not_intercept_outgoing_2(host2)

    run_ib_send_lat_2()

    RegLib.test_log(0, "Waiting for snoop pid to stop and validate")
    (waited, status) = os.waitpid(snoop_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Snoop Proc Test 4 failed")

    RegLib.test_log(0, "Waiting for capture pid to stop and validate")
    (waited, status) = os.waitpid(capture_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        RegLib.test_fail("Cpature Proc Test 4 failed")

    print "Completed"

    restore_default_params()

    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

