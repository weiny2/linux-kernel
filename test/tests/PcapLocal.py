#!/usr/bin/python

# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time
import fcntl
import array
import select
import signal
import sys
from ctypes import *

def print_packet(bytes):
    # Packet Capture Metadata is as follows:
    #struct capture_md {
    #	u8 port;
    #	u8 dir;
    #	u8 reserved[6];
    #	union {
    #		u64 pbc;
    #		u64 rhf;
    #	} u;
    #};
    port = bytes[0]
    direction = "INVALID"
    if bytes[1] == 0:
        direction = " EGRESS "

    if bytes[1] == 1:
        direction = "INGRESS "

    pbcrhf = c_ulonglong.from_buffer(bytes[8:])

    md_len = 16
    vl_offset = md_len
    dlid_upper_offset = 2 + md_len
    dlid_lower_offset = 3 + md_len
    pkt_len_offset = 5 + md_len
    slid_upper_offset = 6 + md_len
    slid_lower_offset = 7 + md_len

    bytes_read = len(bytes)
    packet_bytes = bytes_read - md_len

    dlid_upper = bytes[dlid_upper_offset] << 8
    dlid_lower = bytes[dlid_lower_offset]
    dlid = dlid_upper | dlid_lower

    slid_upper = bytes[slid_upper_offset] << 8
    slid_lower = bytes[slid_lower_offset]
    slid = slid_upper | slid_lower

    vl = bytes[vl_offset] >> 4

    pkt_len = bytes[pkt_len_offset]

    print "BytesRead:", bytes_read, "PacketBytes:", packet_bytes,
    print "Port:", port, "Dir:", direction, "PBC|RHF:", pbcrhf,
    print "SLID:", slid, "DLID:", dlid, "VL:", vl, "PktWords:", pkt_len

def signal_handler(signal, frame):

    print "Caught Ctrl+C"

    # Don't worry about closing the FD or file_obj pyhon gets mad if we
    # close one then the other. Just let the interrpruter do all the
    # work.
    sys.exit(0)


def main():

    IB_PACKET_SIZE = 5000 #iba_pcap.h = 4208 but not big enough for 4K MTU
    FILTER_BY_SLID = 0x0
    FILTER_BY_DLID = 0x1
    SET_FILTER_IOCTL = 7044
    CLEAR_FILTER_IOCTL = 7043

    SNOOP_MODE = os.O_RDWR
    CAPTURE_MODE = os.O_RDONLY

    dev = "/dev/hfi_diagpkt0"
    set_ioctl = 0
    test_fail = 0

    # Open the device and create a file object.
    # Need a file object for sending ioctl
    RegLib.test_log(0, "Opening device")
    fd = os.open(dev, CAPTURE_MODE)
    if fd <= 0:
        RegLib.test_fail("Could not open device")
    else:
        RegLib.test_log(0, "FD is %d" % fd)
    file_obj = os.fdopen(fd)
    if not file_obj:
        RegLib.test_fail("Could not open capture device %s" % c_dev0)

    # Filter value we need to pass to kernel. We need a pointer to it.
    filter_buf = array.array('i', [0x2])
    (filter_addr, filter_len) = filter_buf.buffer_info()

    # Need a buffer to write to kernel. Includes the filter opcode
    # The length of the filter value, and a pointer to that filter
    # value.
    filter_opcode = FILTER_BY_DLID
    filter_len = 0x4
    buf = array.array('i', [filter_opcode, filter_len, filter_addr])

    # clear any filters
    RegLib.test_log(0, "Clearing existing filters")
    ret = fcntl.ioctl(file_obj, CLEAR_FILTER_IOCTL, buf)
    print "CLEAR IOCTL return", ret

    # Now we can send the IOCTL, 7044 means set filter
    if set_ioctl:
        RegLib.test_log(0, "Sending IOCTL")
        ret = fcntl.ioctl(file_obj, SET_FILTER_IOCTL, buf)
        print "IOCTL return", ret

    # Before going into our infinite loop set up a trap for ctrl+c
    signal.signal(signal.SIGINT, signal_handler)

    # Go collect some packets
    RegLib.test_log(0, "Waiting for a packet")
    while True:
        packet = os.read(fd, IB_PACKET_SIZE)
        packet_len = str(len(packet))
        bytes = bytearray(packet)
        print_packet(bytes)

if __name__ == "__main__":
    main()

