/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _INTEL_CTA_TELEM_H
#define _INTEL_CTA_TELEM_H

#include <linux/intel-dvsec.h>

/* Telemetry types */
#define CTA_TELEM_TELEMETRY	0
#define CTA_TELEM_CRASHLOG	1

struct telem_endpoint;

struct telem_header {
	u8	access_type;
	u8	telem_type;
	u16	size;
	u32	guid;
	u32	base_offset;
	u8	tbir;
};

struct telem_endpoint_info {
	struct pci_dev		*pdev;
	struct telem_header	header;
};

/* TELEMETRY API */

/**
 * cta_telem_get_next_endpoint() - Get next device id for a telemetry endpoint
 * @start:  starting devid to look from
 *
 * This functions can be used in a while loop predicate to retrieve the devid
 * of all available telemetry endpoints. Functions cta_telem_get_next_endpoint()
 * and cta_telem_register_endpoint() can be used inside of the loop to examine
 * endpoint info and register to receive a pointer to the endpoint. The pointer
 * is then usable in the telemetry read calls to access the telemetry data.
 *
 * Return:
 * * devid       - devid of the next present endpoint from start
 * * 0           - when no more endpoints are present after start
 */
int cta_telem_get_next_endpoint(int start);

/**
 * cta_telem_register_endpoint() - Register a telemetry endpoint
 * @devid: device id/handle of the telemetry endpoint
 *
 * Increments the kref usage counter for the endpoint.
 *
 * Return:
 * * endpoint    - On success returns pointer to the telemetry endpoint
 * * -ENXIO      - telemetry endpoint not found
 */
struct telem_endpoint *cta_telem_register_endpoint(int devid);

/**
 * cta_telem_unregister_endpoint() - Unregister a telemetry endpoint
 * @ep:   ep structure to populate.
 *
 * Decrements the kref usage counter for the endpoint.
 */
void cta_telem_unregister_endpoint(struct telem_endpoint *ep);

/**
 * cta_telem_get_endpoint_info() - Get info for an endpoint from its devid
 * @devid:  device id/handle of the telemetry endpoint
 * @info:   Endpoint info structure to be populated
 *
 * Return:
 * * 0           - Success
 * * -ENXIO      - telemetry endpoint not found for the devid
 * * -EINVAL     - @info is NULL
 */
int cta_telem_get_endpoint_info(int devid,
				struct telem_endpoint_info *info);

/**
 * cta_telem_read32() - Read dwords from telemetry sram
 * @ep:     Telemetry endpoint to be read
 * @offset: Register offset in bytes
 * @data:   Allocated dword buffer
 * @count:  Number of dwords requested
 *
 * Callers must ensure reads are aligned. When the call returns -ENODEV,
 * the device has been removed and callers should unregister the telemetry
 * endpoint.
 *
 * Return:
 * * 0           - Success
 * * -ENODEV	 - The device is not present.
 * * -EINVAL	 - The offset is out out bounds
 * * -EPIPE	 - The device was removed during the read. Data written
 *		   but should be considered invalid.
 */
int cta_telem_read32(struct telem_endpoint *ep, u32 offset, u32 *data,
		     u32 count);

/**
 * cta_telem_read64() - Read qwords from counter sram
 * @ep:     Telemetry endpoint to be read
 * @offset: Register offset in bytes
 * @data:   Allocated qword buffer
 * @count:  Number of qwords requested
 *
 * Callers must ensure reads are aligned. When the call returns -ENODEV,
 * the device has been removed and callers should unregister the telemetry
 * endpoint.
 *
 * Return:
 * * 0           - Success
 * * -ENODEV	 - The device is not present.
 * * -EINVAL	 - The offset is out out bounds
 * * -EPIPE	 - The device was removed during the read. Data written
 *		   but should be considered not valid.
 */
int cta_telem_read64(struct telem_endpoint *ep, u32 offset, u64 *data,
		     u32 count);

/* Notifiers */

#define CTA_TELEM_NOTIFY_ADD	0
#define CTA_TELEM_NOTIFY_REMOVE	1

/**
 * cta_telem_register_notifier() - Receive notification endpoint events
 * @nb:   Notifier block
 *
 * Events:
 *   CTA_TELEM_NOTIFY_ADD   - An endpoint has been added. Notifier data
 *                            is the devid
 *   CTA_TELEM_NOTIF_REMOVE - An endpoint has been removed. Notifier data
 *                            is the devid
 */
int cta_telem_register_notifier(struct notifier_block *nb);

/**
 * cta_telem_unregister_notifier() - Unregister notification of endpoint events
 * @nb:   Notifier block
 *
 */
int cta_telem_unregister_notifier(struct notifier_block *nb);

#endif
