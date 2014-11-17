#!/usr/bin/python

# Test MpiTest
# Author: Kyle Liddell (kyle.liddell@intel.com)
#

import RegLib
import time
import sys
import re
import os
import time

c_test_list = [ "MPI_Address_c",
                "MPI_Allgather_c",
                "MPI_Allgatherv_c",
                "MPI_Allreduce_c",
                "MPI_Allreduce_loc_c",
                "MPI_Allreduce_user_c",
                "MPI_Alltoall_c",
                "MPI_Alltoallv_c",
                "MPI_Barrier_c",
                "MPI_Bcast_c",
                "MPI_Bsend_ator_c",
                "MPI_Bsend_init_null_c",
                "MPI_Bsend_init_overtake_c",
                "MPI_Bsend_init_rtoa_c",
                "MPI_Bsend_null_c",
                "MPI_Bsend_overtake_c",
                "MPI_Bsend_rtoa_c",
                "MPI_Bsend_rtoaq_c",
                "MPI_Cancel_irecv_c",
                "MPI_Cancel_persist_recv_c",
                "MPI_Cart_coords_c",
                "MPI_Cart_create_nonperiodic_c",
                "MPI_Cart_create_periodic_c",
                "MPI_Cart_create_undef_c",
                "MPI_Cart_get_c",
                "MPI_Cart_map_c",
                "MPI_Cart_rank_c",
                "MPI_Cart_shift_nonperiodic_c",
                "MPI_Cart_shift_periodic_c",
                "MPI_Cart_sub_c",
                "MPI_Cartdim_get_c",
                "MPI_Comm_compare_c",
                "MPI_Comm_compare_null_c",
                "MPI_Comm_create_c",
                "MPI_Comm_dup_c",
                "MPI_Comm_group_c",
                "MPI_Comm_split1_c",
                "MPI_Comm_split2_c",
                "MPI_Comm_split3_c",
                "MPI_Comm_split4_c",
                "MPI_Dims_create_system_c",
                "MPI_Dims_create_user_c",
                "MPI_Errhandler_fatal_c",
                "MPI_Errhandler_free_c",
                "MPI_Errhandler_get_c",
                "MPI_Errhandler_set_c",
                "MPI_Error_string_c",
                "MPI_Finalize_c",
                "MPI_Gather_c",
                "MPI_Gatherv_c",
                "MPI_Get_elements_basic_type_c",
                "MPI_Get_processor_name_c",
                "MPI_Graph_create_noreorder_c",
                "MPI_Graph_create_reorder_c",
                "MPI_Graph_create_undef_c",
                "MPI_Graph_get_c",
                "MPI_Graph_map_c",
                "MPI_Graph_neighbors_c",
                "MPI_Graph_neighbors_count_c",
                "MPI_Graphdims_get_c",
                "MPI_Group_compare_c",
                "MPI_Group_difference1_c",
                "MPI_Group_difference2_c",
                "MPI_Group_difference3_c",
                "MPI_Group_excl1_c",
                "MPI_Group_excl2_c",
                "MPI_Group_incl1_c",
                "MPI_Group_incl2_c",
                "MPI_Group_intersection1_c",
                "MPI_Group_intersection2_c",
                "MPI_Group_intersection3_c",
                "MPI_Group_range_excl1_c",
                "MPI_Group_range_excl2_c",
                "MPI_Group_range_excl3_c",
                "MPI_Group_range_incl1_c",
                "MPI_Group_range_incl2_c",
                "MPI_Group_range_incl3_c",
                "MPI_Group_trans_ranks_c",
                "MPI_Group_union1_c",
                "MPI_Group_union2_c",
                "MPI_Group_union3_c",
                "MPI_Ibsend_ator_c",
                "MPI_Ibsend_null_c",
                "MPI_Ibsend_overtake_c",
                "MPI_Ibsend_rtoa_c",
                "MPI_Ibsend_rtoaq_c",
                "MPI_Init_attr_c",
                "MPI_Initialized_c",
                "MPI_Intercomm_create1_c",
                "MPI_Intercomm_create2_c",
                "MPI_Intercomm_merge1_c",
                "MPI_Intercomm_merge2_c",
                "MPI_Intercomm_merge3_c",
                "MPI_Iprobe_return_c",
                "MPI_Iprobe_source_c",
                "MPI_Iprobe_tag_c",
                "MPI_Irecv_comm_c",
                "MPI_Irecv_null_c",
                "MPI_Irecv_pack_c",
                "MPI_Irsend_null_c",
                "MPI_Irsend_rtoa_c",
                "MPI_Isend_ator2_c",
                "MPI_Isend_ator_c",
                "MPI_Isend_fairness_c",
                "MPI_Isend_flood_c",
                "MPI_Isend_null_c",
                "MPI_Isend_off_c",
                "MPI_Isend_overtake2_c",
                "MPI_Isend_overtake_c",
                "MPI_Isend_rtoa_c",
                "MPI_Isend_self_c",
                "MPI_Issend_ator_c",
                "MPI_Issend_null_c",
                "MPI_Issend_overtake1_c",
                "MPI_Issend_overtake2_c",
                "MPI_Issend_overtake3_c",
                "MPI_Issend_rtoa_c",
                "MPI_Keyval1_c",
                "MPI_Keyval2_c",
                "MPI_Keyval3_c",
                "MPI_Pack_basic_type_c",
                "MPI_Pack_displs_c",
                "MPI_Pack_size_basic_c",
                "MPI_Pack_size_types_c",
                "MPI_Pack_user_type_c",
                "MPI_Pcontrol_c",
                "MPI_Probe_source_c",
                "MPI_Probe_tag_c",
                "MPI_Recv_comm_c",
                "MPI_Recv_init_comm_c",
                "MPI_Recv_init_null_c",
                "MPI_Recv_init_pack_c",
                "MPI_Recv_null_c",
                "MPI_Recv_pack_c",
                "MPI_Reduce_c",
                "MPI_Reduce_loc_c",
                "MPI_Reduce_scatter_c",
                "MPI_Reduce_scatter_loc_c",
                "MPI_Reduce_scatter_user_c",
                "MPI_Reduce_user_c",
                "MPI_Request_free_c",
                "MPI_Request_free_p_c",
                "MPI_Rsend_init_null_c",
                "MPI_Rsend_init_rtoa_c",
                "MPI_Rsend_null_c",
                "MPI_Rsend_rtoa_c",
                "MPI_Scan_c",
                "MPI_Scan_loc_c",
                "MPI_Scan_user_c",
                "MPI_Scatter_c",
                "MPI_Scatterv_c",
                "MPI_Send_ator2_c",
                "MPI_Send_ator_c",
                "MPI_Send_fairness_c",
                "MPI_Send_flood_c",
                "MPI_Send_init_ator_c",
                "MPI_Send_init_null_c",
                "MPI_Send_init_off_c",
                "MPI_Send_init_overtake2_c",
                "MPI_Send_init_overtake_c",
                "MPI_Send_init_rtoa_c",
                "MPI_Send_null_c",
                "MPI_Send_off_c",
                "MPI_Send_overtake_c",
                "MPI_Send_rtoa_c",
                "MPI_Sendrecv_null_c",
                "MPI_Sendrecv_replace_null_c",
                "MPI_Sendrecv_replace_ring_c",
                "MPI_Sendrecv_replace_rtoa_c",
                "MPI_Sendrecv_replace_self_c",
                "MPI_Sendrecv_ring_c",
                "MPI_Sendrecv_rtoa_c",
                "MPI_Sendrecv_self_c",
                "MPI_Ssend_ator_c",
                "MPI_Ssend_init_ator_c",
                "MPI_Ssend_init_null_c",
                "MPI_Ssend_init_overtake_c",
                "MPI_Ssend_init_rtoa_c",
                "MPI_Ssend_null_c",
                "MPI_Ssend_overtake_c",
                "MPI_Ssend_rtoa_c",
                "MPI_Startall1_c",
                "MPI_Startall2_c",
                "MPI_Test_c",
                "MPI_Test_cancelled_false_c",
                "MPI_Test_p_c",
                "MPI_Testall_c",
                "MPI_Testall_p_c",
                "MPI_Testany_c",
                "MPI_Testany_p_c",
                "MPI_Testsome_c",
                "MPI_Testsome_p_c",
                "MPI_Topo_test_cart_c",
                "MPI_Topo_test_graph_c",
                "MPI_Type_contiguous_basic_c",
                "MPI_Type_contiguous_idispls_c",
                "MPI_Type_contiguous_types_c",
                "MPI_Type_extent_MPI_LB_UB_c",
                "MPI_Type_extent_types_c",
                "MPI_Type_free_pending_msg_c",
                "MPI_Type_free_types_c",
                "MPI_Type_hindexed_basic_c",
                "MPI_Type_hindexed_blklen_c",
                "MPI_Type_hindexed_displs_c",
                "MPI_Type_hindexed_types_c",
                "MPI_Type_hvector_basic_c",
                "MPI_Type_hvector_blklen_c",
                "MPI_Type_hvector_stride_c",
                "MPI_Type_hvector_types_c",
                "MPI_Type_indexed_basic_c",
                "MPI_Type_indexed_blklen_c",
                "MPI_Type_indexed_displs_c",
                "MPI_Type_indexed_types_c",
                "MPI_Type_size_MPI_LB_UB_c",
                "MPI_Type_size_basic_c",
                "MPI_Type_size_types_c",
                "MPI_Type_struct_basic_c",
                "MPI_Type_struct_blklen_c",
                "MPI_Type_struct_displs_c",
                "MPI_Type_struct_types_c",
                "MPI_Type_ub_MPI_UB_c",
                "MPI_Type_ub_neg_displ_c",
                "MPI_Type_ub_pos_displ_c",
                "MPI_Type_vector_basic_c",
                "MPI_Type_vector_blklen_c",
                "MPI_Type_vector_stride_c",
                "MPI_Type_vector_types_c",
                "MPI_Waitall_c",
                "MPI_Waitall_p_c",
                "MPI_Waitany_c",
                "MPI_Waitany_p_c",
                "MPI_Waitsome_c",
                "MPI_Waitsome_p_c",
                "MPI_Wtime_c",
                "MPI_collective_message_c",
                "MPI_collective_overlap_c",
                "MPI_defs_c",
                "MPI_pdefs_c"
            ]

def do_ssh(host, cmd):
    RegLib.test_log(5, "Running " + cmd)
    return (host.send_ssh(cmd, 0))

def main():

    #############################
    # create a test info object #
    #############################
    test_info = RegLib.TestInfo()

    RegLib.test_log(0, "Test: MpiTest.py started")
    RegLib.test_log(0, "Dumping test parameters")

    print test_info

    host_count = test_info.get_np()
    host1 = test_info.get_host_record(0)

    print "Found ", host_count, "hosts setting -np for mpirun accordingly"


    extra_args = test_info.parse_extra_args()
    arg_str = str(extra_args).strip('[]').strip('\'')

    ################
    # body of test #
    ################

    if test_info.is_mpiverbs():
        psm_libs = False
    else:
        psm_libs = test_info.get_psm_lib()
        if psm_libs:
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
            base_dir = "/usr/mpi/gcc/openmpi-1.8.2a1/tests/intel_mpitest/"
        else:
            base_dir = "/usr/mpi/gcc/openmpi-1.8.2a1-hfi/tests/intel_mpitest/"
        print "Using default base_dir of", base_dir
    else:
        print "Using user defined base_dir of", base_dir

    benchmark = base_dir

    host_list = test_info.get_host_list()
    hosts = ",".join(host_list)

    RegLib.test_log(0, "Starting MPI Test benchmark " + benchmark)
    cmd_base = "export LD_LIBRARY_PATH=" + test_info.get_mpi_lib_path()
    if psm_libs:
        cmd_base = cmd_base + ":" + psm_libs
    cmd_base = cmd_base + ";"
    cmd_base = cmd_base + " " +  test_info.get_mpi_bin_path() + "/mpirun"
    cmd_base = cmd_base + test_info.get_mpi_opts()
    cmd_base = cmd_base + " LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
    cmd_base = cmd_base + " -np " + str(host_count)
    cmd_base = cmd_base + " -H " + hosts

    cmd_base = cmd_base + " " + base_dir

    # test MPI_Abort_c separately, since it returns an error if it succeeds
    prog = "MPI_Abort_c"
    cmd = cmd_base + prog
    RegLib.test_log(5, "MPI CMD: " + cmd)

    err = do_ssh(host1, cmd)
    if err != 1:
        RegLib.test_fail("Failed!")

    for prog in c_test_list:
        cmd = cmd_base + prog
        RegLib.test_log(5, "MPI CMD: " + cmd)

        err = do_ssh(host1, cmd)
        if err:
            RegLib.test_fail("Failed!")

    RegLib.test_pass("All MPITest tests passed")

if __name__ == "__main__":
    main()

