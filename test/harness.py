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
    { "test_name" : "build.sh",
      "args" : "%KBUILD_DIR% %HFI_SRC%",
      "type" : "misc",
      "desc" : "Do a build of the driver."
    },

    # Load an aleady built driver on 2 nodes and bring the links up
    { "test_name" : "LoadModule.py",
      "args" : "--nodelist %HOST[2]% --hfisrc %HFI_SRC% --simics",
      "type" : "default,",
      "desc" : "Load the hfi.ko on 2 nodes, restart opensm and make sure active state is reached"
    },

    # IB Send Lat test
    { "test_name" : "IbSendLat.py",
      "args" : "--nodelist %HOST[2]% --simics",
      "type" : "default,quick,verbs",
      "desc" : "Run ib_send_lat for 5 iterations.",
    },

    # IB Send BW tests
    { "test_name" : "IbSendBwUD.py",
      "args" : "--nodelist %HOST[2]% --simics",
      "type" : "default,quick,verbs",
      "desc" : "Run ib_send_bw for 5 iterations with various sizes using UD.",
    },

    { "test_name" : "IbSendBwRC.py",
      "args" : "--nodelist %HOST[2]% --simics",
      "type" : "default,quick,verbs",
      "desc" : "Run ib_send_bw for 5 iterations with various sizes using RC.",
    },

    { "test_name" : "IpoibPing.py",
      "args" : "--nodelist %HOST[2]% --simics",
      "type" : "default,quick,verbs",
      "desc" : "Run ping for 5 packets using ipoib.",
    },

    { "test_name" : "IbSendBwRC-a.py",
      "args" : "--nodelist %HOST[2]% --simics",
      "type" : "default,verbs",
      "desc" : "Run ib_send_bw for 16 iterations using sizes up to 2^23 using RC.",
    },

    { "test_name" : "IpoibQperf.py",
      "args" : "--nodelist %HOST[2]% --simics",
      "type" : "default,verbs",
      "desc" : "Run qperf/tcp_bw for 8 to 64 bytes.",
    },

   # OSU MPI tests
    { "test_name" : "OsuMpi.py",
      "args" : "--nodelist %HOST[2]% --simics",
      "type" : "mpi,default",
      "desc" : "Run OSU MPI benchmarks",
    },
]


# Collect CLI args
test_info = RegLib.TestInfo()

# Map arg variables to their TestInfo counterpart
variable_map = {
    "KBUILD_DIR" : test_info.get_kbuild_dir,
    "HFI_SRC" : test_info.get_hfi_src,
    "HOST" : test_info.get_host_name_by_index,
}

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
for test in test_list:

    curr_name = test["test_name"]
    curr_args = test["args"]
    curr_type = test["type"]

    test_types = re.split(r',', test_info.get_test_types())
    for test_type in test_types:
        run_it = 0
        if test_type == curr_type:
            run_it = 1

        if test_type == "all":
            run_it = 1

        # tests can be classified as multiple types so check them all
        curr_types = re.split(r',',curr_type)
        for temp_type in curr_types:
            if temp_type == test_type:
                run_it = 1

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

            RegLib.test_log(5, "Running test: " + curr_name)
            RegLib.test_log(5, "Raw Test args: " + curr_args)
            RegLib.test_log(5, "Processed Test args: " + processed_args)
            RegLib.test_log(5, "Test Type: " + curr_type)

            INFO("Staring Test " + curr_name + " [" + processed_args + "]")
            args = shlex.split("./tests/" + curr_name + " " + processed_args)

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

if len(warnings):
    for warning in warnings:
        print "[WARN]", warning
if len(tests_failed):
    for failure in tests_failed:
        print "[FAIL]", failure
if len(tests_passed):
    for passed in tests_passed:
        print "[PASS]", passed
print ""


