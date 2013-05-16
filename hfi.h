#ifndef HFI_H
#define HFI_H
/* TODO: Need copyright */
/* ISSUE: expects the includer to #include stuff */

#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/notifier.h>

#define HFI_DRIVER_NAME "hfi"
#define HFI_DRIVER_VERSION "0.0.1"

#define WFR_NUM_MSIX 256	/* # MSI-X interrupts supported */

struct hfi_ib_device;

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
	/* MSI-X interrupt vectors */
	struct msix_entry msix[WFR_NUM_MSIX];
	int nmsi;

	/* IB-specific things */
	struct hfi_ib_device *vdev;	/* verbs device */
};


static inline struct hfi_ib_device *vdev_from_dd(struct hfi_devdata *dd)
{
	return dd->vdev;
}


/* file_ops.c */
int __init device_file_init(void);
void device_file_cleanup(void);
int hfi_device_create(struct hfi_devdata *dd);
void hfi_device_remove(struct hfi_devdata *dd);


/* firmware.c */
int load_device_firmware(struct hfi_devdata *dd);
void unload_device_firmware(struct hfi_devdata *dd);

/* verbs.c */
int register_ib_device(struct hfi_devdata *dd);
void unregister_ib_device(struct hfi_devdata *dd);


#endif /* HFI_H */
