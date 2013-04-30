#ifndef HFI_H
#define HFI_H
/* TODO: Need copyright */
/* ISSUE: expects the includer to #include stuff */

#include <linux/cdev.h>

#define HFI_DRIVER_NAME "hfi"
#define HFI_DRIVER_VERSION "0.0.1"

struct hfi_devdata {
	struct pci_dev *pdev;
	/* user space character device structures */
	struct cdev ui_cdev;
	struct device *ui_device;

	/* device number, e.g. hfi0 */
	int devnum;

	/* memory-mapped chip registers, start and end */
	void __iomem *kregbase;
	void __iomem *kregend;
	/* device bar0 details */
	u64 bar0;
	u64 bar0len;
};


/* hfi_file_ops.c */
int __init device_file_init(void);
void device_file_cleanup(void);
int hfi_device_create(struct hfi_devdata *dd);
void hfi_device_remove(struct hfi_devdata *dd);

#endif /* HFI_H */
