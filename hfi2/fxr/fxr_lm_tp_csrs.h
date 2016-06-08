// This file had been gnerated by ./src/gen_csr_hdr.py
// Created on: Thu Jun  2 19:11:24 2016
//

#ifndef ___FXR_lm_tp_CSRS_H__
#define ___FXR_lm_tp_CSRS_H__

// TP_CFG_RESET desc:
typedef union {
    struct {
        uint64_t          block_reset  :  1; // Software reset of TPORT block excluding CSRs. 1 = Resets TPORT block. 0 = Has no affect
        uint64_t          UNUSED_63_1  : 63; // 
    } field;
    uint64_t val;
} TP_CFG_RESET_t;

// TP_CFG_MISC_CTRL desc:
typedef union {
    struct {
        uint64_t      pkey_chk_enable  :  1; // Egress port Partition Key checking control. 0 = disables outbound partition key enforcement for this port. 1= enables partition key enforcement on all outbound packets for this port.
        uint64_t    disable_hoq_check  :  1; // Disable Tx HOQ checking. 0 = normal HOQ function 1 = turn off HOQ discarding at this port
        uint64_t    disable_sll_check  :  1; // Disable Tx SLL checking. 0 = normal SLL function 1 = turn off SLL discarding at this port
        uint64_t disable_epoch_cnt_check  :  1; // Disables checking for epoch counters non-zero when the route info pipeline is empty. Enabling this assumes that MBEs in the rinfo memories will never occur.
        uint64_t       fc_buffer_size  :  8; // Size of Otter Creek Flow Control Buffer, 16 Byte Flits. (Min is 0x3, Max is 0x14)
        uint64_t disable_reliable_vl8_0_pkts  :  9; // Disable reliable VL8:0 packets. 1 = Discard VL8:0 packets when the VL8:0 packet length is larger than the current available VL8:0 credits. 0 = normal (VL8:0 reliable)
        uint64_t disable_reliable_vl15_pkts  :  1; // Disable reliable VL15 packets. 1 = Discard VL15 packets when the VL15 packet length is larger than the current available VL15 credits. 0 = normal (VL15 reliable)
        uint64_t    vl8_0_arb_disable  :  9; // Disable VL8:0 to the transmit arbitration logic. Packet will accumulate in the VL8:0 fifo's.
        uint64_t     vl15_arb_disable  :  1; // Disable VL15 to the transmit arbitration logic. Packet tags will accumulate in the VL15 fifo.
        uint64_t      disable_all_vls  :  1; // TPort software link disable mode. Packets from all ports will be discarded at TPORT. Drop request's issued VL15, VL8:0.
        uint64_t          wormhole_en  :  1; // If set enables wormhole crediting (1 credit per flit), else use VCT crediting(use length field at head time). FW must set Au to 16 bytes and crdt_updt_threshold to 10.
        uint64_t disable_clock_gating  :  1; // Disables logic additional logic added for clock gating.
        uint64_t        rx_device_sel  :  1; // Set to 0 if connected to a Gen2 device. Set to 1 if connected to a Gen1 device, FW must set max nesting levels to 0 There is no tx_device_sel, when connected to LWEG FW must set WH mode and SB credit return mode
        uint64_t       operational_vl  : 10; // Virtual Lanes (VLt) Operational on this port. Bits 45:36 = VL_Mask[15,8:0] operational_vl is a bit enable mask for which VLt's are operational. Packets received on VLt's that are not operational are discarded.
        uint64_t      disable_8b_pkts  :  1; // Disables transmission of 8B packets
        uint64_t      disable_9b_pkts  :  1; // Disables transmission of 9B packets
        uint64_t     disable_10b_pkts  :  1; // Disables transmission of 10B packets
        uint64_t     disable_16b_pkts  :  1; // Disables transmission of 16B packets
        uint64_t         UNUSED_63_50  : 14; // 
    } field;
    uint64_t val;
} TP_CFG_MISC_CTRL_t;

// TP_CFG_TIMEOUT_CTRL desc:
typedef union {
    struct {
        uint64_t      test_to_counter  :  1; // Transmit packet request timeout timer control. Simulation timer control. ---- only used in validation testing. 0 = normal, 1us/inc 1 = sim mode, 2 clocks/inc
        uint64_t           req_to_inc  :  5; // Sets the time a packet request can wait for Ingress port packet EOP. Request Timeout = 32us x (2^Req_TO_Inc) 0x0 = 32us 0x1 = 64us 0x2 = 128us 0x3 = 256us ... 0x13 = 16.78 seconds 0x14 = 33.55 seconds 0x1F:0x15 = Infinite
        uint64_t           UNUSED_7_6  :  2; // 
        uint64_t        clocks_per_us  : 11; // Number of clocks per microsecond (default 1200)
        uint64_t         UNUSED_63_19  : 45; // 
    } field;
    uint64_t val;
} TP_CFG_TIMEOUT_CTRL_t;

// TP_CFG_VL_MTU desc:
typedef union {
    struct {
        uint64_t           vl0_tx_mtu  :  4; // TPORT Max MTU (length check) for VL0
        uint64_t           vl1_tx_mtu  :  4; // TPORT Max MTU (length check) for VL1
        uint64_t           vl2_tx_mtu  :  4; // TPORT Max MTU (length check) for VL2
        uint64_t           vl3_tx_mtu  :  4; // TPORT Max MTU (length check) for VL3
        uint64_t           vl4_tx_mtu  :  4; // TPORT Max MTU (length check) for VL4
        uint64_t           vl5_tx_mtu  :  4; // TPORT Max MTU (length check) for VL5
        uint64_t           vl6_tx_mtu  :  4; // TPORT Max MTU (length check) for VL6
        uint64_t           vl7_tx_mtu  :  4; // TPORT Max MTU (length check) for VL7
        uint64_t           vl8_tx_mtu  :  4; // TPORT Max MTU (length check) for VL8
        uint64_t         UNUSED_59_36  : 24; // 
        uint64_t          vl15_tx_mtu  :  4; // TPORT Max MTU (length check) for VL15 (Max Transfer, payload in bytes) 0x1 = 256 + 128 bytes for header data (default) 0x2 = 512 + 128 bytes for header data 0x3 = 1024 + 128 bytes for header data 0x4 = 2048 + 128 bytes for header data 0x5 = 4096 + 128 bytes for header data 0x6 = 8192 + 128 bytes for header data 0x7 = 10240 + 128 bytes for header data 0x0, 0x8-0xF = reserved
    } field;
    uint64_t val;
} TP_CFG_VL_MTU_t;

// TP_CFG_VL_HOQ_LIFE desc:
typedef union {
    struct {
        uint64_t               entry0  :  5; // Sets the time a packet can live at the head of the VL0 queue. Request Timeout = 4us x (2^Req_TO_Inc) (+1%/-25%) 0x0 = 4us 0x1 = 8us 0x2 = 16us 0x3 = 32us ... 0x13 = 2.10 seconds 0x14 = 4.19 seconds 0x1F:0x15 = Infinite
        uint64_t               entry1  :  5; // Sets the time a packet can live at the head of the VL1 queue. Request Timeout = same as entry0
        uint64_t               entry2  :  5; // Sets the time a packet can live at the head of the VL2 queue. Request Timeout = same as entry0
        uint64_t               entry3  :  5; // Sets the time a packet can live at the head of the VL3 queue. Request Timeout = same as entry0
        uint64_t               entry4  :  5; // Sets the time a packet can live at the head of the VL4 queue. Request Timeout = same as entry0
        uint64_t               entry5  :  5; // Sets the time a packet can live at the head of the VL5 queue. Request Timeout = same as entry0
        uint64_t               entry6  :  5; // Sets the time a packet can live at the head of the VL6 queue. Request Timeout = same as entry0
        uint64_t               entry7  :  5; // Sets the time a packet can live at the head of the VL7 queue. Request Timeout = same as entry0
        uint64_t               entry8  :  5; // Sets the time a packet can live at the head of the VL8 queue. Request Timeout = same as entry0
        uint64_t               entry9  :  5; // Sets the time a packet can live at the head of the VL15 queue. Request Timeout = same as entry0
        uint64_t         vl_stall_cnt  :  3; // Specifies the number of sequential packets dropped that causes the port to enter the VL_Stall_State. Valid for VL15,8:0 Range: bits 52:50 encoded = 8:1 packets
        uint64_t         UNUSED_63_53  : 11; // 
    } field;
    uint64_t val;
} TP_CFG_VL_HOQ_LIFE_t;

// TP_CFG_SC_TO_VLT_MAP desc:
typedef union {
    struct {
        uint64_t               entry0  :  4; // SC n+0 to VL_t
        uint64_t               entry1  :  4; // SC n+1 to VL_t
        uint64_t               entry2  :  4; // SC n+2 to VL_t
        uint64_t               entry3  :  4; // SC n+3 to VL_t
        uint64_t               entry4  :  4; // SC n+4 to VL_t
        uint64_t               entry5  :  4; // SC n+5 to VL_t
        uint64_t               entry6  :  4; // SC n+6 to VL_t
        uint64_t               entry7  :  4; // SC n+7 to VL_t
        uint64_t               entry8  :  4; // SC n+8 to VL_t
        uint64_t               entry9  :  4; // SC n+9 to VL_t
        uint64_t              entry10  :  4; // SC n+10 to VL_t
        uint64_t              entry11  :  4; // SC n+11 to VL_t
        uint64_t              entry12  :  4; // SC n+12 to VL_t
        uint64_t              entry13  :  4; // SC n+13 to VL_t
        uint64_t              entry14  :  4; // SC n+14 to VL_t
        uint64_t              entry15  :  4; // SC n+15 to VL_t
    } field;
    uint64_t val;
} TP_CFG_SC_TO_VLT_MAP_t;

// TP_CFG_VLP_TO_VLT_MAP desc:
typedef union {
    struct {
        uint64_t               entry0  :  4; // VLp n+0 to VL_t
        uint64_t               entry1  :  4; // VLp n+1 to VL_t
        uint64_t               entry2  :  4; // VLp n+2 to VL_t
        uint64_t               entry3  :  4; // VLp n+3 to VL_t
        uint64_t               entry4  :  4; // VLp n+4 to VL_t
        uint64_t               entry5  :  4; // VLp n+5 to VL_t
        uint64_t               entry6  :  4; // VLp n+6 to VL_t
        uint64_t               entry7  :  4; // VLp n+7 to VL_t
        uint64_t               entry8  :  4; // VLp n+8 to VL_t
        uint64_t         UNUSED_59_36  : 24; // 
        uint64_t              entry15  :  4; // VLp n+15 to VL_t
    } field;
    uint64_t val;
} TP_CFG_VLP_TO_VLT_MAP_t;

// TP_CFG_BWCHECK_UNUSED desc:
typedef union {
    struct {
        uint64_t vl0_conforming_priority  :  2; // Same definition as above
        uint64_t  vl0_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl1_conforming_priority  :  2; // Same definition as above
        uint64_t  vl1_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl2_conforming_priority  :  2; // Same definition as above
        uint64_t  vl2_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl3_conforming_priority  :  2; // Same definition as above
        uint64_t  vl3_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl4_conforming_priority  :  2; // Same definition as above
        uint64_t  vl4_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl5_conforming_priority  :  2; // Same definition as above
        uint64_t  vl5_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl6_conforming_priority  :  2; // Same definition as above
        uint64_t  vl6_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl7_conforming_priority  :  2; // Same definition as above
        uint64_t  vl7_bwmeter_pointer  :  3; // Same definition as above
        uint64_t vl8_conforming_priority  :  2; // 2'd3 = <reserved, interpreted as low> 2'd2 = high 2'd1 = medium 2'd0 = low
        uint64_t  vl8_bwmeter_pointer  :  3; // Pointer to the Bandwidth Meter associated with this VL. 3'd0 = BWMeter0 ... 3'd7 = BWMeter7
        uint64_t         UNUSED_63_45  : 19; // 
    } field;
    uint64_t val;
} TP_CFG_BWCHECK_UNUSED_t;

// TP_CFG_PREEMPT_CTRL desc:
typedef union {
    struct {
        uint64_t     preemption_limit  :  8; // Interleaved byte count limit. This register sets the maximum number of bytes a large packet can be interleaved by. After this limit is reached, a packet can no longer be preempted. It can still be bubble filled however. Preempt byte limit = (256 x preemption_limit ) 0x0 = 0 byte limit (Large packets can not be preempted) 0x1 = 256 byte limit 0x2 = 512 byte limit 0x3 = 768 byte limit 0x4 = 1024 byte limit --- default ... 0xFD= 64768 byte limit 0xFE= 65024 byte limit 0xFF = unlimited number of bytes can preempt large packet Preemption is allowed on packet boundaries as per the preempt matrix.
        uint64_t      small_pkt_limit  :  8; // Small packet limit. This register sets the size of packets that can Preempt large packets. It does not apply to bubble filling however. The small packet limit applies to all operational VL's. Packet size = 32 bytes + (32 x small_pkt_limit ) 0x0 = 32 bytes or less 0x1 = 64 bytes or less 0x2 = 96 bytes or less --- default 0xD = 384 bytes or less 0xE = 448 bytes or less 0xF = 512 bytes or less ... 0xFF = 8192 Bytes or less Preemption is allowed on packet boundaries as per the preempt matrix.
        uint64_t      large_pkt_limit  :  4; // Large packet limit. This register sets the size of packets that can be Preempted. It does not apply to bubble filling however. The large packet limit applies to all operational data VL's. Packet size = 512 bytes + (512 x large_pkt_limit ) 0x0 = 512 bytes or more 0x1 = 1024 bytes or more --- default ... 0xE = 7680 bytes or more 0xF = 8192 bytes or more Preemption is allowed on packet boundaries as per the preempt matrix.
        uint64_t             min_tail  : 10; // This register sets the minimum number of Fabric Packet flits for a given packet prior to a tail after which preemption cannot occur. Packet will not be preempted after this point. This rule does not apply to bubble filling or to a truncated packet. even if bubbles occur (assuming consistent config of whole fabric by FM, such bubbles will be limited to link replay events and should be infrequent).Range is 1 to 648 (allowing values of 8 to 10240). 0x0 = 8 Bytes 0x1 = 8 Bytes 0x2 = 24 Bytes 0x3 = 40 Bytes 0x4 = 56 Bytes --- default 0x27E = 10200 Bytes 0x27F = 10216 Bytes if ( min_tail == 0) Num_bytes = 8 Bytes else Num_bytes =(min_tail -1) * 16 Bytes + 8 Bytes
        uint64_t         UNUSED_31_30  :  2; // 
        uint64_t          min_initial  : 10; // This registers sets the minimum number of Fabric Packet flits for a given packet before preemption can occur after a head flit is encountered. Packet will not be preempted prior to this point. This rule does not apply to bubble filling., even if bubbles occur (assuming consistent config of whole fabric by FM, such bubbles will be limited to link replay events and should be infrequent). Range is 1 to 639 (allowing values of 16 to 10240) -- Count includes flit containing SOP. 0x0 = 16 Bytes --- default 0x1 = 32 Bytes 0x2 = 48 Bytes 0x3 = 64 Bytes ... 0x27E = 10224 Bytes 0x27F = 10240 Bytes Num_bytes = ( min_initial +1) * 16 Bytes
        uint64_t max_nest_level_tx_enabled  :  4; // Maximum nesting levels of preemption which the transmitter shall use. The FM must configure this value <= min (MaxNestLevelRxSupported of both ports for the link). If this value is set to 0, use of preemption or bubble filling is disabled and the remaining preemption configuration registers, Preempting table, Preemption Matrix, and Limit of Preempting are ignored
        uint64_t        tail_distance  :  8; // Tail minimum distance. This sets the minimum distance from the tail of a packet on one VL to any flit on a different VL or the next head on the same VL. This distance is not guaranteed if there is a pkey error in a 9B GRH packet. 0x0 = 0 Bytes --- default 0x1 = 16 Bytes 0x2 = 32 Bytes 0x3 = 48 Bytes ... 0xFE = 4064 Bytes 0xFF = 4080 Bytes Num_bytes = ( tail_distance ) * 16 Bytes
        uint64_t        head_distance  :  4; // Head minimum distance. This sets the minimum distance from the head of a packet on one VL to any flit on a different VL or the next head on the same VL. Count includes flit containing SOP. 0x0 = 16 Bytes --- default 0x1 = 32 Bytes 0x2 = 48 Bytes 0x3 = 64 Bytes ... 0xE = 240 Bytes 0xF = 256 Bytes Num_bytes = ( head_distance +1) * 16 Bytes
        uint64_t         UNUSED_63_58  :  6; // 
    } field;
    uint64_t val;
} TP_CFG_PREEMPT_CTRL_t;

// TP_CFG_PREEMPT_MATRIX desc:
typedef union {
    struct {
        uint64_t               entry0  : 10; // VL N+0 Packet preemption matrix mask. Bit 9:0 = VL15,VL8:VL0 (preempt packet) If the mask bit is set to a 1, this VL is allowed to preempt the VL in the mask, otherwise it is not.
        uint64_t               entry1  : 10; // Same as Bits 9:0
        uint64_t               entry2  : 10; // Same as Bits 9:0
        uint64_t               entry3  : 10; // Same as Bits 9:0
        uint64_t               entry4  : 10; // Same as Bits 9:0
        uint64_t         UNUSED_63_50  : 14; // 
    } field;
    uint64_t val;
} TP_CFG_PREEMPT_MATRIX_t;

// TP_CFG_PKEY_TABLE desc:
typedef union {
    struct {
        uint64_t               entry0  : 16; // Partition Key table entry 0.
        uint64_t               entry1  : 16; // Partition Key table entry 1.
        uint64_t               entry2  : 16; // Partition Key table entry 2.
        uint64_t               entry3  : 16; // Partition Key table entry 3.
    } field;
    uint64_t val;
} TP_CFG_PKEY_TABLE_t;

// TP_CFG_PKEY_CHECK_CTRL desc:
typedef union {
    struct {
        uint64_t              pkey_8b  : 16; // The Pkey Check Control register is ignored for STL 9B and 16B packets. STL 8B: Packet Pkey = {Pkey_Check_8B[15:0]}
        uint64_t         UNUSED_19_16  :  4; // 
        uint64_t             pkey_10b  : 12; // The Pkey Check Control register is ignored for STL 9B and 16B packets. STL 10B: Packet Pkey = {Pkey_Check_10B[31:20],Pkt:Pkey[3:0]}
        uint64_t         UNUSED_63_32  : 32; // 
    } field;
    uint64_t val;
} TP_CFG_PKEY_CHECK_CTRL_t;

// TP_ERR_STS_0 desc:
typedef union {
    struct {
        uint64_t         pkey_discard  :  1; // Tx Pkey enable =1 and - Tx Pkey fail, discarded packet
        uint64_t        tport_pkt_ebp  :  1; // TPORT received packet not marked EBP and marked it EBP. - Error due to MBE, parity, or timeout. OR; TPORT received packet with a length error and marked it EBP. - Sets whether or not the packet from the crossbar was marked EBP.
        uint64_t        subsw_pkt_ebp  :  1; // Tport received a packet from the Subsw marked EBP.
        uint64_t            short_pkt  :  1; // TPORT received a packet that was shorter than the length in the associated tag. -Packet was marked with an EBP and the remaining credits were returned to the credit manager.
        uint64_t             long_pkt  :  1; // TPORT received a packet that was longer than the length in the associated tag. Packet was clipped and marked with an EBP.
        uint64_t        short_grh_pkt  :  1; // TPORT received a GRH packet that was shorter than 8 Bytes but the length field in the packet matched the packet length. Packet was marked with an EBP.
        uint64_t         tailless_pkt  :  1; // Will a packet was in progress, Tport received a head of another packet before the tail of the currecnt packet.
        uint64_t         headless_pkt  :  1; // Tport received a body of a packet and while the packet was not in progress, meaning there was a body before a head.
        uint64_t        drop_data_pkt  :  1; // Non-VL15 Packet was dropped, while in flight to OC because link was not in Active/Send state.
        uint64_t            mtu_error  :  1; // MTU error - drop packet Bit set indicates error was detected.
        uint64_t            vlt_error  :  1; // Packet received on a non-operational VLt, operational VLt's are set in Misc Control Registers . - drop packet Bit set indicates error was detected.
        uint64_t          invalid_vlt  :  1; // Packet SC was mapped to an invalid VLt, only 8:0 are valid - drop packet Bit set indicates error was detected.
        uint64_t             l2_error  :  1; // A packet with an L2 format not supported by neighbor. - drop packet Bit set indicates error was detected.
        uint64_t            congested  :  1; // TPORT entered a VL congestion state and a least 1 packet was marked.
        uint64_t         UNUSED_63_14  : 50; // 
    } field;
    uint64_t val;
} TP_ERR_STS_0_t;

// TP_ERR_CLR_0 desc:
typedef union {
    struct {
        uint64_t                value  : 14; // Set to a 1 to clear interrupt
        uint64_t         UNUSED_63_14  : 50; // 
    } field;
    uint64_t val;
} TP_ERR_CLR_0_t;

// TP_ERR_FRC_0 desc:
typedef union {
    struct {
        uint64_t                value  : 14; // Set to a 1 to force an interrupt
        uint64_t         UNUSED_63_14  : 50; // 
    } field;
    uint64_t val;
} TP_ERR_FRC_0_t;

// TP_ERR_EN_HOST_0 desc:
typedef union {
    struct {
        uint64_t                value  : 14; // Set to a 1 to enable interrupt
        uint64_t         UNUSED_63_14  : 50; // 
    } field;
    uint64_t val;
} TP_ERR_EN_HOST_0_t;

// TP_ERR_FIRST_HOST_0 desc:
typedef union {
    struct {
        uint64_t                value  : 14; // When set, this error was the first interrupt
        uint64_t         UNUSED_63_14  : 50; // 
    } field;
    uint64_t val;
} TP_ERR_FIRST_HOST_0_t;

// TP_ERR_STS_1 desc:
typedef union {
    struct {
        uint64_t           request_to  :  1; // Tport packet timeout (wait>Request TO reg) detected. A packet is in progress from the Subsw but the tail has not been received, detect timeout when wait > Request_TO reg. - Insert EBP
        uint64_t            subsw_sbe  :  1; // Tport received a packet from the subsw with ECC error detected an SBE (Includes all VLs and CBUF's) - corrected error
        uint64_t            subsw_mbe  :  1; // Tport received a packet from the subsw with ECC error detected an SBE (Includes all VLs and CBUF's) Head -discard entire packet Body - insert bad tail flit, discard flits up to and including next good tail or next good head. Tail - insert bad tail flit
        uint64_t             subsw_pe  :  1; // Tport received sideband information from the subsw with parity error (Includes all VLs and CBUF's) Head -discard entire packet Body - insert bad tail flit, discard flits up to and including next good tail or next good head. Tail - insert bad tail flit
        uint64_t           rinfoq_sbe  :  1; // Rinfo FIFO SBE - corrected error
        uint64_t           rinfoq_mbe  :  1; // Rinfo FIFO mbe Body - insert bad tail flit, discard flits up to and including next good tail or next good head. Tail - insert bad tail flit
        uint64_t     rinfoq_underflow  :  1; // TPORT route info queue underflow. - should never happen Note: If rinfoq_overflow is also set there are no EOP flits buffer.
        uint64_t      rinfoq_overflow  :  1; // TPORT route info queue overflow. - SOP word: discard packet - Pkt Data, EOP word: terminate packet with EBP Note: If rinfoq_overflow is also set there are no EOP flits buffer
        uint64_t     rinfoq_spill_sbe  :  1; // Rinfo Spill FIFO SBE - corrected error
        uint64_t     rinfoq_spill_mbe  :  1; // Rinfo Spill FIFO mbe Body - insert bad tail flit, discard flits up to and including next good tail or next good head. Tail - insert bad tail flit
        uint64_t rinfoq_spill_underflow  :  1; // TPORT route info queue underflow. - should never happen Note: If rinfoq_overflow is also set there are no EOP flits buffer.
        uint64_t rinfoq_spill_overflow  :  1; // TPORT route info queue overflow. - SOP word: discard packet - Pkt Data, EOP word: terminate packet with EBP Note: If rinfoq_overflow is also set there are no EOP flits buffer
        uint64_t      dataq_underflow  :  1; // TPORT data queue underflow. - should never happen Note: If dataq_overflow is also set there are no EOP flits buffer.
        uint64_t       dataq_overflow  :  1; // TPORT data queue overflow. TBD, probably just discard flit --- - SOP word: discard packet - Pkt Data, EOP word: terminate packet with EBP Note: If dataq_overflow is also set there are no EOP flits buffer
        uint64_t dataq_spill_underflow  :  1; // TPORT route info queue overflow. - should never happen Note: If dataq_overflow is also set there are no EOP flits buffer.
        uint64_t dataq_spill_overflow  :  1; // TPORT Spill data queue overflow. T- SOP word: discard packet - Pkt Data, EOP word: terminate packet with EBP Note: If dataq_overflow is also set there are no EOP flits buffer
        uint64_t         csr_addr_err  :  1; // Tport received a CSR access to an invalid address or a write to an address that contains only Read-Only CSRs.
        uint64_t             lb_wr_pe  :  1; // Local bus parity error detected on write data from CPORT.
        uint64_t      credit_overflow  :  1; // STL mode: Detected a condition where the Transmitter's Consumed credit accounting exceeded programmed max. VL15, VL7:0 dedicated+shared > max credits at remote RBUF *** this should not occur.
        uint64_t  oc_credit_underflow  :  1; // OC flow control counter underflow. Counter was at 0 and an ack was detected due to an ack from OC or a squashed flit.
        uint64_t          hoq_discard  : 10; // TPORT detected a Head of VL15, VL8:0 Queue timeout. bits 29:20 = VL15,VL8:0 - Timed out packet tag generates a packet drop request. Bit set indicates error was detected.
        uint64_t          sll_timeout  : 10; // TPORT detected a VL15, VL8:0 Switch Lifetime Limit timeout. bits 39:30 = VL15,VL8:0 - Timed out packet tag generates a packet drop request. Bit set indicates error was detected.
        uint64_t       perf_cntr_perr  :  1; // A performance counter parity error
        uint64_t   perf_cntr_rollover  :  1; // A performance counter rolled over
        uint64_t crdt_rtrn_illegal_vl  :  1; // VL written into the Credit Return Table is not in range. Legal range is 15,8:0.
        uint64_t    crdt_rtrn_par_err  :  1; // Credit return block detected a parity error on the credit flit acknowledge bus.
        uint64_t     crdt_rtrn_vl_err  :  1; // Credit return block detected an illegal vl pointer on credit return from remote device.
        uint64_t         UNUSED_63_45  : 19; // 
    } field;
    uint64_t val;
} TP_ERR_STS_1_t;

// TP_ERR_CLR_1 desc:
typedef union {
    struct {
        uint64_t                value  : 45; // Set to a 1 to clear interrupt
        uint64_t         UNUSED_63_45  : 19; // 
    } field;
    uint64_t val;
} TP_ERR_CLR_1_t;

// TP_ERR_FRC_1 desc:
typedef union {
    struct {
        uint64_t                value  : 45; // Set to a 1 to force an interrupt
        uint64_t         UNUSED_63_45  : 19; // 
    } field;
    uint64_t val;
} TP_ERR_FRC_1_t;

// TP_ERR_EN_HOST_1 desc:
typedef union {
    struct {
        uint64_t                value  : 45; // Set to a 1 to enable interrupt
        uint64_t         UNUSED_63_45  : 19; // 
    } field;
    uint64_t val;
} TP_ERR_EN_HOST_1_t;

// TP_ERR_FIRST_HOST_1 desc:
typedef union {
    struct {
        uint64_t                value  : 45; // When set, this error was the first interrupt
        uint64_t         UNUSED_63_45  : 19; // 
    } field;
    uint64_t val;
} TP_ERR_FIRST_HOST_1_t;

// TP_ERR_ERROR_INFO desc:
typedef union {
    struct {
        uint64_t        cb0_pkt_error  :  1; // Indicated that colbus 0 dectected a packet error
        uint64_t       cb0_data_error  :  1; // Indicated that colbus 0 dectected a data error
        uint64_t         cb0_error_vl  : 10; // Vls {15,8:0} of colbus 0 in which either error was detected, does not include subsw parity errors.
        uint64_t         UNUSED_15_12  :  4; // 
        uint64_t        cb1_pkt_error  :  1; // Indicated that colbus 1 dectected a packet error
        uint64_t       cb1_data_error  :  1; // Indicated that colbus 1 dectected a data error
        uint64_t         cb1_error_vl  : 10; // Vls {15,8:0} of colbus 1 in which either error was detected, does not include subsw parity errors.
        uint64_t         UNUSED_31_28  :  4; // 
        uint64_t        cb2_pkt_error  :  1; // Indicated that colbus 2 dectected a packet error
        uint64_t       cb2_data_error  :  1; // Indicated that colbus 2 dectected a data error
        uint64_t         cb2_error_vl  : 10; // Vls {15,8:0} of colbus 2 in which either error was detected, does not include subsw parity errors.
        uint64_t         UNUSED_47_44  :  4; // 
        uint64_t        cb3_pkt_error  :  1; // Indicated that colbus 3 dectected a packet error
        uint64_t       cb3_data_error  :  1; // Indicated that colbus 3 dectected a data error
        uint64_t         cb3_error_vl  : 10; // Vls {15,8:0} of colbus 3 in which either error was detected, does not include subsw parity errors.
        uint64_t         UNUSED_63_60  :  4; // 
    } field;
    uint64_t val;
} TP_ERR_ERROR_INFO_t;

// TP_ERR_PKEY_ERROR_INFO desc:
typedef union {
    struct {
        uint64_t        tx_first_slid  : 20; // Captured packet SLID from first Pkey discard.
        uint64_t        tx_first_pkey  : 16; // Captured packet Pkey from first Pkey discard.
        uint64_t         UNUSED_63_36  : 28; // 
    } field;
    uint64_t val;
} TP_ERR_PKEY_ERROR_INFO_t;

// TP_ERR_SBE_ERROR_CNT desc:
typedef union {
    struct {
        uint64_t                count  : 16; // Total number of SBE errors
        uint64_t                 synd  :  8; // First bad syndrome
        uint64_t         UNUSED_63_24  : 40; // 
    } field;
    uint64_t val;
} TP_ERR_SBE_ERROR_CNT_t;

// TP_ERR_MBE_ERROR_CNT desc:
typedef union {
    struct {
        uint64_t                count  : 16; // Total number of MBE errors
        uint64_t                 synd  :  8; // First bad syndrome
        uint64_t         UNUSED_63_24  : 40; // 
    } field;
    uint64_t val;
} TP_ERR_MBE_ERROR_CNT_t;

// TP_ERR_PE_ERROR_CNT desc:
typedef union {
    struct {
        uint64_t                count  : 16; // Total number of parity errors
        uint64_t         UNUSED_63_16  : 48; // 
    } field;
    uint64_t val;
} TP_ERR_PE_ERROR_CNT_t;

// TP_STS_STATE desc:
typedef union {
    struct {
        uint64_t        vlt_hoq_valid  : 10; // VLt 15,8:0 have a request at the head of the MLRU arbitration queue. bit 9 = VL15 has a request at the head of queue bit 8:0 = VL8:0 has a request at the head of queue
        uint64_t vlt_arb_waiting_for_cr  : 10; // VL Tag arbitration waiting for remote credit. bit 9 = VL15 tag waiting for arbitration bit 8:0 = VL8:0 tag waiting for arbitration 0 = Packet not waiting for arbitration (no pkt or pkt and credit) 1 = tag waiting for arbitration (pkt and no credit)
        uint64_t   all_Requests_in_oc  :  1; // TPORT Request State 0 = TPORT has an outstanding request 1 = All Tag requests and subsequent data has been sent to Otter Creek
        uint64_t     interleave_depth  :  5; // TPORT is currently in a packet interleave mode. 0 = interleaving not active 9:1= current number of levels of interleaving active
        uint64_t            ssw_vl_ip  : 10; // VLp 15,8:0 has a packet in progress. All 4 colbus's are OR'd together.
        uint64_t               ssw_ip  :  4; // Colbus 3:0 has a packet in progress
        uint64_t      hoq_stall_state  : 10; // Tagbuf VL15, VL8:0 Head-Of-Queue stall discard state. bit 19 = VL15 Head-Of-Queue stall state bit 18:10 = VL7:0 Head-Of-Queue stall state 0 = not in stall discard state 1 = in stall discard state
        uint64_t           link_state  :  3; // Reflects the value of Port link state: 0x0 = Reserved 0x1 = Down 0x2 = Initialize 0x3 = Armed 0x4 = Active 0x5-0x6 =Reserved 0x7 = ForceSend (Arm & RemoteArm & SendInArm)
        uint64_t         UNUSED_63_53  : 11; // 
    } field;
    uint64_t val;
} TP_STS_STATE_t;

// TP_DBG_LMON_MUX_SEL desc:
typedef union {
    struct {
        uint64_t             mux_sel0  :  4; // Mux select data path 0
        uint64_t             mux_sel1  :  4; // Mux select data path 1
        uint64_t          UNUSED_63_8  : 56; // 
    } field;
    uint64_t val;
} TP_DBG_LMON_MUX_SEL_t;

// TP_DBG_ERROR_INJECTION desc:
typedef union {
    struct {
        uint64_t                armed  :  1; // Arm Error injection. Injects 1 error at the first opportunity. If this clears then a trigger occurred.
        uint64_t               colbus  :  2; // Selects column bus in which the error is injected
        uint64_t             err_type  :  1; // Selects the block in which the error is injected 0 - subsw data 1 - subsw rinfo parity error (does not used check_byte)
        uint64_t            flit_type  :  2; // Selects the flit type where the error is injected 0 - Head 1 - Body 2 - Tail 3 - Reserved
        uint64_t           UNUSED_7_6  :  2; // 
        uint64_t          check_byte0  :  8; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome of flit 0 to be inverted.
        uint64_t          check_byte1  :  8; // When an error is injected, each bit that is set to one in this field causes the corresponding bit of the error detection syndrome of flit 1 to be inverted
        uint64_t         UNUSED_63_24  : 40; // 
    } field;
    uint64_t val;
} TP_DBG_ERROR_INJECTION_t;

// TP_CFG_PERF_CTRL desc:
typedef union {
    struct {
        uint64_t         enable_cntrs  :  1; // Enables the event counter block.
        uint64_t            clr_cntrs  :  1; // When writing a '1' to this registers, a single pulse is generated and a '0' is written back to the register. All the counters associated with the event counter block are cleared.
        uint64_t       clr_cntrs_comp  :  1; // When = 0, the event counters are in the process of being cleared. When = 1, the clear sequence has completed.
        uint64_t      parity_err_addr  :  7; // Memory address where the parity error occurred.
        uint64_t          errinj_addr  :  7; // Ram address to inject error on. 0x0 - 0x7F
        uint64_t        errinj_parity  :  4; // Parity mask.
        uint64_t          errinj_mode  :  2; // Inject error once, 0x1- 0x3 reserved
        uint64_t            errinj_en  :  1; // Error injection enabled.
        uint64_t         errinj_reset  :  1; // Resets error injection control logic. When written to a '1' a single pulse is generated and a '0' is written back to the register.This needs to be asserted between errors injected.
        uint64_t     errinj_triggered  :  1; // Error was injected.
        uint64_t         UNUSED_63_26  : 38; // 
    } field;
    uint64_t val;
} TP_CFG_PERF_CTRL_t;

// TP_PRF_COUNTERS desc:
typedef union {
    struct {
        uint64_t                  cnt  : 64; // 
    } field;
    uint64_t val;
} TP_PRF_COUNTERS_t;

#endif /* ___FXR_lm_tp_CSRS_H__ */