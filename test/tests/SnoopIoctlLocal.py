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


IOCTL_SET_LINK_STATE_EXTRA = 3225426823

def get_link_state(val):
    val = val & 0xF

    if val ==  0x1:
        return "IB_PORT_DOWN"
    elif val == 0x2:
        return "IB_PORT_INIT"
    elif val == 0x3:
        return "IB_PORT_ARMED"
    elif val == 0x4:
        return "IB_PORT_ACTIVE"
    else:
        RegLib.test_fail("Unknown link state")

def get_phys_link_state(val):
    val = (val >> 4) & 0xF

    if val == 0:
        return "IB_PORTPHYSSTATE_NO_CHANGE"
    elif val == 1:
        return "IB_PORTPHYSSTATE_SLEEP"
    elif val == 2:
        return "IB_PORTPHYSSTATE_POLL"
    elif val == 3:
        return "IB_PORTPHYSSTATE_DISABLED"
    elif val == 4:
        return "IB_PORTPHYSSTATE_CFG_TRAIN"
    elif val == 5:
        return "IB_PORTPHYSSTATE_LINKUP"
    elif val == 6:
        return "IB_PORTPHYSSTATE_LINK_ERR_RECOVER"
    elif val == 7:
        return "IB_PORTPHYSSTATE_PHY_TEST"
    elif val == 9:
        return "OPA_PORTPHYSSTATE_OFFLINE"
    else:
        RegLib.test_fail("Unknown phys state %s" % val)

def is_active(link, phys):
    if (link == "IB_PORT_ACTIVE") and (phys == "IB_PORTPHYSSTATE_LINKUP"):
        return True
    else:
        return False

def is_init(link, phys):
    if (link == "IB_PORT_INIT") and (phys == "IB_PORTPHYSSTATE_LINKUP"):
        return True
    else:
        return False

def is_armed(link, phys):
    if (link == "IB_PORT_ARMED") and (phys == "IB_PORTPHYSSTATE_LINKUP"):
        return True
    else:
        return False

def is_polling(link, phys):
    if phys == "IB_PORTPHYSSTATE_POLL":
        return True
    else:
        return False

def is_training(link, phys):
    if phys == "IB_PORTPHYSSTATE_CFG_TRAIN":
        return True
    else:
        return False

def is_offline(link, phys):
    if phys == "OPA_PORTPHYSSTATE_OFFLINE":
        return True
    else:
        return False

def is_phy_test(link, phys):
    if phys == "IB_PORTPHYSSTATE_PHY_TEST":
        return True
    else:
        return False

def send_simple_ioctl(file_obj, ioctl):

    buf = array.array('i', [0])
    RegLib.test_log(0, "Sending ioctl %d" % ioctl)
    ret = fcntl.ioctl(file_obj, ioctl, buf)
    if ret:
        RegLib.test_fail("Could not set ioctl")
    return buf[0]

def send_complex_ioctl(file_obj, ioctl, buf):
    RegLib.test_log(0, "Sending ioctl %d" %ioctl)
    #(addr, len) = buf.buffer_info()
    str_buf = buf.decode()
    ret = fcntl.ioctl(file_obj, ioctl, str_buf)
    out_bytes = ret.encode("hex")

    return out_bytes

def create_hfi_link_info():

    # This is how the structure is laid out in wfr matches wfr-lite exept for
    # the port number added. This is required.
    #    struct hfi_link_info {
    #            __be64 node_guid;
    #            u8 port_mode;
    #            u8 port_state;
    #            __be16 link_speed_active;
    #            __be16 link_width_active;
    #            __be16 vl15_init;
    #            unsigned short port_number;
    #            u8 res[47]; /* This makes this a full IB SMP payload */
    #    };

    hfi_link_info = bytearray(64)

    for i in range(64):
       hfi_link_info[i] = 0x0

    return hfi_link_info

def get_version(file_obj):
    ioctl = 7045
    val = send_simple_ioctl(file_obj, ioctl)
    RegLib.test_log(0, "IOCTL_GET_VERSION:")
    RegLib.test_log(0, "\tVers: %s" % val)

def get_link_phys_state(file_obj):
        ioctl = 7040
        val = send_simple_ioctl(file_obj, ioctl)
        phys = get_phys_link_state(val)
        link = get_link_state(val)
        RegLib.test_log(0, "IOCTL_GET_LINK_STATE:")
        RegLib.test_log(0, "\tLink:     %s" % link)
        RegLib.test_log(0, "\tPhysLink: %s" % phys)
        return (link, phys)

def parse_complex_ioctl(info):

        # Guid is in BE as are a few other fields, not sure why
        guid = ""
        for i in [14, 12, 10, 8, 6, 4, 2, 0]:
            guid = guid + info[i] + info[i+1] + " "
        print "GUID", guid
        print "Port Mode: %s%s" % (info[16], info[17])
        port_state = int(info[18] + info[19], base=16)
        phys = get_phys_link_state(port_state)
        link = get_link_state(port_state)
        print "Port State: %s%s (%s|%s)" % (info[18], info[19], link,phys)
        print "Link Speed: %s%s %s%s" % (info[22], info[23], info[20], info[21])
        print "Link Width: %s%s %s%s" % (info[26], info[27], info[24], info[25])
        print "Raw Info:", info

        return (link, phys)

def get_link_phys_state_extra(file_obj):
        ioctl = 3225426822
        buffer = create_hfi_link_info()
        info = send_complex_ioctl(file_obj, int(ioctl), buffer)
        (link, phys) = parse_complex_ioctl(info)
        return (link, phys)

def change_port_state(file_obj, ioctl, logical, phys):
    RegLib.test_log(0, "Modifying port to %s|%s" % (logical, phys))

    if logical == "IB_PORT_NOP": logical_val = 0
    elif logical == "IB_PORT_DOWN": logical_val = 1
    elif logical == "IB_PORT_INIT": logical_val = 2
    elif logical == "IB_PORT_ARMED": logical_val = 3
    elif logical == "IB_PORT_ACTIVE": logical_val = 4
    else: RegLib.test_fail("Unknown logical state")

    if phys == "IB_PORTPHYSSTATE_NO_CHANGE": phys_val = 0
    elif phys == "IB_PORTPHYSSTATE_SLEEP": phys_val = 1
    elif phys == "IB_PORTPHYSSTATE_POLL": phys_val = 2
    elif phys == "IB_PORTPHYSSTATE_DISABLED": phys_val = 3
    elif phys == "IB_PORTPHYSSTATE_LINKUP": phys_val = 5
    else: RegLib.test_fail("Unknown Physical state")

    link_settings = create_hfi_link_info()

    link_phys = 0
    link_phys = phys_val
    link_phys = link_phys << 4
    link_phys = link_phys | logical_val
    RegLib.test_log(0, "Setting state to 0x%x" % link_phys)

    link_settings[9] = link_phys

    info = send_complex_ioctl(file_obj, ioctl, link_settings)

    (rlink, rphys) = parse_complex_ioctl(info)

    return (rlink, rphys)

def check_sanity(file_obj, link, phys):
    (link_check, phys_check) = get_link_phys_state(file_obj)

    if (link_check != link):
        RegLib.test_fail("Failed sanity check 1")

    if (phys_check != phys):
        RegLib.test_fail("Failed sanity check 2")

def arm(file_obj):
    ioctl = IOCTL_SET_LINK_STATE_EXTRA
    RegLib.test_log(0,"Arming...")
    (link, phys) = change_port_state(file_obj, ioctl, "IB_PORT_ARMED", "IB_PORTPHYSSTATE_NO_CHANGE")
    if not is_armed(link, phys):
        RegLib.test_fail("Failed to bring to armed state")
    check_sanity(file_obj, link, phys)

def make_active(file_obj):
    ioctl = IOCTL_SET_LINK_STATE_EXTRA
    RegLib.test_log(0, "Bringing to active...")
    (link, phys) = change_port_state(file_obj, ioctl, "IB_PORT_ACTIVE", "IB_PORTPHYSSTATE_NO_CHANGE")
    if not is_active(link, phys):
        RegLib.test_fail("Failed to bring to active state")
    check_sanity(file_obj, link, phys)

def disable(file_obj):
    ioctl = IOCTL_SET_LINK_STATE_EXTRA
    RegLib.test_log(0, "Disabling...")
    (link, phys) = change_port_state(file_obj, ioctl, "IB_PORT_NOP", "IB_PORTPHYSSTATE_DISABLED")
    check_sanity(file_obj, link, phys)

def set_polling(file_obj):
    ioctl = IOCTL_SET_LINK_STATE_EXTRA
    RegLib.test_log(0, "Setting to polling and waiting for link to come up...")
    (link, phys) = change_port_state(file_obj, ioctl, "IB_PORT_NOP", "IB_PORTPHYSSTATE_POLL")
    attempts = 0
    while attempts < 60:
        if is_polling(link, phys) or is_training(link, phys) or is_phy_test(link, phys) or is_offline(link, phys):
            RegLib.test_log(0, "Polling or training, check again in 1 second")
            time.sleep(1)
            attempts += 1
            (link, phys) = get_link_phys_state_extra(file_obj)
            (link_check, phys_check) = get_link_phys_state(file_obj)
            if phys_check == "IB_PORT_PHYSSTATE_LINKUP":
                break
        else:
            break

    check_sanity(file_obj, link, "IB_PORTPHYSSTATE_LINKUP")

def show_sys_state():
    RegLib.test_log(0, "----------------------------")
    RegLib.test_log(0, "Dumping logical state from sysfs")
    os.system("cat /sys/class/infiniband/hfi1_0/ports/1/state")
    RegLib.test_log(0, "Dumping phyhsical state from sysfs")
    os.system("cat /sys/class/infiniband/hfi1_0/ports/1/phys_state")
    RegLib.test_log(0, "----------------------------")

# Not using but leave here in case we need to add it back in for debugging
# later.
def get_crc_errors(hfidiags):
    # adapted from Dean's script
    RegLib.test_log(0, "Dumping CRC error count")
    os.system("%s -t -e \"w DC8051_CFG_HOST_CMD_0 0x0100000000000500\"" % hfidiags)
    os.system("%s -t -e \"w DC8051_CFG_HOST_CMD_0 0x0100000000000501\"" % hfidiags)
    os.system("%s -t -e \"w DC8051_CFG_CSR_ACCESS_SEL.LCB 1\"" % hfidiags)
    os.system("%s -t -e \"r LCB_ERR_INFO_TOTAL_CRC_ERR\"" % hfidiags)
    os.system("%s -t -e \"w DC8051_CFG_CSR_ACCESS_SEL.LCB 0\"" % hfidiags)
    os.system("%s -t -e \"w DC8051_CFG_HOST_CMD_0 0x0200000000000500\"" % hfidiags)
    os.system("%s -t -e \"w DC8051_CFG_HOST_CMD_0 0x0200000000000501\"" % hfidiags)

def main():
    SNOOP_MODE = os.O_RDWR
    dev = "/dev/hfi1_diagpkt0"
    RegLib.test_log(0, "Opening device")
    fd = os.open(dev, SNOOP_MODE)
    if fd <= 0:
        RegLib.test_fail("Could not open device")
    else:
        RegLib.test_log(0, "FD is %d" % fd)
    file_obj = os.fdopen(fd)
    if not file_obj:
        RegLib.test_fail("Could not open snoop device %s" % dev)

    get_version(file_obj)

    test_info = RegLib.TestInfo()
    diags_path = test_info.get_diag_lib()
    hfidiags = "hfidiags"
    if diags_path != "DEFAULT":
        hfidiags = diags_path + "/hfidiags/hfidiags"
    RegLib.test_log(0, "Using %s for hfidiags" % hfidiags)

    show_sys_state()

    # Should be able to go to disabled from any state
    disable(file_obj)
    show_sys_state()

    # Go to polling
    set_polling(file_obj)
    show_sys_state()

    # now arm
    arm(file_obj)
    show_sys_state()

    # finally we can go active
    make_active(file_obj)
    show_sys_state()

    RegLib.test_pass("Completed")

if __name__ == "__main__":
    main()

