#!/usr/bin/python

# Test PerfReg
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time
from datetime import date

test_failed = 0

def check_output(output):
    global test_failed
    RegLib.test_log(0, "Checking output...")

    for line in output:
        RegLib.test_log(0, "%s" % line.rstrip())
        if ("fail" in line) or ("Test_Failed" in line):
            RegLib.test_log(0, "Test [%s] failed!" % line.rstrip())
            test_failed = test_failed + 1

def test_mpi(host1, host2, temp_dir, perf_test_dir):
    wfr_mpi="/usr/mpi/gcc/openmpi-1.10.2-hfi"
    wfr_mpivars="/usr/mpi/gcc/openmpi-1.10.2-hfi/bin/mpivars.sh"
    mpi_dir = perf_test_dir + "/scripts/regression/osu-perf-check "
    RegLib.test_log(5, "Running MPI test")
    RegLib.test_log(5, "This test will set the CPU freq")

    hosts=host1.get_name() + "_" + host2.get_name() + ".hosts"

    host_cmd="echo %s > %s/%s" % (host1.get_name(), temp_dir, hosts)
    (ret) = host1.send_ssh(host_cmd, buffered=0)
    if (ret):
        RegLib.test_fail("Could not create host file")

    host_cmd="echo %s >> %s/%s" % (host2.get_name(), temp_dir, hosts)
    (ret) = host1.send_ssh(host_cmd, buffered=0)
    if (ret):
        RegLib.test_fail("Could not add host2 to host file")

    turbo="noturbo"

    mpi_cmd="cd %s && source %s && ./run.sh %s %s" % (temp_dir, wfr_mpivars, hosts, turbo)
    RegLib.test_log(5, "MPI Cmd: %s" % mpi_cmd)
    # Figure out what the MPI test is going to name the output directory
    # Technically we could figure this out and the day could roll between now
    # and starting the MPI test in which case we'd get a failure, however that
    # chance is very slim and not really even worth dealing with
    today = date.today()
    output_dir="OUTPUT-%s" % today.strftime("%m-%d-%y")
    (ret) = host1.send_ssh(mpi_cmd, buffered=0, use_tty=True)
    if ret:
        RegLib.test_fail("MPI test failed, bailing and leaving logs")

    # Check the summary file for failures
    summary_file = temp_dir + "/" + output_dir + "/no-turbo/summary"
    RegLib.test_log(0, "Checking summary file %s" % summary_file)
    cmd = "cat " + summary_file
    (ret, output) = host1.send_ssh(cmd, buffered = 1)
    if ret:
        RegLib.test_fail("Could not get MPI summary")

    check_output(output)

    return output

def start_ipostl(host1, host2):

    # If its already up don't bother mucking
    ping = "ping -c 3 -W 5 " + host2.get_name() + "-ib"
    (ret) = host1.send_ssh(ping, buffered=0)
    if (ret == 0):
        RegLib.test_log(5, "IPoSTL already up")
        return

    (ret) = host1.send_ssh("modprobe ib_ipoib", buffered=0, run_as_root=True)
    if (ret):
        RegLib.test_fail("Could not load ipoib on host1")

    (ret) = host2.send_ssh("modprobe ib_ipoib", buffered=0, run_as_root=True)
    if (ret):
        RegLib.test_fail("Could not load ipoib on host2")

    RegLib.test_log(0, "Waiting 10 seconds for IPoIB to load")
    time.sleep(10)

    (ret) = host1.send_ssh("ifup ib0", buffered=0, run_as_root=True)
    if (ret):
        RegLib.test_fail("Could not start ib0 on host1")

    (ret) = host2.send_ssh("ifup ib0", buffered=0, run_as_root=True)
    if (ret):
        RegLib.test_fail("Could not start ib0 on host2")

    RegLib.test_log(0, "Waiting 10 seconds for ib0 to come up")
    time.sleep(10)

    (ret) = host1.send_ssh(ping, buffered=0)
    if (ret):
        RegLib.test_fail("Could not bring up ib0")

def test_ipostl(host1, host2, temp_dir, perf_test_dir):
    host1_ib = host1.get_name() + "-ib"
    host2_ib = host2.get_name() + "-ib"

    ipostl_dir = perf_test_dir + "/scripts/regression/ipostl-perf-check"
    today = date.today()
    output_dir="IPoSTL_OUTPUT-%s" % today.strftime("%m-%d-%y")
    log_dir = temp_dir + "/" + output_dir
    test_cmd = "cd %s && ./run-ipostl.sh %s %s %s %s" % (ipostl_dir, host2.get_name(), host2_ib, host1_ib, log_dir)

    RegLib.test_log(0, "Running IPoSTL test with %s" % test_cmd)
    (ret) = host1.send_ssh(test_cmd, buffered=0, use_tty=True)
    if (ret):
        RegLib.test_fail("IPoSTL test failed!")

    # Now go get the summary file so we can check it
    summary_file = log_dir + "/" + "ipostl-summary.txt"
    RegLib.test_log(0, "Checking summary file %s" % summary_file)
    cmd = "cat " + summary_file
    (ret, output) = host1.send_ssh(cmd, buffered = 1)
    if ret:
        RegLib.test_fail("Could not get MPI summary")

    check_output(output)

    return output

def test_verbs(host1, host2, temp_dir, perf_test_dir):
    verbs_test_dir = "/nfs_scratch/driver_perf/Verbs/verbs-perf-check-r1.0"
    host2_ib = host2.get_name() + "-ib"
    today = date.today()
    output_dir="VERBS_OUTPUT-%s" % today.strftime("%m-%d-%y")
    log_dir = temp_dir + "/" + output_dir
    test_cmd = "cd %s && ./run-Verbs-LZ-1QP-Tests.sh %s %s" % (verbs_test_dir, host2_ib, log_dir)

    RegLib.test_log(0, "Running verbs test with  %s" % test_cmd)
    (ret) = host1.send_ssh(test_cmd, buffered = 0, use_tty=True)
    if (ret):
        RegLib.test_fail("Verbs Test failed to run")

    summary_file = log_dir + "/VerbsLZSummary.csv"
    RegLib.test_log(0, "Checking summary file %s" % summary_file)
    cmd = "cat " + summary_file
    (ret, output) = host1.send_ssh(cmd, buffered = 1)
    if ret:
        RegLib.test_fail("Could not get Verbs summary")

    check_output(output)

    return output

def main():
    global test_failed

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()
    print test_info

    count = test_info.get_host_count()
    if count < 2:
        RegLib.test_fail("Not enough hosts")

    print "Found ", count, "hosts"
    host1 = test_info.get_host_record(0)
    print host1

    host2 = test_info.get_host_record(1)
    print host2

    if host1.get_name() == host2.get_name():
        RegLib.test_fail("Perf test not supported for loopback")

    ################
    # body of test #
    ################
    perf_test_dir="/nfs/sc/disks/fabric_work/ddalessa/perf_analysis/perfromance_team-fab_perf"
    unix_time = time.localtime()
    str_time = time.strftime("%Y%m%d%H%M")
    temp_dir = ""
    if test_info.get_perf_dir() == None:
        temp_dir = "/tmp"
    else:
        temp_dir = test_info.get_perf_dir()
        if not os.path.exists(temp_dir):
            RegLib.test_fail("Could not find chosen perf dir!")

    temp_dir = temp_dir + "/" + os.getenv("USER") + "." + str_time

    mpi_dir = perf_test_dir + "/scripts/regression/osu-perf-check "
    cpy_cmd = "cp -r %s %s" % (mpi_dir, temp_dir)

    # Why are we creating this temp directory?
    # Because the MPI tests write to CWD. This doesn't work if everyone is
    # sharing one copy of the tests. So until they change this just create a
    # temporary copy. We'll also put the IPoSTL and verbs results there.
    # If the tests pass we'll remove the directory but if the test fails then
    # leave it for triage.
    (ret) = host1.send_ssh(cpy_cmd, buffered=0)
    if ret:
        RegLib.test_fail("Could not copy tmp dir")

    (ret) = host2.send_ssh(cpy_cmd, buffered=0)
    if ret:
        RegLib.test_fail("Could not copy tmp dir")

    # Run the tests
    mpi_res = test_mpi(host1, host2, temp_dir, perf_test_dir)
    start_ipostl(host1, host2)
    ipostl_res = test_ipostl(host1, host2, temp_dir, perf_test_dir)
    verbs_res = test_verbs(host1, host2, temp_dir, perf_test_dir)

    RegLib.test_log(0, "---------------")
    RegLib.test_log(0, "Results Summary")
    RegLib.test_log(0, "---------------")
    for line in mpi_res:
        RegLib.test_log(0, "%s" % line.rstrip())
    RegLib.test_log(0, "--")
    for line in ipostl_res:
        RegLib.test_log(0, "%s" % line.rstrip())
    RegLib.test_log(0, "--")
    for line in verbs_res:
        RegLib.test_log(0, "%s" % line.rstrip())


    if test_failed > 0:
        RegLib.test_log(0, "WARNING -- WARNING -- WARNING")
        RegLib.test_log(0, "Your change has failed at least one performance test")
        RegLib.test_log(0, "Failed %d tests!" % test_failed)
        RegLib.test_log(0, "WARNING -- WARNING -- WARNING")


    if test_info.get_perf_dir() != None:
        RegLib.test_pass("Test complete, retained data in %s" % temp_dir)

    RegLib.test_log(0, "Tests completed removing temp dir")
    rm_cmd = "rm -rf %s" % (temp_dir)
    (ret) = host1.send_ssh(rm_cmd, buffered=0)
    if ret:
        RegLib.test_fail("Could not rm tmp dir")
    (ret) = host2.send_ssh(rm_cmd, buffered=0)
    if ret:
        RegLib.test_fail("Could not rm tmp dir")

    RegLib.test_pass("Test passed")

if __name__ == "__main__":
    main()

