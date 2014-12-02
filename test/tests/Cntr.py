#!/usr/bin/python

# Author: Denny Dalessandro (dennis.dalessandro@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time
import subprocess

class Counter:
    """Represents a counter"""

    def __init__(self, name, stats_tag, diags_tag, pma_tag, vl):
        self.name = name
        self.s_tag = stats_tag
        self.d_tag = diags_tag
        self.p_tag = pma_tag
        self.vl = vl
        self.valid = 1 # start out as good

    def get_data(self, stats_output, diags_output, pma_output):
        self.s_val = self.get_stats_val(stats_output)
        self.p_val = self.get_pma_val(pma_output)
        self.d_val = self.get_diags_val(diags_output)

    def get_stats_val(self, stats_output):
        RegLib.test_log(0, "%s ::> finding stats value %s" % (self.name, self.s_tag))
        ret = 0
        
        for line in stats_output:
            matchObj = re.search(r"%s\s+(\d+)K" % self.s_tag, line)
            if matchObj:
                val = matchObj.group(1)
                #RegLib.test_log(0, "\t\t\t= %s" % val)
                ret = int(val)
                ret = ret * 1000
                RegLib.test_log(0, "\t\t\tDetected K value")
                RegLib.test_log(0, "\t\t\t= 0x%x" % ret)         
                return ret

            matchObj = re.search(r"%s\s+(\d+)" % self.s_tag, line)
            if matchObj:
                val = matchObj.group(1)
                ret = int(val)
                RegLib.test_log(0, "\t\t\t= 0x%x" % ret)
                return ret
        return ret

    def get_diags_val(self, diags_output):
        RegLib.test_log(0, "%s ::> finding diags value %s" % (self.name, self.d_tag))
        ret = 0
        for line in diags_output:
            matchObj = re.search(r"%s @ \S+\s+ (\S+)" % self.d_tag, line)
            if matchObj:
                val = matchObj.group(1)
                #RegLib.test_log(0, "\t\t\t= %s" % val)
                ret = int(val, 0) 
                RegLib.test_log(0, "\t\t\t= 0x%x" % ret)
        return ret

    def get_pma_val(self, pma_output):
        RegLib.test_log(0, "%s ::> finding pma value %s" % (self.name, self.p_tag))
        ret = 0
        vl_found = 0
        if self.vl == -1:
            vl_found = 1
        for line in pma_output:
            if vl_found == 1:
                matchObj = re.search(r"%s.+\((\d+) flits" % self.p_tag, line)
                if matchObj:
                    val = matchObj.group(1)
                    ret = int(val, 0) 
                    RegLib.test_log(0, "\t\t\t= 0x%x" % ret)
                    return ret
            else:
                matchObj = re.search(r"VL Number\s+(\d+)", line)
                if matchObj:
                    val = matchObj.group(1)
                    if int(val) == self.vl:
                        vl_found = 1
                        RegLib.test_log(0, "\t\t\tFound VL %d" % self.vl)

    def mark_bad(self):
        self.valid = 0
        RegLib.test_log(0, "Marking counter as bad")

    def get_name(self):
        return self.name

    def get_status(self):
        if self.valid == 0:
            return "INVALID"
        else:
            return "VALID"
    
    def is_bad(self):
        if self.valid == 0:
            return True
        else:
            return False

    def validate(self, threshold, saturate_ok):
        s_val = self.s_val
        p_val = self.p_val
        d_val = self.d_val
        
        if s_val == 0:
            RegLib.test_log(0, "ERROR: s_val == 0")
            self.mark_bad()
            return

        if d_val == 0:
            RegLib.test_log(0, "ERROR: d_val == 0")
            self.mark_bad()
            return

        if p_val == 0:
            RegLib.test_log(0, "ERROR: p_val == 0")
            self.mark_bad()
            return
        max_val=0xFFFFFFFFFFFFFFFF
        if ((s_val != d_val) or (s_val != p_val) or (p_val != d_val)) :
            # based on the order the counters could be slightly different 
            if (d_val != p_val):
                diff = d_val - p_val
                pct_diff = (float(diff) / float(p_val)) * 100
                
                if pct_diff > threshold:
                    if saturate_ok == 1 and (d_val == max_val or p_val == max_val):
                        RegLib.test_log(0, "WARNING: d_val != p_val but one is saturated")
                    else:
                        RegLib.test_log(0, "ERROR: d_val != p_val beyond range [%f]" % pct_diff)
                        self.mark_bad()
                else:
                    RegLib.test_log(0, "WARNING: d_val != p_val within range [%f]" % pct_diff)
        
            if (s_val != p_val):
                diff = p_val - s_val
                pct_diff = (float(diff) / float(s_val)) * 100
                
                if pct_diff > threshold:
                     if saturate_ok == 1 and (p_val == max_val or s_val == max_val):
                        RegLib.test_log(0, "WARNING: d_val != p_val but one is saturated")
                     else:
                        RegLib.test_log(0, "ERROR: s_val != p_val beyond range [%f]" % pct_diff)
                        self.mark_bad()
                else:
                    RegLib.test_log(0, "WARNING: s_val != p_val within range [%f]" % pct_diff)
               
            if (s_val != d_val):
                diff = d_val - s_val
                pct_diff = (float(diff) / float(d_val)) * 100
                
                if pct_diff > threshold:
                     if saturate_ok == 1 and (d_val == max_val or s_val == max_val):
                        RegLib.test_log(0, "WARNING: d_val != s_val but one is saturated")
                     else:
                        RegLib.test_log(0, "ERROR: s_val != d_val beyond range [%f]" % pct_diff)
                        self.mark_bad()
                else:
                    RegLib.test_log(0, "WARNING: s_val != d_val within range [%f]" % pct_diff)

def do_test(test_info, diags_read_cmd, pma_query, hfi_stats, host1, host2, threshold, saturate_ok=0):

    # To reconcile the counters we need iba_pmaquery, hfistats, and hfi_diags
    # output. Hfi stats needs to come first because if the value is too large
    # it gets divided by 1000 and we lose that precision.
    
    RegLib.test_log(0, "Running %s" % hfi_stats)
    (err, stats_out) = host1.send_ssh(hfi_stats, run_as_root = True)
    if err:
        RegLib.test_fail("Could not get hfistats output")

    RegLib.test_log(0, "Running %s" % pma_query)
    (err, pma_out) = host1.send_ssh(pma_query, run_as_root = True)
    if err:
        RebLib.test_fail("Could not get pmaquery output")

    RegLib.test_log(0, "Running %s" % diags_read_cmd)
    (err, diags_out) = host1.send_ssh(diags_read_cmd, run_as_root = True)
    if err:
        RegLib.test_fail("Could not get hfidiags output")

    for line in stats_out:
        print line,

    for line in pma_out:
        print line,

    for line in diags_out:
        print line,

    cntr_list = []
    
    cntr_list.append(Counter("DcTxFlits", 
                             "DcXmitFlits",
                             "DCC_PRF_PORT_XMIT_DATA_CNT",
                             "Xmit Data",
                             -1))

    cntr_list.append(Counter("DcRcvFlits",
                             "DcRcvFlits",
                             "DCC_PRF_PORT_RCV_DATA_CNT",
                             "Rcv Data",
                             -1))
    
    cntr_list.append(Counter("TxFlitVL0",
                             "TxFlitVL0",
                             "SendCounterArray64\[3\]",
                             "Xmit Data",
                             0))

    cntr_list.append(Counter("TxFlitVL15",
                             "TxFlitVL15",
                             "SendCounterArray64\[11\]",
                             "Xmit Data",
                             15))

    # DcRxFlit counters are 0 on simics
    if test_info.is_simics() == False:
        cntr_list.append(Counter("DcRxFlitVl0",
                                 "DcRxFlitVl0",
                                 "DCC_PRF_PORT_VL_RCV_DATA_CNT\[0\]",
                                 "Rcv Data",
                                 0))

        cntr_list.append(Counter("DcRxFlitVl15",
                                 "DcRxFlitVl15",
                                 "DCC_PRF_PORT_VL_RCV_DATA_CNT\[8\]",
                                 "Rcv Data",
                                 15))

    for cntr in cntr_list:
        print ""
        cntr.get_data(stats_out, diags_out, pma_out)
        cntr.validate(threshold, saturate_ok)
    print ""

    # Check for any bad counter values
    for cntr in cntr_list:
        if cntr.is_bad() == True:
            RegLib.test_fail("bad counter values found!")

def do_main(test_info, mpi_cmd, diags_read_cmd, pma_query, hfi_stats, host1, host2, threshold,
            diags_32limit_cmd, diags_64limit_cmd):

  
    RegLib.test_log(0, "-------------------------")
    RegLib.test_log(0, "Test 1 Basic Counter Test")
    RegLib.test_log(0, "-------------------------")

    proc = subprocess.Popen(mpi_cmd, shell=True)
    status = proc.wait()
    if status:
        error("MPI Cmd failed")

    do_test(test_info, diags_read_cmd, pma_query, hfi_stats, host1, host2, threshold)


    RegLib.test_log(0, "------------------------")
    RegLib.test_log(0, "Test 2 32 Bit Limit Test")
    RegLib.test_log(0, "------------------------")

    RegLib.test_log(0, "Running %s" % diags_32limit_cmd)
    (err, diags_out) = host1.send_ssh(diags_32limit_cmd, run_as_root = True)
    if err:
        RegLib.test_fail("Could not get hfidiags output")

    proc = subprocess.Popen(mpi_cmd, shell=True)
    status = proc.wait()
    if status:
        error("MPI Cmd failed")
    do_test(test_info, diags_read_cmd, pma_query, hfi_stats, host1, host2, threshold)

    RegLib.test_log(0, "------------------------")
    RegLib.test_log(0, "Test 3 64 Bit Limit Test")
    RegLib.test_log(0, "------------------------")

    RegLib.test_log(0, "Running %s" % diags_64limit_cmd)
    (err, diags_out) = host1.send_ssh(diags_64limit_cmd, run_as_root = True)
    if err:
        RegLib.test_fail("Could not get hfidiags output")

    proc = subprocess.Popen(mpi_cmd, shell=True)
    status = proc.wait()
    if status:
        error("MPI Cmd failed")

    do_test(test_info, diags_read_cmd, pma_query, hfi_stats, host1, host2, threshold, 1)


def main():

    directory = os.path.dirname(os.path.realpath(__file__))
    diags = "/nfs/sc/disks/fabric_work/ddalessa/wfr/weekly-reg/wfr-diagtools-sw/hfidiags/hfidiags"
    diags_read_script = directory + "/Cntr.diags"
    diags_32limit_script = directory + "/Cntr32limit.diags"
    diags_64limit_script = directory + "/Cntr64limit.diags"
    pma_query = "iba_pmaquery -w 0x8009" # gets VL 0, 3, 15
    pma_query_clear = "iba_pmaquery -o clearportstatus -n 0x2"
    ifs_sweep = "service ifs_fm sweep && sleep 5 && service ifs_fm sweep && sleep 5"
    hfi_stats = "hfistats -i 0"
    mpi_cmd = directory + "/MpiStress.py --mpiverbs --args \"-L 5 -M 5 -w 10 -m 65536 -z\" --nodelist "
   
    threshold = 5 # Percent threshold

    test_info = RegLib.TestInfo()
    if test_info.is_simics() == True:
        diags = "/host" + diags
        diags_read_script = "/host" + diags_read_script
        diags_32limit_script = "/host" + diags_32limit_script
        diags_64limit_script = "/host" + diags_64limit_script
    
    diags_read_cmd = diags + " -s " + diags_read_script
    diags_32limit_cmd = diags + " -s " + diags_32limit_script
    diags_64limit_cmd = diags + " -s " + diags_64limit_script
      

    host1 = test_info.get_host_record(0)
    host2 = test_info.get_host_record(1)

    mpi_cmd = mpi_cmd + " " + host1.get_name() + "," + host2.get_name()
    RegLib.test_log(0, "MPI Cmd is %s" % mpi_cmd)
      
    do_main(test_info, mpi_cmd, diags_read_cmd, pma_query, hfi_stats, host1, host2, threshold,
            diags_32limit_cmd, diags_64limit_cmd)

    RegLib.test_log(0, "First pass completed. Zeroing and doing pass 2")

    (err) = host1.send_ssh(pma_query_clear, buffered=0, run_as_root=True)

    (err) = host1.send_ssh(ifs_sweep, buffered=0, run_as_root=True)

    do_main(test_info, mpi_cmd, diags_read_cmd, pma_query, hfi_stats, host1, host2, threshold,
            diags_32limit_cmd, diags_64limit_cmd)

    RegLib.test_pass("Success!")

if __name__ == "__main__":
    main()

