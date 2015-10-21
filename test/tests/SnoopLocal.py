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

packet_list = []

def signal_handler(signal, frame):

    print "Caught Ctrl+C"

    # Don't worry about closing the FD or file_obj pyhon gets mad if we
    # close one then the other. Just let the interrpruter do all the
    # work.

    for packet in packet_list:
        bytes = bytearray(packet)
        vl = bytes[0] >> 4
        dlid_upper = bytes[2] << 8
        dlid = dlid_upper | bytes[3]
        pkt_len = bytes[5]
        slid_upper = bytes[6] << 8
        slid = slid_upper | bytes[7]
        print ".....logged", len(packet), "byte pkt",
        print "vl", vl, "pktlen", pkt_len, "Dwords",
        print "dlid", dlid, "slid", slid

    RegLib.test_pass("Success!")
    sys.exit(0)


def main():

    IB_PACKET_SIZE = 5000 #iba_pcap.h = 4208 but not big enough for 4K MTU
    FILTER_BY_SLID = 0x0
    FILTER_BY_DLID = 0x1
    SET_FILTER_IOCTL = 7044
    CLEAR_FILTER_IOCTL = 7043
    CLEAR_QUEUE_IOCTL = 7042
    SET_OPTS_IOCTL = 7046
    corrupt_dlid = 0
    drop_packet = 1
    filter_by = FILTER_BY_DLID
    set_filter_ioctl = 0
    flip_lids = 0

    SNOOP_MODE = os.O_RDWR
    CAPTURE_MODE = os.O_RDONLY

    test_info = RegLib.TestInfo()

    (filter_by,
     filter_value,
     corrupt_dlid,
     drop_packet,
     flip_lids,
     lid1,
     lid2,
     drop_send_flag) = test_info.parse_extra_args()

    print "Filter By:", filter_by
    print "Filter Val:", filter_value
    print "Corrupt Dlid:", corrupt_dlid
    print "Drop packet:", drop_packet
    print "Flip Lids:", flip_lids
    print "lid 1:", lid1
    print "lid 2:", lid2
    print "Drop send flag:", drop_send_flag

    dev = "/dev/hfi1_diagpkt0"

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
        RegLib.test_fail("Could not open capture device %s" % c_dev0)

    # Filter value we need to pass to kernel. We need a pointer to it.
    value = int(filter_value, base=16)
    filter_buf = array.array('i', [value])
    (filter_addr, filter_len) = filter_buf.buffer_info()

    # Need a buffer to write to kernel. Includes the filter opcode
    # The length of the filter value, and a pointer to that filter
    # value.
    filter_opcode = int(filter_by, base=16)
    filter_len = 0x4
    buf = array.array('i', [filter_opcode, filter_len, filter_addr])

    if filter_opcode > 0:
        set_filter_ioctl = 1

    # clear any filters
    RegLib.test_log(0, "Cleraing existing filters")
    ret = fcntl.ioctl(file_obj, CLEAR_FILTER_IOCTL, buf)
    print "CLEAR IOCTL return", ret

   # Empty the snoop ring
    RegLib.test_log(0, "Clearing ring buffer")
    ret = fcntl.ioctl(file_obj, CLEAR_QUEUE_IOCTL, buf)
    print "CLEAR IOCTL return", ret
    if ret:
        RegLib.test_fail("Could not clear filters")

    # Now we can send the IOCTL, 7044 means set filter
    if set_filter_ioctl:
        RegLib.test_log(0, "Sending IOCTL")
        ret = fcntl.ioctl(file_obj, SET_FILTER_IOCTL, buf)
        print "IOCTL return", ret

    drop_flag = int(drop_send_flag)
    RegLib.test_log(0, "Need to set the drop flag to %d" % drop_flag)
    value = drop_flag
    flag_buf = array.array('i', [value])
    (flag_addr, flag_len) = flag_buf.buffer_info()
    flag_len = 0x4
    buf = array.array('i', [value, flag_len, flag_addr])
    ret = fcntl.ioctl(file_obj, SET_OPTS_IOCTL, flag_buf)
    if ret:
        RegLib.teset_fail("Could not set drop flag")

    # Before going into our infinite loop set up a trap for ctrl+c
    signal.signal(signal.SIGINT, signal_handler)

    # Go collect some packets
    while True:
        RegLib.test_log(0, "Waiting for a packet")
        packet = os.read(fd, IB_PACKET_SIZE)
        packet_list.append(packet)
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

        if drop_packet == "1":
            continue

        # packet contains the CRC. We need to throw that away before writing it
        # back out via the snoop device.
        if corrupt_dlid == "1":
            outpacket = packet[:3] # [VL|LVER] [SL|Res|LNH] [upper DLID]
            outpacket = outpacket + "B" #[lower DLID = 66 or 0x42]
            outpacket = outpacket + packet[4:-4]
        elif flip_lids == "1":
            if "0x" + str(dlid) == lid1:
                outpacket = packet[:2] #[VL|LVER] [SL|Res|LNH]
                outpacket = outpacket + packet[6] + packet[7] #[DLID]
                outpacket = outpacket + packet[4] + packet[5] #[RES|PktLen]
                outpacket = outpacket + packet[2] + packet[3] #[SLID]
                outpacket = outpacket + packet[8] + packet[9] #[Opcode][SE..Tver]
                outpacket = outpacket + packet[10] + packet[11] #[Pkey]
                outpacket = outpacket + packet[12] #[F|B|Res]

                # Get DestQP from DETH of the incoming packet
                outpacket = outpacket + packet[25] + packet[26] + packet[27]

                # Ack | Res | PSN
                outpacket = outpacket + packet[16] + packet[17] + packet[18] + packet[19]

                # Build the DETH
                outpacket = outpacket + packet[20] + packet[21] + packet[22] + packet[23]
                outpacket = outpacket + packet[24]

                #Add in the correct source QP
                outpacket = outpacket + packet[13] + packet[14] + packet[15]

                # Now fill in the rest of the packet tossing the CRC
                outpacket = outpacket + packet[28:-4]

                bytes = bytearray(outpacket)
                vl = bytes[0] >> 4
                dlid_upper = bytes[2] << 8
                dlid = dlid_upper | bytes[3]
                pkt_len = bytes[5]
                slid_upper = bytes[6] << 8
                slid = slid_upper | bytes[7]
                print "OUT: Found a", len(packet), "byte pkt",
                print "OUT: vl", vl, "pktlen", pkt_len, "Dwords",
                print "OUT: dlid", dlid, "slid", slid

            else:
                print "FILTERING BORKED"
                continue
        else:
            outpacket = packet[:-4]

        byte_count = len(outpacket)
        print "byte count is", byte_count

        written = os.write(fd, outpacket)
        RegLib.test_log(0, "Wrote " + str(written) + " bytes of packet back")


if __name__ == "__main__":
    main()

