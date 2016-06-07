// Auto-generarted on 03/19/2015 12:00:47
// Copyright 2010-2014 Intel Corporation

#ifndef TRANSLATION_STRUCTS_H
#define TRANSLATION_STRUCTS_H

// RootEntry
union RootEntry_t {
    struct {
    struct {
        uint64_t P		: 1;
        uint64_t RSVD0		: 11;
        uint64_t CTP		: 52;
    };
    struct {
        uint64_t RSVD1		: 64;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// ExtendedRootEntry
union ExtendedRootEntry_t {
    struct {
    struct {
        uint64_t LP		: 1;
        uint64_t RSVD0		: 11;
        uint64_t LCTP		: 52;
    };
    struct {
        uint64_t UP		: 1;
        uint64_t RSVD1		: 11;
        uint64_t UCTP		: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// ContextEntry
union ContextEntry_t {
    struct {
    struct {
        uint64_t P		: 1;
        uint64_t FPD		: 1;
        uint64_t T		: 2;
        uint64_t RSVD0		: 8;
        uint64_t SLPTPTR	: 52;
    };
    struct {
        uint64_t AW		: 3;
        uint64_t AVAIL		: 4;
        uint64_t RSVD1		: 1;
        uint64_t DID		: 16;
        uint64_t RSVD2		: 40;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// ExtendedContextEntry
union ExtendedContextEntry_t {
    struct {
    struct {
        uint64_t P		: 1;
        uint64_t FPD		: 1;
        uint64_t T		: 3;
        uint64_t EMT		: 3;
        uint64_t DINVE		: 1;
        uint64_t PRE		: 1;
        uint64_t NESTE		: 1;
        uint64_t PASIDE		: 1;
        uint64_t SLPTPTR	: 52;
    };
    struct {
        uint64_t AW		: 3;
        uint64_t PGE		: 1;
        uint64_t NXE		: 1;
        uint64_t WPE		: 1;
        uint64_t CD		: 1;
        uint64_t EMTE		: 1;
        uint64_t DID		: 16;
        uint64_t SMEP		: 1;
        uint64_t EAFE		: 1;
        uint64_t ERE		: 1;
        uint64_t SLEE		: 1;
        uint64_t RSVD0		: 4;
        uint64_t PAT		: 32;
    };
    struct {
        uint64_t PTS		: 4;
        uint64_t RSVD1		: 8;
        uint64_t PASIDPTR	: 52;
    };
    struct {
        uint64_t RSVD2		: 12;
        uint64_t PASIDSTPTR	: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[4];
};

// Pml4eEntry
union Pml4eEntry_t {
    struct {
        uint64_t P		: 1;
        uint64_t R_W		: 1;
        uint64_t U_S		: 1;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t A		: 1;
        uint64_t IGN0		: 1;
        uint64_t RSVD0		: 1;
        uint64_t IGN1		: 2;
        uint64_t EA		: 1;
        uint64_t IGN2		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN3		: 11;
        uint64_t XD		: 1;
    };
    uint64_t val;
};

// PasidEntry
union PasidEntry_t {
    struct {
        uint64_t P		: 1;
        uint64_t RSVD0		: 2;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t RSVD1		: 6;
        uint64_t SRE		: 1;
        uint64_t FLPTPTR	: 52;
    };
    uint64_t val;
};

// PDPEntry1G
union PDPEntry1G_t {
    struct {
        uint64_t P		: 1;
        uint64_t R_W		: 1;
        uint64_t U_S		: 1;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t A		: 1;
        uint64_t D		: 1;
        uint64_t PS		: 1;
        uint64_t G		: 1;
        uint64_t IGN0		: 1;
        uint64_t EA		: 1;
        uint64_t IGN1		: 1;
        uint64_t PAT		: 1;
        uint64_t RSVD0		: 17;
        uint64_t ADDR		: 22;
        uint64_t IGN2		: 11;
        uint64_t XD		: 1;
    };
    uint64_t val;
};

// PDPEntryPD
union PDPEntryPD_t {
    struct {
        uint64_t P		: 1;
        uint64_t R_W		: 1;
        uint64_t U_S		: 1;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t A		: 1;
        uint64_t IGN0		: 1;
        uint64_t PS		: 1;
        uint64_t IGN1		: 2;
        uint64_t EA		: 1;
        uint64_t IGN2		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN3		: 11;
        uint64_t XD		: 1;
    };
    uint64_t val;
};

// PDEntry2MB
union PDEntry2MB_t {
    struct {
        uint64_t P		: 1;
        uint64_t R_W		: 1;
        uint64_t U_S		: 1;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t A		: 1;
        uint64_t D		: 1;
        uint64_t PS		: 1;
        uint64_t G		: 1;
        uint64_t IGN0		: 1;
        uint64_t EA		: 1;
        uint64_t IGN1		: 1;
        uint64_t PAT		: 1;
        uint64_t RSVD0		: 8;
        uint64_t ADDR		: 31;
        uint64_t IGN2		: 11;
        uint64_t XD		: 1;
    };
    uint64_t val;
};

// PDEntryPT
union PDEntryPT_t {
    struct {
        uint64_t P		: 1;
        uint64_t R_W		: 1;
        uint64_t U_S		: 1;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t A		: 1;
        uint64_t IGN0		: 1;
        uint64_t PS		: 1;
        uint64_t IGN1		: 2;
        uint64_t EA		: 1;
        uint64_t IGN2		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN3		: 11;
        uint64_t XD		: 1;
    };
    uint64_t val;
};

// PTEntry
union PTEntry_t {
    struct {
        uint64_t P		: 1;
        uint64_t R_W		: 1;
        uint64_t U_S		: 1;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t A		: 1;
        uint64_t D		: 1;
        uint64_t PAT		: 1;
        uint64_t G		: 1;
        uint64_t IGN0		: 1;
        uint64_t EA		: 1;
        uint64_t IGN1		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN2		: 11;
        uint64_t XD		: 1;
    };
    uint64_t val;
};

// FLPtPtrEntry
union FLPtPtrEntry_t {
    struct {
        uint64_t P		: 1;
        uint64_t R_W		: 1;
        uint64_t U_S		: 1;
        uint64_t PWT		: 1;
        uint64_t PCD		: 1;
        uint64_t A		: 1;
        uint64_t RSVD0		: 4;
        uint64_t EA		: 1;
        uint64_t RSVD1		: 1;
        uint64_t ADDR		: 40;
        uint64_t RSVD2		: 11;
        uint64_t XD		: 1;
    };
    uint64_t val;
};

// SLPtPtrEntry
union SLPtPtrEntry_t {
    struct {
        uint64_t R		: 1;
        uint64_t W		: 1;
        uint64_t X		: 1;
        uint64_t RSVD0		: 4;
        uint64_t PS		: 1;
        uint64_t RSVD1		: 4;
        uint64_t ADDR		: 40;
        uint64_t RSVD2		: 10;
        uint64_t RSVD3		: 1;
    };
    uint64_t val;
};

// SLPml4Entry
union SLPml4Entry_t {
    struct {
        uint64_t R		: 1;
        uint64_t W		: 1;
        uint64_t X		: 1;
        uint64_t IGN0		: 4;
        uint64_t RSVD0		: 1;
        uint64_t IGN1		: 3;
        uint64_t RSVD1		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN2		: 10;
        uint64_t RSVD2		: 1;
        uint64_t IGN3		: 1;
    };
    uint64_t val;
};

// SLPDPEntry1G
union SLPDPEntry1G_t {
    struct {
        uint64_t R		: 1;
        uint64_t W		: 1;
        uint64_t X		: 1;
        uint64_t EMT		: 3;
        uint64_t IPAT		: 1;
        uint64_t PS		: 1;
        uint64_t IGN0		: 3;
        uint64_t SNP		: 1;
        uint64_t RSVD0		: 18;
        uint64_t ADDR		: 22;
        uint64_t IGN1		: 10;
        uint64_t TM		: 1;
        uint64_t IGN2		: 1;
    };
    uint64_t val;
};

// SLPDPEntryPD
union SLPDPEntryPD_t {
    struct {
        uint64_t R		: 1;
        uint64_t W		: 1;
        uint64_t X		: 1;
        uint64_t IGN0		: 4;
        uint64_t PS		: 1;
        uint64_t IGN1		: 3;
        uint64_t RSVD0		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN2		: 10;
        uint64_t RSVD1		: 1;
        uint64_t IGN3		: 1;
    };
    uint64_t val;
};

// SLPDEntry2MB
union SLPDEntry2MB_t {
    struct {
        uint64_t R		: 1;
        uint64_t W		: 1;
        uint64_t X		: 1;
        uint64_t EMT		: 3;
        uint64_t IPAT		: 1;
        uint64_t PS		: 1;
        uint64_t IGN0		: 3;
        uint64_t SNP		: 1;
        uint64_t RSVD0		: 9;
        uint64_t ADDR		: 31;
        uint64_t IGN1		: 10;
        uint64_t TM		: 1;
        uint64_t IGN2		: 1;
    };
    uint64_t val;
};

// SLPDEntryPT
union SLPDEntryPT_t {
    struct {
        uint64_t R		: 1;
        uint64_t W		: 1;
        uint64_t X		: 1;
        uint64_t IGN0		: 4;
        uint64_t PS		: 1;
        uint64_t IGN1		: 3;
        uint64_t RSVD0		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN2		: 10;
        uint64_t RSVD1		: 1;
        uint64_t IGN3		: 1;
    };
    uint64_t val;
};

// SLPTEntry
union SLPTEntry_t {
    struct {
        uint64_t R		: 1;
        uint64_t W		: 1;
        uint64_t X		: 1;
        uint64_t EMT		: 3;
        uint64_t IPAT		: 1;
        uint64_t IGN0		: 4;
        uint64_t SNP		: 1;
        uint64_t ADDR		: 40;
        uint64_t IGN1		: 10;
        uint64_t TM		: 1;
        uint64_t IGN2		: 1;
    };
    uint64_t val;
};

// cc_inv_dsc
union cc_inv_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t G		: 2;
        uint64_t RSVD0		: 10;
        uint64_t DID		: 16;
        uint64_t SID		: 16;
        uint64_t FM		: 2;
        uint64_t RSVD1		: 14;
    };
    struct {
        uint64_t RSVD2		: 64;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// pc_inv_dsc
union pc_inv_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t G		: 2;
        uint64_t RSVD0		: 10;
        uint64_t DID		: 16;
        uint64_t PASID		: 20;
        uint64_t RSVD1		: 12;
    };
    struct {
        uint64_t RSVD2		: 64;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// iotlb_inv_dsc
union iotlb_inv_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t G		: 2;
        uint64_t DW		: 1;
        uint64_t DR		: 1;
        uint64_t RSVD0		: 8;
        uint64_t DID		: 16;
        uint64_t RSVD1		: 32;
    };
    struct {
        uint64_t AM		: 6;
        uint64_t IH		: 1;
        uint64_t RSVD2		: 5;
        uint64_t ADDR		: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// ext_iotlb_inv_dsc
union ext_iotlb_inv_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t G		: 2;
        uint64_t RSVD0		: 10;
        uint64_t DID		: 16;
        uint64_t PASID		: 20;
        uint64_t RSVD1		: 12;
    };
    struct {
        uint64_t AM		: 6;
        uint64_t IH		: 1;
        uint64_t GL		: 1;
        uint64_t RSVD2		: 4;
        uint64_t ADDR		: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// dev_tlb_inv_dsc
union dev_tlb_inv_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t RSVD0		: 12;
        uint64_t MaxInvsPend	: 5;
        uint64_t RSVD1		: 11;
        uint64_t SID		: 16;
        uint64_t RSVD2		: 16;
    };
    struct {
        uint64_t S		: 1;
        uint64_t RSVD3		: 11;
        uint64_t ADDR		: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// ext_dev_tlb_inv_dsc
union ext_dev_tlb_inv_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t MaxInvsPend	: 5;
        uint64_t RSVD0		: 7;
        uint64_t SID		: 16;
        uint64_t PASID		: 20;
        uint64_t RSVD1		: 12;
    };
    struct {
        uint64_t G		: 1;
        uint64_t RSVD2		: 10;
        uint64_t S		: 1;
        uint64_t ADDR		: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// iec_inv_dsc
union iec_inv_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t G		: 1;
        uint64_t RSVD0		: 22;
        uint64_t IM		: 5;
        uint64_t IIDX		: 16;
        uint64_t RSVD1		: 16;
    };
    struct {
        uint64_t RSVD2		: 64;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// inv_wait_dsc
union inv_wait_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t IF		: 1;
        uint64_t SW		: 1;
        uint64_t FN		: 1;
        uint64_t RSVD0		: 25;
        uint64_t StatusData	: 32;
    };
    struct {
        uint64_t RSVD1		: 2;
        uint64_t StatusAddress	: 62;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// page_req_dsc
union page_req_dsc_t {
    struct {
    struct {
        uint64_t SRR		: 1;
        uint64_t BOF		: 1;
        uint64_t PasidPresent	: 1;
        uint64_t LPIG		: 1;
        uint64_t PASID		: 20;
        uint64_t BusNum		: 8;
        uint64_t PrivateData	: 23;
        uint64_t PRGI		: 9;
    };
    struct {
        uint64_t READ		: 1;
        uint64_t WRITE		: 1;
        uint64_t EXEC		: 1;
        uint64_t PRIV		: 1;
        uint64_t DevFunc	: 8;
        uint64_t ADDR		: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// page_grp_resp_dsc
union page_grp_resp_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t PasidPresent	: 1;
        uint64_t RSVD0		: 11;
        uint64_t RequesterID	: 16;
        uint64_t PASID		: 20;
        uint64_t RSVD1		: 12;
    };
    struct {
        uint64_t ResponseCode	: 4;
        uint64_t RSVD2		: 28;
        uint64_t PrivateData	: 23;
        uint64_t PRGI		: 9;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

// page_strm_resp_dsc
union page_strm_resp_dsc_t {
    struct {
    struct {
        uint64_t TYPE		: 4;
        uint64_t PASID		: 20;
        uint64_t BusNum		: 8;
        uint64_t PrivateData	: 23;
        uint64_t PRGI		: 9;
    };
    struct {
        uint64_t ResponseCode	: 4;
        uint64_t DevFunc	: 8;
        uint64_t ADDR		: 52;
    };
    } __attribute__ ((__packed__));
    uint64_t val[2];
};

#endif /* TRANSLATION_STRUCTS_H */
