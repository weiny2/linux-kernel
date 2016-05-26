#!/usr/bin/python

# Test harness for WFR Driver. The job of this script is basically just to run a
# set of tests.

# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import os
import sys
import re
import subprocess
import shlex

# Need to go find the RegLib
file_path = os.path.dirname (os.path.abspath (__file__))
sys.path.append (file_path + "/tests")
import RegLib

warnings = []
tests_passed = []
tests_failed = []

def WARN(msg):
    global warnings
    print "\n----------\nWARNING\n----------"
    print msg, "\n"
    warnings.append(msg)

def INFO(msg):
    print "\n----\nINFO\n----"
    print msg, "\n"


def PASS(msg):
    global tests_passed
    print "\n------------\nTEST SUCCESS\n------------"
    print msg, "\n"
    tests_passed.append(msg)

def FAIL(msg):
    global tests_failed
    print "\n!!!!!!!!!!!\nTEST FAILED\n!!!!!!!!!!!"
    print msg, "\n"
    tests_failed.append(msg)

# List of tests that the harness knows about. 
test_list = [

    # Do a build of the driver
    { "test_name" : "ModuleBuild",
      "test_exe"  : "build.sh",
      "args" : "%KBUILD_DIR% %HFI_SRC%",
      "type" : "misc",
      "desc" : "Do a build of the driver."
    },

    # Load an aleady built driver on 2 nodes and bring the links up
    { "test_name" : "ModuleLoad",
      "test_exe" : "LoadModule.py",
      "args" : "--nodelist %HOST[2]% --hfisrc %HFI_SRC% --linuxsrc %LINUX_SRC% --sm %SM%",
      "type" : "default,upstream,perf,qib,verbs",
      "desc" : "Load the hfi.ko on 2 nodes, restart opensm and make sure active state is reached"
    },

    # IB Send Lat test
    { "test_name" : "IbSendLat-Verbs",
      "test_exe" : "IbSendLat.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_send_lat for 5 iterations.",
    },

    # IB Send BW tests
    { "test_name" : "IbSendBwUD-Verbs",
      "test_exe" : "IbSendBwUD.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_send_bw for 5 iterations with various sizes using UD.",
    },

    { "test_name" : "IbSendBwRC-Verbs",
      "test_exe" : "IbSendBwRC.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_send_bw for 5 iterations with various sizes using RC.",
    },

    { "test_name" : "IbWriteBwRC-Verbs",
      "test_exe" : "IbWriteBwRC.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_write_bw for 5 iterations with various sizes using RC.",
    },

    { "test_name" : "IbWriteBwUC-Verbs",
      "test_exe" : "IbWriteBwUC.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_write_bw for 5 iterations with various sizes using UC.",
    },

    { "test_name" : "IbReadBwRC-Verbs",
      "test_exe" : "IbReadBwRC.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_read_bw for 5 iterations with various sizes using RC.",
    },

    { "test_name" : "IbSendBwUC-Verbs",
      "test_exe" : "IbSendBwUC.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_send_bw for 5 iterations with various sizes using UC.",
    },

    { "test_name" : "IbAtomicBw-Verbs",
      "test_exe" : "IbAtomicBw.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ib_atomic_bw for 5 iterations with the two RC atomic operations.",
    },

    { "test_name" : "IPoIB-Verbs",
      "test_exe" : "IpoibPing.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run ping for 5 packets using ipoib.",
    },

    { "test_name" : "IbSendBwRC-8MB",
      "test_exe" : "IbSendBwRC-a.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,verbs,qib",
      "desc" : "Run ib_send_bw for 16 iterations using sizes up to 2^23 using RC.",
    },

    { "test_name" : "IbSendBwRC-badSL",
      "test_exe" : "IbSendBwRC-badSL.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "bad",
      "desc" : "Run ib_send_bw with bad SL values to ensure they fail to modify_qp",
    },

    { "test_name" : "IPoIB-Qperf",
      "test_exe" : "IpoibQperf.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,verbs,installed,qib",
      "desc" : "Run qperf/tcp_bw for 8 to 64 bytes.",
    },

   # OSU MPI tests
    { "test_name" : "OSU-MPI-Psm",
      "test_exe" : "OsuMpi.py",
      "args" : "--nodelist %HOST[2]% --psm %PSM_LIB% --psmopts %PSM_OPTS%",
      "type" : "mpi,mpipsm,default,upstream,qib",
      "desc" : "Run OSU MPI benchmarks with PSM",
    },

    { "test_name" : "OSU-MPI-Psm-One-node",
      "test_exe" : "OsuMpi.py",
      "args" : "--nodelist %HOST[1]% --psm %PSM_LIB% --psmopts %PSM_OPTS%",
      "type" : "mpi,mpipsm,default,upstream,qib",
      "desc" : "Run OSU MPI benchmarks on one node with PSM",
    },

    { "test_name" : "OSU-MPI-Verbs",
      "test_exe" : "OsuMpi.py",
      "args" : "--nodelist %HOST[2]% --mpiverbs",
      "type" : "default,upstream,mpi,mpiverbs,verbs,qib",
      "desc" : "Run OSU MPI benchmarks with verbs",
    },

    { "test_name" : "OpcodeCounters",
      "test_exe" : "OpcodeCounters.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,quick,quick_upstream,verbs,installed,qib",
      "desc" : "Run test opcode counters after quick tests have been run.",
    },

    # IMB (Intel MPI Benchmark) tests
    { "test_name" : "IMB-Psm",
      "test_exe" : "IMB.py",
      "args" : "--nodelist %HOST[2]% --psm %PSM_LIB% --psmopts %PSM_OPTS% --args \"-time 1 -iter 10\"",
      "type" : "default,upstream,mpi,mpipsm,qib",
      "desc" : "Run full IMB suite with PSM",
    },

    { "test_name" : "IMB-Verbs",
      "test_exe" : "IMB.py",
      "args" : "--nodelist %HOST[2]% --mpiverbs --args \"-time 1 -iter 10\"",
      "type" : "default,upstream,mpi,mpiverbs,verbs,qib",
      "desc" : "Run full IMB suite with verbs",
    },

    # wfr-diagtools-sw tests
    { "test_name" : "HfiPktTest-PIO-Buffer",
      "test_exe" : "HfiPktTest.py",
      "args" : "--nodelist %HOST[1]% --psm %PSM_LIB% --sw-diags %DIAG_LIB%",
      "type" : "diagtools,default,upstream,installed",
      "desc" : "Run hfi_pkt_test PIO buffer benchmark.",
    },

    { "test_name" : "HfiPktTest-Ping-Pong",
      "test_exe" : "HfiPktTest.py",
      "args" : "--nodelist %HOST[2]% --psm %PSM_LIB% --sw-diags %DIAG_LIB%",
      "type" : "diagtools,default,upstream,installed",
      "desc" : "Run hfi_pkt_test ping-pong benchmark.",
    },

    { "test_name" : "HfiPktSend",
      "test_exe" : "HfiPktSend.py",
      "args" : "--nodelist %HOST[1]% --psm %PSM_LIB% --test-pkt-dir %TEST_PKT_DIR% --sw-diags %DIAG_LIB%",
      "type" : "diagtools,default,installed",
      "desc" : "Run hfi_pkt_send tests.",
    },

    # Fast MPI tests
    { "test_name" : "MPI-Stress-Psm",
      "test_exe" : "MpiStress.py",
      "args" : "--nodelist %HOST[2]% --psm %PSM_LIB% --psmopts %PSM_OPTS% --args \"-L 2 -M 2 -w 3 -m 1048576 -z\"",
      "type" : "mpi,mpipsm,quick,quick_upstream,installed",
      "desc" : "Run quick MPI stress with PSM",
    },

    { "test_name" : "MPI-Stress-Verbs",
      "test_exe" : "MpiStress.py",
      "args" : "--nodelist %HOST[2]% --mpiverbs --args \"-L 2 -M 2 -w 3 -m 1048576 -z\"",
      "type" : "mpi,mpiverbs,verbs,quick,quick_upstream,installed",
      "desc" : "Run quick MPI stress with verbs",
    },

    { "test_name" : "MPI-Stress-PSM-Long",
      "test_exe" : "MpiStress.py",
      "args" : "--nodelist %HOST[2]% --psm %PSM_LIB% --psmopts %PSM_OPTS% --args \"-L 10 -M 10 -w 20 -z\"",
      "type" : "mpi,mpipsm,default,upstream,integrity,qib",
      "desc" : "Run MPI stress with PSM",
    },

    { "test_name" : "MPI-Stress-Verbs-Long",
      "test_exe" : "MpiStress.py",
      "args" : "--nodelist %HOST[2]% --mpiverbs --args \"-L 10 -M 10 -w 20 -z\"",
      "type" : "default,upstream,mpi,mpiverbs,verbs,integrity,qib",
      "desc" : "Run MPI stress with verbs",
    },

    { "test_name" : "Hpcc-Verbs",
      "test_exe" : "Hpcc.py",
      "args" : "--nodelist %HOST[2]% --mpiverbs",
      "type" : "default,upstream,mpi,mpiverbs,verbs,qib",
      "desc" : "Run Hpcc with verbs",
    },

    { "test_name" : "Hpcc-Psm",
      "test_exe" : "Hpcc.py",
      "args" : "--nodelist %HOST[2]% --psm %PSM_LIB%",
      "type" : "default,upstream,mpi,mpipsm,qib",
      "desc" : "Run Hpcc with psm",
    },

    # Snoop/Capture tests
    { "test_name" : "SnoopHijack",
      "test_exe" : "Snoop.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "snoop,default,quick,installed",
      "desc" : "Run snoop hijack tests.",
    },

    { "test_name" : "PacketCapture",
      "test_exe" : "Pcap.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "snoop,default,quick,installed",
      "desc" : "Run simple packet capture tests.",
    },

    { "test_name" : "SnoopIntegrity",
      "test_exe" : "SnoopInteg.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "snoop,default,quick,installed",
      "desc" : "Sweep fabric and check MD5 sums of packets *requires ifs_fm*",
    },

    { "test_name" : "SnoopFilter",
      "test_exe" : "SnoopFilter.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "snoop_unit,nosm",
      "desc" : "Run snoop filter tests",
    },

    { "test_name" : "SnoopIoctl",
      "test_exe" : "SnoopIoctl.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "snoop,default",
      "desc" : "Run snoop IOCTL tests (modifies HFI state and kills SM.",
    },

    { "test_name" : "8K-MTU-Verbs",
      "test_exe" : "MTUTest.py",
      "args" : "--nodelist %HOST[2]% --hfisrc %HFI_SRC% --linuxsrc %LINUX_SRC%",
      "type" : "mgmt,verbs",
      "desc" : "Test 4K and 8K MTU with verbs traffic"
    },

    { "test_name" : "Loopback-Test",
      "test_exe" : "Loopback.py",
      "args" : "--nodelist %HOST[1]% --hfisrc %HFI_SRC% --linuxsrc %LINUX_SRC% --psm %PSM_LIB% --psmopts %PSM_OPTS% --args \"-L 2 -M 2 -w 3 -m 1048576 -z\"",
      "type" : "default,upstream,quick,quick_upstream,nosm",
      "desc" : "Test loopback. LCB on Simics, Serdes on FPGA, both on ASIC."
    },

    { "test_name" : "RestoreSanity",
      "test_exe" : "LoadModule.py",
      "args" : "--nodelist %HOST[2]% --hfisrc %HFI_SRC% --linuxsrc %LINUX_SRC% --sm %SM%",
      "type" : "default,upstream,quick,quick_upstream,qib,snoop",
      "desc" : "Load the hfi.ko on 2 nodes, restart opensm and make sure active state is reached"
    },

    { "test_name" : "MPI-Test-PSM",
      "test_exe" : "MpiTest.py",
      "args" : "--nodelist %HOST[2]% --psm %PSM_LIB% --psmopts %PSM_OPTS% --np %NP%",
      "type" : "mpipsm",
      "desc" : "Run the Intel MPI Test Suite"
    },

    { "test_name" : "MPI-Test-Verbs",
      "test_exe" : "MpiTest.py",
      "args" : "--nodelist %HOST[2]% --mpiverbs --np %NP%",
      "type" : "mpiverbs",
      "desc" : "Run the Intel MPI Test Suite"
    },

    { "test_name" : "Perf-Regression",
      "test_exe" : "PerfReg.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "perf,installed",
      "desc" : "Run the required performance regression tests"
    },
    { "test_name" : "Adaptive-PIO",
      "test_exe" : "AdaptivePIO.py",
      "args" : "--nodelist %HOST[2]%",
      "type" : "default,upstream,verbs",
      "desc" : "Run the Adaptive PIO regression tests"
    },

]


# Collect CLI args
test_info = RegLib.TestInfo()

# Map arg variables to their TestInfo counterpart
variable_map = {
    "KBUILD_DIR" : test_info.get_kbuild_dir,
    "HFI_SRC" : test_info.get_hfi_src,
    "LINUX_SRC" : test_info.get_linux_src,
    "HOST" : test_info.get_host_name_by_index,
    "PSM_LIB" : test_info.get_psm_lib,
    "TEST_PKT_DIR" : test_info.get_test_pkt_dir,
    "DIAG_LIB" : test_info.get_diag_lib,
    "PSM_OPTS" : test_info.get_psm_opts,
    "NP" : test_info.get_np,
    "SM" : test_info.which_sm,
}

# Build a list of all test types. This will be used when the user
# specifies tests by name rather than type.
all_test_types = ",".join([x["type"] for x in test_list])
all_test_types = set([x for x in all_test_types.split(",") if x])

if test_info.list_only:
    print "Listing Tests:"
    for test in test_list:
        name = test["test_name"]
        args = test["args"]
        ttype = test["type"]
        desc = test["desc"]
        print "[", ttype, "]",  name, "::", args
        print desc
        print ""
    sys.exit(0)

# For each test in the list, extract variables from the args and replace with
# what we have in the test_info settings if we can't figure out an argument we
# bail and fail. Once we have swizzled the command line run the test and look at
# the retrun value.
simics = test_info.is_simics()
sm = test_info.which_sm()
fullset = True
if test_info.get_test_list():
    tlist = test_info.get_test_list()
    tests_to_run = filter(lambda x: x["test_name"] in tlist, test_list)
    fullset = False
else:
    tests_to_run = test_list
for test in tests_to_run:

    curr_name = test["test_name"]
    curr_exe = test["test_exe"]
    curr_args = test["args"]
    curr_type = test["type"]

    if fullset: test_types = re.split(r',', test_info.get_test_types())
    else: test_types = all_test_types
    for test_type in test_types:
        run_it = 0
        
        if test_type == curr_type:
            run_it = 1

        if test_type == "all":
            run_it = 1

        # tests can be classified as multiple types so check them all
        # we could also need to exclude some test types
        curr_types = re.split(r',',curr_type)
        for temp_type in curr_types:

            if temp_type == test_type:
                run_it = 1

            if run_it:
                if sm == "remote":
                    if temp_type == "nosm":
                        run_it = 0
                        WARN(curr_name + " : " + "SKIPPED!")
                        break

            if run_it:
                if curr_name == "Perf-Regression":
                    if simics == True:
                        run_it = 0
                        WARN(curr_name + " : " + "SKIPPED!")
                        break

        if run_it:
            # Now we need to replace the variables in the curr_args with the
            # right values
            tokens = re.split(' ', curr_args)
            processed_args = ""
            for arg in tokens:
                matchObj = re.match(r"^%(.*)%$", arg)
                if matchObj:
                    arg = matchObj.group(1)
                    valid = 0

                    # Determine if this is a range variable
                    count = -1
                    rangeObj = re.match(r"(.+)\[(\d+)\]", arg)
                    if rangeObj:
                        arg = rangeObj.group(1)
                        count = rangeObj.group(2)
                    for var in variable_map.keys():
                        mapped = var
                        comma_sep_list = ""
                        if mapped == arg:
                            if count == -1:
                                arg = variable_map[var]()
                            elif count == 0:
                                WARN("Test FAILIURE unable to parse " + arg)
                            else:
                                for i in range(int(count)):
                                    val = variable_map[arg](i)
                                    comma_sep_list = comma_sep_list + val + ","
                                arg = RegLib.chomp_comma(comma_sep_list)
                            valid = 1
                    if valid == 0:
                        WARN("Did not find a mapping for" + arg)
                processed_args += arg + " "
            if simics == True:
                processed_args += "--simics"
            if test_type == "qib":
                processed_args += "--qib"

            # We may need to pass on some module parameters too
            modparms = test_info.get_mod_parms()
            if modparms != "":
                processed_args += " --modparm \"" + modparms + "\""

            RegLib.test_log(5, "Running test: " + curr_name)
            RegLib.test_log(5, "Raw Test args: " + curr_args)
            RegLib.test_log(5, "Processed Test args: " + processed_args)
            RegLib.test_log(5, "Test Type: " + curr_type)

            INFO("Staring Test " + curr_exe + " [" + processed_args + "]")
            args = shlex.split(os.path.dirname(os.path.realpath(__file__)) +
                    "/tests/" + curr_exe + " " + processed_args)

            test_runner = subprocess.Popen(args)
            result = test_runner.wait()

            if result: # test failed
                FAIL(curr_name + " : " + processed_args)
            else:
                PASS(curr_name + " : " + processed_args)

            break

print "-------------"
print "Test Summary:"
print "-------------"

final_status = 0
if len(warnings):
    for warning in warnings:
        print "[WARN]", warning
if len(tests_failed):
    for failure in tests_failed:
        print "[FAIL]", failure
        final_status = 255
if len(tests_passed):
    for passed in tests_passed:
        print "[PASS]", passed
print ""

sys.exit(final_status)

