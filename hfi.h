#ifndef HFI_H
#define HFI_H
/* TODO: Need copyright */
/* ISSUE: expects the includer to #include stuff */

#define HFI_DRIVER_NAME "hfi"
#define HFI_DRIVER_VERSION "0.0.1"

struct hfi_devdata {
	struct pci_dev *pdev;

	/* device number, e.g. hfi0 */
	int devnum;

	/* memory-mapped chip registers, start and end */
	u64 __iomem *kregbase;
	u64 __iomem *kregend;
	/* device bar0 */
	u64 bar0;
	u64 bar0len;
};
#endif /* HFI_H */
