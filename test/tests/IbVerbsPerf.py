#!/usr/bin/python

# Test IbVerbsPerf.py by Dennis Dalessandro (dennis.dalessandro@intel.com)

import RegLib
import time
import sys
import re
import os
import time
import datetime
import tempfile

def do_ssh(host, cmd, run_as_root=True):
    RegLib.test_log(0, "Running SSH CMD: " + cmd)
    return (host.send_ssh(cmd, run_as_root=run_as_root))

def ipoib_loaded(host):
    driver_name = "ib_ipoib"
    cmd = "/sbin/lsmod"
    (err, out) = do_ssh(host, cmd)
    loaded = 0
    for line in out:
        if driver_name in line:
            return True
    return False

def create_test_dir():

    # Create a test directory for the data
    unix_time = time.localtime()
    str_time = time.strftime("%Y%m%d%H%M")

    test_dir = "ibv_perf.%s" % str_time
    RegLib.test_log(0, "Creating test output dir %s" % test_dir)

    if os.path.exists(test_dir):
        RegLib.test_fail("Could not create test output dir")

    os.mkdir(test_dir)
    if not os.path.exists(test_dir):
        RegLib.test_fail("Could not create test dir")

    return test_dir

def start_server(host, cmd, dir, file):

    child_pid = os.fork()
    if child_pid == 0:
        (err, out) = do_ssh(host, cmd)
        if err:
            RegLib.test_log(0, "Child SSH exit status bad")

        f = open(dir + "/" + file + ".server", "w")

        for line in out:
            f.write(line)
            print line.strip()
        f.close()
        sys.exit(err)

    return child_pid

def do_client(host, cmd, dir, file):

    (err, out) = do_ssh(host, cmd)

    f = open(dir + "/" + file + ".client", "w")

    for line in out:
        f.write(line)
        print line.strip()
    f.close()

    return err

def kill_all(host):
    # Kill off any straglers. Do not worry about figuring out the
    # right thing, just kill all of it
    do_ssh(host, "killall -9 ib_write_bw")
    do_ssh(host, "killall -9 ib_read_bw")
    do_ssh(host, "killall -9 ib_send_lat")
    do_ssh(host, "killall -9 ib_read_lat")


def main():
    server_delay = 3

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()
    RegLib.test_log(0, "Test: IbWriteBwPerf.py started")
    RegLib.test_log(0, "Dumping test parameters")

    # Dump out the test and host info. We need 2 hosts for this test
    print test_info
    count = test_info.get_host_count()
    if count < 2:
        RegLib.test_fail("Not enough hosts")

    print "Found ", count, "hosts"
    host1 = test_info.get_host_record(0)
    print host1
    host2 = test_info.get_host_record(1)
    print host2

    ################
    # body of test #
    ################

    # We need IPoIB so we can use RDMA CM and automagically get 8K MTU
    if host1.get_name() == host2.get_name():
        RegLib.test_fail("No point in donig this loopback")

    unload_ipoib = False # if we load it then unload it
    if (ipoib_loaded(host1) == False) or (ipoib_loaded(host2) == False):
        (err, out) = do_ssh(host1, "modprobe ib_ipoib", True)
        if err:
            RegLib.test_fail("Could not load ipoib on host1")
        (err, out) = do_ssh(host2, "modprobe ib_ipoib", True)
        if err:
            RegLib.test_fail("Could not load ipoib on host2")
        RegLib.test_log(0, "Waiting 10 seconds for Ipoib to get running")
        time.sleep(10)
        unload_ipoib = True

    # Always attempt to bring up ib0
    (err, out) = do_ssh(host1, "ifup ib0", True)
    if err:
        print out
        RegLib.test_fail("Could not ifup ib0 on host1")

    (err, out) = do_ssh(host2, "ifup ib0", True)
    if err:
        print out
        RegLib.test_fail("Could not ifup ib0 on host1")

    ibv_perf_path = test_info.get_perf_path()

    if not ibv_perf_path.endswith("/"):
        ibv_perf_path = ibv_perf_path + "/"

    RegLib.test_log(0, "Perf path is %s" % ibv_perf_path)

    # Must start with a blank space
    ibv_std_opts = " -R -n 5000 -a"
    test_list = [   ibv_perf_path + "ib_write_bw" + ibv_std_opts,
                    ibv_perf_path + "ib_read_bw" + ibv_std_opts,
                    ibv_perf_path + "ib_write_bw -b" + ibv_std_opts,
                    ibv_perf_path + "ib_read_bw -b" + ibv_std_opts,
                    ibv_perf_path + "ib_send_lat" + ibv_std_opts,
                    ibv_perf_path + "ib_read_lat" + ibv_std_opts,
                ]

    test_names = [ "ib_write_bw", "ib_read_bw", "ib_write_bw_bidir",
                   "ib_read_bw_bidir", "ib_send_lat", "ib_read_lat"]

    if len(test_list) != len(test_names):
        RegLib.test_fail("Fool programmer")

    test_index = 0

    test_dir = create_test_dir()

    while test_index < len(test_list):
        test = test_list[test_index]

        cmd = "taskset -c 4 %s -d hfi1_0" % (test)
        child_pid = start_server(host1, cmd, test_dir, test_names[test_index])

        RegLib.test_log(0, "Giving server thread %d seconds to start %s" % (server_delay, test))
        time.sleep(server_delay)

        server_name = host1.get_name() + "-ib"
        cmd = cmd + " " + server_name
        test_fail = 0

        ret = do_client(host2, cmd, test_dir, test_names[test_index])
        if ret:
            test_fail = 1
            RegLib.test_log(0, "Client Failed!")
            kill_all(host2)
            kill_all(host1)

        (pid, status) = os.waitpid(child_pid, 0)
        if status != 0:
            if not test_fail:
                RegLib.test_log(0, "Child proc reported unexpected bad exit status")
                kill_all(host1)
                test_fail = 1
            else:
                RegLib.test_log(0, "Child proc expected bad exit status.. OK")

        if test_fail:
            RegLib.test_log(0, "Trying test again")
        else:
            # Move on to next test
            test_index = test_index + 1

    if unload_ipoib == True:
        (err, out) = do_ssh(host1, "rmmod ib_ipoib")
        if err:
            RegLib.test_fail("Could not unload ipoib on host1")

        (err, out) = do_ssh(host2, "rmmod ib_ipoib")
        if err:
            RegLib.test_fail("Could not unload ipoib on host2")

        RegLib.test_log(0, "Waiting 10 seconds for Ipoib to stop running")
        time.sleep(10)

    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

