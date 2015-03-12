#!/usr/bin/python

# Test IMB (Intel MPI Benchmarks)
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
# Author: Andrew Friedley (andrew.friedley@intel.com)
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

    RegLib.test_log(0, "Test: IMB.py started")
    RegLib.test_log(0, "Dumping test parameters")

    print test_info

    host_count = test_info.get_host_count()
    host1 = test_info.get_host_record(0)

    print "Found ", host_count, "hosts setting -np for mpirun accordingly"


    extra_args = test_info.parse_extra_args()
    arg_str = str(extra_args).strip('[]').strip('\'')
    print "IMB arg string is ", arg_str

    ################
    # body of test #
    ################

    psm_libs = test_info.get_psm_lib()
    if psm_libs == "DEFAULT":
        psm_libs = None
        print "Using default PSM lib"
    else:
        if test_info.is_simics():
            psm_libs = "/host" + psm_libs
        print "We have PSM path set to", psm_libs

    base_dir = test_info.get_base_dir()
    if base_dir == "":
        if test_info.is_mpiverbs():
            base_dir = "/usr/mpi/gcc/openmpi-1.8.2a1/tests/IMB-3.2"
        else:
            base_dir = "/usr/mpi/gcc/openmpi-1.8.2a1-hfi/tests/IMB-3.2"
        print "Using default base_dir of", base_dir
    else:
        print "Using user defined base_dir of", base_dir

    benchmark = base_dir + "/IMB-MPI1"

    host_list = test_info.get_host_list()
    hosts = ",".join(host_list)

    if len(host_list) == 1:
        # Only provided one host: run two ranks on that host.
        host_count = 2

    RegLib.test_log(0, "Starting IMB " + benchmark)
    cmd = "export LD_LIBRARY_PATH=" + test_info.get_mpi_lib_path()
    if psm_libs:
        cmd = cmd + ":" + psm_libs
    cmd = cmd + ";"
    cmd = cmd + " " +  test_info.get_mpi_bin_path() + "/mpirun"
    cmd = cmd + test_info.get_mpi_opts()
    cmd = cmd + " LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    cmd = cmd + " -np " + str(host_count)
    cmd = cmd + " -H " + hosts
    cmd = cmd + " %s %s" % (benchmark, arg_str)

    RegLib.test_log(5,  "MPI CMD: " + cmd)

    err = do_ssh(host1, cmd)
    if err:
        RegLib.test_fail("Failed!")

    RegLib.test_pass("All MPI tests passed")

if __name__ == "__main__":
    main()

