#ifndef __IOMMU_CR_TOP_REGS__
#define __IOMMU_CR_TOP_REGS__

//                                                                             
// File:       iommu_cr_top_regs.h                                             
// Creator:    birelan1                                                        
// Time:       Friday Jan 30, 2015 [11:13:34 am]                               
//                                                                             
// Path:       /nfs/pdx/disks/ccdo.val.work.325/valid/work/iommu_work/clones/iommu20-tb_integration/target/iommu_fxr/aceroot/source/registers/run_dir/iommu_cr_top/nebulon_run/942
// Arguments:  /p/hdk/rtl/cad/x86-64_linux26/denali/blueprint/3.7.4/Linux/blueprint
//             -chdr -generic_generator -creg -I                               
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/include -I   
//             /nfs/pdx/disks/ccdo.val.work.325/valid/work/iommu_work/clones/iommu20-tb_integration/tools/srdl/common
//             -ovm -xml iommu_cr_top.rdl                                      
//                                                                             
// Sources:    /nfs/pdx/disks/ccdo.val.work.325/valid/work/iommu_work/clones/iommu20-tb_integration/cfg/MacroRtlGen.pm:906ce75de0de479d6fb31ea7946cd71e
//             /nfs/pdx/disks/ccdo.val.work.325/valid/work/iommu_work/clones/iommu20-tb_integration/tools/srdl/FXR/iommu_cr_top.rdl:9018fff5134c671dad0e4e23ed906e71
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/GeneratorEngine.pm:c6fcb207743339c094bb1261ee396659
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/chdr.pm:97c681267db0fbfff5a3ce13e3a832fe
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/creg.pm:1276b999aca898a72f0055e9924a3505
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/generator_common.pm:4973c402c43f97e18c4aba02585df570
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/generic_generator.pm:16ebe8f2fa2b5115a491220f60f876b2
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/ovm.pm:e21c7be001887ff41e17c43b43b1fcdc
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/walk_through.pm:86490f30544c8b90497ebfee2e989cd3
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/xml.pm:c933da0fbf8f76dc4dab574099e5e4cf
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/generators/xml_file_util.pm:ff01fe4e53c689a1e04cefe3e42728f6
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/include/lib_udp.rdl:34d6f45292e06a11e6a71125356fa028
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/perllib/AddrMap.pm:d3b83656de27d270119d9a6c1dc8ee25
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/perllib/BaseCRType.pm:5b1f0e5793d36f373766e6dcad8aecaf
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/perllib/Field.pm:cc2e2da35544025b158ef57613fd151a
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/perllib/Interface.pm:b27173ab2791cb475f780a4b24805c50
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/perllib/RegInterface.pm:ca2efea6aad2b2751ebab456a10a8408
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/perllib/Register.pm:b2bd54bec254574701d01662a97ae685
//             /p/hdk/rtl/cad/x86-64_linux26/intel/nebulon/2.07p2/perllib/Utils.pm:d6b725cf4b798bcc9e83a15e369b291a
//             /nfs/pdx/disks/ccdo.val.work.325/valid/work/iommu_work/clones/iommu20-tb_integration/target/iommu_fxr/aceroot/source/registers/run_dir/iommu_cr_top/nebulon_run/942/security.pm:a567b26ea10cff6e2a971a4ee7f711cb
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/Digest/base.pm:54bebd0439900c1d44d212cba39747b5
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/Env.pm:6dec9d75987d3067a4cdd7a6e44e21dd
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/FindBin.pm:c4dce089f3a8b0dfe95b672ad5c8855e
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/Math/BigInt.pm:fdb5563799a02f5326f9b46672bf987f
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/Math/BigInt/Calc.pm:bae26ce0234167c6a48596e2ca28280f
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/Tie/Array.pm:d269749e14ac76e9a62d11f1ff1c3f5e
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/Tie/Hash.pm:48ff7bdd919a5322e501f9364bfc9bb5
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/bigint.pm:6651fae4c5e96a1985bf61aba280eaa1
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/constant.pm:6440054c09eae254e0e64bfb327f2250
//             /usr/intel/pkgs/perl/5.8.7/lib64/5.8.7/integer.pm:4a9b2a5bdd05b8f12371f0bd13135b95
//             /usr/intel/pkgs/perl/5.8.7/lib64/site_perl/Data/Dump.pm:a6b6352d1a1c0e4214ce030f97323e7a
//             /usr/intel/pkgs/perl/5.8.7/lib64/site_perl/Devel/StackTrace.pm:3b99e973435e2dea00aa3a8d6f1b9ddd
//             /usr/intel/pkgs/perl/5.8.7/lib64/site_perl/Tie/IxHash.pm:005b42b66e99dd12132c3a6f6d40803e
//                                                                             
// Blueprint:   3.7.4 (Tue Jun 23 00:17:01 PDT 2009)                           
// Machine:    plxc17659                                                       
// OS:         Linux 2.6.16.60-0.58.1.3239.0.PTF.638363-smp                    
// Description:                                                                
//                                                                             
// No Description Provided                                                     
//                                                                             
// Copyright (C) 2015 Intel Corp. All rights reserved                          
// THIS FILE IS AUTOMATICALLY GENERATED BY DENALI BLUEPRINT, DO NOT EDIT       
//                                                                             


#define IOMMU_REGS_SB_MSGPORT     0x0
#define IOMMU_REGS_SB_VER_REG_0_0_0_VTDBAR_MSGREGADDR 0x0
#define IOMMU_REGS_SB_CAP_REG_0_0_0_VTDBAR_MSGREGADDR 0x8
#define IOMMU_REGS_SB_ECAP_REG_0_0_0_VTDBAR_MSGREGADDR 0x10
#define IOMMU_REGS_SB_GCMD_REG_0_0_0_VTDBAR_MSGREGADDR 0x18
#define IOMMU_REGS_SB_GSTS_REG_0_0_0_VTDBAR_MSGREGADDR 0x1C
#define IOMMU_REGS_SB_RTADDR_REG_0_0_0_VTDBAR_MSGREGADDR 0x20
#define IOMMU_REGS_SB_CCMD_REG_0_0_0_VTDBAR_MSGREGADDR 0x28
#define IOMMU_REGS_SB_FSTS_REG_0_0_0_VTDBAR_MSGREGADDR 0x34
#define IOMMU_REGS_SB_FECTL_REG_0_0_0_VTDBAR_MSGREGADDR 0x38
#define IOMMU_REGS_SB_FEDATA_REG_0_0_0_VTDBAR_MSGREGADDR 0x3C
#define IOMMU_REGS_SB_FEADDR_REG_0_0_0_VTDBAR_MSGREGADDR 0x40
#define IOMMU_REGS_SB_FEUADDR_REG_0_0_0_VTDBAR_MSGREGADDR 0x44
#define IOMMU_REGS_SB_AFLOG_REG_0_0_0_VTDBAR_MSGREGADDR 0x58
#define IOMMU_REGS_SB_PMEN_REG_0_0_0_VTDBAR_MSGREGADDR 0x64
#define IOMMU_REGS_SB_PLMBASE_REG_0_0_0_VTDBAR_MSGREGADDR 0x68
#define IOMMU_REGS_SB_PLMLIMIT_REG_0_0_0_VTDBAR_MSGREGADDR 0x6C
#define IOMMU_REGS_SB_PHMBASE_REG_0_0_0_VTDBAR_MSGREGADDR 0x70
#define IOMMU_REGS_SB_PHMLIMIT_REG_0_0_0_VTDBAR_MSGREGADDR 0x78
#define IOMMU_REGS_SB_IQH_REG_0_0_0_VTDBAR_MSGREGADDR 0x80
#define IOMMU_REGS_SB_IQT_REG_0_0_0_VTDBAR_MSGREGADDR 0x88
#define IOMMU_REGS_SB_IQA_REG_0_0_0_VTDBAR_MSGREGADDR 0x90
#define IOMMU_REGS_SB_ICS_REG_0_0_0_VTDBAR_MSGREGADDR 0x9C
#define IOMMU_REGS_SB_IECTL_REG_0_0_0_VTDBAR_MSGREGADDR 0x0A0
#define IOMMU_REGS_SB_IEDATA_REG_0_0_0_VTDBAR_MSGREGADDR 0x0A4
#define IOMMU_REGS_SB_IEADDR_REG_0_0_0_VTDBAR_MSGREGADDR 0x0A8
#define IOMMU_REGS_SB_IEUADDR_REG_0_0_0_VTDBAR_MSGREGADDR 0x0AC
#define IOMMU_REGS_SB_IRTA_REG_0_0_0_VTDBAR_MSGREGADDR 0x0B8
#define IOMMU_REGS_SB_PRESTS_REG_0_0_0_VTDBAR_MSGREGADDR 0x0DC
#define IOMMU_REGS_SB_PRECTL_REG_0_0_0_VTDBAR_MSGREGADDR 0x0E0
#define IOMMU_REGS_SB_PREDATA_REG_0_0_0_VTDBAR_MSGREGADDR 0x0E4
#define IOMMU_REGS_SB_PREADDR_REG_0_0_0_VTDBAR_MSGREGADDR 0x0E8
#define IOMMU_REGS_SB_PREUADDR_REG_0_0_0_VTDBAR_MSGREGADDR 0x0EC
#define IOMMU_REGS_SB_FRCDL_REG_0_0_0_VTDBAR_MSGREGADDR 0x400
#define IOMMU_REGS_SB_FRCDH_REG_0_0_0_VTDBAR_MSGREGADDR 0x408
#define IOMMU_REGS_SB_IVA_REG_0_0_0_VTDBAR_MSGREGADDR 0x500
#define IOMMU_REGS_SB_IOTLB_REG_0_0_0_VTDBAR_MSGREGADDR 0x508
#define IOMMU_REGS_SB_INTRTADDR_0_0_0_VTDBAR_MSGREGADDR 0x0F00
#define IOMMU_REGS_SB_INTIRTA_0_0_0_VTDBAR_MSGREGADDR 0x0F08
#define IOMMU_REGS_SB_SAIRDGRP1_0_0_0_VTDBAR_MSGREGADDR 0x0F10
#define IOMMU_REGS_SB_SAIWRGRP1_0_0_0_VTDBAR_MSGREGADDR 0x0F18
#define IOMMU_REGS_SB_SAICTGRP1_0_0_0_VTDBAR_MSGREGADDR 0x0F20
#define IOMMU_REGS_SB_SEQ_LOG_0_0_0_VTDBAR_MSGREGADDR 0x0F40
#define IOMMU_REGS_SB_IOMMU_IDLE_0_0_0_VTDBAR_MSGREGADDR 0x0F44
#define IOMMU_REGS_SB_SLVREQSENT_0_0_0_VTDBAR_MSGREGADDR 0x0F48
#define IOMMU_REGS_SB_SLVACKRCVD_0_0_0_VTDBAR_MSGREGADDR 0x0F4C
#define IOMMU_REGS_SB_MTRRCAP_0_0_0_VTDBAR_MSGREGADDR 0x100
#define IOMMU_REGS_SB_MTRRDEFAULT_0_0_0_VTDBAR_MSGREGADDR 0x108
#define IOMMU_REGS_SB_MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_MSGREGADDR 0x120
#define IOMMU_REGS_SB_MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_MSGREGADDR 0x128
#define IOMMU_REGS_SB_MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_MSGREGADDR 0x130
#define IOMMU_REGS_SB_MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_MSGREGADDR 0x138
#define IOMMU_REGS_SB_MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_MSGREGADDR 0x140
#define IOMMU_REGS_SB_MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_MSGREGADDR 0x148
#define IOMMU_REGS_SB_MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_MSGREGADDR 0x150
#define IOMMU_REGS_SB_MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_MSGREGADDR 0x158
#define IOMMU_REGS_SB_MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_MSGREGADDR 0x160
#define IOMMU_REGS_SB_MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_MSGREGADDR 0x168
#define IOMMU_REGS_SB_MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_MSGREGADDR 0x170
#define IOMMU_REGS_SB_MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_MSGREGADDR 0x180
#define IOMMU_REGS_SB_MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_MSGREGADDR 0x190
#define IOMMU_REGS_SB_MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_MSGREGADDR 0x1A0
#define IOMMU_REGS_SB_MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_MSGREGADDR 0x1B0
#define IOMMU_REGS_SB_MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_MSGREGADDR 0x1C0
#define IOMMU_REGS_SB_MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_MSGREGADDR 0x1D0
#define IOMMU_REGS_SB_MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_MSGREGADDR 0x1E0
#define IOMMU_REGS_SB_MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_MSGREGADDR 0x1F0
#define IOMMU_REGS_SB_MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_MSGREGADDR 0x200
#define IOMMU_REGS_SB_MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_MSGREGADDR 0x210
#define IOMMU_REGS_SB_MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_MSGREGADDR 0x188
#define IOMMU_REGS_SB_MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_MSGREGADDR 0x198
#define IOMMU_REGS_SB_MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_MSGREGADDR 0x1A8
#define IOMMU_REGS_SB_MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_MSGREGADDR 0x1B8
#define IOMMU_REGS_SB_MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_MSGREGADDR 0x1C8
#define IOMMU_REGS_SB_MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_MSGREGADDR 0x1D8
#define IOMMU_REGS_SB_MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_MSGREGADDR 0x1E8
#define IOMMU_REGS_SB_MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_MSGREGADDR 0x1F8
#define IOMMU_REGS_SB_MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_MSGREGADDR 0x208
#define IOMMU_REGS_SB_MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_MSGREGADDR 0x218
#define IOMMU_REGS_BASE 0x0
#define IOMMU_REGS_VER_REG_0_0_0_VTDBAR_MMOFFSET 0x0
#define IOMMU_REGS_CAP_REG_0_0_0_VTDBAR_MMOFFSET 0x8
#define IOMMU_REGS_ECAP_REG_0_0_0_VTDBAR_MMOFFSET 0x10
#define IOMMU_REGS_GCMD_REG_0_0_0_VTDBAR_MMOFFSET 0x18
#define IOMMU_REGS_GSTS_REG_0_0_0_VTDBAR_MMOFFSET 0x1C
#define IOMMU_REGS_RTADDR_REG_0_0_0_VTDBAR_MMOFFSET 0x20
#define IOMMU_REGS_CCMD_REG_0_0_0_VTDBAR_MMOFFSET 0x28
#define IOMMU_REGS_FSTS_REG_0_0_0_VTDBAR_MMOFFSET 0x34
#define IOMMU_REGS_FECTL_REG_0_0_0_VTDBAR_MMOFFSET 0x38
#define IOMMU_REGS_FEDATA_REG_0_0_0_VTDBAR_MMOFFSET 0x3C
#define IOMMU_REGS_FEADDR_REG_0_0_0_VTDBAR_MMOFFSET 0x40
#define IOMMU_REGS_FEUADDR_REG_0_0_0_VTDBAR_MMOFFSET 0x44
#define IOMMU_REGS_AFLOG_REG_0_0_0_VTDBAR_MMOFFSET 0x58
#define IOMMU_REGS_PMEN_REG_0_0_0_VTDBAR_MMOFFSET 0x64
#define IOMMU_REGS_PLMBASE_REG_0_0_0_VTDBAR_MMOFFSET 0x68
#define IOMMU_REGS_PLMLIMIT_REG_0_0_0_VTDBAR_MMOFFSET 0x6C
#define IOMMU_REGS_PHMBASE_REG_0_0_0_VTDBAR_MMOFFSET 0x70
#define IOMMU_REGS_PHMLIMIT_REG_0_0_0_VTDBAR_MMOFFSET 0x78
#define IOMMU_REGS_IQH_REG_0_0_0_VTDBAR_MMOFFSET 0x80
#define IOMMU_REGS_IQT_REG_0_0_0_VTDBAR_MMOFFSET 0x88
#define IOMMU_REGS_IQA_REG_0_0_0_VTDBAR_MMOFFSET 0x90
#define IOMMU_REGS_ICS_REG_0_0_0_VTDBAR_MMOFFSET 0x9C
#define IOMMU_REGS_IECTL_REG_0_0_0_VTDBAR_MMOFFSET 0x0A0
#define IOMMU_REGS_IEDATA_REG_0_0_0_VTDBAR_MMOFFSET 0x0A4
#define IOMMU_REGS_IEADDR_REG_0_0_0_VTDBAR_MMOFFSET 0x0A8
#define IOMMU_REGS_IEUADDR_REG_0_0_0_VTDBAR_MMOFFSET 0x0AC
#define IOMMU_REGS_IRTA_REG_0_0_0_VTDBAR_MMOFFSET 0x0B8
#define IOMMU_REGS_PRESTS_REG_0_0_0_VTDBAR_MMOFFSET 0x0DC
#define IOMMU_REGS_PRECTL_REG_0_0_0_VTDBAR_MMOFFSET 0x0E0
#define IOMMU_REGS_PREDATA_REG_0_0_0_VTDBAR_MMOFFSET 0x0E4
#define IOMMU_REGS_PREADDR_REG_0_0_0_VTDBAR_MMOFFSET 0x0E8
#define IOMMU_REGS_PREUADDR_REG_0_0_0_VTDBAR_MMOFFSET 0x0EC
#define IOMMU_REGS_FRCDL_REG_0_0_0_VTDBAR_MMOFFSET 0x400
#define IOMMU_REGS_FRCDH_REG_0_0_0_VTDBAR_MMOFFSET 0x408
#define IOMMU_REGS_IVA_REG_0_0_0_VTDBAR_MMOFFSET 0x500
#define IOMMU_REGS_IOTLB_REG_0_0_0_VTDBAR_MMOFFSET 0x508
#define IOMMU_REGS_INTRTADDR_0_0_0_VTDBAR_MMOFFSET 0x0F00
#define IOMMU_REGS_INTIRTA_0_0_0_VTDBAR_MMOFFSET 0x0F08
#define IOMMU_REGS_SAIRDGRP1_0_0_0_VTDBAR_MMOFFSET 0x0F10
#define IOMMU_REGS_SAIWRGRP1_0_0_0_VTDBAR_MMOFFSET 0x0F18
#define IOMMU_REGS_SAICTGRP1_0_0_0_VTDBAR_MMOFFSET 0x0F20
#define IOMMU_REGS_SEQ_LOG_0_0_0_VTDBAR_MMOFFSET 0x0F40
#define IOMMU_REGS_IOMMU_IDLE_0_0_0_VTDBAR_MMOFFSET 0x0F44
#define IOMMU_REGS_SLVREQSENT_0_0_0_VTDBAR_MMOFFSET 0x0F48
#define IOMMU_REGS_SLVACKRCVD_0_0_0_VTDBAR_MMOFFSET 0x0F4C
#define IOMMU_REGS_MTRRCAP_0_0_0_VTDBAR_MMOFFSET 0x100
#define IOMMU_REGS_MTRRDEFAULT_0_0_0_VTDBAR_MMOFFSET 0x108
#define IOMMU_REGS_MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_MMOFFSET 0x120
#define IOMMU_REGS_MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_MMOFFSET 0x128
#define IOMMU_REGS_MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_MMOFFSET 0x130
#define IOMMU_REGS_MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_MMOFFSET 0x138
#define IOMMU_REGS_MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_MMOFFSET 0x140
#define IOMMU_REGS_MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_MMOFFSET 0x148
#define IOMMU_REGS_MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_MMOFFSET 0x150
#define IOMMU_REGS_MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_MMOFFSET 0x158
#define IOMMU_REGS_MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_MMOFFSET 0x160
#define IOMMU_REGS_MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_MMOFFSET 0x168
#define IOMMU_REGS_MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_MMOFFSET 0x170
#define IOMMU_REGS_MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_MMOFFSET 0x180
#define IOMMU_REGS_MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_MMOFFSET 0x190
#define IOMMU_REGS_MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_MMOFFSET 0x1A0
#define IOMMU_REGS_MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_MMOFFSET 0x1B0
#define IOMMU_REGS_MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_MMOFFSET 0x1C0
#define IOMMU_REGS_MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_MMOFFSET 0x1D0
#define IOMMU_REGS_MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_MMOFFSET 0x1E0
#define IOMMU_REGS_MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_MMOFFSET 0x1F0
#define IOMMU_REGS_MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_MMOFFSET 0x200
#define IOMMU_REGS_MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_MMOFFSET 0x210
#define IOMMU_REGS_MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_MMOFFSET 0x188
#define IOMMU_REGS_MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_MMOFFSET 0x198
#define IOMMU_REGS_MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_MMOFFSET 0x1A8
#define IOMMU_REGS_MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_MMOFFSET 0x1B8
#define IOMMU_REGS_MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_MMOFFSET 0x1C8
#define IOMMU_REGS_MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_MMOFFSET 0x1D8
#define IOMMU_REGS_MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_MMOFFSET 0x1E8
#define IOMMU_REGS_MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_MMOFFSET 0x1F8
#define IOMMU_REGS_MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_MMOFFSET 0x208
#define IOMMU_REGS_MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_MMOFFSET 0x218

#ifndef VER_REG_0_0_0_VTDBAR_FLAG
#define VER_REG_0_0_0_VTDBAR_FLAG
// VER_REG_0_0_0_VTDBAR desc:  Register to report the architecture version supported. Backward
// compatibility for the architecture is maintained with new revision
// numbers, allowing software to load remapping hardware drivers written
// for prior architecture versions.
typedef union {
    struct {
        uint32_t  MINOR                :   4;    //  Indicates supported
                                                 // architecture minor version.
        uint32_t  MAJOR                :   4;    //  Indicates supported
                                                 // architecture version.
        uint32_t  RSVD_0               :  24;    // Nebulon auto filled RSVD [31:8]

    }                                field;
    uint32_t                         val;
} VER_REG_0_0_0_VTDBAR_t;
#endif
#define VER_REG_0_0_0_VTDBAR_OFFSET 0x00
#define VER_REG_0_0_0_VTDBAR_SCOPE 0x01
#define VER_REG_0_0_0_VTDBAR_SIZE 32
#define VER_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define VER_REG_0_0_0_VTDBAR_RESET 0x00000010

#define VER_REG_0_0_0_VTDBAR_MINOR_LSB 0x0000
#define VER_REG_0_0_0_VTDBAR_MINOR_MSB 0x0003
#define VER_REG_0_0_0_VTDBAR_MINOR_RANGE 0x0004
#define VER_REG_0_0_0_VTDBAR_MINOR_MASK 0x0000000f
#define VER_REG_0_0_0_VTDBAR_MINOR_RESET_VALUE 0x00000000

#define VER_REG_0_0_0_VTDBAR_MAJOR_LSB 0x0004
#define VER_REG_0_0_0_VTDBAR_MAJOR_MSB 0x0007
#define VER_REG_0_0_0_VTDBAR_MAJOR_RANGE 0x0004
#define VER_REG_0_0_0_VTDBAR_MAJOR_MASK 0x000000f0
#define VER_REG_0_0_0_VTDBAR_MAJOR_RESET_VALUE 0x00000001


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef CAP_REG_0_0_0_VTDBAR_FLAG
#define CAP_REG_0_0_0_VTDBAR_FLAG
// CAP_REG_0_0_0_VTDBAR desc:  Register to report general remapping hardware capabilities
typedef union {
    struct {
        uint64_t  ND                   :   3;    //  [list][*]000b = Hardware
                                                 // supports 4-bit domain-ids with
                                                 // support for up to 16
                                                 // domains.[*]001b = Hardware
                                                 // supports 6-bit domain-ids with
                                                 // support for up to 64
                                                 // domains.[*]010b = Hardware
                                                 // supports 8-bit domain-ids with
                                                 // support for up to 256
                                                 // domains.[*]011b = Hardware
                                                 // supports 10-bit domain-ids
                                                 // with support for up to 1024
                                                 // domains.[*]100b = Hardware
                                                 // supports 12-bit domain-ids
                                                 // with support for up to 4K
                                                 // domains.[*]100b = Hardware
                                                 // supports 14-bit domain-ids
                                                 // with support for up to 16K
                                                 // domains.[*]110b = Hardware
                                                 // supports 16-bit domain-ids
                                                 // with support for up to 64K
                                                 // domains.[*]111b =
                                                 // Reserved.[/list]
        uint64_t  AFL                  :   1;    //  [list][*]0 = Indicates
                                                 // advanced fault logging is not
                                                 // supported. Only primary fault
                                                 // logging is supported.[*]1 =
                                                 // Indicates advanced fault
                                                 // logging is supported.[/list]
        uint64_t  RWBF                 :   1;    //  [list][*]0 = Indicates no
                                                 // write-buffer flushing is
                                                 // needed to ensure changes to
                                                 // memory-resident structures are
                                                 // visible to hardware.[*]1 =
                                                 // Indicates software must
                                                 // explicitly flush the write
                                                 // buffers to ensure updates made
                                                 // to memory-resident remapping
                                                 // structures are visible to
                                                 // hardware.[/list]
        uint64_t  PLMR                 :   1;    //  [list][*]0 = Indicates
                                                 // protected low-memory region is
                                                 // not supported.[*]1 = Indicates
                                                 // protected low-memory region is
                                                 // supported.[/list]
        uint64_t  PHMR                 :   1;    //  [list][*]0 = Indicates
                                                 // protected high-memory region
                                                 // is not supported.[*]1 =
                                                 // Indicates protected
                                                 // high-memory region is
                                                 // supported.[/list]
        uint64_t  CM                   :   1;    //  [list][*]0 = Not-present and
                                                 // erroneous entries are not
                                                 // cached in any of the
                                                 // renmapping caches.
                                                 // Invalidations are not required
                                                 // for modifications to
                                                 // individual not present or
                                                 // invalid entries. However, any
                                                 // modifications that result in
                                                 // decreasing the effective
                                                 // permissions or partial
                                                 // permission increases require
                                                 // invalidations for them to be
                                                 // effective.[*]1 = Not-present
                                                 // and erroneous mappings may be
                                                 // cached in the remapping
                                                 // caches. Any software updates
                                                 // to the remapping structures
                                                 // (including updates to
                                                 // not-present or erroneous
                                                 // entries) require explicit
                                                 // invalidation.[/list][p]Hardware
                                                 // implementations of this
                                                 // architecture must support a
                                                 // value of 0 in this field.[/p]
        uint64_t  SAGAW                :   5;    //  [p]This 5-bit field indicates
                                                 // the supported adjusted guest
                                                 // address widths (which in turn
                                                 // represents the levels of
                                                 // page-table walks for the 4KB
                                                 // base page size) supported by
                                                 // the hardware
                                                 // implementation.[/p][p]A value
                                                 // of 1 in any of these bits
                                                 // indicates the corresponding
                                                 // adjusted guest address width
                                                 // is supported. The adjusted
                                                 // guest address widths
                                                 // corresponding to various bit
                                                 // positions within this field
                                                 // are:[/p][list][*]0 = 30-bit
                                                 // AGAW (2-level page table)[*]1
                                                 // = 39-bit AGAW (3-level page
                                                 // table)[*]2 = 48-bit AGAW
                                                 // (4-level page table)[*]3 =
                                                 // 57-bit AGAW (5-level page
                                                 // table)[*]4 = 64-bit AGAW
                                                 // (6-level page
                                                 // table)[/list][p]Software must
                                                 // ensure that the adjusted guest
                                                 // address width used to setup
                                                 // the page tables is one of the
                                                 // supported guest address widths
                                                 // reported in this field.[/p]
        uint64_t  RSVD_0               :   3;    // Nebulon auto filled RSVD [15:13]
        uint64_t  MGAW                 :   6;    //  [p]This field indicates the
                                                 // maximum DMA virtual
                                                 // addressability supported by
                                                 // remapping hardware. The
                                                 // Maximum Guest Address Width
                                                 // (MGAW) is computed as (N+1),
                                                 // where N is the value reported
                                                 // in this field. For example, a
                                                 // hardware implementation
                                                 // supporting 48-bit MGAW reports
                                                 // a value of 47 (101111b) in
                                                 // this field.[/p][p]If the value
                                                 // in this field is X,
                                                 // untranslated and translated
                                                 // DMA requests to addresses
                                                 // above 2(x+1)-1 are always
                                                 // blocked by hardware.
                                                 // Translations requests to
                                                 // address above 2(x+1)-1 from
                                                 // allowed devices return a null
                                                 // Translation Completion Data
                                                 // Entry with R=W=0.[/p][p]Guest
                                                 // addressability for a given DMA
                                                 // request is limited to the
                                                 // minimum of the value reported
                                                 // through this field and the
                                                 // adjusted guest address width
                                                 // of the corresponding
                                                 // page-table structure.
                                                 // (Adjusted guest address widths
                                                 // supported by hardware are
                                                 // reported through the SAGAW
                                                 // field).[/p][p]Implementations
                                                 // are recommended to support
                                                 // MGAW at least equal to the
                                                 // physical addressability (host
                                                 // address width) of the
                                                 // platform.[/p]
        uint64_t  ZLR                  :   1;    //  [list][*]0 = Indicates the
                                                 // remapping hardware unit blocks
                                                 // (and treats as fault) zero
                                                 // length DMA read requests to
                                                 // write-only pages.[*]1 =
                                                 // Indicates the remapping
                                                 // hardware unit supports zero
                                                 // length DMA read requests to
                                                 // write-only pages.[/list][p]DMA
                                                 // remapping hardware
                                                 // implementations are
                                                 // recommended to report ZLR
                                                 // field as Set.[/p]
        uint64_t  RSVD_1               :   1;    // Nebulon auto filled RSVD [23:23]
        uint64_t  FRO                  :  10;    //  [p]This field specifies the
                                                 // location to the first fault
                                                 // recording register relative to
                                                 // the register base address of
                                                 // this remapping hardware
                                                 // unit.[/p][p]If the register
                                                 // base address is X, and the
                                                 // value reported in this field
                                                 // is Y, the address for the
                                                 // first fault recording register
                                                 // is calculated as X+(16*Y).[/p]
        uint64_t  SLLPS                :   4;    //  [p]This field indicates the
                                                 // super page sizes supported by
                                                 // hardware.[/p][p]A value of 1
                                                 // in any of these bits indicates
                                                 // the corresponding super-page
                                                 // size is supported. The
                                                 // super-page sizes corresponding
                                                 // to various bit positions
                                                 // within this field
                                                 // are:[/p][list][*]0 = 21-bit
                                                 // offset to page frame (2MB)[*]1
                                                 // = 30-bit offset to page frame
                                                 // (1GB)[*]2 = 39-bit offset to
                                                 // page frame (512GB)[*]3 =
                                                 // 48-bit offset to page frame
                                                 // (1TB)[/list][p]Hardware
                                                 // implementations supporting a
                                                 // specific super-page size must
                                                 // support all smaller super-page
                                                 // sizes, i.e. only valid values
                                                 // for this field are 0000b,
                                                 // 0001b, 0011b, 0111b,
                                                 // 1111b.[/p]
        uint64_t  RSVD_2               :   1;    // Nebulon auto filled RSVD [38:38]
        uint64_t  PSI                  :   1;    //  [list][*]0 = Hardware
                                                 // supports only domain and
                                                 // global invalidates for
                                                 // IOTLB.{*}1 = Hardware supports
                                                 // page selective, domain and
                                                 // global invalidates for
                                                 // IOTLB.[/list][p]Hardware
                                                 // implementations reporting this
                                                 // field as set are recommended
                                                 // to support a Maximum Address
                                                 // Mask Value (MAMV) value of at
                                                 // least 9 (or 18 if supporting
                                                 // 1GB pages with second level
                                                 // translation).[/p]
        uint64_t  NFR                  :   8;    //  [p]Number of fault recording
                                                 // registers is computed as N+1,
                                                 // where N is the value reported
                                                 // in this
                                                 // field.[/p][p]Implementations
                                                 // must support at least one
                                                 // fault recording register (NFR
                                                 // = 0) for each remapping
                                                 // hardware unit in the
                                                 // platform.[/p][p]The maximum
                                                 // number of fault recording
                                                 // registers per remapping
                                                 // hardware unit is 256.[/p]
        uint64_t  MAMV                 :   6;    //  [p]The value in this field
                                                 // indicates the maximum
                                                 // supported value for the
                                                 // Address Mask (AM) field in the
                                                 // Invalidation Address register
                                                 // (IVA_REG) and IOTLB
                                                 // Invalidation Descriptor
                                                 // (iotlb_inv_dsc) used for
                                                 // invalidations of second-level
                                                 // translation.[/p][p]This field
                                                 // is valid only when the PSI
                                                 // field in Capability register
                                                 // is reported as Set.[/p]
        uint64_t  DWD                  :   1;    //  [list][*]0 = Hardware does
                                                 // not support draining of DMA
                                                 // write requests.[*]1 = Hardware
                                                 // supports draining of DMA write
                                                 // requests.[/list]
        uint64_t  DRD                  :   1;    //  [list][*]0 = Hardware does
                                                 // not support draining of DMA
                                                 // read requests.[*]1 = Hardware
                                                 // supports draining of DMA read
                                                 // requests.[/list]
        uint64_t  FL1GP                :   1;    //  A value of 1 in this field
                                                 // indicates 1-GByte page size is
                                                 // supported for first-level
                                                 // translation.
        uint64_t  RSVD_3               :   7;    // Nebulon auto filled RSVD [63:57]

    }                                field;
    uint64_t                         val;
} CAP_REG_0_0_0_VTDBAR_t;
#endif
#define CAP_REG_0_0_0_VTDBAR_OFFSET 0x08
#define CAP_REG_0_0_0_VTDBAR_SCOPE 0x01
#define CAP_REG_0_0_0_VTDBAR_SIZE 64
#define CAP_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x11
#define CAP_REG_0_0_0_VTDBAR_RESET 0x1c0008c40660462

#define CAP_REG_0_0_0_VTDBAR_ND_LSB 0x0000
#define CAP_REG_0_0_0_VTDBAR_ND_MSB 0x0002
#define CAP_REG_0_0_0_VTDBAR_ND_RANGE 0x0003
#define CAP_REG_0_0_0_VTDBAR_ND_MASK 0x00000007
#define CAP_REG_0_0_0_VTDBAR_ND_RESET_VALUE 0x00000002

#define CAP_REG_0_0_0_VTDBAR_AFL_LSB 0x0003
#define CAP_REG_0_0_0_VTDBAR_AFL_MSB 0x0003
#define CAP_REG_0_0_0_VTDBAR_AFL_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_AFL_MASK 0x00000008
#define CAP_REG_0_0_0_VTDBAR_AFL_RESET_VALUE 0x00000000

#define CAP_REG_0_0_0_VTDBAR_RWBF_LSB 0x0004
#define CAP_REG_0_0_0_VTDBAR_RWBF_MSB 0x0004
#define CAP_REG_0_0_0_VTDBAR_RWBF_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_RWBF_MASK 0x00000010
#define CAP_REG_0_0_0_VTDBAR_RWBF_RESET_VALUE 0x00000000

#define CAP_REG_0_0_0_VTDBAR_PLMR_LSB 0x0005
#define CAP_REG_0_0_0_VTDBAR_PLMR_MSB 0x0005
#define CAP_REG_0_0_0_VTDBAR_PLMR_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_PLMR_MASK 0x00000020
#define CAP_REG_0_0_0_VTDBAR_PLMR_RESET_VALUE 0x00000001

#define CAP_REG_0_0_0_VTDBAR_PHMR_LSB 0x0006
#define CAP_REG_0_0_0_VTDBAR_PHMR_MSB 0x0006
#define CAP_REG_0_0_0_VTDBAR_PHMR_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_PHMR_MASK 0x00000040
#define CAP_REG_0_0_0_VTDBAR_PHMR_RESET_VALUE 0x00000001

#define CAP_REG_0_0_0_VTDBAR_CM_LSB 0x0007
#define CAP_REG_0_0_0_VTDBAR_CM_MSB 0x0007
#define CAP_REG_0_0_0_VTDBAR_CM_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_CM_MASK 0x00000080
#define CAP_REG_0_0_0_VTDBAR_CM_RESET_VALUE 0x00000000

#define CAP_REG_0_0_0_VTDBAR_SAGAW_LSB 0x0008
#define CAP_REG_0_0_0_VTDBAR_SAGAW_MSB 0x000c
#define CAP_REG_0_0_0_VTDBAR_SAGAW_RANGE 0x0005
#define CAP_REG_0_0_0_VTDBAR_SAGAW_MASK 0x00001f00
#define CAP_REG_0_0_0_VTDBAR_SAGAW_RESET_VALUE 0x00000004

#define CAP_REG_0_0_0_VTDBAR_MGAW_LSB 0x0010
#define CAP_REG_0_0_0_VTDBAR_MGAW_MSB 0x0015
#define CAP_REG_0_0_0_VTDBAR_MGAW_RANGE 0x0006
#define CAP_REG_0_0_0_VTDBAR_MGAW_MASK 0x003f0000
#define CAP_REG_0_0_0_VTDBAR_MGAW_RESET_VALUE 0x00000026

#define CAP_REG_0_0_0_VTDBAR_ZLR_LSB 0x0016
#define CAP_REG_0_0_0_VTDBAR_ZLR_MSB 0x0016
#define CAP_REG_0_0_0_VTDBAR_ZLR_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_ZLR_MASK 0x00400000
#define CAP_REG_0_0_0_VTDBAR_ZLR_RESET_VALUE 0x00000001

#define CAP_REG_0_0_0_VTDBAR_FRO_LSB 0x0018
#define CAP_REG_0_0_0_VTDBAR_FRO_MSB 0x0021
#define CAP_REG_0_0_0_VTDBAR_FRO_RANGE 0x000a
#define CAP_REG_0_0_0_VTDBAR_FRO_MASK 0x3ff000000
#define CAP_REG_0_0_0_VTDBAR_FRO_RESET_VALUE 0x00000040

#define CAP_REG_0_0_0_VTDBAR_SLLPS_LSB 0x0022
#define CAP_REG_0_0_0_VTDBAR_SLLPS_MSB 0x0025
#define CAP_REG_0_0_0_VTDBAR_SLLPS_RANGE 0x0004
#define CAP_REG_0_0_0_VTDBAR_SLLPS_MASK 0x3c00000000
#define CAP_REG_0_0_0_VTDBAR_SLLPS_RESET_VALUE 0x00000003

#define CAP_REG_0_0_0_VTDBAR_PSI_LSB 0x0027
#define CAP_REG_0_0_0_VTDBAR_PSI_MSB 0x0027
#define CAP_REG_0_0_0_VTDBAR_PSI_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_PSI_MASK 0x8000000000
#define CAP_REG_0_0_0_VTDBAR_PSI_RESET_VALUE 0x00000001

#define CAP_REG_0_0_0_VTDBAR_NFR_LSB 0x0028
#define CAP_REG_0_0_0_VTDBAR_NFR_MSB 0x002f
#define CAP_REG_0_0_0_VTDBAR_NFR_RANGE 0x0008
#define CAP_REG_0_0_0_VTDBAR_NFR_MASK 0xff0000000000
#define CAP_REG_0_0_0_VTDBAR_NFR_RESET_VALUE 0x00000000

#define CAP_REG_0_0_0_VTDBAR_MAMV_LSB 0x0030
#define CAP_REG_0_0_0_VTDBAR_MAMV_MSB 0x0035
#define CAP_REG_0_0_0_VTDBAR_MAMV_RANGE 0x0006
#define CAP_REG_0_0_0_VTDBAR_MAMV_MASK 0x3f000000000000
#define CAP_REG_0_0_0_VTDBAR_MAMV_RESET_VALUE 0x00000000

#define CAP_REG_0_0_0_VTDBAR_DWD_LSB 0x0036
#define CAP_REG_0_0_0_VTDBAR_DWD_MSB 0x0036
#define CAP_REG_0_0_0_VTDBAR_DWD_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_DWD_MASK 0x40000000000000
#define CAP_REG_0_0_0_VTDBAR_DWD_RESET_VALUE 0x00000001

#define CAP_REG_0_0_0_VTDBAR_DRD_LSB 0x0037
#define CAP_REG_0_0_0_VTDBAR_DRD_MSB 0x0037
#define CAP_REG_0_0_0_VTDBAR_DRD_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_DRD_MASK 0x80000000000000
#define CAP_REG_0_0_0_VTDBAR_DRD_RESET_VALUE 0x00000001

#define CAP_REG_0_0_0_VTDBAR_FL1GP_LSB 0x0038
#define CAP_REG_0_0_0_VTDBAR_FL1GP_MSB 0x0038
#define CAP_REG_0_0_0_VTDBAR_FL1GP_RANGE 0x0001
#define CAP_REG_0_0_0_VTDBAR_FL1GP_MASK 0x100000000000000
#define CAP_REG_0_0_0_VTDBAR_FL1GP_RESET_VALUE 0x00000001


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef ECAP_REG_0_0_0_VTDBAR_FLAG
#define ECAP_REG_0_0_0_VTDBAR_FLAG
// ECAP_REG_0_0_0_VTDBAR desc:  Register to report remapping hardware extended capabilities
typedef union {
    struct {
        uint64_t  C                    :   1;    //  [p]This field indicates if
                                                 // hardware access to the root,
                                                 // context, extended-context and
                                                 // interrupt-remap tables, and
                                                 // second-level paging structures
                                                 // for requests-without-PASID,
                                                 // are coherent (snooped) or
                                                 // not.[/p][list][*]0 = Indicates
                                                 // hardware accesses to remapping
                                                 // structures are
                                                 // non-coherent.[*]1 = Indicates
                                                 // hardware accesses to remapping
                                                 // structures are
                                                 // coherent.[/list][p]Hardware
                                                 // access to advanced fault log,
                                                 // invalidation queue,
                                                 // invalidation semaphore,
                                                 // page-request queue,
                                                 // PASID-table, PASID-state
                                                 // table, and first-level
                                                 // page-tables are always
                                                 // coherent.[/p]
        uint64_t  QI                   :   1;    //  [list][*]0 = Hardware does
                                                 // not support queued
                                                 // invalidations.[*]1 = Hardware
                                                 // supports queued
                                                 // invalidations.[/list]
        uint64_t  DT                   :   1;    //  [list][*]0 = Hardware does
                                                 // not support device-IOTLBs.[*]1
                                                 // = Hardware supports
                                                 // Device-IOTLBs.[/list][p]Implementations
                                                 // reporting this field as Set
                                                 // must also support Queued
                                                 // Invalidation
                                                 // (QI).[/p][p]Hardware
                                                 // implementations supporting I/O
                                                 // Page Requests (PRS field Set
                                                 // in Extended Capability
                                                 // register) must report a value
                                                 // of 1b in this field.[/p]
        uint64_t  IR                   :   1;    //  [list][*]0 = Hardware does
                                                 // not support interrupt
                                                 // remapping.[*]1 = Hardware
                                                 // supports interrupt
                                                 // remapping.[/list][p]Implementations
                                                 // reporting this field as Set
                                                 // must also support Queued
                                                 // Invalidation (QI).[/p]
        uint64_t  EIM                  :   1;    //  [list][*]0 = On Intel®64
                                                 // platforms, hardware supports
                                                 // only 8-bit APIC-IDs (xAPIC
                                                 // mode).[*]1 = On Intel®64
                                                 // platforms, hardware supports
                                                 // 32-bit APIC-IDs (x2APIC
                                                 // mode).[/list][p]This field is
                                                 // valid only on Intel®64
                                                 // platforms reporting Interrupt
                                                 // Remapping support (IR field
                                                 // Set).[/p]
        uint64_t  RSVD_0               :   1;    // Nebulon auto filled RSVD [5:5]
        uint64_t  PT                   :   1;    //  [list][*]0 = Hardware does
                                                 // not support pass-through
                                                 // translation type in context
                                                 // entries and
                                                 // extended-context-entries.[*]1
                                                 // = Hardware supports
                                                 // pass-through translation type
                                                 // in context entries and
                                                 // extended-context-entries.[/list][p]Pass-through
                                                 // translation is specified
                                                 // through Translation-Type (T)
                                                 // field value of 10b in
                                                 // context-entries, or T field
                                                 // value of 010b in
                                                 // extended-context-entries.[/p][p]Hardware
                                                 // implementations supporting
                                                 // PASID must report a value of
                                                 // 1b in this field.[/p]
        uint64_t  SC                   :   1;    //  [list][*]0 = Hardware does
                                                 // not support 1-setting of the
                                                 // SNP field in the page-table
                                                 // entries.[*]1 = Hardware
                                                 // supports the 1-setting of the
                                                 // SNP field in the page-table
                                                 // entries.[/list]
        uint64_t  IRO                  :  10;    //  [p]This field specifies the
                                                 // offset to the IOTLB registers
                                                 // relative to the register base
                                                 // address of this remapping
                                                 // hardware unit.[/p][p]If the
                                                 // register base address is X,
                                                 // and the value reported in this
                                                 // field is Y, the address for
                                                 // the first IOTLB invalidation
                                                 // register is calculated as
                                                 // X+(16*Y).[/p]
        uint64_t  RSVD_1               :   2;    // Nebulon auto filled RSVD [19:18]
        uint64_t  MHMV                 :   4;    //  [p]The value in this field
                                                 // indicates the maximum
                                                 // supported value for the Handle
                                                 // Mask (HM) field in the
                                                 // interrupt entry cache
                                                 // invalidation descriptor
                                                 // (iec_inv_dsc).[/p][p]This
                                                 // field is valid only when the
                                                 // IR field in Extended
                                                 // Capability register is
                                                 // reported as Set.[/p]
        uint64_t  ECS                  :   1;    //  [list][*]0 = Hardware does
                                                 // not support
                                                 // extended-root-entries and
                                                 // extended-context-entries.[*]1
                                                 // = Hardware supports
                                                 // extended-root-entries and
                                                 // extended-context-entries.[/list][p]Implementations
                                                 // reporting PASID or PRS fields
                                                 // as Set, must report this field
                                                 // as Set.[/p]
        uint64_t  MTS                  :   1;    //  [list][*]0 = Hardware does
                                                 // not support Memory Type in
                                                 // first-level translation and
                                                 // Extended Memory type in
                                                 // second-level translation.[*]1
                                                 // = Hardware supports Memory
                                                 // Type in first-level
                                                 // translation and Extended
                                                 // Memory type in second-level
                                                 // translation.[/list][p]This
                                                 // field is valid only when PASID
                                                 // and ECS fields are reported as
                                                 // Set.[/p][p]Remapping hardware
                                                 // units with, one or more
                                                 // devices that operate in
                                                 // processor coherency domain,
                                                 // under its scope must report
                                                 // this field as Set.[/p]
        uint64_t  NEST                 :   1;    //  [list][*]0 = Hardware does
                                                 // not support nested
                                                 // translations.[*]1 = Hardware
                                                 // supports nested
                                                 // translations.[/list][p]This
                                                 // field is valid only when PASID
                                                 // field is reported as Set.[/p]
        uint64_t  DIS                  :   1;    //  [list][*]0 = Hardware does
                                                 // not support deferred
                                                 // invalidations of IOTLB and
                                                 // Device-TLB.[*]1 = Hardware
                                                 // supports deferred
                                                 // invalidations of IOTLB and
                                                 // Device-TLB.[/list][p]This
                                                 // field is valid only when PASID
                                                 // field is reported as Set.[/p]
        uint64_t  RSVD_2                :   1;    //  [list][*]0 = Hardware does
                                                 // not support requests tagged
                                                 // with Process Address Space
                                                 // IDs.[*]1 = Hardware supports
                                                 // requests tagged with Process
                                                 // Address Space IDs.[/list]
        uint64_t  PRS                  :   1;    //  [list][*]0 = Hardware does
                                                 // not support Page Requests.[*]1
                                                 // = Hardware supports Page
                                                 // Requests[/list][p]This field
                                                 // is valid only when Device-TLB
                                                 // (DT) field is reported as
                                                 // Set.[/p]
        uint64_t  ERS                  :   1;    //  [list][*]0 = H/W does not
                                                 // support requests-with-PASID
                                                 // seeking execute
                                                 // permission.[*]1 = H/W supports
                                                 // requests-with-PASID seeking
                                                 // execute
                                                 // permission.[/list][p]This
                                                 // field is valid only when PASID
                                                 // field is reported as Set.[/p]
        uint64_t  SRS                  :   1;    //  [list][*]0 = H/W does not
                                                 // support requests-with-PASID
                                                 // seeking supervisor
                                                 // privilege.[*]1 = H/W supports
                                                 // requests-with-PASID seeking
                                                 // supervisor
                                                 // privilege.[/list][p]The field
                                                 // is valid only when PASID field
                                                 // is reported as Set.[/p]
        uint64_t  POT                  :   1;    //  [list][*]0 = Hardware does
                                                 // not support PASID-only
                                                 // Translation Type in
                                                 // extended-context-entries.[*]1
                                                 // = Hardware supports PASID-only
                                                 // Translation Type in
                                                 // extended-context-entries.[/list][p]This
                                                 // field is valid only when PASID
                                                 // field is reported as Set.[/p]
        uint64_t  NWFS                 :   1;    //  [list][*]0 = Hardware ignores
                                                 // the No Write (NW) flag in
                                                 // Device-TLB
                                                 // translationrequests, and
                                                 // behaves as if NW is always
                                                 // 0.[*]1 = Hardware supports the
                                                 // No Write (NW) flag in
                                                 // Device-TLB
                                                 // translationrequests.[/list][p]This
                                                 // field is valid only when
                                                 // Device-TLB support (DT) field
                                                 // is reported as Set.[/p]
        uint64_t  EAFS                 :   1;    //  [list][*]0 = Hardware does
                                                 // not support the
                                                 // extended-accessed (EA) bit in
                                                 // first-level paging-structure
                                                 // entries.[*]1 = Hardware
                                                 // supports the extended accessed
                                                 // (EA) bit in first-level
                                                 // paging-structure
                                                 // entries.[/list][p]This field
                                                 // is valid only when PASID field
                                                 // is reported as Set.[/p]
        uint64_t  PSS                  :   5;    //  [p]This field reports the
                                                 // PASID size supported by the
                                                 // remapping hardware for
                                                 // requests-with-PASID. A value
                                                 // of N in this field indicates
                                                 // hardware supports PASID field
                                                 // of N+1 bits (For example,
                                                 // value of 7 in this field,
                                                 // indicates 8-bit PASIDs are
                                                 // supported).[/p][p]Requests-with-PASID
                                                 // with PASID value beyond the
                                                 // limit specified by this field
                                                 // are treated as error by the
                                                 // remapping hardware.[/p][p]This
                                                 // field is valid only when PASID
                                                 // field is reported as Set.[/p]
        uint64_t  PASID                :   1;    //  [list][*]0 = Hardware does
                                                 // not support requests tagged
                                                 // with Process Address Space
                                                 // IDs.[*]1 = Hardware supports
                                                 // requests tagged with Process
                                                 // Address Space IDs.[/list]
        uint64_t  RSVD_3               :  23;    // Nebulon auto filled RSVD [63:41]

    }                                field;
    uint64_t                         val;
} ECAP_REG_0_0_0_VTDBAR_t;
#endif
#define ECAP_REG_0_0_0_VTDBAR_OFFSET 0x10
#define ECAP_REG_0_0_0_VTDBAR_SCOPE 0x01
#define ECAP_REG_0_0_0_VTDBAR_SIZE 64
#define ECAP_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x15
#define ECAP_REG_0_0_0_VTDBAR_RESET 0x7e3ff0505e

#define ECAP_REG_0_0_0_VTDBAR_C_LSB 0x0000
#define ECAP_REG_0_0_0_VTDBAR_C_MSB 0x0000
#define ECAP_REG_0_0_0_VTDBAR_C_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_C_MASK 0x00000001
#define ECAP_REG_0_0_0_VTDBAR_C_RESET_VALUE 0x00000000

#define ECAP_REG_0_0_0_VTDBAR_QI_LSB 0x0001
#define ECAP_REG_0_0_0_VTDBAR_QI_MSB 0x0001
#define ECAP_REG_0_0_0_VTDBAR_QI_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_QI_MASK 0x00000002
#define ECAP_REG_0_0_0_VTDBAR_QI_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_DT_LSB 0x0002
#define ECAP_REG_0_0_0_VTDBAR_DT_MSB 0x0002
#define ECAP_REG_0_0_0_VTDBAR_DT_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_DT_MASK 0x00000004
#define ECAP_REG_0_0_0_VTDBAR_DT_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_IR_LSB 0x0003
#define ECAP_REG_0_0_0_VTDBAR_IR_MSB 0x0003
#define ECAP_REG_0_0_0_VTDBAR_IR_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_IR_MASK 0x00000008
#define ECAP_REG_0_0_0_VTDBAR_IR_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_EIM_LSB 0x0004
#define ECAP_REG_0_0_0_VTDBAR_EIM_MSB 0x0004
#define ECAP_REG_0_0_0_VTDBAR_EIM_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_EIM_MASK 0x00000010
#define ECAP_REG_0_0_0_VTDBAR_EIM_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_PT_LSB 0x0006
#define ECAP_REG_0_0_0_VTDBAR_PT_MSB 0x0006
#define ECAP_REG_0_0_0_VTDBAR_PT_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_PT_MASK 0x00000040
#define ECAP_REG_0_0_0_VTDBAR_PT_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_SC_LSB 0x0007
#define ECAP_REG_0_0_0_VTDBAR_SC_MSB 0x0007
#define ECAP_REG_0_0_0_VTDBAR_SC_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_SC_MASK 0x00000080
#define ECAP_REG_0_0_0_VTDBAR_SC_RESET_VALUE 0x00000000

#define ECAP_REG_0_0_0_VTDBAR_IRO_LSB 0x0008
#define ECAP_REG_0_0_0_VTDBAR_IRO_MSB 0x0011
#define ECAP_REG_0_0_0_VTDBAR_IRO_RANGE 0x000a
#define ECAP_REG_0_0_0_VTDBAR_IRO_MASK 0x0003ff00
#define ECAP_REG_0_0_0_VTDBAR_IRO_RESET_VALUE 0x00000050

#define ECAP_REG_0_0_0_VTDBAR_MHMV_LSB 0x0014
#define ECAP_REG_0_0_0_VTDBAR_MHMV_MSB 0x0017
#define ECAP_REG_0_0_0_VTDBAR_MHMV_RANGE 0x0004
#define ECAP_REG_0_0_0_VTDBAR_MHMV_MASK 0x00f00000
#define ECAP_REG_0_0_0_VTDBAR_MHMV_RESET_VALUE 0x0000000f

#define ECAP_REG_0_0_0_VTDBAR_ECS_LSB 0x0018
#define ECAP_REG_0_0_0_VTDBAR_ECS_MSB 0x0018
#define ECAP_REG_0_0_0_VTDBAR_ECS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_ECS_MASK 0x01000000
#define ECAP_REG_0_0_0_VTDBAR_ECS_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_MTS_LSB 0x0019
#define ECAP_REG_0_0_0_VTDBAR_MTS_MSB 0x0019
#define ECAP_REG_0_0_0_VTDBAR_MTS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_MTS_MASK 0x02000000
#define ECAP_REG_0_0_0_VTDBAR_MTS_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_NEST_LSB 0x001a
#define ECAP_REG_0_0_0_VTDBAR_NEST_MSB 0x001a
#define ECAP_REG_0_0_0_VTDBAR_NEST_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_NEST_MASK 0x04000000
#define ECAP_REG_0_0_0_VTDBAR_NEST_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_DIS_LSB 0x001b
#define ECAP_REG_0_0_0_VTDBAR_DIS_MSB 0x001b
#define ECAP_REG_0_0_0_VTDBAR_DIS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_DIS_MASK 0x08000000
#define ECAP_REG_0_0_0_VTDBAR_DIS_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_PASID_LSB 0x001c
#define ECAP_REG_0_0_0_VTDBAR_PASID_MSB 0x001c
#define ECAP_REG_0_0_0_VTDBAR_PASID_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_PASID_MASK 0x10000000
#define ECAP_REG_0_0_0_VTDBAR_PASID_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_PRS_LSB 0x001d
#define ECAP_REG_0_0_0_VTDBAR_PRS_MSB 0x001d
#define ECAP_REG_0_0_0_VTDBAR_PRS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_PRS_MASK 0x20000000
#define ECAP_REG_0_0_0_VTDBAR_PRS_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_ERS_LSB 0x001e
#define ECAP_REG_0_0_0_VTDBAR_ERS_MSB 0x001e
#define ECAP_REG_0_0_0_VTDBAR_ERS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_ERS_MASK 0x40000000
#define ECAP_REG_0_0_0_VTDBAR_ERS_RESET_VALUE 0x00000000

#define ECAP_REG_0_0_0_VTDBAR_SRS_LSB 0x001f
#define ECAP_REG_0_0_0_VTDBAR_SRS_MSB 0x001f
#define ECAP_REG_0_0_0_VTDBAR_SRS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_SRS_MASK 0x80000000
#define ECAP_REG_0_0_0_VTDBAR_SRS_RESET_VALUE 0x00000000

#define ECAP_REG_0_0_0_VTDBAR_POT_LSB 0x0020
#define ECAP_REG_0_0_0_VTDBAR_POT_MSB 0x0020
#define ECAP_REG_0_0_0_VTDBAR_POT_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_POT_MASK 0x100000000
#define ECAP_REG_0_0_0_VTDBAR_POT_RESET_VALUE 0x00000000

#define ECAP_REG_0_0_0_VTDBAR_NWFS_LSB 0x0021
#define ECAP_REG_0_0_0_VTDBAR_NWFS_MSB 0x0021
#define ECAP_REG_0_0_0_VTDBAR_NWFS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_NWFS_MASK 0x200000000
#define ECAP_REG_0_0_0_VTDBAR_NWFS_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_EAFS_LSB 0x0022
#define ECAP_REG_0_0_0_VTDBAR_EAFS_MSB 0x0022
#define ECAP_REG_0_0_0_VTDBAR_EAFS_RANGE 0x0001
#define ECAP_REG_0_0_0_VTDBAR_EAFS_MASK 0x400000000
#define ECAP_REG_0_0_0_VTDBAR_EAFS_RESET_VALUE 0x00000001

#define ECAP_REG_0_0_0_VTDBAR_PSS_LSB 0x0023
#define ECAP_REG_0_0_0_VTDBAR_PSS_MSB 0x0027
#define ECAP_REG_0_0_0_VTDBAR_PSS_RANGE 0x0005
#define ECAP_REG_0_0_0_VTDBAR_PSS_MASK 0xf800000000
#define ECAP_REG_0_0_0_VTDBAR_PSS_RESET_VALUE 0x0000000f


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef GCMD_REG_0_0_0_VTDBAR_FLAG
#define GCMD_REG_0_0_0_VTDBAR_FLAG
// GCMD_REG_0_0_0_VTDBAR desc:  Register to control remapping hardware. If multiple control fields in
// this register need to be modified, software must serialize the
// modifications through multiple writes to this register.
typedef union {
    struct {
        uint32_t  RSVD_0               : 23;    // Nebulon auto filled RSVD [0:22]
        uint32_t  CFI                  :   1;    //  [p]This field is valid only
                                                 // for Intel®64 implementations
                                                 // supporting
                                                 // interrupt-remapping.[/p][p]Software
                                                 // writes to this field to enable
                                                 // or disable Compatibility
                                                 // Format interrupts on Intel®64
                                                 // platforms. The value in this
                                                 // field is effective only when
                                                 // interrupt-remapping is enabled
                                                 // and Extended Interrupt Mode
                                                 // (x2APIC mode) is not
                                                 // enabled.[/p][list][*]0 = Block
                                                 // Compatibility format
                                                 // interrupts.[*]1 = Process
                                                 // Compatibility format
                                                 // interrupts as pass-through
                                                 // (bypass interrupt
                                                 // remapping).[/list][p]Hardware
                                                 // reports the status of updating
                                                 // this field through the CFIS
                                                 // field in the Global Status
                                                 // register.[/p][p]The value
                                                 // returned on a read of this
                                                 // field is undefined.[/p]
        uint32_t  SIRTP                :   1;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // interrupt-remapping.[/p][p]Software
                                                 // sets this field to set/update
                                                 // the interrupt remapping table
                                                 // pointer used by hardware. The
                                                 // interrupt remapping table
                                                 // pointer is specified through
                                                 // the Interrupt Remapping Table
                                                 // Address (IRTA_REG)
                                                 // register.[/p][p]Hardware
                                                 // reports the status of the Set
                                                 // Interrupt Remap Table Pointer
                                                 // operation through the IRTPS
                                                 // field in the Global Status
                                                 // register.[/p][p]The Set
                                                 // Interrupt Remap Table Pointer
                                                 // operation must be performed
                                                 // before enabling or re-enabling
                                                 // (after disabling)
                                                 // interrupt-remapping hardware
                                                 // through the IRE
                                                 // field.[/p][p]After a Set
                                                 // Interrupt Remap Table Pointer
                                                 // operation, software must
                                                 // globally invalidate the
                                                 // interrupt entry cache. This is
                                                 // required to ensure hardware
                                                 // uses only the
                                                 // interrupt-remapping entries
                                                 // referenced by the new
                                                 // interrupt remap table pointer,
                                                 // and not any stale cached
                                                 // entries.[/p][p]While interrupt
                                                 // remapping is active, software
                                                 // may update the interrupt
                                                 // remapping table pointer
                                                 // through this field. However,
                                                 // to ensure valid in-flight
                                                 // interrupt requests are
                                                 // deterministically remapped,
                                                 // software must ensure that the
                                                 // structures referenced by the
                                                 // new interrupt remap table
                                                 // pointer are programmed to
                                                 // provide the same remapping
                                                 // results as the structures
                                                 // referenced by the previous
                                                 // interrupt remap table
                                                 // pointer.[/p][p]Clearing this
                                                 // bit has no effect. The value
                                                 // returned on a read of this
                                                 // field is undefined.[/p]
        uint32_t  IRE                  :   1;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // interrupt
                                                 // remapping.[/p][list][*]0 =
                                                 // Disable interrupt-remapping
                                                 // hardware.[*]1 = Enable
                                                 // interrupt-remapping
                                                 // hardware.[/list][p]Hardware
                                                 // reports the status of the
                                                 // interrupt remapping enable
                                                 // operation through the IRES
                                                 // field in the Global Status
                                                 // register.[/p][p]There may be
                                                 // active interrupt requests in
                                                 // the platform when software
                                                 // updates this field. Hardware
                                                 // must enable or disable
                                                 // interrupt-remapping logic only
                                                 // at deterministic transaction
                                                 // boundaries, so that any
                                                 // in-flight interrupts are
                                                 // either subject to remapping or
                                                 // not at all.[/p][p]Hardware
                                                 // implementations must drain any
                                                 // in-flight interrupts requests
                                                 // queued in the Root-Complex
                                                 // before completing the
                                                 // interrupt-remapping enable
                                                 // command and reflecting the
                                                 // status of the command through
                                                 // the IRES field in the Global
                                                 // Status register.[/p][p]The
                                                 // value returned on a read of
                                                 // this field is undefined.[/p]
        uint32_t  QIE                  :   1;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // queued
                                                 // invalidations.[/p][p]Software
                                                 // writes to this field to enable
                                                 // or disable queued
                                                 // invalidations.[/p][list][*]0 =
                                                 // Disable queued
                                                 // invalidations.[*]1 = Enable
                                                 // use of queued
                                                 // invalidations.[/list][p]Hardware
                                                 // reports the status of queued
                                                 // invalidation enable operation
                                                 // through QIES field in the
                                                 // Global Status
                                                 // register.[/p][p]The value
                                                 // returned on a read of this
                                                 // field is undefined.[/p]
        uint32_t  WBF                  :   1;    //  [p]This bit is valid only for
                                                 // implementations requiring
                                                 // write buffer
                                                 // flushing.[/p][p]Software sets
                                                 // this field to request that
                                                 // hardware flush the
                                                 // Root-Complex internal write
                                                 // buffers. This is done to
                                                 // ensure any updates to the
                                                 // memory-resident remapping
                                                 // structures are not held in any
                                                 // internal write posting
                                                 // buffers.[/p][p]Hardware
                                                 // reports the status of the
                                                 // write buffer flushing
                                                 // operation through the WBFS
                                                 // field in the Global Status
                                                 // register.[/p][p]Clearing this
                                                 // bit has no effect. The value
                                                 // returned on a read of this
                                                 // field is undefined.[/p]
        uint32_t  EAFL                 :   1;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // advanced fault
                                                 // logging.[/p][p]Software writes
                                                 // to this field to request
                                                 // hardware to enable or disable
                                                 // advanced fault
                                                 // logging:[/p][list][*]0 =
                                                 // Disable advanced fault
                                                 // logging. In this case,
                                                 // translation faults are
                                                 // reported through the Fault
                                                 // Recording registers.[*]1 =
                                                 // Enable use of memory-resident
                                                 // fault log. When enabled,
                                                 // translation faults are
                                                 // recorded in the
                                                 // memory-resident log. The fault
                                                 // log pointer must be set in
                                                 // hardware (through the SFL
                                                 // field) before enabling advaned
                                                 // fault logging. Hardware
                                                 // reports the status of the
                                                 // advaned fault logging enable
                                                 // operation through the AFLS
                                                 // field in the Global Status
                                                 // register.[/list][p]The value
                                                 // returned on read of this field
                                                 // is undefined.[/p]
        uint32_t  SFL                  :   1;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // advanced fault
                                                 // logging.[/p][p]Software sets
                                                 // this field to request hardware
                                                 // to set/update the fault-log
                                                 // pointer used by hardware. The
                                                 // fault-log pointer is specified
                                                 // through Advanced Fault Log
                                                 // register.[/p][p]Hardware
                                                 // reports the status of the Set
                                                 // Fault Log operation through
                                                 // the FLS field in the Global
                                                 // Status register.[/p][p]The
                                                 // fault log pointer must be set
                                                 // before enabling advanced fault
                                                 // logging (through EAFL field).
                                                 // Once advanced fault logging is
                                                 // enabled, the fault log pointer
                                                 // may be updated through this
                                                 // field while DMA remapping is
                                                 // active.[/p][p]Clearing this
                                                 // bit has no effect. The value
                                                 // returned on read of this field
                                                 // is undefined.[/p]
        uint32_t  SRTP                 :   1;    //  [p]Software sets this field
                                                 // to set/update the root-entry
                                                 // table pointer used by
                                                 // hardware. The root-entry table
                                                 // pointer is specified through
                                                 // the Root-entry Table Address
                                                 // (RTA_REG)
                                                 // register.[/p][p]Hardware
                                                 // reports the status of the Set
                                                 // Root Table Pointer operation
                                                 // through the RTPS field in the
                                                 // Global Status
                                                 // register.[/p][p]The Set Root
                                                 // Table Pointer operation must
                                                 // be performed before enabling
                                                 // or re-enabling (after
                                                 // disabling) DMA remapping
                                                 // through the TE
                                                 // field.[/p][p].After a Set Root
                                                 // Table Pointer operation,
                                                 // software must globally
                                                 // invalidate the context cache
                                                 // and then globally invalidate
                                                 // of IOTLB. This is required to
                                                 // ensure hardware uses only the
                                                 // remapping structures
                                                 // referenced by the new root
                                                 // table pointer, and not stale
                                                 // cached entries.[/p][p]While
                                                 // DMA remapping hardware is
                                                 // active, software may update
                                                 // the root table pointer through
                                                 // this field. However, to ensure
                                                 // valid in-flight DMA requests
                                                 // are deterministically
                                                 // remapped, software must ensure
                                                 // that the structures referenced
                                                 // by the new root table pointer
                                                 // are programmed to provide the
                                                 // same remapping results as the
                                                 // structures referenced by the
                                                 // previous root-table
                                                 // pointer.[/p][p]Clearing this
                                                 // bit has no effect. The value
                                                 // returned on read of this field
                                                 // is undefined.[/p]
        uint32_t  TE                   :   1;    //  [p]Software writes to this
                                                 // field to request hardware to
                                                 // enable/disable
                                                 // DMA-remapping:[/p][list][*]0 =
                                                 // Disable DMA remapping.[*]1 =
                                                 // Enable DMA
                                                 // remapping.[/list][p]Hardware
                                                 // reports the status of the
                                                 // translation enable operation
                                                 // through the TES field in the
                                                 // Global Status
                                                 // register.[/p][p]There may be
                                                 // active DMA requests in the
                                                 // platform when software updates
                                                 // this field. Hardware must
                                                 // enable or disable remapping
                                                 // logic only at determinstic
                                                 // transaction boundaries, so
                                                 // that any in-flight transaction
                                                 // is either subject to remapping
                                                 // or not at all.[/p][p]Hardware
                                                 // implementations supporting DMA
                                                 // draining must drain any
                                                 // in-flight DMA read/write
                                                 // requests queued within the
                                                 // Root-Complex before completing
                                                 // the translation enable command
                                                 // and reflecting the status of
                                                 // the command through the TES
                                                 // field in the Global Status
                                                 // register.[/p][p]The value
                                                 // returned on a read of this
                                                 // field is undefined.[/p]

    }                                field;
    uint32_t                         val;
} GCMD_REG_0_0_0_VTDBAR_t;
#endif
#define GCMD_REG_0_0_0_VTDBAR_OFFSET 0x18
#define GCMD_REG_0_0_0_VTDBAR_SCOPE 0x01
#define GCMD_REG_0_0_0_VTDBAR_SIZE 32
#define GCMD_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x09
#define GCMD_REG_0_0_0_VTDBAR_RESET 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_CFI_LSB 0x0017
#define GCMD_REG_0_0_0_VTDBAR_CFI_MSB 0x0017
#define GCMD_REG_0_0_0_VTDBAR_CFI_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_CFI_MASK 0x00800000
#define GCMD_REG_0_0_0_VTDBAR_CFI_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_SIRTP_LSB 0x0018
#define GCMD_REG_0_0_0_VTDBAR_SIRTP_MSB 0x0018
#define GCMD_REG_0_0_0_VTDBAR_SIRTP_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_SIRTP_MASK 0x01000000
#define GCMD_REG_0_0_0_VTDBAR_SIRTP_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_IRE_LSB 0x0019
#define GCMD_REG_0_0_0_VTDBAR_IRE_MSB 0x0019
#define GCMD_REG_0_0_0_VTDBAR_IRE_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_IRE_MASK 0x02000000
#define GCMD_REG_0_0_0_VTDBAR_IRE_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_QIE_LSB 0x001a
#define GCMD_REG_0_0_0_VTDBAR_QIE_MSB 0x001a
#define GCMD_REG_0_0_0_VTDBAR_QIE_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_QIE_MASK 0x04000000
#define GCMD_REG_0_0_0_VTDBAR_QIE_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_WBF_LSB 0x001b
#define GCMD_REG_0_0_0_VTDBAR_WBF_MSB 0x001b
#define GCMD_REG_0_0_0_VTDBAR_WBF_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_WBF_MASK 0x08000000
#define GCMD_REG_0_0_0_VTDBAR_WBF_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_EAFL_LSB 0x001c
#define GCMD_REG_0_0_0_VTDBAR_EAFL_MSB 0x001c
#define GCMD_REG_0_0_0_VTDBAR_EAFL_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_EAFL_MASK 0x10000000
#define GCMD_REG_0_0_0_VTDBAR_EAFL_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_SFL_LSB 0x001d
#define GCMD_REG_0_0_0_VTDBAR_SFL_MSB 0x001d
#define GCMD_REG_0_0_0_VTDBAR_SFL_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_SFL_MASK 0x20000000
#define GCMD_REG_0_0_0_VTDBAR_SFL_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_SRTP_LSB 0x001e
#define GCMD_REG_0_0_0_VTDBAR_SRTP_MSB 0x001e
#define GCMD_REG_0_0_0_VTDBAR_SRTP_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_SRTP_MASK 0x40000000
#define GCMD_REG_0_0_0_VTDBAR_SRTP_RESET_VALUE 0x00000000

#define GCMD_REG_0_0_0_VTDBAR_TE_LSB 0x001f
#define GCMD_REG_0_0_0_VTDBAR_TE_MSB 0x001f
#define GCMD_REG_0_0_0_VTDBAR_TE_RANGE 0x0001
#define GCMD_REG_0_0_0_VTDBAR_TE_MASK 0x80000000
#define GCMD_REG_0_0_0_VTDBAR_TE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef GSTS_REG_0_0_0_VTDBAR_FLAG
#define GSTS_REG_0_0_0_VTDBAR_FLAG
// GSTS_REG_0_0_0_VTDBAR desc:  Register to report general remapping hardware status.
typedef union {
    struct {
        uint32_t  RSVD_0               : 23;    // Nebulon auto filled RSVD [0:22]
        uint32_t  CFIS                 :   1;    //  [p]This field indicates the
                                                 // status of Compatibility format
                                                 // interrupts on Intel®64
                                                 // implementations supporting
                                                 // interrupt-remapping. The value
                                                 // reported in this field is
                                                 // applicable only when
                                                 // interrupt-remapping is enabled
                                                 // and Extended Interrupt Mode
                                                 // (x2APIC mode) is not
                                                 // enabled.[/p][list][*]0 =
                                                 // Compatibility format
                                                 // interrupts are blocked.[*]1 =
                                                 // Compatibility format
                                                 // interrupts are processed as
                                                 // pass-through (bypassing
                                                 // interrupt remapping).[/list]
        uint32_t  IRTPS                :   1;    //  [p]This field indicates the
                                                 // status of the interrupt
                                                 // remapping table pointer in
                                                 // hardware.[/p][p]This field is
                                                 // cleared by hardware when
                                                 // software sets the SIRTP field
                                                 // in the Global Command
                                                 // register. This field is Set by
                                                 // hardware when hardware
                                                 // completes the set interrupt
                                                 // remap table pointer operation
                                                 // using the value provided in
                                                 // the Interrupt Remapping Table
                                                 // Address register.[/p]
        uint32_t  IRES                 :   1;    //  [p]This field indicates the
                                                 // status of Interrupt-remapping
                                                 // hardware.[/p][list][*]0 =
                                                 // Interrupt-remapping hardware
                                                 // is not enabled.[*]1 =
                                                 // Interrupt-remapping hardware
                                                 // is enabled[/list]
        uint32_t  QIES                 :   1;    //  [p]This field indicates
                                                 // queued invalidation enable
                                                 // status.[/p][list][*]0 = queued
                                                 // invalidation is not
                                                 // enabled.[*]1 = queued
                                                 // invalidation is enabled[/list]
        uint32_t  WBFS                 :   1;    //  [p]This field is valid only
                                                 // for implementations requiring
                                                 // write buffer flushing. This
                                                 // field indicates the status of
                                                 // the write buffer flush
                                                 // command. It
                                                 // is:[/p][list][*]Set by
                                                 // hardware when software sets
                                                 // the WBF field in the Global
                                                 // Command register.[*]Cleared by
                                                 // hardware when hardware
                                                 // completes the write buffer
                                                 // flushing operation.[/list]
        uint32_t  AFLS                 :   1;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // advanced fault logging. It
                                                 // indicates the advanced fault
                                                 // logging status:[/p][list][*]0
                                                 // = Advanced Fault Logging is
                                                 // not enabled.[*]1 = Advanced
                                                 // Fault Logging is
                                                 // enabled.[/list]
        uint32_t  FLS                  :   1;    //  [p]This field:[/p][list][*]Is
                                                 // cleared by hardware when
                                                 // software Sets the SFL field in
                                                 // the Global Command
                                                 // register.[*]Is Set by hardware
                                                 // whn hardware completes the Set
                                                 // Fault Log Pointer operation
                                                 // using the value provided in
                                                 // the Advanced Fault Log
                                                 // register.[/list]
        uint32_t  RTPS                 :   1;    //  [p]This field indicates the
                                                 // status of the root- table
                                                 // pointer in
                                                 // hardware.[/p][p]This field is
                                                 // cleared by hardware when
                                                 // software sets the SRTP field
                                                 // in the Global Command
                                                 // register. This field is set by
                                                 // hardware when hardware
                                                 // completes the Set Root Table
                                                 // Pointer operation using the
                                                 // value provided in the
                                                 // Root-Entry Table Address
                                                 // register.[/p]
        uint32_t  TES                  :   1;    //  [p]This field indicates the
                                                 // status of DMA-remapping
                                                 // hardware.[/p][list][*]0 =
                                                 // DMA-remapping hardware is not
                                                 // enabled.[*]1 = DMA-remapping
                                                 // hardware is enabled[/list]

    }                                field;
    uint32_t                         val;
} GSTS_REG_0_0_0_VTDBAR_t;
#endif
#define GSTS_REG_0_0_0_VTDBAR_OFFSET 0x1c
#define GSTS_REG_0_0_0_VTDBAR_SCOPE 0x01
#define GSTS_REG_0_0_0_VTDBAR_SIZE 32
#define GSTS_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x09
#define GSTS_REG_0_0_0_VTDBAR_RESET 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_CFIS_LSB 0x0017
#define GSTS_REG_0_0_0_VTDBAR_CFIS_MSB 0x0017
#define GSTS_REG_0_0_0_VTDBAR_CFIS_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_CFIS_MASK 0x00800000
#define GSTS_REG_0_0_0_VTDBAR_CFIS_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_IRTPS_LSB 0x0018
#define GSTS_REG_0_0_0_VTDBAR_IRTPS_MSB 0x0018
#define GSTS_REG_0_0_0_VTDBAR_IRTPS_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_IRTPS_MASK 0x01000000
#define GSTS_REG_0_0_0_VTDBAR_IRTPS_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_IRES_LSB 0x0019
#define GSTS_REG_0_0_0_VTDBAR_IRES_MSB 0x0019
#define GSTS_REG_0_0_0_VTDBAR_IRES_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_IRES_MASK 0x02000000
#define GSTS_REG_0_0_0_VTDBAR_IRES_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_QIES_LSB 0x001a
#define GSTS_REG_0_0_0_VTDBAR_QIES_MSB 0x001a
#define GSTS_REG_0_0_0_VTDBAR_QIES_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_QIES_MASK 0x04000000
#define GSTS_REG_0_0_0_VTDBAR_QIES_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_WBFS_LSB 0x001b
#define GSTS_REG_0_0_0_VTDBAR_WBFS_MSB 0x001b
#define GSTS_REG_0_0_0_VTDBAR_WBFS_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_WBFS_MASK 0x08000000
#define GSTS_REG_0_0_0_VTDBAR_WBFS_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_AFLS_LSB 0x001c
#define GSTS_REG_0_0_0_VTDBAR_AFLS_MSB 0x001c
#define GSTS_REG_0_0_0_VTDBAR_AFLS_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_AFLS_MASK 0x10000000
#define GSTS_REG_0_0_0_VTDBAR_AFLS_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_FLS_LSB 0x001d
#define GSTS_REG_0_0_0_VTDBAR_FLS_MSB 0x001d
#define GSTS_REG_0_0_0_VTDBAR_FLS_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_FLS_MASK 0x20000000
#define GSTS_REG_0_0_0_VTDBAR_FLS_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_RTPS_LSB 0x001e
#define GSTS_REG_0_0_0_VTDBAR_RTPS_MSB 0x001e
#define GSTS_REG_0_0_0_VTDBAR_RTPS_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_RTPS_MASK 0x40000000
#define GSTS_REG_0_0_0_VTDBAR_RTPS_RESET_VALUE 0x00000000

#define GSTS_REG_0_0_0_VTDBAR_TES_LSB 0x001f
#define GSTS_REG_0_0_0_VTDBAR_TES_MSB 0x001f
#define GSTS_REG_0_0_0_VTDBAR_TES_RANGE 0x0001
#define GSTS_REG_0_0_0_VTDBAR_TES_MASK 0x80000000
#define GSTS_REG_0_0_0_VTDBAR_TES_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef RTADDR_REG_0_0_0_VTDBAR_FLAG
#define RTADDR_REG_0_0_0_VTDBAR_FLAG
// RTADDR_REG_0_0_0_VTDBAR desc:  Register providing the base address of root-entry table.
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  RTT                  :   1;    //  [p]This field specifies the
                                                 // type of root-table referenced
                                                 // by the Root Table Address
                                                 // (RTA) field:[/p][list][*]0 =
                                                 // Root Table.[*]1 = Extended
                                                 // Root Table[/list]
        uint64_t  RTA                  :  52;    //  [p]This register points to
                                                 // base of page aligned,
                                                 // 4KB-sized root-entry table in
                                                 // system memory. Hardware
                                                 // ignores and not implements
                                                 // bits 63:HAW, where HAW is the
                                                 // host address
                                                 // width.[/p][p]Software
                                                 // specifies the base address of
                                                 // the root-entry table through
                                                 // this register, and programs it
                                                 // in hardware through the SRTP
                                                 // field in the Global Command
                                                 // register.[/p][p]Reads of this
                                                 // register returns value that
                                                 // was last programmed to it.[/p]

    }                                field;
    uint64_t                         val;
} RTADDR_REG_0_0_0_VTDBAR_t;
#endif
#define RTADDR_REG_0_0_0_VTDBAR_OFFSET 0x20
#define RTADDR_REG_0_0_0_VTDBAR_SCOPE 0x01
#define RTADDR_REG_0_0_0_VTDBAR_SIZE 64
#define RTADDR_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define RTADDR_REG_0_0_0_VTDBAR_RESET 0x00000000

#define RTADDR_REG_0_0_0_VTDBAR_RTT_LSB 0x000b
#define RTADDR_REG_0_0_0_VTDBAR_RTT_MSB 0x000b
#define RTADDR_REG_0_0_0_VTDBAR_RTT_RANGE 0x0001
#define RTADDR_REG_0_0_0_VTDBAR_RTT_MASK 0x00000800
#define RTADDR_REG_0_0_0_VTDBAR_RTT_RESET_VALUE 0x00000000

#define RTADDR_REG_0_0_0_VTDBAR_RTA_LSB 0x000c
#define RTADDR_REG_0_0_0_VTDBAR_RTA_MSB 0x003f
#define RTADDR_REG_0_0_0_VTDBAR_RTA_RANGE 0x0034
#define RTADDR_REG_0_0_0_VTDBAR_RTA_MASK 0xfffffffffffff000
#define RTADDR_REG_0_0_0_VTDBAR_RTA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef CCMD_REG_0_0_0_VTDBAR_FLAG
#define CCMD_REG_0_0_0_VTDBAR_FLAG
// CCMD_REG_0_0_0_VTDBAR desc:  Register to manage context cache. The act of writing the uppermost
// byte of the CCMD_REG with the ICC field Set causes the hardware to
// perform the context-cache invalidation.
typedef union {
    struct {
        uint64_t  DID                  :  16;    //  [p]Indicates the id of the
                                                 // domain whose context-entries
                                                 // need to be selectively
                                                 // invalidated. This field must
                                                 // be programmed by software for
                                                 // both domain-selective and
                                                 // device-selective invalidation
                                                 // requests.[/p][p]The Capability
                                                 // register reports the domain-id
                                                 // width supported by hardware.
                                                 // Software must ensure that the
                                                 // value written to this field is
                                                 // within this limit. Hardware
                                                 // may ignore and not implement
                                                 // bits15:N, where N is the
                                                 // supported domain-id width
                                                 // reported in the Capability
                                                 // register.[/p]
        uint64_t  SID                  :  16;    //  [p]Indicates the source-id of
                                                 // the device whose corresponding
                                                 // context-entry needs to be
                                                 // selectively invalidated. This
                                                 // field along with the FM field
                                                 // must be programmed by software
                                                 // for device-selective
                                                 // invalidation requests.[/p]
        uint64_t  FM                   :   2;    //  [p]Software may use the
                                                 // Function Mask to perform
                                                 // device-selective invalidations
                                                 // on behalf of devices
                                                 // supporting PCI Express Phantom
                                                 // Functions...This field
                                                 // specifies which bits of the
                                                 // function number portion (least
                                                 // significant three bits) of the
                                                 // SID field to mask when
                                                 // performing device-selective
                                                 // invalidations. The following
                                                 // encodings are defined for this
                                                 // field:[/p][list][*]00: No bits
                                                 // in the SID field masked.[*]01:
                                                 // Mask most significant bit of
                                                 // function number in the SID
                                                 // field.[*]10: Mask two most
                                                 // significant bit of function
                                                 // number in the SID field.[*]11:
                                                 // Mask all three bits of
                                                 // function number in the SID
                                                 // field.[/list][p]The
                                                 // context-entries corresponding
                                                 // to all the source-ids
                                                 // specified through the FM and
                                                 // SID fields must have to the
                                                 // domain-id specified in the DID
                                                 // field.[/p]
        uint64_t  RSVD_0               :  25;    // Nebulon auto filled RSVD [58:34]
        uint64_t  CAIG                 :   2;    //  [p]Hardware reports the
                                                 // granularity at which an
                                                 // invalidation request was
                                                 // processed through the CAIG
                                                 // field at the time of reporting
                                                 // invalidation completion (by
                                                 // clearing the ICC
                                                 // field).[/p][p]The following
                                                 // are the encodings for this
                                                 // field:[/p][list][*]00:
                                                 // Reserved.[*]01: Global
                                                 // Invalidation performed. This
                                                 // could be in response to a
                                                 // global, domain-selective or
                                                 // device-selective invalidation
                                                 // request.[*]10:
                                                 // Domain-selective invalidation
                                                 // performed using the domain-id
                                                 // specified by software in the
                                                 // DID field. This could be in
                                                 // response to a domain-selective
                                                 // or device-selective
                                                 // invalidation request.[*]11:
                                                 // Device-selective invalidation
                                                 // performed using the source-id
                                                 // and domain-id specified by
                                                 // software in the SID and FM
                                                 // fields. This can only be in
                                                 // response to a device-selective
                                                 // invalidation request.[/list]
        uint64_t  CIRG                 :   2;    //  [p]Software provides the
                                                 // requested invalidation
                                                 // granularity through this field
                                                 // when setting the ICC
                                                 // field:[/p][list][*]00:
                                                 // Reserved.[*]01: Global
                                                 // Invalidation request.[*]10:
                                                 // Domain-selective invalidation
                                                 // request. The target domain-id
                                                 // must be specified in the DID
                                                 // field.[*]11: Device-selective
                                                 // invalidation request. The
                                                 // target source-id(s) must be
                                                 // specified through the SID and
                                                 // FM fields, and the domain-id
                                                 // (that was programmed in the
                                                 // context-entry for these
                                                 // device(s)) must be provided in
                                                 // the DID
                                                 // field.[/list][p]Hardware
                                                 // implementations may process an
                                                 // invalidation request by
                                                 // performing invalidation at a
                                                 // coarser granularity than
                                                 // requested. Hardware indicates
                                                 // completion of the invalidation
                                                 // request by clearing the ICC
                                                 // field. At this time, hardware
                                                 // also indicates the granularity
                                                 // at which the actual
                                                 // invalidation was performed
                                                 // through the CAIG field.[/p]
        uint64_t  ICC                  :   1;    //  [p]Software requests
                                                 // invalidation of context-cache
                                                 // by setting this field.
                                                 // Software must also set the
                                                 // requested invalidation
                                                 // granularity by programming the
                                                 // CIRG field. Software must read
                                                 // back and check the ICC field
                                                 // is Clear to confirm the
                                                 // invalidation is complete.
                                                 // Software must not update this
                                                 // register when this field is
                                                 // set.[/p][p]Hardware clears the
                                                 // ICC field to indicate the
                                                 // invalidation request is
                                                 // complete. Hardware also
                                                 // indicates the granularity at
                                                 // which the invalidation
                                                 // operation was performed
                                                 // through the CAIG
                                                 // field.[/p][p]Software must
                                                 // submit a context-cache
                                                 // invalidation request through
                                                 // this field only when there are
                                                 // no invalidation requests
                                                 // pending at this remapping
                                                 // hardware unit.[/p][p]Since
                                                 // information from the
                                                 // context-cache may be used by
                                                 // hardware to tag IOTLB entries,
                                                 // software must perform
                                                 // domain-selective (or global)
                                                 // invalidation of IOTLB after
                                                 // the context cache invalidation
                                                 // has completed.[/p][p]Hardware
                                                 // implementations reporting
                                                 // write-buffer flushing
                                                 // requirement (RWBF=1 in
                                                 // Capability register) must
                                                 // implicitly perform a write
                                                 // buffer flush before
                                                 // invalidating the context
                                                 // cache.[/p]

    }                                field;
    uint64_t                         val;
} CCMD_REG_0_0_0_VTDBAR_t;
#endif
#define CCMD_REG_0_0_0_VTDBAR_OFFSET 0x28
#define CCMD_REG_0_0_0_VTDBAR_SCOPE 0x01
#define CCMD_REG_0_0_0_VTDBAR_SIZE 64
#define CCMD_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x06
#define CCMD_REG_0_0_0_VTDBAR_RESET 0x800000000000000

#define CCMD_REG_0_0_0_VTDBAR_DID_LSB 0x0000
#define CCMD_REG_0_0_0_VTDBAR_DID_MSB 0x000f
#define CCMD_REG_0_0_0_VTDBAR_DID_RANGE 0x0010
#define CCMD_REG_0_0_0_VTDBAR_DID_MASK 0x0000ffff
#define CCMD_REG_0_0_0_VTDBAR_DID_RESET_VALUE 0x00000000

#define CCMD_REG_0_0_0_VTDBAR_SID_LSB 0x0010
#define CCMD_REG_0_0_0_VTDBAR_SID_MSB 0x001f
#define CCMD_REG_0_0_0_VTDBAR_SID_RANGE 0x0010
#define CCMD_REG_0_0_0_VTDBAR_SID_MASK 0xffff0000
#define CCMD_REG_0_0_0_VTDBAR_SID_RESET_VALUE 0x00000000

#define CCMD_REG_0_0_0_VTDBAR_FM_LSB 0x0020
#define CCMD_REG_0_0_0_VTDBAR_FM_MSB 0x0021
#define CCMD_REG_0_0_0_VTDBAR_FM_RANGE 0x0002
#define CCMD_REG_0_0_0_VTDBAR_FM_MASK 0x300000000
#define CCMD_REG_0_0_0_VTDBAR_FM_RESET_VALUE 0x00000000

#define CCMD_REG_0_0_0_VTDBAR_CAIG_LSB 0x003b
#define CCMD_REG_0_0_0_VTDBAR_CAIG_MSB 0x003c
#define CCMD_REG_0_0_0_VTDBAR_CAIG_RANGE 0x0002
#define CCMD_REG_0_0_0_VTDBAR_CAIG_MASK 0x1800000000000000
#define CCMD_REG_0_0_0_VTDBAR_CAIG_RESET_VALUE 0x00000001

#define CCMD_REG_0_0_0_VTDBAR_CIRG_LSB 0x003d
#define CCMD_REG_0_0_0_VTDBAR_CIRG_MSB 0x003e
#define CCMD_REG_0_0_0_VTDBAR_CIRG_RANGE 0x0002
#define CCMD_REG_0_0_0_VTDBAR_CIRG_MASK 0x6000000000000000
#define CCMD_REG_0_0_0_VTDBAR_CIRG_RESET_VALUE 0x00000000

#define CCMD_REG_0_0_0_VTDBAR_ICC_LSB 0x003f
#define CCMD_REG_0_0_0_VTDBAR_ICC_MSB 0x003f
#define CCMD_REG_0_0_0_VTDBAR_ICC_RANGE 0x0001
#define CCMD_REG_0_0_0_VTDBAR_ICC_MASK 0x8000000000000000
#define CCMD_REG_0_0_0_VTDBAR_ICC_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef FSTS_REG_0_0_0_VTDBAR_FLAG
#define FSTS_REG_0_0_0_VTDBAR_FLAG
// FSTS_REG_0_0_0_VTDBAR desc:  Register indicating the various error status.
typedef union {
    struct {
        uint32_t  PFO                  :   1;    //  [p]Hardware sets this field
                                                 // to indicate overflow of fault
                                                 // recording registers. Software
                                                 // writing 1 clears this field.
                                                 // When this field is Set,
                                                 // hardware does not record any
                                                 // new faults until software
                                                 // clears this field.[/p]
        uint32_t  PPF                  :   1;    //  [p]This field indicates if
                                                 // there are one or more pending
                                                 // faults logged in the fault
                                                 // recording registers. Hardware
                                                 // computes this field as the
                                                 // logical OR of Fault (F) fields
                                                 // across all the fault recording
                                                 // registers of this remapping
                                                 // hardware unit.[/p][list][*]0 =
                                                 // No pending faults in any of
                                                 // the fault recording
                                                 // registers.[*]1 = One or more
                                                 // fault recording registers has
                                                 // pending faults. The FRI field
                                                 // is updated by hardware
                                                 // whenever the PPF field is set
                                                 // by hardware. Also, depending
                                                 // on the programming of Fault
                                                 // Event Control register, a
                                                 // fault event is generated when
                                                 // hardware sets this
                                                 // field.[/list]
        uint32_t  AFO                  :   1;    //  [p]Hardware sets this field
                                                 // to indicate advanced fault log
                                                 // overflow condition. At this
                                                 // time, a fault event is
                                                 // generated based on the
                                                 // programming of the Fault Event
                                                 // Control
                                                 // register.[/p][p]Software
                                                 // writing 1 to this field clears
                                                 // it.[/p][p]Hardware
                                                 // implementations not supporting
                                                 // advanced fault logging
                                                 // implement this bit as
                                                 // RsvdZ.[/p]
        uint32_t  APF                  :   1;    //  [p]When this field is Clear,
                                                 // hardware sets this field when
                                                 // the first fault record (at
                                                 // index 0) is written to a fault
                                                 // log. At this time, a fault
                                                 // event is generated based on
                                                 // the programming of the Fault
                                                 // Event Control
                                                 // register.[/p][p]Software
                                                 // writing 1 to this field clears
                                                 // it. Hardware implementations
                                                 // not supporting advanced fault
                                                 // logging implement this bit as
                                                 // RsvdZ.[/p]
        uint32_t  IQE                  :   1;    //  [p]Hardware detected an error
                                                 // associated with the
                                                 // invalidation queue. This could
                                                 // be due to either a hardware
                                                 // error while fetching a
                                                 // descriptor from the
                                                 // invalidation queue, or
                                                 // hardware detecting an
                                                 // erroneous or invalid
                                                 // descriptor in the invalidation
                                                 // queue. At this time, a fault
                                                 // event may be generated based
                                                 // on the programming of the
                                                 // Fault Event Control
                                                 // register.[/p][p]Hardware
                                                 // implementations not supporting
                                                 // queued invalidations implement
                                                 // this bit as RsvdZ.[/p]
        uint32_t  ICE                  :   1;    //  [p]Hardware received an
                                                 // unexpected or invalid
                                                 // Device-IOTLB invalidation
                                                 // completion. This could be due
                                                 // to either an invalid ITag or
                                                 // invalid source-id in an
                                                 // invalidation completion
                                                 // response. At this time, a
                                                 // fault event may be generated
                                                 // based on the programming of
                                                 // hte Fault Event Control
                                                 // register.[/p][p]Hardware
                                                 // implementations not supporting
                                                 // Device-IOTLBs implement this
                                                 // bit as RsvdZ.[/p]
        uint32_t  ITE                  :   1;    //  [p]Hardware detected a
                                                 // Device-IOTLB invalidation
                                                 // completion time-out. At this
                                                 // time, a fault event may be
                                                 // generated based on the
                                                 // programming of the Fault Event
                                                 // Control
                                                 // register.[/p][p]Hardware
                                                 // implementations not supporting
                                                 // device Device-IOTLBs implement
                                                 // this bit as RsvdZ.[/p]
        uint32_t  PRO                  :   1;    //  [p]Hardware detected a Page
                                                 // Request Overflow error.
                                                 // Hardware implementations not
                                                 // supporting the Page Request
                                                 // Queue implement this bit as
                                                 // RsvdZ.[/p]
        uint32_t  FRI                  :   8;    //  [p]This field is valid only
                                                 // when the PPF field is
                                                 // Set.[/p][p]The FRI field
                                                 // indicates the index (from
                                                 // base) of the fault recording
                                                 // register to which the first
                                                 // pending fault was recorded
                                                 // when the PPF field was Set by
                                                 // hardware.[/p][p]The value read
                                                 // from this field is undefined
                                                 // when the PPF field is
                                                 // clear.[/p]
        uint32_t  RSVD_0               :  16;    // Nebulon auto filled RSVD [31:16]

    }                                field;
    uint32_t                         val;
} FSTS_REG_0_0_0_VTDBAR_t;
#endif
#define FSTS_REG_0_0_0_VTDBAR_OFFSET 0x34
#define FSTS_REG_0_0_0_VTDBAR_SCOPE 0x01
#define FSTS_REG_0_0_0_VTDBAR_SIZE 32
#define FSTS_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x09
#define FSTS_REG_0_0_0_VTDBAR_RESET 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_PFO_LSB 0x0000
#define FSTS_REG_0_0_0_VTDBAR_PFO_MSB 0x0000
#define FSTS_REG_0_0_0_VTDBAR_PFO_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_PFO_MASK 0x00000001
#define FSTS_REG_0_0_0_VTDBAR_PFO_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_PPF_LSB 0x0001
#define FSTS_REG_0_0_0_VTDBAR_PPF_MSB 0x0001
#define FSTS_REG_0_0_0_VTDBAR_PPF_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_PPF_MASK 0x00000002
#define FSTS_REG_0_0_0_VTDBAR_PPF_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_AFO_LSB 0x0002
#define FSTS_REG_0_0_0_VTDBAR_AFO_MSB 0x0002
#define FSTS_REG_0_0_0_VTDBAR_AFO_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_AFO_MASK 0x00000004
#define FSTS_REG_0_0_0_VTDBAR_AFO_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_APF_LSB 0x0003
#define FSTS_REG_0_0_0_VTDBAR_APF_MSB 0x0003
#define FSTS_REG_0_0_0_VTDBAR_APF_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_APF_MASK 0x00000008
#define FSTS_REG_0_0_0_VTDBAR_APF_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_IQE_LSB 0x0004
#define FSTS_REG_0_0_0_VTDBAR_IQE_MSB 0x0004
#define FSTS_REG_0_0_0_VTDBAR_IQE_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_IQE_MASK 0x00000010
#define FSTS_REG_0_0_0_VTDBAR_IQE_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_ICE_LSB 0x0005
#define FSTS_REG_0_0_0_VTDBAR_ICE_MSB 0x0005
#define FSTS_REG_0_0_0_VTDBAR_ICE_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_ICE_MASK 0x00000020
#define FSTS_REG_0_0_0_VTDBAR_ICE_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_ITE_LSB 0x0006
#define FSTS_REG_0_0_0_VTDBAR_ITE_MSB 0x0006
#define FSTS_REG_0_0_0_VTDBAR_ITE_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_ITE_MASK 0x00000040
#define FSTS_REG_0_0_0_VTDBAR_ITE_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_PRO_LSB 0x0007
#define FSTS_REG_0_0_0_VTDBAR_PRO_MSB 0x0007
#define FSTS_REG_0_0_0_VTDBAR_PRO_RANGE 0x0001
#define FSTS_REG_0_0_0_VTDBAR_PRO_MASK 0x00000080
#define FSTS_REG_0_0_0_VTDBAR_PRO_RESET_VALUE 0x00000000

#define FSTS_REG_0_0_0_VTDBAR_FRI_LSB 0x0008
#define FSTS_REG_0_0_0_VTDBAR_FRI_MSB 0x000f
#define FSTS_REG_0_0_0_VTDBAR_FRI_RANGE 0x0008
#define FSTS_REG_0_0_0_VTDBAR_FRI_MASK 0x0000ff00
#define FSTS_REG_0_0_0_VTDBAR_FRI_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef FECTL_REG_0_0_0_VTDBAR_FLAG
#define FECTL_REG_0_0_0_VTDBAR_FLAG
// FECTL_REG_0_0_0_VTDBAR desc:  Register specifying the fault event interrupt message control bits.
typedef union {
    struct {
        uint32_t  RSVD_0               : 30;    // Nebulon auto filled RSVD [0:29]
        uint32_t  IP                   :   1;    //  [p]Hardware sets the IP field
                                                 // whenever it detects an
                                                 // interrupt condition, which is
                                                 // defined as:[/p][list][*]When
                                                 // primary fault logging is
                                                 // active, an interrupt condition
                                                 // occurs when hardware records a
                                                 // fault through one of the Fault
                                                 // Recording registers and sets
                                                 // the PPF field in Fault Status
                                                 // register.[*]When advanced
                                                 // fault logging is active, an
                                                 // interrupt condition occurs
                                                 // when hardware records a fault
                                                 // in the first fault record (at
                                                 // index 0) of the current fault
                                                 // log and sets the APF field in
                                                 // the Fault Status
                                                 // register.[*]Hardware detected
                                                 // error associated with the
                                                 // Invalidation Queue, setting
                                                 // the IQE field in the Fault
                                                 // Status register.[*]Hardware
                                                 // detected invalid Device-IOTLB
                                                 // invalidation completion,
                                                 // setting the ICE field in the
                                                 // Fault Status
                                                 // register.[*]Hardware detected
                                                 // Device-IOTLB invalidation
                                                 // completion time-out, setting
                                                 // the ITE field in the Fault
                                                 // Status register.[/list][p]If
                                                 // any of the status fields in
                                                 // the Fault Status register was
                                                 // already Set at the time of
                                                 // setting any of these fields,
                                                 // it is not treated as a new
                                                 // interrupt condition.[/p][p]The
                                                 // IP field is kept set by
                                                 // hardware while the interrupt
                                                 // message is held pending. The
                                                 // interrupt message could be
                                                 // held pending due to interrupt
                                                 // mask (IM field) being Set or
                                                 // other transient hardware
                                                 // conditions.[/p][p]The IP field
                                                 // is cleared by hardware as soon
                                                 // as the interrupt message
                                                 // pending condition is serviced.
                                                 // This could be due to
                                                 // either:[/p][list][*]Hardware
                                                 // issuing the interrupt message
                                                 // due to either change in the
                                                 // transient hardware condition
                                                 // that caused interrupt message
                                                 // to be held pending, or due to
                                                 // software clearing the IM
                                                 // field.[*]Software servicing
                                                 // all the pending interrupt
                                                 // status fields in the Fault
                                                 // Status register as
                                                 // follows:[list][*]When primary
                                                 // fault logging is active,
                                                 // software clearing the Fault
                                                 // (F) field in all the Fault
                                                 // Recording registers with
                                                 // faults, causing the PPF field
                                                 // in Fault Status register to be
                                                 // evaluated as clear.[*]Software
                                                 // clearing other status fields
                                                 // in the Fault Status register
                                                 // by writing back the value read
                                                 // from the respective
                                                 // fields.[/list][/list]
        uint32_t  IM                   :   1;    //  [list][*]0 = No masking of
                                                 // interrupt. When an interrupt
                                                 // condition is detected,
                                                 // hardware issues an interrupt
                                                 // message (using the Fault Event
                                                 // Data and Fault Event Address
                                                 // register values).[*]1 = This
                                                 // is the value on reset.
                                                 // Software may mask interrupt
                                                 // message generation by setting
                                                 // this field. Hardware is
                                                 // prohibited from sending the
                                                 // interrupt message when this
                                                 // field is set.[/list]

    }                                field;
    uint32_t                         val;
} FECTL_REG_0_0_0_VTDBAR_t;
#endif
#define FECTL_REG_0_0_0_VTDBAR_OFFSET 0x38
#define FECTL_REG_0_0_0_VTDBAR_SCOPE 0x01
#define FECTL_REG_0_0_0_VTDBAR_SIZE 32
#define FECTL_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define FECTL_REG_0_0_0_VTDBAR_RESET 0x80000000

#define FECTL_REG_0_0_0_VTDBAR_IP_LSB 0x001e
#define FECTL_REG_0_0_0_VTDBAR_IP_MSB 0x001e
#define FECTL_REG_0_0_0_VTDBAR_IP_RANGE 0x0001
#define FECTL_REG_0_0_0_VTDBAR_IP_MASK 0x40000000
#define FECTL_REG_0_0_0_VTDBAR_IP_RESET_VALUE 0x00000000

#define FECTL_REG_0_0_0_VTDBAR_IM_LSB 0x001f
#define FECTL_REG_0_0_0_VTDBAR_IM_MSB 0x001f
#define FECTL_REG_0_0_0_VTDBAR_IM_RANGE 0x0001
#define FECTL_REG_0_0_0_VTDBAR_IM_MASK 0x80000000
#define FECTL_REG_0_0_0_VTDBAR_IM_RESET_VALUE 0x00000001


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef FEDATA_REG_0_0_0_VTDBAR_FLAG
#define FEDATA_REG_0_0_0_VTDBAR_FLAG
// FEDATA_REG_0_0_0_VTDBAR desc:  Register specifying the interrupt message data
typedef union {
    struct {
        uint32_t  IMD                  :  16;    //  [p]Data value in the
                                                 // interrupt request.[/p]
        uint32_t  EIMD                 :  16;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // 32-bit interrupt data
                                                 // fields.[/p][p]Hardware
                                                 // implementations supporting
                                                 // only 16-bit interrupt data may
                                                 // treat this field as RsvdZ.[/p]

    }                                field;
    uint32_t                         val;
} FEDATA_REG_0_0_0_VTDBAR_t;
#endif
#define FEDATA_REG_0_0_0_VTDBAR_OFFSET 0x3c
#define FEDATA_REG_0_0_0_VTDBAR_SCOPE 0x01
#define FEDATA_REG_0_0_0_VTDBAR_SIZE 32
#define FEDATA_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define FEDATA_REG_0_0_0_VTDBAR_RESET 0x00000000

#define FEDATA_REG_0_0_0_VTDBAR_IMD_LSB 0x0000
#define FEDATA_REG_0_0_0_VTDBAR_IMD_MSB 0x000f
#define FEDATA_REG_0_0_0_VTDBAR_IMD_RANGE 0x0010
#define FEDATA_REG_0_0_0_VTDBAR_IMD_MASK 0x0000ffff
#define FEDATA_REG_0_0_0_VTDBAR_IMD_RESET_VALUE 0x00000000

#define FEDATA_REG_0_0_0_VTDBAR_EIMD_LSB 0x0010
#define FEDATA_REG_0_0_0_VTDBAR_EIMD_MSB 0x001f
#define FEDATA_REG_0_0_0_VTDBAR_EIMD_RANGE 0x0010
#define FEDATA_REG_0_0_0_VTDBAR_EIMD_MASK 0xffff0000
#define FEDATA_REG_0_0_0_VTDBAR_EIMD_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef FEADDR_REG_0_0_0_VTDBAR_FLAG
#define FEADDR_REG_0_0_0_VTDBAR_FLAG
// FEADDR_REG_0_0_0_VTDBAR desc:  Register specifying the interrupt message address.
typedef union {
    struct {
        uint32_t  RSVD_0               : 2;    // Nebulon auto filled RSVD [0:1]
        uint32_t  MA                   :  30;    //  [p]When fault events are
                                                 // enabled, the contents of this
                                                 // register specify the
                                                 // DWORD-aligned address (bits
                                                 // 31:2) for the interrupt
                                                 // request.[/p]

    }                                field;
    uint32_t                         val;
} FEADDR_REG_0_0_0_VTDBAR_t;
#endif
#define FEADDR_REG_0_0_0_VTDBAR_OFFSET 0x40
#define FEADDR_REG_0_0_0_VTDBAR_SCOPE 0x01
#define FEADDR_REG_0_0_0_VTDBAR_SIZE 32
#define FEADDR_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define FEADDR_REG_0_0_0_VTDBAR_RESET 0x00000000

#define FEADDR_REG_0_0_0_VTDBAR_MA_LSB 0x0002
#define FEADDR_REG_0_0_0_VTDBAR_MA_MSB 0x001f
#define FEADDR_REG_0_0_0_VTDBAR_MA_RANGE 0x001e
#define FEADDR_REG_0_0_0_VTDBAR_MA_MASK 0xfffffffc
#define FEADDR_REG_0_0_0_VTDBAR_MA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef FEUADDR_REG_0_0_0_VTDBAR_FLAG
#define FEUADDR_REG_0_0_0_VTDBAR_FLAG
// FEUADDR_REG_0_0_0_VTDBAR desc:  Register specifying the interrupt message upper address.
typedef union {
    struct {
        uint32_t  MUA                  :  32;    //  [p]Hardware implementations
                                                 // supporting Extended Interrupt
                                                 // Mode are required to implement
                                                 // this register.[/p][p]Hardware
                                                 // implementations not supporting
                                                 // Extended Interrupt Mode may
                                                 // treat this field as RsvdZ.[/p]

    }                                field;
    uint32_t                         val;
} FEUADDR_REG_0_0_0_VTDBAR_t;
#endif
#define FEUADDR_REG_0_0_0_VTDBAR_OFFSET 0x44
#define FEUADDR_REG_0_0_0_VTDBAR_SCOPE 0x01
#define FEUADDR_REG_0_0_0_VTDBAR_SIZE 32
#define FEUADDR_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define FEUADDR_REG_0_0_0_VTDBAR_RESET 0x00000000

#define FEUADDR_REG_0_0_0_VTDBAR_MUA_LSB 0x0000
#define FEUADDR_REG_0_0_0_VTDBAR_MUA_MSB 0x001f
#define FEUADDR_REG_0_0_0_VTDBAR_MUA_RANGE 0x0020
#define FEUADDR_REG_0_0_0_VTDBAR_MUA_MASK 0xffffffff
#define FEUADDR_REG_0_0_0_VTDBAR_MUA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef AFLOG_REG_0_0_0_VTDBAR_FLAG
#define AFLOG_REG_0_0_0_VTDBAR_FLAG
// AFLOG_REG_0_0_0_VTDBAR desc:  Register to specify the base address of the memory-resident fault-log
// region. This register is treated as RsvdZ for implementations not
// supporting advanced translation fault logging (AFL field reported as 0
// in the Capability register).
typedef union {
    struct {
        uint64_t  RSVD_0               : 9;    // Nebulon auto filled RSVD [0:8]
        uint64_t  FLS                  :   3;    //  [p]This field specifies the
                                                 // size of the fault log region
                                                 // pointed by the FLA field. The
                                                 // size of the fault log region
                                                 // is 2X * 4KB, where X is the
                                                 // value programmed in this
                                                 // register.[/p][p]When
                                                 // implemented, reads of this
                                                 // field return the value that
                                                 // was last programmed to it.[/p]
        uint64_t  FLA                  :  52;    //  [p]This field specifies the
                                                 // base of 4KB aligned fault-log
                                                 // region in system memory.
                                                 // Hardware ignores and does not
                                                 // implement bits 63:HAW, where
                                                 // HAW is the host address width.
                                                 // [/p][p]Software specifies the
                                                 // base address and size of the
                                                 // fault log region through this
                                                 // register, and programs it in
                                                 // hardware through the SFL field
                                                 // in the Global Command
                                                 // register. When implemented,
                                                 // reads of this field return the
                                                 // value that was last programmed
                                                 // to it.[/p]

    }                                field;
    uint64_t                         val;
} AFLOG_REG_0_0_0_VTDBAR_t;
#endif
#define AFLOG_REG_0_0_0_VTDBAR_OFFSET 0x58
#define AFLOG_REG_0_0_0_VTDBAR_SCOPE 0x01
#define AFLOG_REG_0_0_0_VTDBAR_SIZE 64
#define AFLOG_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define AFLOG_REG_0_0_0_VTDBAR_RESET 0x00000000

#define AFLOG_REG_0_0_0_VTDBAR_FLS_LSB 0x0009
#define AFLOG_REG_0_0_0_VTDBAR_FLS_MSB 0x000b
#define AFLOG_REG_0_0_0_VTDBAR_FLS_RANGE 0x0003
#define AFLOG_REG_0_0_0_VTDBAR_FLS_MASK 0x00000e00
#define AFLOG_REG_0_0_0_VTDBAR_FLS_RESET_VALUE 0x00000000

#define AFLOG_REG_0_0_0_VTDBAR_FLA_LSB 0x000c
#define AFLOG_REG_0_0_0_VTDBAR_FLA_MSB 0x003f
#define AFLOG_REG_0_0_0_VTDBAR_FLA_RANGE 0x0034
#define AFLOG_REG_0_0_0_VTDBAR_FLA_MASK 0xfffffffffffff000
#define AFLOG_REG_0_0_0_VTDBAR_FLA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PMEN_REG_0_0_0_VTDBAR_FLAG
#define PMEN_REG_0_0_0_VTDBAR_FLAG
// PMEN_REG_0_0_0_VTDBAR desc:  [p]Register to enable the DMA-protected memory regions setup through
// the PLMBASE,..PLMLIMT, PHMBASE, PHMLIMIT registers. This register is
// always treated as RO for implementations not supporting protected
// memory regions (PLMR and PHMR fields reported as Clear in the
// Capability register).[/p][p]Protected memory regions may be used by
// software to securely initialize remapping structures in memory. To
// avoid impact to legacy BIOS usage of memory, software is recommended
// to not overlap protected memory regions with any reserved memory
// regions of the platform reported through the Reserved Memory Region
// Reporting (RMRR) structures.[/p]
typedef union {
    struct {
        uint32_t  PRS                  :   1;    //  [p]This field indicates the
                                                 // status of protected memory
                                                 // region(s):[/p][list][*]0 =
                                                 // Protected memory region(s)
                                                 // disabled.[*]1 = Protected
                                                 // memory region(s)
                                                 // enabled.[/list]
        uint32_t  RSVD_0               :  30;    // Nebulon auto filled RSVD [30:1]
        uint32_t  EPM                  :   1;    //  [p]This field controls DMA
                                                 // accesses to the protected
                                                 // low-memory and protected
                                                 // high-memory
                                                 // regions.[/p][list][*]0 =
                                                 // Protected memory regions are
                                                 // disabled. [*]1 = Protected
                                                 // memory regions are enabled.DMA
                                                 // requests accessing protected
                                                 // memory regions are handled as
                                                 // follows:[list][*]When DMA
                                                 // remapping is not enabled, all
                                                 // DMA requests accessing
                                                 // protected memory regions are
                                                 // blocked.[*]When DMA remapping
                                                 // is enabled:[list][*]DMA
                                                 // requests processed as
                                                 // pass-through (Translation Type
                                                 // value of 10b in Context-Entry)
                                                 // and accessing the protected
                                                 // memory regions are
                                                 // blocked.[*]DMA requests with
                                                 // translated address (AT=10b)
                                                 // and accessing the protected
                                                 // memory regions are
                                                 // blocked.[*]DMA requests that
                                                 // are subject to address
                                                 // remapping, and accessing the
                                                 // protected memory regions may
                                                 // or may not be blocked by
                                                 // hardware. For such requests,
                                                 // software must not depend on
                                                 // hardware protection of the
                                                 // protected memory regions, and
                                                 // instead program the
                                                 // DMA-remapping page-tables to
                                                 // not allow DMA to protected
                                                 // memory
                                                 // regions.[/list][/list][/list][p]Remapping
                                                 // hardware access to the
                                                 // remapping structures are not
                                                 // subject to protected memory
                                                 // region checks.[/p][p]DMA
                                                 // requests blocked due to
                                                 // protected memory region
                                                 // violation are not recorded or
                                                 // reported as remapping
                                                 // faults.[/p][p]Hardware reports
                                                 // the status of the protected
                                                 // memory enable/disable
                                                 // operation through the PRS
                                                 // field in this
                                                 // register.Hardware
                                                 // implementations supporting DMA
                                                 // draining must drain any
                                                 // in-flight translated DMA
                                                 // requests queued within the
                                                 // Root-Complex before indicating
                                                 // the protected memory region as
                                                 // enabled through the PRS
                                                 // field.[/p]

    }                                field;
    uint32_t                         val;
} PMEN_REG_0_0_0_VTDBAR_t;
#endif
#define PMEN_REG_0_0_0_VTDBAR_OFFSET 0x64
#define PMEN_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PMEN_REG_0_0_0_VTDBAR_SIZE 32
#define PMEN_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define PMEN_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PMEN_REG_0_0_0_VTDBAR_PRS_LSB 0x0000
#define PMEN_REG_0_0_0_VTDBAR_PRS_MSB 0x0000
#define PMEN_REG_0_0_0_VTDBAR_PRS_RANGE 0x0001
#define PMEN_REG_0_0_0_VTDBAR_PRS_MASK 0x00000001
#define PMEN_REG_0_0_0_VTDBAR_PRS_RESET_VALUE 0x00000000

#define PMEN_REG_0_0_0_VTDBAR_EPM_LSB 0x001f
#define PMEN_REG_0_0_0_VTDBAR_EPM_MSB 0x001f
#define PMEN_REG_0_0_0_VTDBAR_EPM_RANGE 0x0001
#define PMEN_REG_0_0_0_VTDBAR_EPM_MASK 0x80000000
#define PMEN_REG_0_0_0_VTDBAR_EPM_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PLMBASE_REG_0_0_0_VTDBAR_FLAG
#define PLMBASE_REG_0_0_0_VTDBAR_FLAG
// PLMBASE_REG_0_0_0_VTDBAR desc:  [p]Register to set up the base address of DMA-protected low-memory
// region below 4GB. This register must be set up before enabling
// protected memory through PMEN_REG, and must not be updated when
// protected memory regions are enabled.[/p][p]This register is always
// treated as RO for implementations not supporting protected low memory
// region (PLMR field reported as Clear in the Capability
// register).[/p][p]The alignment of the protected low memory region base
// depends on the number of reserved bits (N:0) of this register.
// Software may determine N by writing all 1s to this register, and
// finding the most significant zero bit position with 0 in the value
// read back from the register. Bits N:0 of this register is decoded by
// hardware as all 0s...Software must setup the protected low memory
// region below 4GB.[/p][p]Software must not modify this register when
// protected memory regions are enabled (PRS field Set in PMEN_REG).[/p]
typedef union {
    struct {
        uint32_t  RSVD_0               : 20;    // Nebulon auto filled RSVD [0:19]
        uint32_t  PLMB                 :  12;    //  [p]This register specifies
                                                 // the base of protected
                                                 // low-memory region in system
                                                 // memory.[/p]

    }                                field;
    uint32_t                         val;
} PLMBASE_REG_0_0_0_VTDBAR_t;
#endif
#define PLMBASE_REG_0_0_0_VTDBAR_OFFSET 0x68
#define PLMBASE_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PLMBASE_REG_0_0_0_VTDBAR_SIZE 32
#define PLMBASE_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define PLMBASE_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PLMBASE_REG_0_0_0_VTDBAR_PLMB_LSB 0x0014
#define PLMBASE_REG_0_0_0_VTDBAR_PLMB_MSB 0x001f
#define PLMBASE_REG_0_0_0_VTDBAR_PLMB_RANGE 0x000c
#define PLMBASE_REG_0_0_0_VTDBAR_PLMB_MASK 0xfff00000
#define PLMBASE_REG_0_0_0_VTDBAR_PLMB_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PLMLIMIT_REG_0_0_0_VTDBAR_FLAG
#define PLMLIMIT_REG_0_0_0_VTDBAR_FLAG
// PLMLIMIT_REG_0_0_0_VTDBAR desc:  [p]Register to set up the limit address of DMA-protected low-memory
// region below 4GB. This register must be set up before enabling
// protected memory through PMEN_REG, and must not be updated when
// protected memory regions are enabled[/p][p]This register is always
// treated as RO for implementations not supporting protected low memory
// region (PLMR field reported as Clear in the Capability
// register)[/p][p]The alignment of the protected low memory region limit
// depends on the number of reserved bits (N:0) of this register.
// Software may determine N by writing all 1s to this register, and
// finding most significant zero bit position with 0 in the value read
// back from the register. Bits N:0 of the limit register is decoded by
// hardware as all 1s[/p][p]The Protected low-memory base and limit
// registers functions as follows:[/p][list][*]Programming the protected
// low-memory base and limit registers with the same value in bits 31:
// (N+1) specifies a protected low-memory region of size 2(N+1)
// bytes[*]Programming the protected low-memory limit register with a
// value less than the protected low-memory base register disables the
// protected low-memory region[/list][p]Software must not modify this
// register when protected memory regions are enabled (PRS field Set in
// PMEN_REG).[/p]
typedef union {
    struct {
        uint32_t  RSVD_0               : 20;    // Nebulon auto filled RSVD [0:19]
        uint32_t  PLML                 :  12;    //  [p]This register specifies
                                                 // the last host physical address
                                                 // of the DMA-protected
                                                 // low-memory region in system
                                                 // memory.[/p]

    }                                field;
    uint32_t                         val;
} PLMLIMIT_REG_0_0_0_VTDBAR_t;
#endif
#define PLMLIMIT_REG_0_0_0_VTDBAR_OFFSET 0x6c
#define PLMLIMIT_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PLMLIMIT_REG_0_0_0_VTDBAR_SIZE 32
#define PLMLIMIT_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define PLMLIMIT_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PLMLIMIT_REG_0_0_0_VTDBAR_PLML_LSB 0x0014
#define PLMLIMIT_REG_0_0_0_VTDBAR_PLML_MSB 0x001f
#define PLMLIMIT_REG_0_0_0_VTDBAR_PLML_RANGE 0x000c
#define PLMLIMIT_REG_0_0_0_VTDBAR_PLML_MASK 0xfff00000
#define PLMLIMIT_REG_0_0_0_VTDBAR_PLML_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PHMBASE_REG_0_0_0_VTDBAR_FLAG
#define PHMBASE_REG_0_0_0_VTDBAR_FLAG
// PHMBASE_REG_0_0_0_VTDBAR desc:  [p]Register to set up the base address of DMA-protected high-memory
// region. This register must be set up before enabling protected memory
// through PMEN_REG, and must not be updated when protected memory
// regions are enabled[/p][p]This register is always treated as RO for
// implementations not supporting protected high memory region (PHMR
// field reported as Clear in the Capability register)[/p][p]The
// alignment of the protected high memory region base depends on the
// number of reserved bits (N:0) of this register. Software may determine
// N by writing all 1s to this register, and finding most significant
// zero bit position below host address width (HAW) in the value read
// back from the register. Bits N:0 of this register are decoded by
// hardware as all 0s[/p][p]Software may setup the protected high memory
// region either above or below 4GB[/p][p]Software must not modify this
// register when protected memory regions are enabled (PRS field Set in
// PMEN_REG).[/p]
typedef union {
    struct {
        uint64_t  RSVD_0               : 20;    // Nebulon auto filled RSVD [0:19]
        uint64_t  PHMB                 :  19;    //  [p]This register specifies
                                                 // the base of protected (high)
                                                 // memory region in system
                                                 // memory[/p][p]Hardware ignores,
                                                 // and does not implement, bits
                                                 // 63:HAW, where HAW is the host
                                                 // address width.[/p]
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} PHMBASE_REG_0_0_0_VTDBAR_t;
#endif
#define PHMBASE_REG_0_0_0_VTDBAR_OFFSET 0x70
#define PHMBASE_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PHMBASE_REG_0_0_0_VTDBAR_SIZE 64
#define PHMBASE_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define PHMBASE_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PHMBASE_REG_0_0_0_VTDBAR_PHMB_LSB 0x0014
#define PHMBASE_REG_0_0_0_VTDBAR_PHMB_MSB 0x0026
#define PHMBASE_REG_0_0_0_VTDBAR_PHMB_RANGE 0x0013
#define PHMBASE_REG_0_0_0_VTDBAR_PHMB_MASK 0x7ffff00000
#define PHMBASE_REG_0_0_0_VTDBAR_PHMB_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PHMLIMIT_REG_0_0_0_VTDBAR_FLAG
#define PHMLIMIT_REG_0_0_0_VTDBAR_FLAG
// PHMLIMIT_REG_0_0_0_VTDBAR desc:  [p]Register to set up the limit address of DMA-protected high-memory
// region. This register must be set up before enabling protected memory
// through PMEN_REG, and must not be updated when protected memory
// regions are enabled[/p][p]This register is always treated as RO for
// implementations not supporting protected high memory region (PHMR
// field reported as Clear in the Capability register)[/p][p]The
// alignment of the protected high memory region limit depends on the
// number of reserved bits (N:0) of this register. Software may determine
// the value of N by writing all 1s to this register, and finding most
// significant zero bit position below host address width (HAW) in the
// value read back from the register. Bits N:0 of the limit register is
// decoded by hardware as all 1s[/p][p]The protected high-memory base &
// limit registers functions as follows[/p][list][*] Programming the
// protected low-memory base and limit registers with the same value in
// bits HAW:(N+1) specifies a protected low-memory region of size 2(N+1)
// bytes[*]Programming the protected high-memory limit register with a
// value less than the protected high-memory base register disables the
// protected high-memory region[/list][p]Software must not modify this
// register when protected memory regions are enabled (PRS field Set in
// PMEN_REG).[/p]
typedef union {
    struct {
        uint64_t  RSVD_0               : 20;    // Nebulon auto filled RSVD [0:19]
        uint64_t  PHML                 :  19;    //  [p]This register specifies
                                                 // the last host physical address
                                                 // of the DMA-protected
                                                 // high-memory region in system
                                                 // memory[/p][p]Hardware ignores
                                                 // and does not implement bits
                                                 // 63:HAW, where HAW is the host
                                                 // address width.[/p]
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} PHMLIMIT_REG_0_0_0_VTDBAR_t;
#endif
#define PHMLIMIT_REG_0_0_0_VTDBAR_OFFSET 0x78
#define PHMLIMIT_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PHMLIMIT_REG_0_0_0_VTDBAR_SIZE 64
#define PHMLIMIT_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define PHMLIMIT_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PHMLIMIT_REG_0_0_0_VTDBAR_PHML_LSB 0x0014
#define PHMLIMIT_REG_0_0_0_VTDBAR_PHML_MSB 0x0026
#define PHMLIMIT_REG_0_0_0_VTDBAR_PHML_RANGE 0x0013
#define PHMLIMIT_REG_0_0_0_VTDBAR_PHML_MASK 0x7ffff00000
#define PHMLIMIT_REG_0_0_0_VTDBAR_PHML_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IQH_REG_0_0_0_VTDBAR_FLAG
#define IQH_REG_0_0_0_VTDBAR_FLAG
// IQH_REG_0_0_0_VTDBAR desc:  [p]Register indicating the invalidation queue head. This register is
// treated as RsvdZ by implementations reporting Queued Invalidation (QI)
// as not supported in the Extended Capability register.[/p]
typedef union {
    struct {
        uint64_t  RSVD_0               : 4;    // Nebulon auto filled RSVD [0:3]
        uint64_t  QH                   :  15;    //  [p]Specifies the offset
                                                 // (128-bit aligned) to the
                                                 // invalidation queue for the
                                                 // command that will be fetched
                                                 // next by
                                                 // hardware[/p][p]Hardware resets
                                                 // this field to 0 whenever the
                                                 // queued invalidation is
                                                 // disabled (QIES field Clear in
                                                 // the Global Status
                                                 // register).[/p]
        uint64_t  RSVD_1               :  45;    // Nebulon auto filled RSVD [63:19]

    }                                field;
    uint64_t                         val;
} IQH_REG_0_0_0_VTDBAR_t;
#endif
#define IQH_REG_0_0_0_VTDBAR_OFFSET 0x80
#define IQH_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IQH_REG_0_0_0_VTDBAR_SIZE 64
#define IQH_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define IQH_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IQH_REG_0_0_0_VTDBAR_QH_LSB 0x0004
#define IQH_REG_0_0_0_VTDBAR_QH_MSB 0x0012
#define IQH_REG_0_0_0_VTDBAR_QH_RANGE 0x000f
#define IQH_REG_0_0_0_VTDBAR_QH_MASK 0x0007fff0
#define IQH_REG_0_0_0_VTDBAR_QH_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IQT_REG_0_0_0_VTDBAR_FLAG
#define IQT_REG_0_0_0_VTDBAR_FLAG
// IQT_REG_0_0_0_VTDBAR desc:  [p]Register indicating the invalidation tail head. This register is
// treated as RsvdZ by implementations reporting Queued Invalidation (QI)
// as not supported in the Extended Capability register.[/p]
typedef union {
    struct {
        uint64_t  RSVD_0               : 4;    // Nebulon auto filled RSVD [0:3]
        uint64_t  QT                   :  15;    //  [p]Specifies the offset
                                                 // (128-bit aligned) to the
                                                 // invalidation queue for the
                                                 // command that will be written
                                                 // next by software.[/p]
        uint64_t  RSVD_1               :  45;    // Nebulon auto filled RSVD [63:19]

    }                                field;
    uint64_t                         val;
} IQT_REG_0_0_0_VTDBAR_t;
#endif
#define IQT_REG_0_0_0_VTDBAR_OFFSET 0x88
#define IQT_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IQT_REG_0_0_0_VTDBAR_SIZE 64
#define IQT_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define IQT_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IQT_REG_0_0_0_VTDBAR_QT_LSB 0x0004
#define IQT_REG_0_0_0_VTDBAR_QT_MSB 0x0012
#define IQT_REG_0_0_0_VTDBAR_QT_RANGE 0x000f
#define IQT_REG_0_0_0_VTDBAR_QT_MASK 0x0007fff0
#define IQT_REG_0_0_0_VTDBAR_QT_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IQA_REG_0_0_0_VTDBAR_FLAG
#define IQA_REG_0_0_0_VTDBAR_FLAG
// IQA_REG_0_0_0_VTDBAR desc:  [p]Register to configure the base address and size of the
// invalidation queue. This register is treated as RsvdZ by
// implementations reporting Queued Invalidation (QI) as not supported in
// the Extended Capability register.[/p]
typedef union {
    struct {
        uint64_t  QS                   :   3;    //  [p]This field specifies the
                                                 // size of the invalidation
                                                 // request queue. A value of X in
                                                 // this field indicates an
                                                 // invalidation request queue of
                                                 // (2X) 4KB pages. The number of
                                                 // entries in the invalidation
                                                 // queue is 2(X + 8).[/p]
        uint64_t  RSVD_0               :   9;    // Nebulon auto filled RSVD [11:3]
        uint64_t  IQA                  :  27;    //  [p]This field points to the
                                                 // base of 4KB aligned
                                                 // invalidation request queue.
                                                 // Hardware ignores and does not
                                                 // implement bits 63:HAW, where
                                                 // HAW is the host address
                                                 // width[/p][p]Reads of this
                                                 // field return the value that
                                                 // was last programmed to it.[/p]
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} IQA_REG_0_0_0_VTDBAR_t;
#endif
#define IQA_REG_0_0_0_VTDBAR_OFFSET 0x90
#define IQA_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IQA_REG_0_0_0_VTDBAR_SIZE 64
#define IQA_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define IQA_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IQA_REG_0_0_0_VTDBAR_QS_LSB 0x0000
#define IQA_REG_0_0_0_VTDBAR_QS_MSB 0x0002
#define IQA_REG_0_0_0_VTDBAR_QS_RANGE 0x0003
#define IQA_REG_0_0_0_VTDBAR_QS_MASK 0x00000007
#define IQA_REG_0_0_0_VTDBAR_QS_RESET_VALUE 0x00000000

#define IQA_REG_0_0_0_VTDBAR_IQA_LSB 0x000c
#define IQA_REG_0_0_0_VTDBAR_IQA_MSB 0x0026
#define IQA_REG_0_0_0_VTDBAR_IQA_RANGE 0x001b
#define IQA_REG_0_0_0_VTDBAR_IQA_MASK 0x7ffffff000
#define IQA_REG_0_0_0_VTDBAR_IQA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef ICS_REG_0_0_0_VTDBAR_FLAG
#define ICS_REG_0_0_0_VTDBAR_FLAG
// ICS_REG_0_0_0_VTDBAR desc:  [p]Register to report completion status of invalidation wait
// descriptor with Interrupt Flag (IF) Set[/p][p]This register is treated
// as RsvdZ by implementations reporting Queued Invalidation (QI) as not
// supported in the Extended Capability register.[/p]
typedef union {
    struct {
        uint32_t  IWC                  :   1;    //  [p]Indicates completion of
                                                 // Invalidation Wait Descriptor
                                                 // with Interrupt Flag (IF) field
                                                 // Set. Hardware implementations
                                                 // not supporting queued
                                                 // invalidations implement this
                                                 // field as RsvdZ.[/p]
        uint32_t  RSVD_0               :  31;    // Nebulon auto filled RSVD [31:1]

    }                                field;
    uint32_t                         val;
} ICS_REG_0_0_0_VTDBAR_t;
#endif
#define ICS_REG_0_0_0_VTDBAR_OFFSET 0x9c
#define ICS_REG_0_0_0_VTDBAR_SCOPE 0x01
#define ICS_REG_0_0_0_VTDBAR_SIZE 32
#define ICS_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define ICS_REG_0_0_0_VTDBAR_RESET 0x00000000

#define ICS_REG_0_0_0_VTDBAR_IWC_LSB 0x0000
#define ICS_REG_0_0_0_VTDBAR_IWC_MSB 0x0000
#define ICS_REG_0_0_0_VTDBAR_IWC_RANGE 0x0001
#define ICS_REG_0_0_0_VTDBAR_IWC_MASK 0x00000001
#define ICS_REG_0_0_0_VTDBAR_IWC_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IECTL_REG_0_0_0_VTDBAR_FLAG
#define IECTL_REG_0_0_0_VTDBAR_FLAG
// IECTL_REG_0_0_0_VTDBAR desc:  [p]Register specifying the invalidation event interrupt control
// bits[/p][p]This register is treated as RsvdZ by implementations
// reporting Queued Invalidation (QI) as not supported in the Extended
// Capability register.[/p]
typedef union {
    struct {
        uint32_t  RSVD_0               : 30;    // Nebulon auto filled RSVD [0:29]
        uint32_t  IP                   :   1;    //  [p]Hardware sets the IP field
                                                 // whenever it detects an
                                                 // interrupt condition. Interrupt
                                                 // condition is defined
                                                 // as:[/p][list][*]An
                                                 // Invalidation Wait Descriptor
                                                 // with Interrupt Flag (IF) field
                                                 // Set completed, setting the IWC
                                                 // field in the Invalidation
                                                 // Completion Status
                                                 // register[*]If the IWC field in
                                                 // the Invalidation Completion
                                                 // Status register was already
                                                 // Set at the time of setting
                                                 // this field, it is not treated
                                                 // as a new interrupt
                                                 // condition[/list][p]The IP
                                                 // field is kept Set by hardware
                                                 // while the interrupt message is
                                                 // held pending. The interrupt
                                                 // message could be held pending
                                                 // due to interrupt mask (IM
                                                 // field) being Set, or due to
                                                 // other transient hardware
                                                 // conditions. The IP field is
                                                 // cleared by hardware as soon as
                                                 // the interrupt message pending
                                                 // condition is serviced. This
                                                 // could be due to
                                                 // either:[/p][list][*]0=
                                                 // Hardware issuing the interrupt
                                                 // message due to either change
                                                 // in the transient hardware
                                                 // condition that caused
                                                 // interrupt message to be held
                                                 // pending or due to software
                                                 // clearing the IM field[*]1=
                                                 // Software servicing the IWC
                                                 // field in the Invalidation
                                                 // Completion Status
                                                 // register.[/list]
        uint32_t  IM                   :   1;    //  [list][*]0= No masking of
                                                 // interrupt. When a invalidation
                                                 // event condition is detected,
                                                 // hardware issues an interrupt
                                                 // message (using the
                                                 // Invalidation Event Data &
                                                 // Invalidation Event Address
                                                 // register values)[*]1= This is
                                                 // the value on reset. Software
                                                 // may mask interrupt message
                                                 // generation by setting this
                                                 // field. Hardware is prohibited
                                                 // from sending the interrupt
                                                 // message when this field is
                                                 // Set.[/list]

    }                                field;
    uint32_t                         val;
} IECTL_REG_0_0_0_VTDBAR_t;
#endif
#define IECTL_REG_0_0_0_VTDBAR_OFFSET 0xa0
#define IECTL_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IECTL_REG_0_0_0_VTDBAR_SIZE 32
#define IECTL_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define IECTL_REG_0_0_0_VTDBAR_RESET 0x80000000

#define IECTL_REG_0_0_0_VTDBAR_IP_LSB 0x001e
#define IECTL_REG_0_0_0_VTDBAR_IP_MSB 0x001e
#define IECTL_REG_0_0_0_VTDBAR_IP_RANGE 0x0001
#define IECTL_REG_0_0_0_VTDBAR_IP_MASK 0x40000000
#define IECTL_REG_0_0_0_VTDBAR_IP_RESET_VALUE 0x00000000

#define IECTL_REG_0_0_0_VTDBAR_IM_LSB 0x001f
#define IECTL_REG_0_0_0_VTDBAR_IM_MSB 0x001f
#define IECTL_REG_0_0_0_VTDBAR_IM_RANGE 0x0001
#define IECTL_REG_0_0_0_VTDBAR_IM_MASK 0x80000000
#define IECTL_REG_0_0_0_VTDBAR_IM_RESET_VALUE 0x00000001


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IEDATA_REG_0_0_0_VTDBAR_FLAG
#define IEDATA_REG_0_0_0_VTDBAR_FLAG
// IEDATA_REG_0_0_0_VTDBAR desc:  [p]Register specifying the Invalidation Event interrupt message
// data[/p][p]This register is treated as RsvdZ by implementations
// reporting Queued Invalidation (QI) as not supported in the Extended
// Capability register.[/p]
typedef union {
    struct {
        uint32_t  IMD                  :  16;    //  [p]Data value in the
                                                 // interrupt request.[/p]
        uint32_t  EIMD                 :  16;    //  [p]This field is valid only
                                                 // for implementations supporting
                                                 // 32-bit interrupt data
                                                 // fields[/p][p]Hardware
                                                 // implementations supporting
                                                 // only 16-bit interrupt data
                                                 // treat this field as Rsvd.[/p]

    }                                field;
    uint32_t                         val;
} IEDATA_REG_0_0_0_VTDBAR_t;
#endif
#define IEDATA_REG_0_0_0_VTDBAR_OFFSET 0xa4
#define IEDATA_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IEDATA_REG_0_0_0_VTDBAR_SIZE 32
#define IEDATA_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define IEDATA_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IEDATA_REG_0_0_0_VTDBAR_IMD_LSB 0x0000
#define IEDATA_REG_0_0_0_VTDBAR_IMD_MSB 0x000f
#define IEDATA_REG_0_0_0_VTDBAR_IMD_RANGE 0x0010
#define IEDATA_REG_0_0_0_VTDBAR_IMD_MASK 0x0000ffff
#define IEDATA_REG_0_0_0_VTDBAR_IMD_RESET_VALUE 0x00000000

#define IEDATA_REG_0_0_0_VTDBAR_EIMD_LSB 0x0010
#define IEDATA_REG_0_0_0_VTDBAR_EIMD_MSB 0x001f
#define IEDATA_REG_0_0_0_VTDBAR_EIMD_RANGE 0x0010
#define IEDATA_REG_0_0_0_VTDBAR_EIMD_MASK 0xffff0000
#define IEDATA_REG_0_0_0_VTDBAR_EIMD_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IEADDR_REG_0_0_0_VTDBAR_FLAG
#define IEADDR_REG_0_0_0_VTDBAR_FLAG
// IEADDR_REG_0_0_0_VTDBAR desc:  [p]Register specifying the Invalidation Event Interrupt message
// address[/p][p]This register is treated as RsvdZ by implementations
// reporting Queued Invalidation (QI) as not supported in the Extended
// Capability register.[/p]
typedef union {
    struct {
        uint32_t  RSVD_0               : 2;    // Nebulon auto filled RSVD [0:1]
        uint32_t  MA                   :  30;    //  [p]When fault events are
                                                 // enabled, the contents of this
                                                 // register specify the
                                                 // DWORD-aligned address (bits
                                                 // 31:2) for the interrupt
                                                 // request.[/p]

    }                                field;
    uint32_t                         val;
} IEADDR_REG_0_0_0_VTDBAR_t;
#endif
#define IEADDR_REG_0_0_0_VTDBAR_OFFSET 0xa8
#define IEADDR_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IEADDR_REG_0_0_0_VTDBAR_SIZE 32
#define IEADDR_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define IEADDR_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IEADDR_REG_0_0_0_VTDBAR_MA_LSB 0x0002
#define IEADDR_REG_0_0_0_VTDBAR_MA_MSB 0x001f
#define IEADDR_REG_0_0_0_VTDBAR_MA_RANGE 0x001e
#define IEADDR_REG_0_0_0_VTDBAR_MA_MASK 0xfffffffc
#define IEADDR_REG_0_0_0_VTDBAR_MA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IEUADDR_REG_0_0_0_VTDBAR_FLAG
#define IEUADDR_REG_0_0_0_VTDBAR_FLAG
// IEUADDR_REG_0_0_0_VTDBAR desc:  [p]Register specifying the Invalidation Event interrupt message upper
// address.[/p]
typedef union {
    struct {
        uint32_t  MUA                  :  32;    //  [p]Hardware implementations
                                                 // supporting Queued
                                                 // Invalidations and Extended
                                                 // Interrupt Mode are required to
                                                 // implement this
                                                 // register[/p][p]Hardware
                                                 // implementations not supporting
                                                 // Queued Invalidations or
                                                 // Extended Interrupt Mode may
                                                 // treat this field as RsvdZ.[/p]

    }                                field;
    uint32_t                         val;
} IEUADDR_REG_0_0_0_VTDBAR_t;
#endif
#define IEUADDR_REG_0_0_0_VTDBAR_OFFSET 0xac
#define IEUADDR_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IEUADDR_REG_0_0_0_VTDBAR_SIZE 32
#define IEUADDR_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define IEUADDR_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IEUADDR_REG_0_0_0_VTDBAR_MUA_LSB 0x0000
#define IEUADDR_REG_0_0_0_VTDBAR_MUA_MSB 0x001f
#define IEUADDR_REG_0_0_0_VTDBAR_MUA_RANGE 0x0020
#define IEUADDR_REG_0_0_0_VTDBAR_MUA_MASK 0xffffffff
#define IEUADDR_REG_0_0_0_VTDBAR_MUA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IRTA_REG_0_0_0_VTDBAR_FLAG
#define IRTA_REG_0_0_0_VTDBAR_FLAG
// IRTA_REG_0_0_0_VTDBAR desc:  [p]Register providing the base address of Interrupt remapping table.
// This register is treated as RsvdZ by implementations reporting
// Interrupt Remapping (IR) as not supported in the Extended Capability
// register.[/p]
typedef union {
    struct {
        uint64_t  S                    :   4;    //  [p]This field specifies the
                                                 // size of the interrupt
                                                 // remapping table. The number of
                                                 // entries in the interrupt
                                                 // remapping table is 2(X+1),
                                                 // where X is the value
                                                 // programmed in this field.[/p]
        uint64_t  RSVD_0               :   7;    // Nebulon auto filled RSVD [10:4]
        uint64_t  EIME                 :   1;    //  [p]This field is used by
                                                 // hardware on Intel®64 platforms
                                                 // as
                                                 // follows:[/p][list][*]0=xAPIC
                                                 // mode is active. Hardware
                                                 // interprets only low 8-bits of
                                                 // Destination-ID field in the
                                                 // IRTEs. The high 24-bits of the
                                                 // Destination-ID field are
                                                 // treated as reserved[*]1=
                                                 // x2APIC mode is active.
                                                 // Hardware interprets all
                                                 // 32-bits of Destination-ID
                                                 // field in the
                                                 // IRTEs[/list][p]This field is
                                                 // implemented as RsvdZ on
                                                 // implementations reporting
                                                 // Extended Interrupt Mode (EIM)
                                                 // field as Clear in Extended
                                                 // Capability register.[/p]
        uint64_t  IRTA                 :  52;    //  [p]This field points to the
                                                 // base of 4KB aligned interrupt
                                                 // remapping table[/p][p]Hardware
                                                 // ignores and does not implement
                                                 // bits 63:HAW, where HAW is the
                                                 // host address width[/p][p]Reads
                                                 // of this field returns value
                                                 // that was last programmed to
                                                 // it.[/p]

    }                                field;
    uint64_t                         val;
} IRTA_REG_0_0_0_VTDBAR_t;
#endif
#define IRTA_REG_0_0_0_VTDBAR_OFFSET 0xb8
#define IRTA_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IRTA_REG_0_0_0_VTDBAR_SIZE 64
#define IRTA_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x03
#define IRTA_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IRTA_REG_0_0_0_VTDBAR_S_LSB 0x0000
#define IRTA_REG_0_0_0_VTDBAR_S_MSB 0x0003
#define IRTA_REG_0_0_0_VTDBAR_S_RANGE 0x0004
#define IRTA_REG_0_0_0_VTDBAR_S_MASK 0x0000000f
#define IRTA_REG_0_0_0_VTDBAR_S_RESET_VALUE 0x00000000

#define IRTA_REG_0_0_0_VTDBAR_EIME_LSB 0x000b
#define IRTA_REG_0_0_0_VTDBAR_EIME_MSB 0x000b
#define IRTA_REG_0_0_0_VTDBAR_EIME_RANGE 0x0001
#define IRTA_REG_0_0_0_VTDBAR_EIME_MASK 0x00000800
#define IRTA_REG_0_0_0_VTDBAR_EIME_RESET_VALUE 0x00000000

#define IRTA_REG_0_0_0_VTDBAR_IRTA_LSB 0x000c
#define IRTA_REG_0_0_0_VTDBAR_IRTA_MSB 0x003f
#define IRTA_REG_0_0_0_VTDBAR_IRTA_RANGE 0x0034
#define IRTA_REG_0_0_0_VTDBAR_IRTA_MASK 0xfffffffffffff000
#define IRTA_REG_0_0_0_VTDBAR_IRTA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PRESTS_REG_0_0_0_VTDBAR_FLAG
#define PRESTS_REG_0_0_0_VTDBAR_FLAG
// PRESTS_REG_0_0_0_VTDBAR desc:  [p]Register to report pending page request in page request queue.
// This register is treated as RsvdZ by implementations reporting Page
// Request Support (PRS) as not supported in the Extended Capability
// register.[/p]
typedef union {
    struct {
        uint32_t  PPR                  :   1;    //  [p]Pending Page Request:
                                                 // Indicates pending page
                                                 // requests to be serviced by
                                                 // software in the page request
                                                 // queue. This field is Set by
                                                 // hardware when a streaming page
                                                 // request entry
                                                 // (page_stream_reg_dsc) or a
                                                 // page group request
                                                 // (page_grp_req_dsc) with Last
                                                 // Page in Group (LPG) field Set,
                                                 // is added to the page request
                                                 // queue.[/p]
        uint32_t  RSVD_0               :  31;    // Nebulon auto filled RSVD [31:1]

    }                                field;
    uint32_t                         val;
} PRESTS_REG_0_0_0_VTDBAR_t;
#endif
#define PRESTS_REG_0_0_0_VTDBAR_OFFSET 0xdc
#define PRESTS_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PRESTS_REG_0_0_0_VTDBAR_SIZE 32
#define PRESTS_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define PRESTS_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PRESTS_REG_0_0_0_VTDBAR_PPR_LSB 0x0000
#define PRESTS_REG_0_0_0_VTDBAR_PPR_MSB 0x0000
#define PRESTS_REG_0_0_0_VTDBAR_PPR_RANGE 0x0001
#define PRESTS_REG_0_0_0_VTDBAR_PPR_MASK 0x00000001
#define PRESTS_REG_0_0_0_VTDBAR_PPR_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PRECTL_REG_0_0_0_VTDBAR_FLAG
#define PRECTL_REG_0_0_0_VTDBAR_FLAG
// PRECTL_REG_0_0_0_VTDBAR desc:  [p]Register specifying the page request event interrupt control bits.
// This register is treated as RsvdZ by implementations reporting Page
// Request Support (PRS) as not supported in the Extended Capability
// register[/p]
typedef union {
    struct {
        uint32_t  RSVD_0               : 30;    // Nebulon auto filled RSVD [0:29]
        uint32_t  IP                   :   1;    //  [p]Interrupt Pending:
                                                 // Hardware sets the IP field
                                                 // whenever it detects an
                                                 // interrupt condition. Interrupt
                                                 // condition is defined
                                                 // as:[/p][list][*]A streaming
                                                 // page request entry
                                                 // (page_stream_req_dsc) or a
                                                 // page group request
                                                 // (page_grp_req_dsc) with Last
                                                 // Page in Group (LPG) field Set,
                                                 // was added to page request
                                                 // queue, resulting in hardware
                                                 // setting the Pending Page
                                                 // Request (PPR) field in Page
                                                 // Request Status register[*]If
                                                 // the PPR field in the Page
                                                 // Request Event Status register
                                                 // was already Set at the time of
                                                 // setting this field, it is not
                                                 // treated as a new interrupt
                                                 // condition[/list][p]The IP
                                                 // field is kept Set by hardware
                                                 // while the interrupt message is
                                                 // held pending. The interrupt
                                                 // message could be held pending
                                                 // due to interrupt mask (IM
                                                 // field) being Set, or due to
                                                 // other transient hardware
                                                 // conditions. The IP field is
                                                 // cleared by hardware as soon as
                                                 // the interrupt message pending
                                                 // condition is serviced. This
                                                 // could be due to
                                                 // either:[/p][list][*]Hardware
                                                 // issuing the interrupt message
                                                 // due to either change in the
                                                 // transient hardware condition
                                                 // that caused interrupt message
                                                 // to be held pending or due to
                                                 // software clearing the IM
                                                 // field[*] Software servicing
                                                 // the PPR field in the Page
                                                 // Request Event Status
                                                 // register.[/list]
        uint32_t  IM                   :   1;    //  [p]Interrupt
                                                 // Mask[/p][list][*]0=No masking
                                                 // of interrupt. When a page
                                                 // request event condition is
                                                 // detected, hardware issues an
                                                 // interrupt message (using the
                                                 // Page Request Event Data and
                                                 // Page Request Event Address
                                                 // register values)[*]1=This is
                                                 // the value on reset. Software
                                                 // may mask interrupt message
                                                 // generation by setting this
                                                 // field. Hardware is prohibited
                                                 // from sending the interrupt
                                                 // message when this field is
                                                 // Set.[/list]

    }                                field;
    uint32_t                         val;
} PRECTL_REG_0_0_0_VTDBAR_t;
#endif
#define PRECTL_REG_0_0_0_VTDBAR_OFFSET 0xe0
#define PRECTL_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PRECTL_REG_0_0_0_VTDBAR_SIZE 32
#define PRECTL_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define PRECTL_REG_0_0_0_VTDBAR_RESET 0x80000000

#define PRECTL_REG_0_0_0_VTDBAR_IP_LSB 0x001e
#define PRECTL_REG_0_0_0_VTDBAR_IP_MSB 0x001e
#define PRECTL_REG_0_0_0_VTDBAR_IP_RANGE 0x0001
#define PRECTL_REG_0_0_0_VTDBAR_IP_MASK 0x40000000
#define PRECTL_REG_0_0_0_VTDBAR_IP_RESET_VALUE 0x00000000

#define PRECTL_REG_0_0_0_VTDBAR_IM_LSB 0x001f
#define PRECTL_REG_0_0_0_VTDBAR_IM_MSB 0x001f
#define PRECTL_REG_0_0_0_VTDBAR_IM_RANGE 0x0001
#define PRECTL_REG_0_0_0_VTDBAR_IM_MASK 0x80000000
#define PRECTL_REG_0_0_0_VTDBAR_IM_RESET_VALUE 0x00000001


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PREDATA_REG_0_0_0_VTDBAR_FLAG
#define PREDATA_REG_0_0_0_VTDBAR_FLAG
// PREDATA_REG_0_0_0_VTDBAR desc:  [p]Register specifying the Page Request Event interrupt message data.
// This register is treated as RsvdZ by implementations reporting Page
// Request Support (PRS) as not supported in the Extended Capability
// register.[/p]
typedef union {
    struct {
        uint32_t  IMD                  :  16;    //  [p]Interrupt Message Data:
                                                 // Data value in the interrupt
                                                 // request. Software requirements
                                                 // for programming this register
                                                 // are described in VTd Spec[/p]
        uint32_t  EIMD                 :  16;    //  [p]Extended Interrupt Message
                                                 // Data[/p]

    }                                field;
    uint32_t                         val;
} PREDATA_REG_0_0_0_VTDBAR_t;
#endif
#define PREDATA_REG_0_0_0_VTDBAR_OFFSET 0xe4
#define PREDATA_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PREDATA_REG_0_0_0_VTDBAR_SIZE 32
#define PREDATA_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define PREDATA_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PREDATA_REG_0_0_0_VTDBAR_IMD_LSB 0x0000
#define PREDATA_REG_0_0_0_VTDBAR_IMD_MSB 0x000f
#define PREDATA_REG_0_0_0_VTDBAR_IMD_RANGE 0x0010
#define PREDATA_REG_0_0_0_VTDBAR_IMD_MASK 0x0000ffff
#define PREDATA_REG_0_0_0_VTDBAR_IMD_RESET_VALUE 0x00000000

#define PREDATA_REG_0_0_0_VTDBAR_EIMD_LSB 0x0010
#define PREDATA_REG_0_0_0_VTDBAR_EIMD_MSB 0x001f
#define PREDATA_REG_0_0_0_VTDBAR_EIMD_RANGE 0x0010
#define PREDATA_REG_0_0_0_VTDBAR_EIMD_MASK 0xffff0000
#define PREDATA_REG_0_0_0_VTDBAR_EIMD_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PREADDR_REG_0_0_0_VTDBAR_FLAG
#define PREADDR_REG_0_0_0_VTDBAR_FLAG
// PREADDR_REG_0_0_0_VTDBAR desc:  [p]Register specifying the Page Request Event Interrupt message
// address. This register is treated as RsvdZ by implementations
// reporting Page Request Support (PRS) as not supported in the Extended
// Capability register.[/p]
typedef union {
    struct {
        uint32_t  RSVD_0               : 2;    // Nebulon auto filled RSVD [0:1]
        uint32_t  MA                   :  30;    //  [p]Message Address: When
                                                 // fault events are enabled, the
                                                 // contents of this register
                                                 // specify the DWORD-aligned
                                                 // address (bits 31:2) for the
                                                 // interrupt request.[/p]

    }                                field;
    uint32_t                         val;
} PREADDR_REG_0_0_0_VTDBAR_t;
#endif
#define PREADDR_REG_0_0_0_VTDBAR_OFFSET 0xe8
#define PREADDR_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PREADDR_REG_0_0_0_VTDBAR_SIZE 32
#define PREADDR_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define PREADDR_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PREADDR_REG_0_0_0_VTDBAR_MA_LSB 0x0002
#define PREADDR_REG_0_0_0_VTDBAR_MA_MSB 0x001f
#define PREADDR_REG_0_0_0_VTDBAR_MA_RANGE 0x001e
#define PREADDR_REG_0_0_0_VTDBAR_MA_MASK 0xfffffffc
#define PREADDR_REG_0_0_0_VTDBAR_MA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef PREUADDR_REG_0_0_0_VTDBAR_FLAG
#define PREUADDR_REG_0_0_0_VTDBAR_FLAG
// PREUADDR_REG_0_0_0_VTDBAR desc:  [p]Register specifying the Page Request Event interrupt message upper
// address.[/p]
typedef union {
    struct {
        uint32_t  MUA                  :  32;    //  [p]Message Upper Address:
                                                 // This field specifies the upper
                                                 // address (bits.. 63:32) for the
                                                 // page request event
                                                 // interrupt.[/p]

    }                                field;
    uint32_t                         val;
} PREUADDR_REG_0_0_0_VTDBAR_t;
#endif
#define PREUADDR_REG_0_0_0_VTDBAR_OFFSET 0xec
#define PREUADDR_REG_0_0_0_VTDBAR_SCOPE 0x01
#define PREUADDR_REG_0_0_0_VTDBAR_SIZE 32
#define PREUADDR_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define PREUADDR_REG_0_0_0_VTDBAR_RESET 0x00000000

#define PREUADDR_REG_0_0_0_VTDBAR_MUA_LSB 0x0000
#define PREUADDR_REG_0_0_0_VTDBAR_MUA_MSB 0x001f
#define PREUADDR_REG_0_0_0_VTDBAR_MUA_RANGE 0x0020
#define PREUADDR_REG_0_0_0_VTDBAR_MUA_MASK 0xffffffff
#define PREUADDR_REG_0_0_0_VTDBAR_MUA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef FRCDL_REG_0_0_0_VTDBAR_FLAG
#define FRCDL_REG_0_0_0_VTDBAR_FLAG
// FRCDL_REG_0_0_0_VTDBAR desc:  [p]Register to record fault information when primary fault logging is
// active. Hardware reports the number and location of fault recording
// registers through the Capability register. This register is relevant
// only for primary fault logging[/p][p] This register is sticky and can
// be cleared only through power good reset or by software clearing the
// RW1C fields by writing a 1.[/p]
typedef union {
    struct {
        uint64_t  RSVD_0               : 12;    // Nebulon auto filled RSVD [0:11]
        uint64_t  FI                   :  52;    //  [p]When the Fault Reason (FR)
                                                 // field indicates one of the
                                                 // DMA-remapping fault
                                                 // conditions, bits 63:12 of this
                                                 // field contain the page address
                                                 // in the faulted DMA request.
                                                 // Hardware treats bits 63:N as
                                                 // reserved (0), where N is the
                                                 // maximum guest address width
                                                 // (MGAW) supported[/p][p]When
                                                 // the Fault Reason (FR) field
                                                 // indicates one of the
                                                 // interrupt-remapping fault
                                                 // conditions, bits 63:48 of this
                                                 // field indicate the
                                                 // interrupt_index computed for
                                                 // the faulted interrupt request,
                                                 // and bits 47:12 are
                                                 // cleared[/p][p]This field is
                                                 // relevant only when the F field
                                                 // is Set.[/p]

    }                                field;
    uint64_t                         val;
} FRCDL_REG_0_0_0_VTDBAR_t;
#endif
#define FRCDL_REG_0_0_0_VTDBAR_OFFSET 0x00
#define FRCDL_REG_0_0_0_VTDBAR_SCOPE 0x01
#define FRCDL_REG_0_0_0_VTDBAR_SIZE 64
#define FRCDL_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define FRCDL_REG_0_0_0_VTDBAR_RESET 0x00000000

#define FRCDL_REG_0_0_0_VTDBAR_FI_LSB 0x000c
#define FRCDL_REG_0_0_0_VTDBAR_FI_MSB 0x003f
#define FRCDL_REG_0_0_0_VTDBAR_FI_RANGE 0x0034
#define FRCDL_REG_0_0_0_VTDBAR_FI_MASK 0xfffffffffffff000
#define FRCDL_REG_0_0_0_VTDBAR_FI_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef FRCDH_REG_0_0_0_VTDBAR_FLAG
#define FRCDH_REG_0_0_0_VTDBAR_FLAG
// FRCDH_REG_0_0_0_VTDBAR desc:  [p]Register to record fault information when primary fault logging is
// active. Hardware reports the number and location of fault recording
// registers through the Capability register. This register is relevant
// only for primary fault logging[/p][p] This register is sticky and can
// be cleared only through power good reset or by software clearing the
// RW1C fields by writing a 1.[/p]
typedef union {
    struct {
        uint64_t  SID                  :  16;    //  [p]Requester-id associated
                                                 // with the fault
                                                 // condition[/p][p] This field is
                                                 // relevant only when the F field
                                                 // is set.[/p]
        uint64_t  RSVD_0               :  13;    // Nebulon auto filled RSVD [28:16]
        uint64_t  PRIV                 :   1;    //  [p]When set, indicates
                                                 // Supervisor privilege was
                                                 // requested by the faulted
                                                 // request. This field is
                                                 // relevant only when the PP
                                                 // field is Set. Hardware
                                                 // implementations not supporting
                                                 // PASID (PASID field Clear in
                                                 // Extended Capability register)
                                                 // implement this field as
                                                 // RsvdZ.[/p]
        uint64_t  EXE                  :   1;    //  [p]When set, indicates
                                                 // Execute permission was
                                                 // requested by the faulted read
                                                 // request. This field is
                                                 // relevant only when the PP
                                                 // field and T field are both
                                                 // Set. Hardware implementations
                                                 // not supporting PASID (PASID
                                                 // field Clear in Extended
                                                 // Capability register) implement
                                                 // this field as RsvdZ.[/p]
        uint64_t  PP                   :   1;    //  [p]When set, indicates the
                                                 // faulted request has a PASID
                                                 // tag. The value of the PASID
                                                 // field is reported in the PASID
                                                 // Value (PV) field. This field
                                                 // is relevant only when the F
                                                 // field is Set, and when the
                                                 // fault reason (FR) indicates
                                                 // one of the non-recoverable
                                                 // address translation fault
                                                 // conditions. Hardware
                                                 // implementations not supporting
                                                 // PASID (PASID field Clear in
                                                 // Extended Capability register)
                                                 // implement this field as
                                                 // RsvdZ.[/p]
        uint64_t  FR                   :   8;    //  [p]Reason for the
                                                 // fault[/p][p] This field is
                                                 // relevant only when the F field
                                                 // is set.[/p]
        uint64_t  PN                   :  20;    //  [p]PASID value in the faulted
                                                 // request. This field is
                                                 // relevant only when the PP
                                                 // field is set. Hardware
                                                 // implementations not supporting
                                                 // PASID (PASID field Clear in
                                                 // Extended Capability register)
                                                 // implement this field as
                                                 // RsvdZ.[/p]
        uint64_t  AT                   :   2;    //  [p]This field captures the AT
                                                 // field from the faulted DMA
                                                 // request[/p][p]Hardware
                                                 // implementations not supporting
                                                 // Device-IOTLBs (DI field Clear
                                                 // in Extended Capability
                                                 // register) treat this field as
                                                 // RsvdZ[/p][p]When supported,
                                                 // this field is valid only when
                                                 // the F field is Set, and when
                                                 // the fault reason (FR)
                                                 // indicates one of the
                                                 // DMA-remapping fault
                                                 // conditions.[/p]
        uint64_t  T                    :   1;    //  [p]Type of the faulted
                                                 // request:[/p][list][*]0=0:
                                                 // Write request[*]1=1: Read
                                                 // request or AtomicOp
                                                 // request[/list][p]This field is
                                                 // relevant only when the F field
                                                 // is Set, and when the fault
                                                 // reason (FR) indicates one of
                                                 // the DMA-remapping fault
                                                 // conditions.[/p]
        uint64_t  F                    :   1;    //  [p]Hardware sets this field
                                                 // to indicate a fault is logged
                                                 // in this Fault Recording
                                                 // register. The F field is set
                                                 // by hardware after the details
                                                 // of the fault is recorded in
                                                 // other fields[/p][p]When this
                                                 // field is Set, hardware may
                                                 // collapse additional faults
                                                 // from the same source-id
                                                 // (SID)[/p][p]Software writes
                                                 // the value read from this field
                                                 // to Clear it.[/p]

    }                                field;
    uint64_t                         val;
} FRCDH_REG_0_0_0_VTDBAR_t;
#endif
#define FRCDH_REG_0_0_0_VTDBAR_OFFSET 0x08
#define FRCDH_REG_0_0_0_VTDBAR_SCOPE 0x01
#define FRCDH_REG_0_0_0_VTDBAR_SIZE 64
#define FRCDH_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x09
#define FRCDH_REG_0_0_0_VTDBAR_RESET 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_SID_LSB 0x0000
#define FRCDH_REG_0_0_0_VTDBAR_SID_MSB 0x000f
#define FRCDH_REG_0_0_0_VTDBAR_SID_RANGE 0x0010
#define FRCDH_REG_0_0_0_VTDBAR_SID_MASK 0x0000ffff
#define FRCDH_REG_0_0_0_VTDBAR_SID_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_PRIV_LSB 0x001d
#define FRCDH_REG_0_0_0_VTDBAR_PRIV_MSB 0x001d
#define FRCDH_REG_0_0_0_VTDBAR_PRIV_RANGE 0x0001
#define FRCDH_REG_0_0_0_VTDBAR_PRIV_MASK 0x20000000
#define FRCDH_REG_0_0_0_VTDBAR_PRIV_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_EXE_LSB 0x001e
#define FRCDH_REG_0_0_0_VTDBAR_EXE_MSB 0x001e
#define FRCDH_REG_0_0_0_VTDBAR_EXE_RANGE 0x0001
#define FRCDH_REG_0_0_0_VTDBAR_EXE_MASK 0x40000000
#define FRCDH_REG_0_0_0_VTDBAR_EXE_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_PP_LSB 0x001f
#define FRCDH_REG_0_0_0_VTDBAR_PP_MSB 0x001f
#define FRCDH_REG_0_0_0_VTDBAR_PP_RANGE 0x0001
#define FRCDH_REG_0_0_0_VTDBAR_PP_MASK 0x80000000
#define FRCDH_REG_0_0_0_VTDBAR_PP_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_FR_LSB 0x0020
#define FRCDH_REG_0_0_0_VTDBAR_FR_MSB 0x0027
#define FRCDH_REG_0_0_0_VTDBAR_FR_RANGE 0x0008
#define FRCDH_REG_0_0_0_VTDBAR_FR_MASK 0xff00000000
#define FRCDH_REG_0_0_0_VTDBAR_FR_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_PN_LSB 0x0028
#define FRCDH_REG_0_0_0_VTDBAR_PN_MSB 0x003b
#define FRCDH_REG_0_0_0_VTDBAR_PN_RANGE 0x0014
#define FRCDH_REG_0_0_0_VTDBAR_PN_MASK 0xfffff0000000000
#define FRCDH_REG_0_0_0_VTDBAR_PN_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_AT_LSB 0x003c
#define FRCDH_REG_0_0_0_VTDBAR_AT_MSB 0x003d
#define FRCDH_REG_0_0_0_VTDBAR_AT_RANGE 0x0002
#define FRCDH_REG_0_0_0_VTDBAR_AT_MASK 0x3000000000000000
#define FRCDH_REG_0_0_0_VTDBAR_AT_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_T_LSB 0x003e
#define FRCDH_REG_0_0_0_VTDBAR_T_MSB 0x003e
#define FRCDH_REG_0_0_0_VTDBAR_T_RANGE 0x0001
#define FRCDH_REG_0_0_0_VTDBAR_T_MASK 0x4000000000000000
#define FRCDH_REG_0_0_0_VTDBAR_T_RESET_VALUE 0x00000000

#define FRCDH_REG_0_0_0_VTDBAR_F_LSB 0x003f
#define FRCDH_REG_0_0_0_VTDBAR_F_MSB 0x003f
#define FRCDH_REG_0_0_0_VTDBAR_F_RANGE 0x0001
#define FRCDH_REG_0_0_0_VTDBAR_F_MASK 0x8000000000000000
#define FRCDH_REG_0_0_0_VTDBAR_F_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IVA_REG_0_0_0_VTDBAR_FLAG
#define IVA_REG_0_0_0_VTDBAR_FLAG
// IVA_REG_0_0_0_VTDBAR desc:  [p]Register to provide the DMA address whose corresponding IOTLB
// entry needs to be invalidated through the corresponding IOTLB
// Invalidate register. This register is a write-only register.[/p]
typedef union {
    struct {
        uint64_t  AM                   :   6;    //  [p]The value in this field
                                                 // specifies the number of low
                                                 // order bits of the ADDR field
                                                 // that must be masked for the
                                                 // invalidation operation. This
                                                 // field enables software to
                                                 // request invalidation of
                                                 // contiguous mappings for
                                                 // size-aligned regions. For
                                                 // example:..Mask ADDR bits
                                                 // Pages..Value masked
                                                 // invalidated.. 0 None 1.. 1 12
                                                 // 2.. 2 13:12 4.. 3 14:12 8.. 4
                                                 // 15:12 16[/p] [p]When
                                                 // invalidating mappings for
                                                 // super-pages, software must
                                                 // specify the appropriate mask
                                                 // value. For example, when
                                                 // invalidating mapping for a 2MB
                                                 // page, software must specify an
                                                 // address mask value of at least
                                                 // 9...Hardware implementations
                                                 // report the maximum supported
                                                 // mask value through the
                                                 // Capability register.[/p]
        uint64_t  IH                   :   1;    //  [p]The field provides hint to
                                                 // hardware about preserving or
                                                 // flushing the non-leaf
                                                 // (page-directory) entries that
                                                 // may be cached in
                                                 // hardware:[/p][list][*]0 =
                                                 // Software may have modified
                                                 // both leaf and non-leaf
                                                 // page-table entries
                                                 // corresponding to mappings
                                                 // specified in the ADDR and AM
                                                 // fields. On a page-selective
                                                 // invalidation request, hardware
                                                 // must flush both the cached
                                                 // leaf and non-leaf page-table
                                                 // entries corresponding to the
                                                 // mappings specified by ADDR and
                                                 // AM fields.[*]1 = Software has
                                                 // not modified any non-leaf
                                                 // page-table entries
                                                 // corresponding to mappings
                                                 // specified in the ADDR and AM
                                                 // fields. On a page-selective
                                                 // invalidation request, hardware
                                                 // may preserve the cached
                                                 // non-leaf page-table entries
                                                 // corresponding to mappings
                                                 // specified by ADDR and AM
                                                 // fields.[/list][p]A value
                                                 // returned on a read of this
                                                 // field is undefined[/p]
        uint64_t  RSVD_0               :   5;    // Nebulon auto filled RSVD [11:7]
        uint64_t  ADDR                 :  52;    //  [p]Software provides the DMA
                                                 // address that needs to be
                                                 // page-selectively invalidated.
                                                 // To make a page-selective
                                                 // invalidation request to
                                                 // hardware, software must first
                                                 // write the appropriate fields
                                                 // in this register, and then
                                                 // issue the appropriate
                                                 // page-selective invalidate
                                                 // command through the IOTLB_REG.
                                                 // Hardware ignores bits 63:N,
                                                 // where N is the maximum guest
                                                 // address width (MGAW)
                                                 // supported.[/p][p]A value
                                                 // retuned on a read of this
                                                 // field is undefined[/p][p]A
                                                 // value returned on a read of
                                                 // this field is undefined[/p]

    }                                field;
    uint64_t                         val;
} IVA_REG_0_0_0_VTDBAR_t;
#endif
#define IVA_REG_0_0_0_VTDBAR_OFFSET 0x00
#define IVA_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IVA_REG_0_0_0_VTDBAR_SIZE 64
#define IVA_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x03
#define IVA_REG_0_0_0_VTDBAR_RESET 0x00000000

#define IVA_REG_0_0_0_VTDBAR_AM_LSB 0x0000
#define IVA_REG_0_0_0_VTDBAR_AM_MSB 0x0005
#define IVA_REG_0_0_0_VTDBAR_AM_RANGE 0x0006
#define IVA_REG_0_0_0_VTDBAR_AM_MASK 0x0000003f
#define IVA_REG_0_0_0_VTDBAR_AM_RESET_VALUE 0x00000000

#define IVA_REG_0_0_0_VTDBAR_IH_LSB 0x0006
#define IVA_REG_0_0_0_VTDBAR_IH_MSB 0x0006
#define IVA_REG_0_0_0_VTDBAR_IH_RANGE 0x0001
#define IVA_REG_0_0_0_VTDBAR_IH_MASK 0x00000040
#define IVA_REG_0_0_0_VTDBAR_IH_RESET_VALUE 0x00000000

#define IVA_REG_0_0_0_VTDBAR_ADDR_LSB 0x000c
#define IVA_REG_0_0_0_VTDBAR_ADDR_MSB 0x003f
#define IVA_REG_0_0_0_VTDBAR_ADDR_RANGE 0x0034
#define IVA_REG_0_0_0_VTDBAR_ADDR_MASK 0xfffffffffffff000
#define IVA_REG_0_0_0_VTDBAR_ADDR_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IOTLB_REG_0_0_0_VTDBAR_FLAG
#define IOTLB_REG_0_0_0_VTDBAR_FLAG
// IOTLB_REG_0_0_0_VTDBAR desc:  [p]Register to invalidate IOTLB. The act of writing the upper byte of
// the IOTLB_REG with IVT field Set causes the hardware to perform the
// IOTLB invalidation.[/p]
typedef union {
    struct {
        uint64_t  RSVD_0               : 32;    // Nebulon auto filled RSVD [0:31]
        uint64_t  DID                  :  16;    //  [p]Indicates the ID of the
                                                 // domain whose IOTLB entries
                                                 // need to be selectively
                                                 // invalidated. This field must
                                                 // be programmed by software for
                                                 // domain-selective and
                                                 // page-selective invalidation
                                                 // requests.[/p][p]The Capability
                                                 // register reports the domain-id
                                                 // width supported by hardware.
                                                 // Software must ensure that the
                                                 // value written to this field is
                                                 // within this limit. Hardware
                                                 // ignores and not implements
                                                 // bits 47:(32+N), where N is the
                                                 // supported domain-id width
                                                 // reported in the Capability
                                                 // register.[/p]
        uint64_t  DW                   :   1;    //  [p]This field is ignored by
                                                 // hardware if the DWD field is
                                                 // reported as Clear in the
                                                 // Capability register. When the
                                                 // DWD field is reported as Set
                                                 // in the Capability register,
                                                 // the following encodings are
                                                 // supported for this
                                                 // field:[/p][list][*]0 =
                                                 // Hardware may complete the
                                                 // IOTLB invalidation without
                                                 // draining DMA write
                                                 // requests[*]1 = Hardware must
                                                 // drain relevant translated DMA
                                                 // write requests.[/list]
        uint64_t  DR                   :   1;    //  [p]This field is ignored by
                                                 // hardware if the DRD field is
                                                 // reported as clear in the
                                                 // Capability register. When the
                                                 // DRD field is reported as Set
                                                 // in the Capability register,
                                                 // the following encodings are
                                                 // supported for this
                                                 // field:[/p][list][*]0 =
                                                 // Hardware may complete the
                                                 // IOTLB invalidation without
                                                 // draining any translated DMA
                                                 // read requests[*]1 = Hardware
                                                 // must drain DMA read
                                                 // requests.[/list]
        uint64_t  RSVD_1               :   7;    // Nebulon auto filled RSVD [56:50]
        uint64_t  IAIG                 :   2;    //  [p]Hardware reports the
                                                 // granularity at which an
                                                 // invalidation request was
                                                 // processed through this field
                                                 // when reporting invalidation
                                                 // completion (by clearing the
                                                 // IVT field). The following are
                                                 // the encodings for this
                                                 // field[/p][list][*]00 =
                                                 // Reserved. This indicates
                                                 // hardware detected an incorrect
                                                 // invalidation request and
                                                 // ignored the request. Examples
                                                 // of incorrect invalidation
                                                 // requests include detecting an
                                                 // unsupported address mask value
                                                 // in Invalidate Address register
                                                 // for page-selective
                                                 // invalidation requests[*]01 =
                                                 // Global Invalidation performed.
                                                 // This could be in response to a
                                                 // global, domain-selective, or
                                                 // page-selective invalidation
                                                 // request[*]10 =
                                                 // Domain-selective invalidation
                                                 // performed using the domain-id
                                                 // specified by software in the
                                                 // DID field. This could be in
                                                 // response to a domain-selective
                                                 // or a page-selective
                                                 // invalidation request[*]11 =
                                                 // Domain-page-selective
                                                 // invalidation performed using
                                                 // the address, mask and hint
                                                 // specified by software in the
                                                 // Invalidate Address register
                                                 // and domain-id specified in DID
                                                 // field. This can be in response
                                                 // to a page-selective
                                                 // invalidation request.[/list]
        uint64_t  RSVD_2               :   1;    // Nebulon auto filled RSVD [59:59]
        uint64_t  IIRG                 :   2;    //  [p]When requesting hardware
                                                 // to invalidate the IOTLB (by
                                                 // setting the IVT field),
                                                 // software writes the requested
                                                 // invalidation granularity
                                                 // through this field. The
                                                 // following are the encodings
                                                 // for the field[/p][list][*]00 =
                                                 // Reserved[*]01 = Global
                                                 // invalidation request[*]10 =
                                                 // Domain-selective invalidation
                                                 // request. The target domain-id
                                                 // must be specified in the DID
                                                 // field[*]11 = Page-selective
                                                 // invalidation request. The
                                                 // target address, mask and
                                                 // invalidation hint must be
                                                 // specified in the Invalidate
                                                 // Address register, and the
                                                 // domain-id must be provided in
                                                 // the DID
                                                 // field[/list][p]Hardware
                                                 // implementations may process an
                                                 // invalidation request by
                                                 // performing invalidation at a
                                                 // coarser granularity than
                                                 // requested. Hardware indicates
                                                 // completion of the invalidation
                                                 // request by clearing the IVT
                                                 // field. At this time, the
                                                 // granularity at which actual
                                                 // invalidation was performed is
                                                 // reported through the IAIG
                                                 // field[/p]
        uint64_t  RSVD_3               :   1;    // Nebulon auto filled RSVD [62:62]
        uint64_t  IVT                  :   1;    //  [p]Software requests IOTLB
                                                 // invalidation by setting this
                                                 // field. Software must also set
                                                 // the requested invalidation
                                                 // granularity by programming the
                                                 // IIRG field[/p][p]Hardware
                                                 // clears the IVT field to
                                                 // indicate the invalidation
                                                 // request is complete. Hardware
                                                 // also indicates the granularity
                                                 // at which the invalidation
                                                 // operation was performed
                                                 // through the IAIG field.
                                                 // Software must not submit
                                                 // another invalidation request
                                                 // through this register while
                                                 // the IVT field is Set, nor
                                                 // update the associated
                                                 // Invalidate Address
                                                 // register[/p][p]Software must
                                                 // not submit IOTLB invalidation
                                                 // requests when there is a
                                                 // context-cache invalidation
                                                 // request pending at this
                                                 // remapping hardware
                                                 // unit.[/p][p]Hardware
                                                 // implementations reporting
                                                 // write-buffer flushing
                                                 // requirement (RWBF=1 in
                                                 // Capability register) must
                                                 // implicitly perform a write
                                                 // buffer flushing before
                                                 // invalidating the IOTLB.[/p]

    }                                field;
    uint64_t                         val;
} IOTLB_REG_0_0_0_VTDBAR_t;
#endif
#define IOTLB_REG_0_0_0_VTDBAR_OFFSET 0x08
#define IOTLB_REG_0_0_0_VTDBAR_SCOPE 0x01
#define IOTLB_REG_0_0_0_VTDBAR_SIZE 64
#define IOTLB_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x06
#define IOTLB_REG_0_0_0_VTDBAR_RESET 0x200000000000000

#define IOTLB_REG_0_0_0_VTDBAR_DID_LSB 0x0020
#define IOTLB_REG_0_0_0_VTDBAR_DID_MSB 0x002f
#define IOTLB_REG_0_0_0_VTDBAR_DID_RANGE 0x0010
#define IOTLB_REG_0_0_0_VTDBAR_DID_MASK 0xffff00000000
#define IOTLB_REG_0_0_0_VTDBAR_DID_RESET_VALUE 0x00000000

#define IOTLB_REG_0_0_0_VTDBAR_DW_LSB 0x0030
#define IOTLB_REG_0_0_0_VTDBAR_DW_MSB 0x0030
#define IOTLB_REG_0_0_0_VTDBAR_DW_RANGE 0x0001
#define IOTLB_REG_0_0_0_VTDBAR_DW_MASK 0x1000000000000
#define IOTLB_REG_0_0_0_VTDBAR_DW_RESET_VALUE 0x00000000

#define IOTLB_REG_0_0_0_VTDBAR_DR_LSB 0x0031
#define IOTLB_REG_0_0_0_VTDBAR_DR_MSB 0x0031
#define IOTLB_REG_0_0_0_VTDBAR_DR_RANGE 0x0001
#define IOTLB_REG_0_0_0_VTDBAR_DR_MASK 0x2000000000000
#define IOTLB_REG_0_0_0_VTDBAR_DR_RESET_VALUE 0x00000000

#define IOTLB_REG_0_0_0_VTDBAR_IAIG_LSB 0x0039
#define IOTLB_REG_0_0_0_VTDBAR_IAIG_MSB 0x003a
#define IOTLB_REG_0_0_0_VTDBAR_IAIG_RANGE 0x0002
#define IOTLB_REG_0_0_0_VTDBAR_IAIG_MASK 0x600000000000000
#define IOTLB_REG_0_0_0_VTDBAR_IAIG_RESET_VALUE 0x00000001

#define IOTLB_REG_0_0_0_VTDBAR_IIRG_LSB 0x003c
#define IOTLB_REG_0_0_0_VTDBAR_IIRG_MSB 0x003d
#define IOTLB_REG_0_0_0_VTDBAR_IIRG_RANGE 0x0002
#define IOTLB_REG_0_0_0_VTDBAR_IIRG_MASK 0x3000000000000000
#define IOTLB_REG_0_0_0_VTDBAR_IIRG_RESET_VALUE 0x00000000

#define IOTLB_REG_0_0_0_VTDBAR_IVT_LSB 0x003f
#define IOTLB_REG_0_0_0_VTDBAR_IVT_MSB 0x003f
#define IOTLB_REG_0_0_0_VTDBAR_IVT_RANGE 0x0001
#define IOTLB_REG_0_0_0_VTDBAR_IVT_MASK 0x8000000000000000
#define IOTLB_REG_0_0_0_VTDBAR_IVT_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef INTRTADDR_0_0_0_VTDBAR_FLAG
#define INTRTADDR_0_0_0_VTDBAR_FLAG
// INTRTADDR_0_0_0_VTDBAR desc:  [p]Register providing the internal base address of root-entry
// table.[/p]
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  RTT                  :   1;    //  [p]This field contains the
                                                 // type of root-table referenced
                                                 // by the Root Table Address
                                                 // (RTA) field after it has been
                                                 // copied internally by the SRTP
                                                 // flow[/p][list][*]0 = Root
                                                 // Table[*]1 = Extended Root
                                                 // Table[/list]
        uint64_t  RTA                  :  52;    //  [p]This register contains the
                                                 // address of the root-entry
                                                 // table after it has been copied
                                                 // internally by the SRTP
                                                 // flow.[/p]

    }                                field;
    uint64_t                         val;
} INTRTADDR_0_0_0_VTDBAR_t;
#endif
#define INTRTADDR_0_0_0_VTDBAR_OFFSET 0x00
#define INTRTADDR_0_0_0_VTDBAR_SCOPE 0x01
#define INTRTADDR_0_0_0_VTDBAR_SIZE 64
#define INTRTADDR_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define INTRTADDR_0_0_0_VTDBAR_RESET 0x00000000

#define INTRTADDR_0_0_0_VTDBAR_RTT_LSB 0x000b
#define INTRTADDR_0_0_0_VTDBAR_RTT_MSB 0x000b
#define INTRTADDR_0_0_0_VTDBAR_RTT_RANGE 0x0001
#define INTRTADDR_0_0_0_VTDBAR_RTT_MASK 0x00000800
#define INTRTADDR_0_0_0_VTDBAR_RTT_RESET_VALUE 0x00000000

#define INTRTADDR_0_0_0_VTDBAR_RTA_LSB 0x000c
#define INTRTADDR_0_0_0_VTDBAR_RTA_MSB 0x003f
#define INTRTADDR_0_0_0_VTDBAR_RTA_RANGE 0x0034
#define INTRTADDR_0_0_0_VTDBAR_RTA_MASK 0xfffffffffffff000
#define INTRTADDR_0_0_0_VTDBAR_RTA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef INTIRTA_0_0_0_VTDBAR_FLAG
#define INTIRTA_0_0_0_VTDBAR_FLAG
// INTIRTA_0_0_0_VTDBAR desc:  Register providing the internal base address of Interrupt remapping
// table.
typedef union {
    struct {
        uint64_t  S                    :   4;    //  [p]This field specifies the
                                                 // size of the interrupt
                                                 // remapping table. The number of
                                                 // entries in the interrupt
                                                 // remapping table is 2(X+1),
                                                 // where X is the value
                                                 // programmed in this field.[/p]
        uint64_t  RSVD_0               :   7;    // Nebulon auto filled RSVD [10:4]
        uint64_t  EIME                 :   1;    //  [p]This field is used by
                                                 // hardware on Intel®64 platforms
                                                 // as follows:[/p][list][*]0 =
                                                 // xAPIC mode is active. Hardware
                                                 // interprets only low 8-bits of
                                                 // Destination-ID field in the
                                                 // IRTEs. The high 24-bits of the
                                                 // Destination-ID field are
                                                 // treated as reserved[*]1 =
                                                 // x2APIC mode is
                                                 // active.[/list][p]Hardware
                                                 // interprets all 32-bits of
                                                 // Destination-ID field in the
                                                 // IRTEs.[/p][p]This field is
                                                 // implemented as RsvdZ on
                                                 // implementations reporting
                                                 // Extended Interrupt Mode (EIM)
                                                 // field as Clear in Extended
                                                 // Capability register.[/p]
        uint64_t  IRTA                 :  52;    //  This register contains the
                                                 // address of the Interrupt
                                                 // root-entry table after it has
                                                 // been copied internally by the
                                                 // SIRTP flow.

    }                                field;
    uint64_t                         val;
} INTIRTA_0_0_0_VTDBAR_t;
#endif
#define INTIRTA_0_0_0_VTDBAR_OFFSET 0x08
#define INTIRTA_0_0_0_VTDBAR_SCOPE 0x01
#define INTIRTA_0_0_0_VTDBAR_SIZE 64
#define INTIRTA_0_0_0_VTDBAR_BITFIELD_COUNT 0x03
#define INTIRTA_0_0_0_VTDBAR_RESET 0x00000000

#define INTIRTA_0_0_0_VTDBAR_S_LSB 0x0000
#define INTIRTA_0_0_0_VTDBAR_S_MSB 0x0003
#define INTIRTA_0_0_0_VTDBAR_S_RANGE 0x0004
#define INTIRTA_0_0_0_VTDBAR_S_MASK 0x0000000f
#define INTIRTA_0_0_0_VTDBAR_S_RESET_VALUE 0x00000000

#define INTIRTA_0_0_0_VTDBAR_EIME_LSB 0x000b
#define INTIRTA_0_0_0_VTDBAR_EIME_MSB 0x000b
#define INTIRTA_0_0_0_VTDBAR_EIME_RANGE 0x0001
#define INTIRTA_0_0_0_VTDBAR_EIME_MASK 0x00000800
#define INTIRTA_0_0_0_VTDBAR_EIME_RESET_VALUE 0x00000000

#define INTIRTA_0_0_0_VTDBAR_IRTA_LSB 0x000c
#define INTIRTA_0_0_0_VTDBAR_IRTA_MSB 0x003f
#define INTIRTA_0_0_0_VTDBAR_IRTA_RANGE 0x0034
#define INTIRTA_0_0_0_VTDBAR_IRTA_MASK 0xfffffffffffff000
#define INTIRTA_0_0_0_VTDBAR_IRTA_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef SAIRDGRP1_0_0_0_VTDBAR_FLAG
#define SAIRDGRP1_0_0_0_VTDBAR_FLAG
// SAIRDGRP1_0_0_0_VTDBAR desc:  Register providing information about read access.
typedef union {
    struct {
        uint64_t  SAIRDGRP1            :  64;    //  Bits set as 1 will allow
                                                 // assigned SAIs read access.

    }                                field;
    uint64_t                         val;
} SAIRDGRP1_0_0_0_VTDBAR_t;
#endif
#define SAIRDGRP1_0_0_0_VTDBAR_OFFSET 0x10
#define SAIRDGRP1_0_0_0_VTDBAR_SCOPE 0x01
#define SAIRDGRP1_0_0_0_VTDBAR_SIZE 64
#define SAIRDGRP1_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define SAIRDGRP1_0_0_0_VTDBAR_RESET 0xc0061000217

#define SAIRDGRP1_0_0_0_VTDBAR_SAIRDGRP1_LSB 0x0000
#define SAIRDGRP1_0_0_0_VTDBAR_SAIRDGRP1_MSB 0x003f
#define SAIRDGRP1_0_0_0_VTDBAR_SAIRDGRP1_RANGE 0x0040
#define SAIRDGRP1_0_0_0_VTDBAR_SAIRDGRP1_MASK 0xffffffffffffffff
#define SAIRDGRP1_0_0_0_VTDBAR_SAIRDGRP1_RESET_VALUE 0xc0061000217


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef SAIWRGRP1_0_0_0_VTDBAR_FLAG
#define SAIWRGRP1_0_0_0_VTDBAR_FLAG
// SAIWRGRP1_0_0_0_VTDBAR desc:  Register providing information about write access.
typedef union {
    struct {
        uint64_t  SAIWRGRP1            :  64;    //  Bits set as 1 will allow
                                                 // assigned SAIs write access.

    }                                field;
    uint64_t                         val;
} SAIWRGRP1_0_0_0_VTDBAR_t;
#endif
#define SAIWRGRP1_0_0_0_VTDBAR_OFFSET 0x18
#define SAIWRGRP1_0_0_0_VTDBAR_SCOPE 0x01
#define SAIWRGRP1_0_0_0_VTDBAR_SIZE 64
#define SAIWRGRP1_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define SAIWRGRP1_0_0_0_VTDBAR_RESET 0xc0061000217

#define SAIWRGRP1_0_0_0_VTDBAR_SAIWRGRP1_LSB 0x0000
#define SAIWRGRP1_0_0_0_VTDBAR_SAIWRGRP1_MSB 0x003f
#define SAIWRGRP1_0_0_0_VTDBAR_SAIWRGRP1_RANGE 0x0040
#define SAIWRGRP1_0_0_0_VTDBAR_SAIWRGRP1_MASK 0xffffffffffffffff
#define SAIWRGRP1_0_0_0_VTDBAR_SAIWRGRP1_RESET_VALUE 0xc0061000217


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef SAICTGRP1_0_0_0_VTDBAR_FLAG
#define SAICTGRP1_0_0_0_VTDBAR_FLAG
// SAICTGRP1_0_0_0_VTDBAR desc:  Register to control read/write access policy.
typedef union {
    struct {
        uint64_t  SAICTGRP1            :  64;    //  Set/reset bits to change the
                                                 // read/write access policy.

    }                                field;
    uint64_t                         val;
} SAICTGRP1_0_0_0_VTDBAR_t;
#endif
#define SAICTGRP1_0_0_0_VTDBAR_OFFSET 0x20
#define SAICTGRP1_0_0_0_VTDBAR_SCOPE 0x01
#define SAICTGRP1_0_0_0_VTDBAR_SIZE 64
#define SAICTGRP1_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define SAICTGRP1_0_0_0_VTDBAR_RESET 0xc0061000200

#define SAICTGRP1_0_0_0_VTDBAR_SAICTGRP1_LSB 0x0000
#define SAICTGRP1_0_0_0_VTDBAR_SAICTGRP1_MSB 0x003f
#define SAICTGRP1_0_0_0_VTDBAR_SAICTGRP1_RANGE 0x0040
#define SAICTGRP1_0_0_0_VTDBAR_SAICTGRP1_MASK 0xffffffffffffffff
#define SAICTGRP1_0_0_0_VTDBAR_SAICTGRP1_RESET_VALUE 0xc0061000200


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef SEQ_LOG_0_0_0_VTDBAR_FLAG
#define SEQ_LOG_0_0_0_VTDBAR_FLAG
// SEQ_LOG_0_0_0_VTDBAR desc:  Register providing the debug data from sequencer.
typedef union {
    struct {
        uint32_t  SEQFLOW              :   5;    //  Sequencer flow.
        uint32_t  RSVD_0               :   3;    // Nebulon auto filled RSVD [7:5]
        uint32_t  SEQSTATE             :   5;    //  Sequencer state within a
                                                 // flow.
        uint32_t  RSVD_1               :   3;    // Nebulon auto filled RSVD [15:13]
        uint32_t  DRPD                 :  11;    //  Dropped Message Log.
        uint32_t  RSVD_2               :   5;    // Nebulon auto filled RSVD [31:27]

    }                                field;
    uint32_t                         val;
} SEQ_LOG_0_0_0_VTDBAR_t;
#endif
#define SEQ_LOG_0_0_0_VTDBAR_OFFSET 0x40
#define SEQ_LOG_0_0_0_VTDBAR_SCOPE 0x01
#define SEQ_LOG_0_0_0_VTDBAR_SIZE 32
#define SEQ_LOG_0_0_0_VTDBAR_BITFIELD_COUNT 0x03
#define SEQ_LOG_0_0_0_VTDBAR_RESET 0x00000000

#define SEQ_LOG_0_0_0_VTDBAR_SEQFLOW_LSB 0x0000
#define SEQ_LOG_0_0_0_VTDBAR_SEQFLOW_MSB 0x0004
#define SEQ_LOG_0_0_0_VTDBAR_SEQFLOW_RANGE 0x0005
#define SEQ_LOG_0_0_0_VTDBAR_SEQFLOW_MASK 0x0000001f
#define SEQ_LOG_0_0_0_VTDBAR_SEQFLOW_RESET_VALUE 0x00000000

#define SEQ_LOG_0_0_0_VTDBAR_SEQSTATE_LSB 0x0008
#define SEQ_LOG_0_0_0_VTDBAR_SEQSTATE_MSB 0x000c
#define SEQ_LOG_0_0_0_VTDBAR_SEQSTATE_RANGE 0x0005
#define SEQ_LOG_0_0_0_VTDBAR_SEQSTATE_MASK 0x00001f00
#define SEQ_LOG_0_0_0_VTDBAR_SEQSTATE_RESET_VALUE 0x00000000

#define SEQ_LOG_0_0_0_VTDBAR_DRPD_LSB 0x0010
#define SEQ_LOG_0_0_0_VTDBAR_DRPD_MSB 0x001a
#define SEQ_LOG_0_0_0_VTDBAR_DRPD_RANGE 0x000b
#define SEQ_LOG_0_0_0_VTDBAR_DRPD_MASK 0x07ff0000
#define SEQ_LOG_0_0_0_VTDBAR_DRPD_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef IOMMU_IDLE_0_0_0_VTDBAR_FLAG
#define IOMMU_IDLE_0_0_0_VTDBAR_FLAG
// IOMMU_IDLE_0_0_0_VTDBAR desc:  Register providing list of idle modules.
typedef union {
    struct {
        uint32_t  IDLE                 :   8;    //  List of reasons for IOMMU
                                                 // being idle.
        uint32_t  RSVD_0               :  24;    // Nebulon auto filled RSVD [31:8]

    }                                field;
    uint32_t                         val;
} IOMMU_IDLE_0_0_0_VTDBAR_t;
#endif
#define IOMMU_IDLE_0_0_0_VTDBAR_OFFSET 0x44
#define IOMMU_IDLE_0_0_0_VTDBAR_SCOPE 0x01
#define IOMMU_IDLE_0_0_0_VTDBAR_SIZE 32
#define IOMMU_IDLE_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define IOMMU_IDLE_0_0_0_VTDBAR_RESET 0x00000000

#define IOMMU_IDLE_0_0_0_VTDBAR_IDLE_LSB 0x0000
#define IOMMU_IDLE_0_0_0_VTDBAR_IDLE_MSB 0x0007
#define IOMMU_IDLE_0_0_0_VTDBAR_IDLE_RANGE 0x0008
#define IOMMU_IDLE_0_0_0_VTDBAR_IDLE_MASK 0x000000ff
#define IOMMU_IDLE_0_0_0_VTDBAR_IDLE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef SLVREQSENT_0_0_0_VTDBAR_FLAG
#define SLVREQSENT_0_0_0_VTDBAR_FLAG
// SLVREQSENT_0_0_0_VTDBAR desc:  Register providing list of slaves to which the sequencer already sent
// the request.
typedef union {
    struct {
        uint32_t  SLVREQSENT           :  32;    //  List of slaves to which
                                                 // request was sent.

    }                                field;
    uint32_t                         val;
} SLVREQSENT_0_0_0_VTDBAR_t;
#endif
#define SLVREQSENT_0_0_0_VTDBAR_OFFSET 0x48
#define SLVREQSENT_0_0_0_VTDBAR_SCOPE 0x01
#define SLVREQSENT_0_0_0_VTDBAR_SIZE 32
#define SLVREQSENT_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define SLVREQSENT_0_0_0_VTDBAR_RESET 0x00000000

#define SLVREQSENT_0_0_0_VTDBAR_SLVREQSENT_LSB 0x0000
#define SLVREQSENT_0_0_0_VTDBAR_SLVREQSENT_MSB 0x001f
#define SLVREQSENT_0_0_0_VTDBAR_SLVREQSENT_RANGE 0x0020
#define SLVREQSENT_0_0_0_VTDBAR_SLVREQSENT_MASK 0xffffffff
#define SLVREQSENT_0_0_0_VTDBAR_SLVREQSENT_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef SLVACKRCVD_0_0_0_VTDBAR_FLAG
#define SLVACKRCVD_0_0_0_VTDBAR_FLAG
// SLVACKRCVD_0_0_0_VTDBAR desc:  Register providing list of slaves which sent acknowledgement to the
// sequencer.
typedef union {
    struct {
        uint32_t  SLVACKRCVD           :  32;    //  List of slaves which sent ack
                                                 // back.

    }                                field;
    uint32_t                         val;
} SLVACKRCVD_0_0_0_VTDBAR_t;
#endif
#define SLVACKRCVD_0_0_0_VTDBAR_OFFSET 0x4c
#define SLVACKRCVD_0_0_0_VTDBAR_SCOPE 0x01
#define SLVACKRCVD_0_0_0_VTDBAR_SIZE 32
#define SLVACKRCVD_0_0_0_VTDBAR_BITFIELD_COUNT 0x01
#define SLVACKRCVD_0_0_0_VTDBAR_RESET 0x00000000

#define SLVACKRCVD_0_0_0_VTDBAR_SLVACKRCVD_LSB 0x0000
#define SLVACKRCVD_0_0_0_VTDBAR_SLVACKRCVD_MSB 0x001f
#define SLVACKRCVD_0_0_0_VTDBAR_SLVACKRCVD_RANGE 0x0020
#define SLVACKRCVD_0_0_0_VTDBAR_SLVACKRCVD_MASK 0xffffffff
#define SLVACKRCVD_0_0_0_VTDBAR_SLVACKRCVD_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRRCAP_0_0_0_VTDBAR_FLAG
#define MTRRCAP_0_0_0_VTDBAR_FLAG
// MTRRCAP_0_0_0_VTDBAR desc:  Register reporting the Memory Type Range Register capability.
typedef union {
    struct {
        uint64_t  VCNT                 :   8;    //  Indicates the number of
                                                 // variable MTRRs are supported.
        uint64_t  FIX                  :   1;    //  Indicates whether Fixed Range
                                                 // MTRRs are supported.
        uint64_t  RSVD_0               :   1;    // Nebulon auto filled RSVD [9:9]
        uint64_t  WC                   :   1;    //  Indicates whether the Write
                                                 // Combining memory type is
                                                 // supported.
        uint64_t  RSVD_1               :  53;    // Nebulon auto filled RSVD [63:11]

    }                                field;
    uint64_t                         val;
} MTRRCAP_0_0_0_VTDBAR_t;
#endif
#define MTRRCAP_0_0_0_VTDBAR_OFFSET 0x00
#define MTRRCAP_0_0_0_VTDBAR_SCOPE 0x01
#define MTRRCAP_0_0_0_VTDBAR_SIZE 64
#define MTRRCAP_0_0_0_VTDBAR_BITFIELD_COUNT 0x03
#define MTRRCAP_0_0_0_VTDBAR_RESET 0x00000000

#define MTRRCAP_0_0_0_VTDBAR_VCNT_LSB 0x0000
#define MTRRCAP_0_0_0_VTDBAR_VCNT_MSB 0x0007
#define MTRRCAP_0_0_0_VTDBAR_VCNT_RANGE 0x0008
#define MTRRCAP_0_0_0_VTDBAR_VCNT_MASK 0x000000ff
#define MTRRCAP_0_0_0_VTDBAR_VCNT_RESET_VALUE 0x00000000

#define MTRRCAP_0_0_0_VTDBAR_FIX_LSB 0x0008
#define MTRRCAP_0_0_0_VTDBAR_FIX_MSB 0x0008
#define MTRRCAP_0_0_0_VTDBAR_FIX_RANGE 0x0001
#define MTRRCAP_0_0_0_VTDBAR_FIX_MASK 0x00000100
#define MTRRCAP_0_0_0_VTDBAR_FIX_RESET_VALUE 0x00000000

#define MTRRCAP_0_0_0_VTDBAR_WC_LSB 0x000a
#define MTRRCAP_0_0_0_VTDBAR_WC_MSB 0x000a
#define MTRRCAP_0_0_0_VTDBAR_WC_RANGE 0x0001
#define MTRRCAP_0_0_0_VTDBAR_WC_MASK 0x00000400
#define MTRRCAP_0_0_0_VTDBAR_WC_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRRDEFAULT_0_0_0_VTDBAR_FLAG
#define MTRRDEFAULT_0_0_0_VTDBAR_FLAG
// MTRRDEFAULT_0_0_0_VTDBAR desc:  Register for enabling and configuring MTRRs.
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Indicates default memory type
                                                 // used for physical memory
                                                 // address ranges that do not
                                                 // have a memory type specified
                                                 // for them by an MTRR. Legal
                                                 // values for this field are
                                                 // 0,1,4, 5 and 6.
        uint64_t  RSVD_0               :   2;    // Nebulon auto filled RSVD [9:8]
        uint64_t  FE                   :   1;    //  When fixed range MTRRs are
                                                 // enabled, they take priority
                                                 // over the variable range MTRRs
                                                 // when overlaps in ranges occur.
                                                 // If the fixed-range MTRRs are
                                                 // disabled, the variable range
                                                 // MTRRs can still be used and
                                                 // can map the range ordinarily
                                                 // covered by the fixed range
                                                 // MTRRs.
        uint64_t  E                    :   1;    //  MTRR Enable -- a value of 0
                                                 // disables MTRRs; UC memory type
                                                 // is applied and FE field has no
                                                 // effect. A value of 1 enables
                                                 // MTRRs; FE field
                                                 // enables/disables fixed-range
                                                 // MTRRs, and areas of memory not
                                                 // mapped by the fixed or
                                                 // variable range MTRRs use the
                                                 // memory type from the MEMTYPE
                                                 // field.
        uint64_t  RSVD_1               :  52;    // Nebulon auto filled RSVD [63:12]

    }                                field;
    uint64_t                         val;
} MTRRDEFAULT_0_0_0_VTDBAR_t;
#endif
#define MTRRDEFAULT_0_0_0_VTDBAR_OFFSET 0x08
#define MTRRDEFAULT_0_0_0_VTDBAR_SCOPE 0x01
#define MTRRDEFAULT_0_0_0_VTDBAR_SIZE 64
#define MTRRDEFAULT_0_0_0_VTDBAR_BITFIELD_COUNT 0x03
#define MTRRDEFAULT_0_0_0_VTDBAR_RESET 0x00000000

#define MTRRDEFAULT_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRRDEFAULT_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRRDEFAULT_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRRDEFAULT_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRRDEFAULT_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRRDEFAULT_0_0_0_VTDBAR_FE_LSB 0x000a
#define MTRRDEFAULT_0_0_0_VTDBAR_FE_MSB 0x000a
#define MTRRDEFAULT_0_0_0_VTDBAR_FE_RANGE 0x0001
#define MTRRDEFAULT_0_0_0_VTDBAR_FE_MASK 0x00000400
#define MTRRDEFAULT_0_0_0_VTDBAR_FE_RESET_VALUE 0x00000000

#define MTRRDEFAULT_0_0_0_VTDBAR_E_LSB 0x000b
#define MTRRDEFAULT_0_0_0_VTDBAR_E_MSB 0x000b
#define MTRRDEFAULT_0_0_0_VTDBAR_E_RANGE 0x0001
#define MTRRDEFAULT_0_0_0_VTDBAR_E_MASK 0x00000800
#define MTRRDEFAULT_0_0_0_VTDBAR_E_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX64K_00000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 64K memory space from 0x00000 -
// 0x7FFFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_OFFSET 0x20
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX16K_80000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 16K memory space from 0x80000 -
// 0x9FFFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_OFFSET 0x28
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 16K memory space from 0xA0000 -
// 0xBFFFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_OFFSET 0x30
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space 0xC0000 - 0xC7FFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_OFFSET 0x38
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space from 0xC8000 - 0xCFFFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_OFFSET 0x40
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space from 0xD0000 - 0xD7FFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_OFFSET 0x48
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space from 0xD80000 -
// 0xDFFFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_OFFSET 0x50
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space from 0xE0000 - 0xE7FFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_OFFSET 0x58
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space from 0xE8000 - 0xEFFFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_OFFSET 0x60
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space from 0xF0000 - 0xF7FFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_OFFSET 0x68
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_FLAG
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_FLAG
// MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR desc:  Fixed Range MTRR covering the 4K memory space from 0xF8000 - 0xFFFFF.
typedef union {
    struct {
        uint64_t  R0                   :   8;    //  Register Field 0
        uint64_t  R1                   :   8;    //  Register Field 1
        uint64_t  R2                   :   8;    //  Register Field 2
        uint64_t  R3                   :   8;    //  Register Field 3
        uint64_t  R4                   :   8;    //  Register Field 4
        uint64_t  R5                   :   8;    //  Register Field 5
        uint64_t  R6                   :   8;    //  Register Field 6
        uint64_t  R7                   :   8;    //  Register Field 7

    }                                field;
    uint64_t                         val;
} MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_OFFSET 0x70
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x08
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R0_LSB 0x0000
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R0_MSB 0x0007
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R0_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R0_MASK 0x000000ff
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R0_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R1_LSB 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R1_MSB 0x000f
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R1_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R1_MASK 0x0000ff00
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R1_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R2_LSB 0x0010
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R2_MSB 0x0017
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R2_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R2_MASK 0x00ff0000
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R2_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R3_LSB 0x0018
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R3_MSB 0x001f
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R3_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R3_MASK 0xff000000
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R3_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R4_LSB 0x0020
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R4_MSB 0x0027
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R4_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R4_MASK 0xff00000000
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R4_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R5_LSB 0x0028
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R5_MSB 0x002f
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R5_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R5_MASK 0xff0000000000
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R5_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R6_LSB 0x0030
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R6_MSB 0x0037
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R6_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R6_MASK 0xff000000000000
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R6_RESET_VALUE 0x00000000

#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R7_LSB 0x0038
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R7_MSB 0x003f
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R7_RANGE 0x0008
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R7_MASK 0xff00000000000000
#define MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_R7_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE0_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE0
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 0
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 0
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_OFFSET 0x80
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE1_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE1
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 1
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 1
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_OFFSET 0x90
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE2_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE2
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 2
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 2
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_OFFSET 0xa0
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE3_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE3
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 3
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 3
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_OFFSET 0xb0
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE4_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE4
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 4
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 4
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_OFFSET 0xc0
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE5_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE5
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 5
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 5
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_OFFSET 0xd0
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE6_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE6
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 6
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 6
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_OFFSET 0xe0
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE7_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE7
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 7
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 7
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_OFFSET 0xf0
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE8_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE8
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 8
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 8
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_OFFSET 0x00
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSBASE9_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR BASE9
typedef union {
    struct {
        uint64_t  MEMTYPE              :   8;    //  Memory type for variable
                                                 // memory type range 9
        uint64_t  RSVD_0               :   4;    // Nebulon auto filled RSVD [11:8]
        uint64_t  PHYSBASE             :  27;    //  Base Address for variable
                                                 // memory type range 9
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_OFFSET 0x10
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_MEMTYPE_LSB 0x0000
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_MEMTYPE_MSB 0x0007
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_MEMTYPE_RANGE 0x0008
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_MEMTYPE_MASK 0x000000ff
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_MEMTYPE_RESET_VALUE 0x00000000

#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_PHYSBASE_LSB 0x000c
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_PHYSBASE_MSB 0x0026
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_PHYSBASE_RANGE 0x001b
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_PHYSBASE_MASK 0x7ffffff000
#define MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_PHYSBASE_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK0_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK0
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 0 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 0
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_OFFSET 0x88
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK1_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK1
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 1 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 1
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_OFFSET 0x98
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK2_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK2
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 2 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 2
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_OFFSET 0xa8
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK3_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK3
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 3 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 3
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_OFFSET 0xb8
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK4_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK4
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 4 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 4
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_OFFSET 0xc8
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK5_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK5
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 5 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 5
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_OFFSET 0xd8
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK6_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK6
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 6 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 6
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_OFFSET 0xe8
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK7_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK7
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 7 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 7
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_OFFSET 0xf8
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK8_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK8
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 8 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 8
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_OFFSET 0x08
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

#ifndef MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_FLAG
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_FLAG
// MTRR_PHYSMASK9_REG_0_0_0_VTDBAR desc:  Variable-Range MTRR MASK9
typedef union {
    struct {
        uint64_t  RSVD_0               : 11;    // Nebulon auto filled RSVD [0:10]
        uint64_t  VALID                :   1;    //  Valid bit for variable range
                                                 // 9 mask
        uint64_t  PHYSMASK             :  27;    //  Address mask for variable
                                                 // memory type range 9
        uint64_t  RSVD_1               :  25;    // Nebulon auto filled RSVD [63:39]

    }                                field;
    uint64_t                         val;
} MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_t;
#endif
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_OFFSET 0x18
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_SCOPE 0x01
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_SIZE 64
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_BITFIELD_COUNT 0x02
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_RESET 0x00000000

#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_VALID_LSB 0x000b
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_VALID_MSB 0x000b
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_VALID_RANGE 0x0001
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_VALID_MASK 0x00000800
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_VALID_RESET_VALUE 0x00000000

#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_PHYSMASK_LSB 0x000c
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_PHYSMASK_MSB 0x0026
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_PHYSMASK_RANGE 0x001b
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_PHYSMASK_MASK 0x7ffffff000
#define MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_PHYSMASK_RESET_VALUE 0x00000000


// --------------------------------------------------------------------------------------------------------------------------------

// starting the array instantiation section
typedef struct {
    VER_REG_0_0_0_VTDBAR_t     VER_REG_0_0_0_VTDBAR; // offset 4'h0, width 32
    uint8_t                    rsvd0[4];
    CAP_REG_0_0_0_VTDBAR_t     CAP_REG_0_0_0_VTDBAR; // offset 4'h8, width 64
    ECAP_REG_0_0_0_VTDBAR_t    ECAP_REG_0_0_0_VTDBAR; // offset 8'h10, width 64
    GCMD_REG_0_0_0_VTDBAR_t    GCMD_REG_0_0_0_VTDBAR; // offset 8'h18, width 32
    GSTS_REG_0_0_0_VTDBAR_t    GSTS_REG_0_0_0_VTDBAR; // offset 8'h1C, width 32
    RTADDR_REG_0_0_0_VTDBAR_t  RTADDR_REG_0_0_0_VTDBAR; // offset 8'h20, width 64
    CCMD_REG_0_0_0_VTDBAR_t    CCMD_REG_0_0_0_VTDBAR; // offset 8'h28, width 64
    uint8_t                    rsvd1[4];
    FSTS_REG_0_0_0_VTDBAR_t    FSTS_REG_0_0_0_VTDBAR; // offset 8'h34, width 32
    FECTL_REG_0_0_0_VTDBAR_t   FECTL_REG_0_0_0_VTDBAR; // offset 8'h38, width 32
    FEDATA_REG_0_0_0_VTDBAR_t  FEDATA_REG_0_0_0_VTDBAR; // offset 8'h3C, width 32
    FEADDR_REG_0_0_0_VTDBAR_t  FEADDR_REG_0_0_0_VTDBAR; // offset 8'h40, width 32
    FEUADDR_REG_0_0_0_VTDBAR_t FEUADDR_REG_0_0_0_VTDBAR; // offset 8'h44, width 32
    uint8_t                    rsvd2[16];
    AFLOG_REG_0_0_0_VTDBAR_t   AFLOG_REG_0_0_0_VTDBAR; // offset 8'h58, width 64
    uint8_t                    rsvd3[4];
    PMEN_REG_0_0_0_VTDBAR_t    PMEN_REG_0_0_0_VTDBAR; // offset 8'h64, width 32
    PLMBASE_REG_0_0_0_VTDBAR_t PLMBASE_REG_0_0_0_VTDBAR; // offset 8'h68, width 32
    PLMLIMIT_REG_0_0_0_VTDBAR_t PLMLIMIT_REG_0_0_0_VTDBAR; // offset 8'h6C, width 32
    PHMBASE_REG_0_0_0_VTDBAR_t PHMBASE_REG_0_0_0_VTDBAR; // offset 8'h70, width 64
    PHMLIMIT_REG_0_0_0_VTDBAR_t PHMLIMIT_REG_0_0_0_VTDBAR; // offset 8'h78, width 64
    IQH_REG_0_0_0_VTDBAR_t     IQH_REG_0_0_0_VTDBAR; // offset 8'h80, width 64
    IQT_REG_0_0_0_VTDBAR_t     IQT_REG_0_0_0_VTDBAR; // offset 8'h88, width 64
    IQA_REG_0_0_0_VTDBAR_t     IQA_REG_0_0_0_VTDBAR; // offset 8'h90, width 64
    uint8_t                    rsvd4[4];
    ICS_REG_0_0_0_VTDBAR_t     ICS_REG_0_0_0_VTDBAR; // offset 8'h9C, width 32
    IECTL_REG_0_0_0_VTDBAR_t   IECTL_REG_0_0_0_VTDBAR; // offset 12'h0A0, width 32
    IEDATA_REG_0_0_0_VTDBAR_t  IEDATA_REG_0_0_0_VTDBAR; // offset 12'h0A4, width 32
    IEADDR_REG_0_0_0_VTDBAR_t  IEADDR_REG_0_0_0_VTDBAR; // offset 12'h0A8, width 32
    IEUADDR_REG_0_0_0_VTDBAR_t IEUADDR_REG_0_0_0_VTDBAR; // offset 12'h0AC, width 32
    uint8_t                    rsvd5[8];
    IRTA_REG_0_0_0_VTDBAR_t    IRTA_REG_0_0_0_VTDBAR; // offset 12'h0B8, width 64
    uint8_t                    rsvd6[28];
    PRESTS_REG_0_0_0_VTDBAR_t  PRESTS_REG_0_0_0_VTDBAR; // offset 12'h0DC, width 32
    PRECTL_REG_0_0_0_VTDBAR_t  PRECTL_REG_0_0_0_VTDBAR; // offset 12'h0E0, width 32
    PREDATA_REG_0_0_0_VTDBAR_t PREDATA_REG_0_0_0_VTDBAR; // offset 12'h0E4, width 32
    PREADDR_REG_0_0_0_VTDBAR_t PREADDR_REG_0_0_0_VTDBAR; // offset 12'h0E8, width 32
    PREUADDR_REG_0_0_0_VTDBAR_t PREUADDR_REG_0_0_0_VTDBAR; // offset 12'h0EC, width 32
    uint8_t                    rsvd7[784];
    FRCDL_REG_0_0_0_VTDBAR_t   FRCDL_REG_0_0_0_VTDBAR; // offset 12'h400, width 64
    FRCDH_REG_0_0_0_VTDBAR_t   FRCDH_REG_0_0_0_VTDBAR; // offset 12'h408, width 64
    uint8_t                    rsvd8[240];
    IVA_REG_0_0_0_VTDBAR_t     IVA_REG_0_0_0_VTDBAR; // offset 12'h500, width 64
    IOTLB_REG_0_0_0_VTDBAR_t   IOTLB_REG_0_0_0_VTDBAR; // offset 12'h508, width 64
    uint8_t                    rsvd9[2544];
    INTRTADDR_0_0_0_VTDBAR_t   INTRTADDR_0_0_0_VTDBAR; // offset 16'h0F00, width 64
    INTIRTA_0_0_0_VTDBAR_t     INTIRTA_0_0_0_VTDBAR; // offset 16'h0F08, width 64
    SAIRDGRP1_0_0_0_VTDBAR_t   SAIRDGRP1_0_0_0_VTDBAR; // offset 16'h0F10, width 64
    SAIWRGRP1_0_0_0_VTDBAR_t   SAIWRGRP1_0_0_0_VTDBAR; // offset 16'h0F18, width 64
    SAICTGRP1_0_0_0_VTDBAR_t   SAICTGRP1_0_0_0_VTDBAR; // offset 16'h0F20, width 64
    uint8_t                    rsvd10[24];
    SEQ_LOG_0_0_0_VTDBAR_t     SEQ_LOG_0_0_0_VTDBAR; // offset 16'h0F40, width 32
    IOMMU_IDLE_0_0_0_VTDBAR_t  IOMMU_IDLE_0_0_0_VTDBAR; // offset 16'h0F44, width 32
    SLVREQSENT_0_0_0_VTDBAR_t  SLVREQSENT_0_0_0_VTDBAR; // offset 16'h0F48, width 32
    SLVACKRCVD_0_0_0_VTDBAR_t  SLVACKRCVD_0_0_0_VTDBAR; // offset 16'h0F4C, width 32
    MTRRCAP_0_0_0_VTDBAR_t     MTRRCAP_0_0_0_VTDBAR; // offset 12'h100, width 64
    MTRRDEFAULT_0_0_0_VTDBAR_t MTRRDEFAULT_0_0_0_VTDBAR; // offset 12'h108, width 64
    uint8_t                    rsvd11[16];
    MTRR_FIX64K_00000_REG_0_0_0_VTDBAR_t MTRR_FIX64K_00000_REG_0_0_0_VTDBAR; // offset 12'h120, width 64
    MTRR_FIX16K_80000_REG_0_0_0_VTDBAR_t MTRR_FIX16K_80000_REG_0_0_0_VTDBAR; // offset 12'h128, width 64
    MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR_t MTRR_FIX16K_A0000_REG_0_0_0_VTDBAR; // offset 12'h130, width 64
    MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_C0000_REG_0_0_0_VTDBAR; // offset 12'h138, width 64
    MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_C8000_REG_0_0_0_VTDBAR; // offset 12'h140, width 64
    MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_D0000_REG_0_0_0_VTDBAR; // offset 12'h148, width 64
    MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_D8000_REG_0_0_0_VTDBAR; // offset 12'h150, width 64
    MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_E0000_REG_0_0_0_VTDBAR; // offset 12'h158, width 64
    MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_E8000_REG_0_0_0_VTDBAR; // offset 12'h160, width 64
    MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_F0000_REG_0_0_0_VTDBAR; // offset 12'h168, width 64
    MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR_t MTRR_FIX4K_F8000_REG_0_0_0_VTDBAR; // offset 12'h170, width 64
    uint8_t                    rsvd12[8];
    MTRR_PHYSBASE0_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE0_REG_0_0_0_VTDBAR; // offset 12'h180, width 64
    uint8_t                    rsvd13[8];
    MTRR_PHYSBASE1_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE1_REG_0_0_0_VTDBAR; // offset 12'h190, width 64
    uint8_t                    rsvd14[8];
    MTRR_PHYSBASE2_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE2_REG_0_0_0_VTDBAR; // offset 12'h1A0, width 64
    uint8_t                    rsvd15[8];
    MTRR_PHYSBASE3_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE3_REG_0_0_0_VTDBAR; // offset 12'h1B0, width 64
    uint8_t                    rsvd16[8];
    MTRR_PHYSBASE4_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE4_REG_0_0_0_VTDBAR; // offset 12'h1C0, width 64
    uint8_t                    rsvd17[8];
    MTRR_PHYSBASE5_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE5_REG_0_0_0_VTDBAR; // offset 12'h1D0, width 64
    uint8_t                    rsvd18[8];
    MTRR_PHYSBASE6_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE6_REG_0_0_0_VTDBAR; // offset 12'h1E0, width 64
    uint8_t                    rsvd19[8];
    MTRR_PHYSBASE7_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE7_REG_0_0_0_VTDBAR; // offset 12'h1F0, width 64
    uint8_t                    rsvd20[8];
    MTRR_PHYSBASE8_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE8_REG_0_0_0_VTDBAR; // offset 12'h200, width 64
    uint8_t                    rsvd21[8];
    MTRR_PHYSBASE9_REG_0_0_0_VTDBAR_t MTRR_PHYSBASE9_REG_0_0_0_VTDBAR; // offset 12'h210, width 64
    MTRR_PHYSMASK0_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK0_REG_0_0_0_VTDBAR; // offset 12'h188, width 64
    uint8_t                    rsvd22[8];
    MTRR_PHYSMASK1_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK1_REG_0_0_0_VTDBAR; // offset 12'h198, width 64
    uint8_t                    rsvd23[8];
    MTRR_PHYSMASK2_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK2_REG_0_0_0_VTDBAR; // offset 12'h1A8, width 64
    uint8_t                    rsvd24[8];
    MTRR_PHYSMASK3_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK3_REG_0_0_0_VTDBAR; // offset 12'h1B8, width 64
    uint8_t                    rsvd25[8];
    MTRR_PHYSMASK4_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK4_REG_0_0_0_VTDBAR; // offset 12'h1C8, width 64
    uint8_t                    rsvd26[8];
    MTRR_PHYSMASK5_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK5_REG_0_0_0_VTDBAR; // offset 12'h1D8, width 64
    uint8_t                    rsvd27[8];
    MTRR_PHYSMASK6_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK6_REG_0_0_0_VTDBAR; // offset 12'h1E8, width 64
    uint8_t                    rsvd28[8];
    MTRR_PHYSMASK7_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK7_REG_0_0_0_VTDBAR; // offset 12'h1F8, width 64
    uint8_t                    rsvd29[8];
    MTRR_PHYSMASK8_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK8_REG_0_0_0_VTDBAR; // offset 12'h208, width 64
    uint8_t                    rsvd30[8];
    MTRR_PHYSMASK9_REG_0_0_0_VTDBAR_t MTRR_PHYSMASK9_REG_0_0_0_VTDBAR; // offset 12'h218, width 64
} iommu_cr_top_t;                                // size:  16'h1100


#endif // _IOMMU_CR_TOP_REGS_H_
