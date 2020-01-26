// SPDX-License-Identifier: GPL-2.0
/*
 * PCI DPC Error Disconnect Recover support driver
 * Author: Kuppuswamy Sathyanarayanan <sathyanarayanan.kuppuswamy@linux.intel.com>
 *
 * Copyright (C) 2020 Intel Corp.
 */

#define dev_fmt(fmt) "EDR: " fmt

#include <linux/pci.h>
#include <linux/pci-acpi.h>

#include "portdrv.h"
#include "dpc.h"
#include "../pci.h"

#define EDR_PORT_ENABLE_DSM		0x0C
#define EDR_PORT_LOCATE_DSM		0x0D
#define EDR_OST_SUCCESS			0x80
#define EDR_OST_FAILED			0x81

/*
 * _DSM wrapper function to enable/disable DPC port.
 * @pdev   : PCI device structure.
 * @enable: status of DPC port (0 or 1).
 *
 * returns 0 on success or errno on failure.
 */
static int acpi_enable_dpc_port(struct pci_dev *pdev, bool enable)
{
	struct acpi_device *adev = ACPI_COMPANION(&pdev->dev);
	union acpi_object *obj, argv4, req;
	int status = 0;

	req.type = ACPI_TYPE_INTEGER;
	req.integer.value = enable;

	argv4.type = ACPI_TYPE_PACKAGE;
	argv4.package.count = 1;
	argv4.package.elements = &req;

	/*
	 * Per the Downstream Port Containment Related Enhancements ECN to
	 * the PCI Firmware Specification r3.2, sec 4.6.12,
	 * EDR_PORT_ENABLE_DSM is optional.  Return success if it's not
	 * implemented.
	 */
	obj = acpi_evaluate_dsm(adev->handle, &pci_acpi_dsm_guid, 5,
				EDR_PORT_ENABLE_DSM, &argv4);
	if (!obj)
		return 0;

	if (obj->type != ACPI_TYPE_INTEGER || obj->integer.value != enable)
		status = -EIO;

	ACPI_FREE(obj);

	return status;
}

/*
 * _DSM wrapper function to locate DPC port.
 * @pdev   : Device which received EDR event.
 *
 * returns pci_dev or NULL.
 */
static struct pci_dev *acpi_locate_dpc_port(struct pci_dev *pdev)
{
	struct acpi_device *adev = ACPI_COMPANION(&pdev->dev);
	union acpi_object *obj;
	u16 port;

	obj = acpi_evaluate_dsm(adev->handle, &pci_acpi_dsm_guid, 5,
				EDR_PORT_LOCATE_DSM, NULL);
	if (!obj)
		return pci_dev_get(pdev);

	if (obj->type != ACPI_TYPE_INTEGER) {
		ACPI_FREE(obj);
		return NULL;
	}

	/*
	 * Firmware returns DPC port BDF details in following format:
	 *	15:8 = bus
	 *	 7:3 = device
	 *	 2:0 = function
	 */
	port = obj->integer.value;

	ACPI_FREE(obj);

	return pci_get_domain_bus_and_slot(pci_domain_nr(pdev->bus),
					   PCI_BUS_NUM(port), port & 0xff);
}

/*
 * _OST wrapper function to let firmware know the status of EDR event.
 * @pdev   : Device used to send _OST.
 * @edev   : Device which experienced EDR event.
 * @status: Status of EDR event.
 */
static int acpi_send_edr_status(struct pci_dev *pdev, struct pci_dev *edev,
				u16 status)
{
	struct acpi_device *adev = ACPI_COMPANION(&pdev->dev);
	u32 ost_status;

	pci_dbg(pdev, "Sending EDR status :%#x\n", status);

	ost_status =  PCI_DEVID(edev->bus->number, edev->devfn);
	ost_status = (ost_status << 16) | status;

	status = acpi_evaluate_ost(adev->handle,
				   ACPI_NOTIFY_DISCONNECT_RECOVER,
				   ost_status, NULL);
	if (ACPI_FAILURE(status))
		return -EINVAL;

	return 0;
}

pci_ers_result_t edr_dpc_reset_link(void *cb_data)
{
	return dpc_reset_link_common(cb_data);
}

static void edr_handle_event(acpi_handle handle, u32 event, void *data)
{
	struct dpc_dev *dpc = data, ndpc;
	struct pci_dev *pdev = dpc->pdev;
	pci_ers_result_t estate = PCI_ERS_RESULT_DISCONNECT;
	u16 status;

	pci_info(pdev, "ACPI event %#x received\n", event);

	if (event != ACPI_NOTIFY_DISCONNECT_RECOVER)
		return;

	/*
	 * Check if _DSM(0xD) is available, and if present locate the
	 * port which issued EDR event.
	 */
	pdev = acpi_locate_dpc_port(pdev);
	if (!pdev) {
		pci_err(dpc->pdev, "No valid port found\n");
		return;
	}

	if (pdev != dpc->pdev) {
		pci_warn(pdev, "Initializing dpc again\n");
		dpc_dev_init(pdev, &ndpc);
		dpc= &ndpc;
	}

	/*
	 * If port does not support DPC, just send the OST:
	 */
	if (!dpc->cap_pos)
		goto send_ost;

	/* Check if there is a valid DPC trigger */
	pci_read_config_word(pdev, dpc->cap_pos + PCI_EXP_DPC_STATUS, &status);
	if (!(status & PCI_EXP_DPC_STATUS_TRIGGER)) {
		pci_err(pdev, "Invalid DPC trigger %#010x\n", status);
		goto send_ost;
	}

	dpc_process_error(dpc);

	/* Clear AER registers */
	pci_aer_clear_err_uncor_status(pdev);
	pci_aer_clear_err_fatal_status(pdev);
	pci_aer_clear_err_status_regs(pdev);

	/*
	 * Irrespective of whether the DPC event is triggered by
	 * ERR_FATAL or ERR_NONFATAL, since the link is already down,
	 * use the FATAL error recovery path for both cases.
	 */
	estate = pcie_do_recovery_common(pdev, pci_channel_io_frozen, -1,
					 edr_dpc_reset_link, dpc);
send_ost:

	/* Use ACPI handle of DPC event device for sending EDR status */
	dpc = data;

	/*
	 * If recovery is successful, send _OST(0xF, BDF << 16 | 0x80)
	 * to firmware. If not successful, send _OST(0xF, BDF << 16 | 0x81).
	 */
	if (estate == PCI_ERS_RESULT_RECOVERED)
		acpi_send_edr_status(dpc->pdev, pdev, EDR_OST_SUCCESS);
	else
		acpi_send_edr_status(dpc->pdev, pdev, EDR_OST_FAILED);

	pci_dev_put(pdev);
}

int pci_acpi_add_edr_notifier(struct pci_dev *pdev)
{
	struct acpi_device *adev = ACPI_COMPANION(&pdev->dev);
	struct dpc_dev *dpc;
	acpi_status astatus;
	int status;

	/*
	 * Per the Downstream Port Containment Related Enhancements ECN to
	 * the PCI Firmware Spec, r3.2, sec 4.5.1, table 4-6, EDR support
	 * can only be enabled if DPC is controlled by firmware.
	 *
	 * TODO: Remove dependency on ACPI FIRMWARE_FIRST bit to
	 * determine ownership of DPC between firmware or OS.
	 */
	if (!pcie_aer_get_firmware_first(pdev) || pcie_ports_dpc_native)
		return -ENODEV;

	if (!adev)
		return 0;

	dpc = devm_kzalloc(&pdev->dev, sizeof(*dpc), GFP_KERNEL);
	if (!dpc)
		return -ENOMEM;

	dpc_dev_init(pdev, dpc);

	astatus = acpi_install_notify_handler(adev->handle,
					      ACPI_SYSTEM_NOTIFY,
					      edr_handle_event, dpc);
	if (ACPI_FAILURE(astatus)) {
		pci_err(pdev, "Install ACPI_SYSTEM_NOTIFY handler failed\n");
		return -EBUSY;
	}

	status = acpi_enable_dpc_port(pdev, true);
	if (status) {
		pci_warn(pdev, "Enable DPC port failed\n");
		acpi_remove_notify_handler(adev->handle,
					   ACPI_SYSTEM_NOTIFY,
					   edr_handle_event);
		return status;
	}

	return 0;
}

void pci_acpi_remove_edr_notifier(struct pci_dev *pdev)
{
	struct acpi_device *adev = ACPI_COMPANION(&pdev->dev);

	if (!adev)
		return;

	acpi_remove_notify_handler(adev->handle, ACPI_SYSTEM_NOTIFY,
				   edr_handle_event);
}
