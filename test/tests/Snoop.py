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
err_cnt = 0
captured = 0

def snoop_do_not_intercept_outgoing(host, lid1, lid2):
    global snoop_path

    cmd = snoop_path + " --args=\"0x1,%s,0,1,0,%s,%s,0\"" % (lid2,lid1,lid2)

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        print "Cmd", cmd
        (res, output) = host.send_ssh(cmd, 1, run_as_root=True)
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
                    dlid = hex(int(dlid))
                    slid = hex(int(slid))
                    if (dlid == lid1) and (slid == lid2):
                        ping = ping+1
                    elif (dlid == lid2) and (slid == lid1):
                        pong = pong + 1
                    else:
                        RegLib.test_log(0, "Looking for Lid %s, %s" % (lid1,lid2))
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 0 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def snoop_intercept_outgoing(host, lid1, lid2):
    global snoop_path

    cmd = snoop_path + " --args=\"0x1,%s,0,0,0,%s,%s,0\"" % (lid2, lid1,lid2)

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(cmd, 1, run_as_root=True)
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
                    dlid = hex(int(dlid))
                    slid = hex(int(slid))
                    if (dlid == lid1) and (slid == lid2):
                        ping = ping+1
                    elif (dlid == lid2) and (slid == lid1):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 0 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def snoop_intercept_outgoing_2(host, lid1, lid2, drop_send="0"):
    global snoop_path

    if drop_send == "0":
        cmd = snoop_path + " --args=\"0x1,%s,0,0,1,%s,%s,%s\"" % (lid1, lid1, lid2, 0)
    else:
        cmd = snoop_path + " --args=\"0x0,%s,0,0,1,%s,%s,%s\"" % (lid1, lid1, lid2, 1)

    print "Snoop cmd:", cmd
    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(cmd, 1, run_as_root=True)
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
                    dlid = hex(int(dlid))
                    slid = hex(int(slid))
                    if (dlid == lid1) and (slid == lid2):
                        ping = ping+1
                    elif (dlid == lid2) and (slid == lid1):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 5 or pong != 0:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid


def snoop_intercept_outgoing_3(host,lid1, lid2):
    global snoop_path

    cmd = snoop_path + " --args=\"0x1,%s,1,0,0,%s,%s,0\"" % (lid2, lid1, lid2)

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting SnoopLocal.py on %s" % host.get_name())
        print "CMD:", cmd
        (res, output) = host.send_ssh(cmd, 1, run_as_root=True)
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
                    dlid = hex(int(dlid))
                    slid = hex(int(slid))
                    if (dlid == lid1) and (slid == lid2):
                        ping = ping+1
                    elif (dlid == lid2) and (slid == lid1):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Snoop Ping:", ping, "Pong", pong
        if ping != 0 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def capture_do_not_intercept_outgoing(host, lid1, lid2):
    global capture_path;

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting PcapLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(capture_path, 1, run_as_root=True)
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
                    dlid = hex(int(dlid))
                    slid = hex(int(slid))
                    if (dlid == lid1) and (slid == lid2):
                        ping = ping+1
                    elif (dlid == lid2) and (slid == lid1):
                        pong = pong + 1
                print "Size:", size, "Dlid", dlid, "Slid", slid
        print "Capture: Ping:", ping, "Pong", pong
        if ping != 5 or pong != 5:
            RegLib.test_log(0, "Got %d pings and %d pongs only" % (ping, pong))
            sys.exit(1)
        else:
            sys.exit(0)

    return pid

def capture_do_not_intercept_outgoing_2(host, lid1, lid2):
    global capture_path;

    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting PcapLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(capture_path, 1, run_as_root=True)
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
                    lid = hex(int(dlid))
                    lid = hex(int(slid))
                    if (dlid == lid1) and (slid == lid2):
                        ping = ping+1
                    elif (dlid == lid2) and (slid == lid1):
                        pong = pong + 1
                    elif (slid == lid1):
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

    cmd = directory + "/IbSendLat.py --nodelist " + host1.get_name() + "," + host2.get_name()
    RegLib.test_log(0, "IbSendLat cmd is %s" % cmd)
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    status = proc.wait()
    if status:
        error("IbSendLat failed")

    #output = proc.stdout.readlines()
    #for line in output:
    #    print RegLib.chomp(line)

    RegLib.test_log(0, "Killing off Snoop")
    cmd = "killall -2 SnoopLocal.py"
    status = host1.send_ssh(cmd, 0, run_as_root=True)
    if status:
        error("Could not stop Snoop on remote")

    RegLib.test_log(0, "Killing off Pcap")
    cmd = "killall -2 PcapLocal.py"
    status = host2.send_ssh(cmd, 0, run_as_root=True)
    if status:
        error("Could not stop Pcap on remote")

def run_ib_send_lat_2():
    global host1
    global host2
    global directory

    RegLib.test_log(0, "Waiting for 5 seconds for Snoop/Pcap pids to get ready")
    time.sleep(5)

    RegLib.test_log(0, "Running IbSendLat, be patient")

    cmd = directory + "/IbSendLat.py --nodelist " + host1.get_name() + "," + host2.get_name()
    proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)

    #verify ib_send_lat has started
    max_attempts = 10
    attempts = 1
    test_failed = 0
    while attempts <= max_attempts:

        cmd = "/usr/bin/ps -ef | /usr/bin/grep ib_send_lat | /usr/bin/grep -v grep"
        status = host2.send_ssh(cmd, 0, run_as_root=True)

        if status == 1:
            time.sleep(5)
            attempts += 1
            continue
        else:
           break

    if attempts > max_attempts:
        RegLib.test_fail("ib_send_lat failed to start")


    # IbSendLat will not finish this is by design so give it 15 seconds
    # before killing it off. That is more than enough time even on simics.
    RegLib.test_log(0, "Sleeping 10 seconds for packets to move")
    time.sleep(5)
    RegLib.test_log(0, "...5 more")
    time.sleep(5)

    RegLib.test_log(0, "Killing off IbSendLat")
    cmd = "killall -9 ib_send_lat"
    status = host1.send_ssh(cmd, 0, run_as_root=False)
    if status:
        RegLib.test_log(5,"Could not stop ib_send_lat on 1")

    cmd = "killall -9 ib_send_lat"
    status = host2.send_ssh(cmd, 0, run_as_root=False)
    if status:
        RegLib.test_log(5,"Could not stop ib_send_lat on 2")

    RegLib.test_log(0, "Killing off Snoop")
    cmd = "killall -2 SnoopLocal.py"
    status = host1.send_ssh(cmd, 0, run_as_root=True)
    if status:
        error("Could not stop Snoop on remote")

    RegLib.test_log(0, "Killing off Pcap")
    cmd = "killall -2 PcapLocal.py"
    status = host2.send_ssh(cmd, 0, run_as_root=True)
    if status:
        error("Could not stop Pcap on remote")

    proc.wait()
    #output = proc.stdout.readlines()
    #for line in output:
    #    print RegLib.chomp(line)

def capture_bypass(host):
    global capture_path;
    captured = 0
    pid = os.fork()
    if pid == 0:
        RegLib.test_log(0, "Starting PcapLocal.py on %s" % host.get_name())
        (res, output) = host.send_ssh(capture_path, 1, run_as_root=True)
        for line in output:
            # SLID and DLID are wonky because the bypass packets are in big endian mode
            # we can use the known values here to find them because normally SLID/DLID are
            # each either 1 or 2 in simics and most surely not the exact combination on anything
            # else. It is not trivial to turn the string form of a 64bit number into an integer
            # in python and mask to look for the packet type, there is just as much chance it could
            # randomly be corrupted or set to refelct bypass as the following 3 checks.
            matchObj = re.search(r"PacketBytes: 72 .+ INGRESS .+SLID: 160 DLID: 36864", line)
            if matchObj:
                print line.strip()
                captured = captured + 1
        RegLib.test_log(0, "Captured %d packets" % captured)

        if captured == 10:
            sys.exit(0)
        else:
            sys.exit(1)

    return pid

# Stop PcapLocal on a host
def stop_pcap(host):
    RegLib.test_log(0, "Killing off Pcap")
    cmd = "killall -2 PcapLocal.py"
    status = host.send_ssh(cmd, 0, run_as_root=True)
    if status:
        error("Could not stop Pcap on remote")

def send_bypass(host, lid, diag_path, pkt_dir):
    RegLib.test_log(0, "Waiting for a packet")

    cmd_pattern = "%shfi1_pkt_send -L %s -c %d -k %s%s"
    count = 10
    cmd = cmd_pattern % (diag_path, lid, count, pkt_dir, "bypass10")
    RegLib.test_log(0, "Going to run %s" % cmd)

    status = host.send_ssh(cmd, 0, run_as_root=True)
    if status:
        error("Could not send bypass packets")

def error(msg):
    global err_cnt

    err_cnt = err_cnt + 1

    RegLib.test_log(0, "XXXXX ERROR XXXXX")
    RegLib.test_log(0, "%s" % msg)
    RegLib.test_log(0, "XXXXX ERROR XXXXX")

def done_check_error():
    global err_cnt

    RegLib.test_log(0, "Test completed, checking errors")

    if err_cnt:
        RegLib.test_fail("Test Failed due to %d errors" % err_cnt)

    RegLib.test_pass("Completed!")

def main():
    global host1
    global host2
    global snoop_path
    global capture_path
    global err_cnt

    test_info = RegLib.TestInfo()
    if test_info.is_simics() == True:
        snoop_path = "/host" + snoop_path
        capture_path = "/host" + capture_path

    print "snoop path is", snoop_path
    print "capture path is", capture_path

    unit_test = test_info.parse_extra_args()
    if unit_test[0] == '':
        unit_test = None

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)

    nodes = host1.get_name() + "," + host2.get_name()

    diag_path = test_info.get_diag_lib()
    if diag_path == "DEFAULT":
	    # Assume the diagtools are already installed somewhere in $PATH.
        diag_path = ""
    else:
	    diag_path += "/build/targ-x86_64/tests/"

    pkt_dir = test_info.get_test_pkt_dir() + "/"

    print "XXXXXXXXXXXXXXX"
    print "Starting Test 1"
    print "XXXXXXXXXXXXXXX"
    # Use snoop to only see INCOMING packets based on DLID. Then ignore them. Do
    # not echo them back. Snoop pid will see 0 pings and 5 pongs.
    # Cpature pid will see all 5 pings and all 5 pongs.
    snoop_pid = snoop_do_not_intercept_outgoing(host1, host1.get_lid(),host2.get_lid())
    capture_pid = capture_do_not_intercept_outgoing(host2, host1.get_lid(), host2.get_lid())

    run_ib_send_lat()

    RegLib.test_log(0, "Waiting for snoop pid to stop and validate")
    (waited, status) = os.waitpid(snoop_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        error("Snoop Proc Test 1 failed")

    RegLib.test_log(0, "Waiting for capture pid to stop and validate")
    (waited, status) = os.waitpid(capture_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        error("Cpature Proc Test 1 failed")

    print "XXXXXXXXXXXXXXX"
    print "Starting Test 2"
    print "XXXXXXXXXXXXXXX"
    # Uses snoop to only see INCOMING packets based on DLID. This time use snoop
    # device to write the response. This makes ib_send_lat not see the packets
    # on the snooping side. The ib_send_lat on the other side knows no
    # difference.
    snoop_pid = snoop_intercept_outgoing_2(host1, host1.get_lid(), host2.get_lid())
    capture_pid = capture_do_not_intercept_outgoing(host2, host1.get_lid(), host2.get_lid())

    run_ib_send_lat_2()

    RegLib.test_log(0, "Waiting for snoop pid to stop and validate")
    (waited, status) = os.waitpid(snoop_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        error("Snoop Proc Test 2 failed")

    RegLib.test_log(0, "Waiting for capture pid to stop and validate")
    (waited, status) = os.waitpid(capture_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        error("Cpature Proc Test 2 failed")

    print "XXXXXXXXXXXXXXX"
    print "Starting Test 3"
    print "XXXXXXXXXXXXXXX"
    # Just like above except do not use filtering. Rely on the DROPSEND
    # capabilitiy to drop the packet. Add an argument to the end of the arg
    # chain for setting the drop send IOCTL.
    snoop_pid = snoop_intercept_outgoing_2(host1, host1.get_lid(),
                                           host2.get_lid(), 1)
    capture_pid = capture_do_not_intercept_outgoing(host2, host1.get_lid(), host2.get_lid())

    run_ib_send_lat_2()

    RegLib.test_log(0, "Waiting for snoop pid to stop and validate")
    (waited, status) = os.waitpid(snoop_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        error("Snoop Proc Test 3 failed")

    RegLib.test_log(0, "Waiting for capture pid to stop and validate")
    (waited, status) = os.waitpid(capture_pid, 0)
    RegLib.test_log(0, "Pid %d exited with %d status" % (waited, status))
    if status:
        error("Cpature Proc Test 3 failed")

    if unit_test:
        print "XXXXXXXXXXXXXXX"
        print "Starting Test 4"
        print "XXXXXXXXXXXXXXX"
        RegLib.test_log(0, "Enabling snoop for bypass");

        RegLib.test_log(0, "Starting packet capture on host2")
        capture_pid = capture_bypass(host2)

        RegLib.test_log(0, "Sleeping 5 seconds to let capture start up")
        time.sleep(5)

        lid = host2.get_lid()
        RegLib.test_log(0, "Sending bypass packets from host1 to lid %s" %lid);
        send_bypass(host1, lid, diag_path, pkt_dir)

        RegLib.test_log(0, "Stopping PCAP and checking results")
        stop_pcap(host2)
        (waited, success) = os.waitpid(capture_pid, 0)

        if success == 0:
            RegLib.test_log(0, "Successfully got 10 bypass packets")
        else:
            error("Wrong number of bypass packets.")

    done_check_error()

if __name__ == "__main__":
    main()

