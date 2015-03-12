#!/usr/bin/python

# Test OSU Mpi Benchmarks
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time

def do_ssh(host, cmd):
    RegLib.test_log(5, "Running " + cmd)
    return (host.send_ssh(cmd, 0))

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()

    RegLib.test_log(0, "Test: OsuMpi.py started")
    RegLib.test_log(0, "Dumping test parameters")

    print test_info

    host_count = test_info.get_host_count()
    host1 = test_info.get_host_record(0)
    
    print "Found ", host_count, "hosts setting -np for mpirun accordingly"

    ################
    # body of test #
    ################

    psm_libs = test_info.get_psm_lib()
    if psm_libs:
        if psm_libs == "DEFAULT":
            psm_libs = None
            print "Using default PSM lib"
        else:
            if test_info.is_simics():
                psm_libs = "/host" + psm_libs
            print "We have PSM path set to", psm_libs
    else:
        RegLib.test_fail("MPI with verbs not yet supported")

    benchmarks = ["osu_latency", "osu_bw"]

    host_list = test_info.get_host_list()
    hosts = ",".join(host_list)

    if len(host_list) == 1:
        # Only provided one host: run two ranks on that host.
        host_count = 2

    for benchmark in benchmarks:
        RegLib.test_log(0, "Starting OSU MPI benchmark " + benchmark)
        cmd = "export LD_LIBRARY_PATH=" + test_info.get_mpi_lib_path()
        if psm_libs:
            cmd = cmd + ":" + psm_libs
        cmd = cmd + ";"
        cmd = cmd + " " +  test_info.get_mpi_bin_path() + "/mpirun"
        cmd = cmd + test_info.get_mpi_opts()
        cmd = cmd + " LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
        cmd = cmd + " -np " + str(host_count)
        cmd = cmd + " -H " + hosts
        cmd = cmd + " " + test_info.get_osu_benchmark_dir() + benchmark
        RegLib.test_log(5,  "MPI CMD: " + cmd)

        err = do_ssh(host1, cmd)
        if err:
            RegLib.test_fail("Failed!")

    RegLib.test_pass("All MPI tests passed")

if __name__ == "__main__":
    main()

