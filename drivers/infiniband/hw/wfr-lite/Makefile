obj-$(CONFIG_INFINIBAND_WFR_LITE) += ib_wfr_lite.o

ib_wfr_lite-y := wfrl_cq.o wfrl_diag.o wfrl_dma.o wfrl_driver.o wfrl_eeprom.o \
	wfrl_file_ops.o wfrl_fs.o wfrl_init.o wfrl_intr.o wfrl_keys.o \
	wfrl_mad.o wfrl_mmap.o wfrl_mr.o wfrl_pcie.o wfrl_pio_copy.o \
	wfrl_qp.o wfrl_qsfp.o wfrl_rc.o wfrl_ruc.o wfrl_sdma.o wfrl_srq.o \
	wfrl_sysfs.o wfrl_twsi.o wfrl_tx.o wfrl_uc.o wfrl_ud.o \
	wfrl_user_pages.o wfrl_user_sdma.o wfrl_verbs_mcast.o wfrl_iba7220.o \
	wfrl_sd7220.o wfrl_iba7322.o wfrl_snoop.o wfrl_verbs.o \
	wfrl_pma.o

# 6120 has no fallback if no MSI interrupts, others can do INTx
ib_wfr_lite-$(CONFIG_PCI_MSI) += wfrl_iba6120.o

ib_wfr_lite-$(CONFIG_X86_64) += wfrl_wc_x86_64.o
ib_wfr_lite-$(CONFIG_PPC64) += wfrl_wc_ppc64.o
