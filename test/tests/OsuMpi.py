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

    RegLib.test_log(0, "Test: LoadModule.py started")
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

    benchmarks = ["pt2pt/osu_latency", "pt2pt/osu_bw"]

    for benchmark in benchmarks:
        RegLib.test_log(0, "Starting OSU MPI benchmark " + benchmark)
        cmd = "export LD_LIBRARY_PATH=" + test_info.get_mpi_lib_path()
        if psm_libs:
            cmd = cmd + ":" + psm_libs
        cmd = cmd + ";"
        cmd = cmd + " " +  test_info.get_mpi_bin_path() + "/mpirun"
        cmd = cmd + test_info.get_mpi_opts()
        cmd = cmd + " LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
        cmd = cmd + " -np " + str(host_count) + " -H "
        host_list = test_info.get_host_list()
        hosts = ""
        for host in host_list:
            hosts = hosts + host + ","
        hosts = RegLib.chomp_comma(hosts)
        cmd = cmd + hosts + " " + test_info.get_osu_benchmark_dir()
        cmd = cmd + "/mpi/" + benchmark
        RegLib.test_log(5,  "MPI CMD: " + cmd)

        err = do_ssh(host1, cmd)
        if err:
            RegLib.test_fail("Failed!")

    RegLib.test_pass("All MPI tests passed")

if __name__ == "__main__":
    main()

