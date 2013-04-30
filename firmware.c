/* TODO: copyright here */

#include <linux/firmware.h>
#include <linux/pci.h>
#include "hfi.h"

#define WFR_8051_FW_NAME "intel/wfr_8051.fw"
#define WFR_SERDES_FW_NAME "intel/wfr_serdes.fw"

#if 1
/* pretend this worked - need more details before we can load firmware */
int load_device_firmware(struct hfi_devdata *dd)
{
	return 0;
}
#else
static int fimware_8051_loaded(void)
{
	/* TODO: implement */
	return 0; /* nope, not loaded */
}

static int firmware_serdes_loaded(void)
{
	/* TODO: implement */
	return 0; /* nope, not loaded */
}

static int get_firmware(const struct firmware **fw, const char *name,
			struct device *device)
{
	int ret;

	ret = request_firmware(fw, name, device);
	if (ret) {
		*fw = NULL;
		pr_err(HFI_DRIVER_NAME": cannot load firmware %s\n",
			name);
	}
	return ret;
}

/* return 0 on sucess, -ERRNO on error */
int load_device_firmware(struct hfi_devdata *dd)
{
	const struct firmware *fw_8051 = NULL;
	const struct firmware *fw_serdes = NULL;
	int ret, have_8051, have_serdes;

	have_8051 = fimware_8051_loaded();
	have_serdes = firmware_serdes_loaded();
	if (have_8051 && have_serdes)
		return 0;

	/* we shouldn't have one or the other */
	if (have_8051 || have_serdes) {
		pr_err(HFI_DRIVER_NAME": fail - firmware half initialized? 8051 %c, SerDes %c\n",
			have_8051 ? 'y' : 'n',
			have_serdes ? 'y' : 'n');
		return -EINVAL;
	}

	ret = get_firmware(&fw_8051, WFR_8051_FW_NAME, &dd->pdev->dev);
	if (ret)
		goto done;
	ret = get_firmware(&fw_serdes, WFR_SERDES_FW_NAME, &dd->pdev->dev);
	if (ret)
		goto done;

	/* TODO: do the actual load */
	pr_info(HFI_DRIVER_NAME": load firmware here...\n");

done:
	if (fw_8051)
		release_firmware(fw_8051);
	if (fw_serdes)
		release_firmware(fw_serdes);
	return ret;
}
#endif

/*
 *  There is nothing to do here.  Once the firmware is loaded, it cannot
 *  be unloaded.
 */
void unload_device_firmware(struct hfi_devdata *dd)
{
}
