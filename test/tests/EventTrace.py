#!/usr/bin/python

# Turn on even tracing and watch it.
# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import signal

trace_points = []
host = None

def signal_handler(signal, frame):
    global trace_points
    global host

    print "Caught Ctrl+C disabling trace points"

    for trace_point in trace_points:
        RegLib.test_log(0, "Enabling trace point: " + trace_point)
        host.send_ssh("echo 0 > %s"%trace_point, 0)

    RegLib.test_pass("Success!")
    sys.exit(0)


def main():
    global trace_points
    global host

    test_info = RegLib.TestInfo()
    host = test_info.get_host_record(0)
    host_name = host.get_name()
    RegLib.test_log(0, "Running on host: " + host_name)

    # Enable tracing
    trace_points.append("/sys/kernel/debug/tracing/events/hfi_trace/enable")
    trace_points.append("/sys/kernel/debug/tracing/events/hfi_snoop/enable")
    trace_points.append("/sys/kernel/debug/tracing/events/hfi_ibhdrs/input_ibhdr/enable")
    trace_points.append("/sys/kernel/debug/tracing/events/hfi_ibhdrs/output_ibhdr/enable")
    
    for trace_point in trace_points:
        RegLib.test_log(0, "Enabling trace point: " + trace_point)
        host.send_ssh("echo 1 > %s"%trace_point, 0)
 
    signal.signal(signal.SIGINT, signal_handler)

    RegLib.test_log(0, "Watching trace_pipe, use ctrl+c to stop....")
    host.send_ssh("cat /sys/kernel/debug/tracing/trace_pipe", 0)

if __name__ == "__main__":
    main()

