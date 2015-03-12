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

server = 0
client = 1
file_obj = None
fd = 0

def set_filter(filter, value, special_filter_val):
    global file_obj

    # Sometimes the IOCTL value doesn't match the actual byte value in the
    # packet, rather it is something strange. For instance UD packet type is
    # 0x64 but the filter logic looks for 0x3!

    SET_FILTER_IOCTL = 7044
    CLEAR_FILTER_IOCTL = 7043
    CLEAR_QUEUE_IOCTL = 7042

    if special_filter_val != -1:
        RegLib.test_log(0,"Setting special filter value %d" % special_filter_val)
        value = special_filter_val

    filter_buf = array.array('i', [value])
    (filter_addr, filter_len) = filter_buf.buffer_info()

    # Need a buffer to write to kernel. Includes the filter opcode
    # The length of the filter value, and a pointer to that filter
    # value.
    filter_len = 0x4
    buf = array.array('i', [filter, filter_len, filter_addr])

    # clear any filters
    RegLib.test_log(0, "Cleraing existing filters")
    ret = fcntl.ioctl(file_obj, CLEAR_FILTER_IOCTL, buf)
    print "CLEAR IOCTL return", ret
    if ret:
        RegLib.test_fail("Could not clear filters")

    # Empty the snoop ring
    RegLib.test_log(0, "Cleraing ring buffer")
    ret = fcntl.ioctl(file_obj, CLEAR_QUEUE_IOCTL, buf)
    print "CLEAR IOCTL return", ret
    if ret:
        RegLib.test_fail("Could not clear filters")


    # Now we can send the IOCTL, 7044 means set filter
    RegLib.test_log(0, "Sending IOCTL")
    ret = fcntl.ioctl(file_obj, SET_FILTER_IOCTL, buf)
    print "IOCTL return", ret

    if ret:
        RegLib.test_fail("Could not set filter")

def init_packet():

    packet = bytearray(284)  # 4 bytes less, NO CRC

    for i in range(284):
        packet[i] = 0x0

    packet[0] = 0xF0    # LRH.link vers
    packet[1] = 0x02    # LRH.lnh
    packet[2] = 0x00    # LRH.dlid
    packet[3] = 0x02    #
    packet[4] = 0x0     # LRH.len
    packet[5] = 0x48    #
    packet[6] = 0x0     # LRH.slid
    packet[7] = 0x1

    packet[8] = 0x64    # BTH.opcode
    packet[9] = 0x0     # BTH.header version
    packet[10] = 0xff   # BTH.Pkey
    packet[11] = 0xff   #
    packet[12] = 0x0    # BTH.Reserved
    packet[13] = 0x0    # BTH.Dest queue pair
    packet[14] = 0x0    #
    packet[15] = 0x0    #

    packet[16] = 0x0    # BTH.reserved
    packet[17] = 0x0    # BTH.seq number
    packet[18] = 0x0    #
    packet[19] = 0x0    #

    packet[24] = 0x0    # DETH.reserved
    packet[25] = 0x0    # DETH.source queue pair
    packet[26] = 0x0    #
    packet[27] = 0xaa    #

    # make up some data so we are pretty sure we can find the packet we are
    # looking for.
    packet[100] = 0x1
    packet[105] = 0x2
    packet[106] = 0x3

    return packet

def valid_packet(incoming_bytes):

    if (incoming_bytes[100] == 0x1) and (incoming_bytes[105] == 0x2) and (incoming_bytes[106] == 0x3):
        return True
    else:
        return False

def do_packet_test(filter, offset, good_val, bad_val, special_filter_val = -1):
    global server
    global client
    global fd

    packet = init_packet()

    # packet[27 - 283] is the data payload
    if mode == server:
        RegLib.test_log(0,"Running in server mode")

        # Write a bad packet
        packet[offset] = bad_val
        written = os.write(fd, packet)
        if written != 284:
            RegLib.test_fail("Failed to write packet")
        RegLib.test_log(0, "Wrote packet of %d bytes" % written)

        # Write a good packet
        packet[offset] = good_val
        written = os.write(fd, packet)
        if written != 284:
            RegLib.test_fail("Failed to write packet")
        RegLib.test_log(0, "Wrote packet of %d bytes" % written)

        # Write a bad packet
        packet[offset] = bad_val
        written = os.write(fd, packet)
        if written != 284:
            RegLib.test_fail("Failed to write packet")
        RegLib.test_log(0, "Wrote packet of %d bytes" % written)

        # Write a good packet
        packet[offset] = good_val
        written = os.write(fd, packet)
        if written != 284:
            RegLib.test_fail("Failed to write packet")
        RegLib.test_log(0, "Wrote packet of %d bytes" % written)

    else:
        RegLib.test_log(1,"Running in client mode")
        set_filter(filter, good_val, special_filter_val)
        count = 0
        while count != 2:
            packet = get_packet()
            incoming_bytes = bytearray(packet)
            if len(packet) == 288:
                if valid_packet(incoming_bytes):
                    if incoming_bytes[offset] != good_val:
                        RegLib.test_fail("Got Wrong packet!")
                    count+=1
            RegLib.test_log(0, "Not the packet I want trying again")


def get_packet():
    global fd
    IB_PACKET_SIZE = 5000
    RegLib.test_log(0, "Waiting for a packet")
    packet = os.read(fd, IB_PACKET_SIZE)
    packet_len = str(len(packet))
    bytes = bytearray(packet)
    vl = bytes[0] >> 4
    dlid_upper = bytes[2] << 8
    dlid = dlid_upper | bytes[3]
    pkt_len = bytes[5]
    slid_upper = bytes[6] << 8
    slid = slid_upper | bytes[7]
    print "Found a", len(packet), "byte pkt",
    print "vl", vl, "pktlen", pkt_len, "Dwords",
    print "dlid", dlid, "slid", slid
    return packet


def main():
    global mode
    global server
    global client
    global file_obj
    global fd

    IB_PACKET_SIZE = 5000 #iba_pcap.h = 4208 but not big enough for 4K MTU
    FILTER_BY_SLID = 0x0
    FILTER_BY_DLID = 0x1
    FILTER_BY_MAD = 0x2
    FILTER_BY_QP = 0x3
    FILTER_BY_PACKET_TYPE = 0x4
    FILTER_BY_SL = 0x5
    FILTER_BY_PKEY = 0x6

    filter_by = FILTER_BY_DLID
    SNOOP_MODE = os.O_RDWR

    test_info = RegLib.TestInfo()
    args = test_info.parse_extra_args()

    filter_by = args[0]
    if args[1] == "server":
        mode = server
    elif args[1] == "client":
        mode = client
    else:
        RegLib.test_fail("Did not specify server or client")

    dev = "/dev/hfi_diagpkt0"

    # Open the device and create a file object.
    # Need a file object for sending ioctl
    RegLib.test_log(0, "Opening device")
    fd = os.open(dev, SNOOP_MODE)
    if fd <= 0:
        RegLib.test_fail("Could not open device")
    else:
        RegLib.test_log(0, "FD is %d" % fd)
    file_obj = os.fdopen(fd)
    if not file_obj:
        RegLib.test_fail("Could not open snoop device %s" % dev)

    filter_type = int(filter_by, base=16)

    if filter_type == FILTER_BY_MAD:
        RegLib.test_log(0, "Testing MAD mgmt class filter")
        do_packet_test(filter_type, 29, 0x10, 0x0)

    if filter_type == FILTER_BY_QP:
        RegLib.test_log(0, "Testing QP filter")
        do_packet_test(filter_type, 15, 0xFF, 0x0)

    if filter_type == FILTER_BY_PACKET_TYPE:
        RegLib.test_log(0, "Testing packet type")
        do_packet_test(filter_type, 8, 0x64, 0x00, 0x3)

    if filter_type == FILTER_BY_SL:
        RegLib.test_log(0, "Testing SL")
        do_packet_test(filter_type, 1, 0x12, 0x2, 0x1)

    if filter_type == FILTER_BY_PKEY:
        RegLib.test_log(0, "Testing Pkey")
        do_packet_test(filter_type, 11, 0x11, 0xFF, 0x7F11)

    if filter_type == FILTER_BY_SLID:
        RegLib.test_log(0, "Testing SLID")
        do_packet_test(filter_type, 7, 0x5, 0x1)

    if filter_type == FILTER_BY_DLID:
        RegLib.test_log(0, "Testing DLID")
        do_packet_test(filter_type, 3, 0x5, 0x1, 0x5)


    RegLib.test_pass("Completed!")


if __name__ == "__main__":
    main()

