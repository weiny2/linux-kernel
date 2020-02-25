// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for the Intel Integrated Type-C Mux - TCSS
 * (C) Copyright 2018-2019 Intel Corporation
 * This driver is based on ChromeOS Embedded Controller extcon
 * This driver will use Intel PMC IPC to communicate with TCSS
 * extcon -> PMC IPC -> PMC module -> TCSS module
 */

#include <linux/extcon-provider.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/intel_scu_ipc.h>

#define PMC_IPC_USBC_CMD_ID			0xA7

/* Connection request/response */
#define PMC_IPC_TCSS_CONN_REQ_RES               0x0
#define PMC_IPC_CONN_REQ_WRITE_SIZE_BYTE        2

/* Disconnection request/response */
#define PMC_IPC_TCSS_DIS_REQ_RES                0x1
#define PMC_IPC_DIS_REQ_WRITE_SIZE_BYTE         2

/* Safe mode request/response */
#define PMC_IPC_TCSS_SFMODE_REQ_RES             0x2
#define PMC_IPC_SFMODE_REQ_WRITE_SIZE_BYTE      1

/* Alternate mode request/response */
#define PMC_IPC_TCSS_ALTMODE_REQ_RES            0x3
#define PMC_IPC_ALT_REQ_WRITE_SIZE_BYTE         8

/* Hot Plug Detect request/response */
#define PMC_IPC_TCSS_HPD_REQ_RES                0x4
#define PMC_IPC_HPD_REQ_WRITE_SIZE_BYTE         2

/*
 * TCSS request in hierarchy order
 * 1. Connect
 * 2. Disconnect
 * 3. Safe
 * 4. Alternate
 * 5. Display HPD
 */
#define TCSS_REQ_CONNECT	(1 << PMC_IPC_TCSS_CONN_REQ_RES)
#define TCSS_REQ_DISCONNECT	(1 << PMC_IPC_TCSS_DIS_REQ_RES)
#define TCSS_REQ_SAFE_MODE	(1 << PMC_IPC_TCSS_SFMODE_REQ_RES)
#define TCSS_REQ_ALT_MODE	(1 << PMC_IPC_TCSS_ALTMODE_REQ_RES)
#define TCSS_REQ_HPD		(1 << PMC_IPC_TCSS_HPD_REQ_RES)

struct tcss_req {
	uint8_t request;
	uint8_t req_cmd;
};

/*
 * TBT Cable Speed Support
 * 0x03 10Gb/s and 20Gb/s only
 */
#define USBC_CABLE_USB32_GEN2	0x03

/* 4th generation TBT (10.0, 10.3125, 20.0 and 20.625 Gb/s) */
#define TBT_CABLE_GEN4	0x01

enum {
	TCSS_ERROR = -1,
	TCSS_SUCCESS,
};

/* Connection states */
enum pmc_ipc_conn_mode {
	PMC_IPC_TCSS_DISCONNECT_MODE,
	PMC_IPC_TCSS_USB_MODE,
	PMC_IPC_TCSS_ALTERNATE_MODE,
	PMC_IPC_TCSS_SAFE_MODE,
	PMC_IPC_TCSS_TOTAL_MODES,
};

struct tcss_mux {
	bool dp;
	bool usb;
	bool ufp;
	bool polarity;
	bool hpd_lvl;
	bool hpd_irq;
	bool dbg_acc;
	bool cable;
	bool tbt;
	bool tbt_type;
	bool tbt_cable_type;
	bool tbt_link;

	uint8_t dp_mode;
	uint8_t tbt_cable_speed;
	uint8_t tbt_cable_gen;
};

struct cros_ec_tcss_data {
	enum pmc_ipc_conn_mode conn_mode;
	uint8_t usb3_port;
	uint8_t usb2_port;
	struct tcss_mux mux_info;
};

struct cros_ec_tcss_info {
	uint8_t num_ports;
	struct work_struct tcss_work;
	struct work_struct bh_work;
	struct notifier_block notifier;
	struct device *dev;
	struct cros_ec_device *ec;
	struct cros_ec_tcss_data *tcss;
	struct intel_scu_ipc_dev *scu;
};

static const struct tcss_req tcss_requests[] = {
	{TCSS_REQ_DISCONNECT, PMC_IPC_TCSS_DIS_REQ_RES},
	{TCSS_REQ_CONNECT, PMC_IPC_TCSS_CONN_REQ_RES},
	{TCSS_REQ_SAFE_MODE, PMC_IPC_TCSS_SFMODE_REQ_RES},
	{TCSS_REQ_ALT_MODE, PMC_IPC_TCSS_ALTMODE_REQ_RES},
	{TCSS_REQ_HPD, PMC_IPC_TCSS_HPD_REQ_RES},
};

/* [Previous mode] [next mode] */
static const uint8_t tcss_mode_states[][PMC_IPC_TCSS_TOTAL_MODES] = {
	/* From Disconnect mode -> USB mode */
	[PMC_IPC_TCSS_DISCONNECT_MODE] = {
					  [PMC_IPC_TCSS_DISCONNECT_MODE] = 0,
					  [PMC_IPC_TCSS_USB_MODE] =
					  TCSS_REQ_CONNECT,
					  [PMC_IPC_TCSS_ALTERNATE_MODE] = 0,
					  [PMC_IPC_TCSS_SAFE_MODE] = 0,
					  },

	/* From USB mode -> Disconnect mode -> Safe mode */
	[PMC_IPC_TCSS_USB_MODE] = {
				   [PMC_IPC_TCSS_DISCONNECT_MODE] =
				   TCSS_REQ_DISCONNECT,
				   [PMC_IPC_TCSS_USB_MODE] = 0,
				   [PMC_IPC_TCSS_ALTERNATE_MODE] =
				   TCSS_REQ_SAFE_MODE,
				   [PMC_IPC_TCSS_SAFE_MODE] =
				   TCSS_REQ_SAFE_MODE,
				   },

	/* From Safe mode -> Disconnect mode -> Alternate mode */
	[PMC_IPC_TCSS_SAFE_MODE] = {
				    [PMC_IPC_TCSS_DISCONNECT_MODE] =
				    TCSS_REQ_DISCONNECT,
				    [PMC_IPC_TCSS_USB_MODE] = 0,
				    [PMC_IPC_TCSS_ALTERNATE_MODE] =
				    TCSS_REQ_ALT_MODE,
				    [PMC_IPC_TCSS_SAFE_MODE] = 0,
				    },

	/*
	 * From Alternate mode -> Disconnect mode
	 *                     -> Alternate mode with HPD
	 *                     -> Alternate mode without HPD
	 */
	[PMC_IPC_TCSS_ALTERNATE_MODE] = {
					 [PMC_IPC_TCSS_DISCONNECT_MODE] =
					 TCSS_REQ_DISCONNECT,
					 [PMC_IPC_TCSS_USB_MODE] = 0,
					 [PMC_IPC_TCSS_ALTERNATE_MODE] =
					 TCSS_REQ_ALT_MODE,
					 [PMC_IPC_TCSS_SAFE_MODE] = 0,
					 },
};

BUILD_ASSERT(ARRAY_SIZE(mode_array) == PMC_IPC_TCSS_TOTAL_MODES);

/**
 * cros_ec_pd_command() - Send a command to the EC.
 * @info: pointer to struct cros_ec_tcss_info
 * @command: EC command
 * @version: EC command version
 * @outdata: EC command output data
 * @outsize: Size of outdata
 * @indata: EC command input data
 * @insize: Size of indata
 *
 * Return: 0 on success, < 0 on failure.
 */
static int cros_ec_pd_command(struct cros_ec_tcss_info *info,
			      unsigned int command, unsigned int version,
			      void *outdata, unsigned int outsize,
			      void *indata, unsigned int insize)
{
	struct cros_ec_command *msg;
	int ret;

	msg = kzalloc(sizeof(*msg) + max(outsize, insize), GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->version = version;
	msg->command = command;
	msg->outsize = outsize;
	msg->insize = insize;

	memcpy(msg->data, outdata, outsize);

	ret = cros_ec_cmd_xfer_status(info->ec, msg);
	if (ret > TCSS_ERROR)
		memcpy(indata, msg->data, insize);

	kfree(msg);
	return ret;
}

/**
 * cros_ec_usb_get_pd_mux_state() - Get PD mux state for given port.
 * @info: pointer to struct cros_ec_tcss_info
 * @resp: pointer to struct ec_response_usb_pd_mux_info
 * @port: USB-C port number
 *
 * Return: status of cros_ec_pd_command()
 */
static int cros_ec_usb_get_pd_mux_state(struct cros_ec_tcss_info *info,
					struct ec_response_usb_pd_mux_info
					*resp, uint8_t port)
{
	struct ec_params_usb_pd_mux_info req;

	req.port = port;

	return cros_ec_pd_command(info, EC_CMD_USB_PD_MUX_INFO, 0,
				  &req, sizeof(req), resp, sizeof(*resp));
}

/**
 * cros_ec_usb_pd_control() - Get role info about possible PD device attached
 * to a given port.
 * @info: pointer to struct cros_ec_tcss_info
 * @resp: pointer to struct ec_response_usb_pd_control_v3
 * @port: USB-C port number
 *
 * Return: status of cros_ec_pd_command()
 */
static int cros_ec_usb_pd_control(struct cros_ec_tcss_info *info,
				  struct ec_response_usb_pd_control_v3
				  *resp, uint8_t port)
{
	struct ec_params_usb_pd_control pd_control;

	pd_control.port = port;
	pd_control.role = USB_PD_CTRL_ROLE_NO_CHANGE;
	pd_control.mux = USB_PD_CTRL_MUX_NO_CHANGE;
	pd_control.swap = USB_PD_CTRL_SWAP_NONE;

	return cros_ec_pd_command(info, EC_CMD_USB_PD_CONTROL, 3,
				  &pd_control, sizeof(pd_control), resp,
				  sizeof(*resp));
}

/**
 * cros_ec_pd_get_num_ports() - Get number of EC charge ports.
 * @info: pointer to struct cros_ec_tcss_info
 *
 * Return: status of cros_ec_pd_command()
 */
static int cros_ec_pd_get_num_ports(struct cros_ec_tcss_info *info)
{
	struct ec_response_usb_pd_ports resp;
	int ret;

	ret = cros_ec_pd_command(info, EC_CMD_USB_PD_PORTS, 0,
				 NULL, 0, &resp, sizeof(resp));
	if (ret > TCSS_ERROR)
		info->num_ports = resp.num_ports;

	return ret;
}

/**
 * cros_ec_pd_get_port_info() - Get USB-C port's USB2 & USB3 num info.
 * @info: pointer to struct cros_ec_tcss_info
 * @port: USB-C port number
 *
 * Return: status of cros_ec_pd_command()
 */
static int cros_ec_pd_get_port_info(struct cros_ec_tcss_info *info,
				    uint8_t port)
{
	struct cros_ec_tcss_data *tcss_data = &info->tcss[port];
	struct ec_response_locate_chip resp;
	struct ec_params_locate_chip req;
	int ret;

	req.type = EC_CHIP_TYPE_TCPC;
	req.index = port;

	ret = cros_ec_pd_command(info, EC_CMD_LOCATE_CHIP, 0,
				 &req, sizeof(req), &resp, sizeof(resp));
	if (ret > TCSS_ERROR) {
		/* Assign USB2 & USB3 numbers */
		tcss_data->usb2_port = resp.reserved & 0x0F;
		tcss_data->usb3_port = (resp.reserved & 0xF0) >> 4;
	}

	return ret;
}

static int tcss_cros_ec_req(struct cros_ec_tcss_info *info, int req_type,
			    uint8_t port, struct tcss_mux *mux_data)
{
	struct cros_ec_tcss_data *tcss_info = &info->tcss[port];
	struct device *dev = info->dev;
	u32 write_size, tcss_res, i;
	u8 tcss_req[8] = { 0 };
	int ret;

	tcss_req[0] = req_type | tcss_info->usb3_port << 4;

	switch (req_type) {
	case PMC_IPC_TCSS_CONN_REQ_RES:
		tcss_req[1] = tcss_info->usb2_port |
		    (mux_data->ufp ? (1 << 4) : 0) |
		    (mux_data->polarity ? (1 << 5) : 0) |
		    (mux_data->polarity ? (1 << 6) : 0) |
		    (mux_data->dbg_acc ? (1 << 7) : 0);
		write_size = PMC_IPC_CONN_REQ_WRITE_SIZE_BYTE;
		break;

	case PMC_IPC_TCSS_DIS_REQ_RES:
		tcss_req[1] = tcss_info->usb2_port;
		write_size = PMC_IPC_DIS_REQ_WRITE_SIZE_BYTE;
		break;

	case PMC_IPC_TCSS_SFMODE_REQ_RES:
		write_size = PMC_IPC_SFMODE_REQ_WRITE_SIZE_BYTE;
		break;

	case PMC_IPC_TCSS_ALTMODE_REQ_RES:
		if (mux_data->dp)
			tcss_req[1] |= (1 << 4);
		else if (mux_data->tbt)
			tcss_req[1] |= (1 << 5);

		tcss_req[4] =
		    (mux_data->polarity ? (1 << 1) : 0) |
		    (mux_data->cable ? (1 << 2) : 0) |
		    (mux_data->ufp ? (1 << 3) : 0) |
		    (mux_data->polarity ? (1 << 4) : 0) |
		    (mux_data->polarity ? (1 << 5) : 0);

		if (mux_data->dp_mode <= MODE_DP_PIN_F)
			tcss_req[5] = ffs(mux_data->dp_mode);

		tcss_req[5] |= mux_data->hpd_lvl ? (1 << 6) : 0;

		if (mux_data->tbt) {
			tcss_req[6] =
			    (mux_data->tbt_type ? (1 << 1) : 0) |
			    (mux_data->tbt_cable_type ? (1 << 2) : 0) |
			    (mux_data->tbt_link ? (1 << 5) : 0);

			if (mux_data->tbt_cable_speed <= USBC_CABLE_USB32_GEN2)
				tcss_req[7] |= (mux_data->tbt_cable_speed << 1);

			tcss_req[7] |=
			    ((mux_data->tbt_cable_gen == TBT_CABLE_GEN4) ?
			      (1 << 4) : 0);
		}
		write_size = PMC_IPC_ALT_REQ_WRITE_SIZE_BYTE;
		break;

	case PMC_IPC_TCSS_HPD_REQ_RES:
		tcss_req[1] = (mux_data->hpd_lvl ? (1 << 4) : 0) |
		    (mux_data->hpd_irq ? (1 << 5) : 0);
		write_size = PMC_IPC_HPD_REQ_WRITE_SIZE_BYTE;
		break;

	default:
		dev_err(dev, "Invalid req type to PMC = %d", req_type);
		return TCSS_ERROR;
	}

	for (i = 0; i < write_size; i++)
		dev_dbg(dev, "[%d] 0x%x\n", i, tcss_req[i]);

	ret = intel_scu_ipc_dev_command(info->scu, PMC_IPC_USBC_CMD_ID, 0,
				    tcss_req, write_size, &tcss_res,
				    sizeof(tcss_res));
	if (ret) {
		dev_err(dev, "IPC to PMC failed with error %d\n", ret);
		return TCSS_ERROR;
	}
	dev_dbg(dev, "tcss_res=0x%x", tcss_res);

	switch (req_type) {
	case PMC_IPC_TCSS_CONN_REQ_RES:
	case PMC_IPC_TCSS_DIS_REQ_RES:
		if (((tcss_res & 0xf) != req_type) ||
		    ((tcss_res & 0xf0) != (tcss_info->usb3_port << 4)) ||
		    ((tcss_res & 0xf00) != (tcss_info->usb2_port << 8)) ||
		    (tcss_res & (0x3 << 16))) {
			dev_err(dev, "PMC TCSS req %d failed\n", req_type);
			return TCSS_ERROR;
		}
		break;
	case PMC_IPC_TCSS_SFMODE_REQ_RES:
	case PMC_IPC_TCSS_ALTMODE_REQ_RES:
	case PMC_IPC_TCSS_HPD_REQ_RES:
		if (((tcss_res & 0xf) != req_type) ||
		    ((tcss_res & 0xf0) != (tcss_info->usb3_port << 4)) ||
		    (tcss_res & (0x3 << 8))) {
			dev_err(dev, "PMC TCSS req %d failed\n", req_type);
			return TCSS_ERROR;
		}
		break;
	}

	return TCSS_SUCCESS;
}

static const char *tcss_cros_ec_conn_mode(const struct cros_ec_tcss_data
					  *mux_data)
{
	switch (mux_data->conn_mode) {
	case PMC_IPC_TCSS_SAFE_MODE:
		return "Safe Mode";

	case PMC_IPC_TCSS_ALTERNATE_MODE:
		return "Alternate mode";

	case PMC_IPC_TCSS_USB_MODE:
		return "USB mode";

	case PMC_IPC_TCSS_DISCONNECT_MODE:
		return "Disconnect mode";

	default:
		return "Unknown mode";
	}
}

static int tcss_cros_ec_get_current_state(struct cros_ec_tcss_info *info,
					  struct cros_ec_tcss_data *mux_data,
					  uint8_t port)
{
	struct ec_response_usb_pd_control_v3 usb_pd_resp;
	struct ec_response_usb_pd_mux_info usb_mux_resp;
	int ret;

	ret = cros_ec_usb_get_pd_mux_state(info, &usb_mux_resp, port);
	if (ret <= TCSS_ERROR) {
		dev_err(info->dev, "failed getting mux state = %d\n", ret);
		return ret;
	}

	ret = cros_ec_usb_pd_control(info, &usb_pd_resp, port);
	if (ret <= TCSS_ERROR) {
		dev_err(info->dev, "failed getting role err = %d\n", ret);
		return ret;
	}

	mux_data->mux_info.ufp = (usb_pd_resp.cc_state == PD_CC_DFP_ATTACHED);
	mux_data->mux_info.dbg_acc = (usb_pd_resp.cc_state == PD_CC_DEBUG_ACC);
	mux_data->mux_info.dp_mode = usb_pd_resp.dp_mode;

	mux_data->mux_info.cable =
	    usb_pd_resp.tbt_flags & USB_PD_MUX_TBT_ACTIVE_CABLE;

	mux_data->mux_info.dp = usb_mux_resp.flags & USB_PD_MUX_DP_ENABLED;

	mux_data->mux_info.usb = usb_mux_resp.flags & USB_PD_MUX_USB_ENABLED;

	mux_data->mux_info.polarity =
	    usb_mux_resp.flags & USB_PD_MUX_POLARITY_INVERTED;

	mux_data->mux_info.hpd_lvl = usb_mux_resp.flags & USB_PD_MUX_HPD_LVL;

	mux_data->mux_info.hpd_irq = usb_mux_resp.flags & USB_PD_MUX_HPD_IRQ;

	mux_data->mux_info.tbt =
	    usb_mux_resp.flags & USB_PD_MUX_TBT_COMPAT_ENABLED;

	mux_data->mux_info.tbt_type =
	    usb_pd_resp.tbt_flags & USB_PD_MUX_TBT_ADAPTER;

	mux_data->mux_info.tbt_cable_type =
	    usb_pd_resp.tbt_flags & USB_PD_MUX_TBT_CABLE_TYPE;

	mux_data->mux_info.tbt_link =
	    usb_pd_resp.tbt_flags & USB_PD_MUX_TBT_LINK;

	mux_data->mux_info.tbt_cable_speed = usb_pd_resp.tbt_cable_speed;

	mux_data->mux_info.tbt_cable_gen = usb_pd_resp.tbt_cable_gen;

	if (usb_mux_resp.flags & USB_PD_MUX_SAFE_MODE)
		mux_data->conn_mode = PMC_IPC_TCSS_SAFE_MODE;
	else if (mux_data->mux_info.dp || mux_data->mux_info.tbt)
		mux_data->conn_mode = PMC_IPC_TCSS_ALTERNATE_MODE;
	else if (mux_data->mux_info.usb)
		mux_data->conn_mode = PMC_IPC_TCSS_USB_MODE;
	else
		mux_data->conn_mode = PMC_IPC_TCSS_DISCONNECT_MODE;

	dev_dbg(info->dev, " Port=%d, Connection mode = %s\n", port,
				tcss_cros_ec_conn_mode(mux_data));

	return 0;
}

uint8_t tcss_cros_ec_get_next_state(struct cros_ec_tcss_data *mux_data,
				    struct cros_ec_tcss_data *prev_mux_data)
{
	uint8_t ret =
	    tcss_mode_states[prev_mux_data->conn_mode][mux_data->conn_mode];

	/* before entering alternate mode enter safe mode */
	if (ret == TCSS_REQ_SAFE_MODE)
		mux_data->conn_mode = PMC_IPC_TCSS_SAFE_MODE;
	/* check for HPD */
	else if (ret == TCSS_REQ_ALT_MODE) {
		if (mux_data->mux_info.dp && (mux_data->mux_info.hpd_irq ||
		(mux_data->mux_info.hpd_lvl !=
		prev_mux_data->mux_info.hpd_lvl))) {
			if (prev_mux_data->conn_mode ==
			    PMC_IPC_TCSS_ALTERNATE_MODE)
				ret = TCSS_REQ_HPD;
			else
				ret |= TCSS_REQ_HPD;
		}
	}
	return ret;
}

int tcss_cros_ec_detect_cable(struct cros_ec_tcss_info *info,
			      bool force, uint8_t port)
{
	struct cros_ec_tcss_data *tcss_info = &info->tcss[port];
	struct cros_ec_tcss_data mux_data;
	uint8_t next_seq;
	int ret, i;

	ret = tcss_cros_ec_get_current_state(info, &mux_data, port);
	if (ret <= TCSS_ERROR)
		return ret;

	next_seq = tcss_cros_ec_get_next_state(&mux_data, tcss_info);
	if (!next_seq)
		return TCSS_SUCCESS;

	dev_dbg(info->dev, "p=%d,ufp=%d,dp=%d,usb=%d,pol=%d,hpd=%d,irq=%d,mode=%d,con=%d\n",
		port, mux_data.mux_info.ufp, mux_data.mux_info.dp,
		mux_data.mux_info.usb, mux_data.mux_info.polarity,
		mux_data.mux_info.hpd_lvl, mux_data.mux_info.hpd_irq,
		mux_data.mux_info.dp_mode, mux_data.conn_mode);

	dev_dbg(info->dev,
		"tbt=%d, tbt_type=%d, tbt_cable_type=%d, tbt_link=%d, tbt_cable_speed=%d, tbt_cable_gen=%d\n",
		mux_data.mux_info.tbt, mux_data.mux_info.tbt_type,
		mux_data.mux_info.tbt_cable_type, mux_data.mux_info.tbt_link,
		mux_data.mux_info.tbt_cable_speed,
		mux_data.mux_info.tbt_cable_gen);

	for (i = 0; i < ARRAY_SIZE(tcss_requests); i++) {
		if (next_seq & tcss_requests[i].request) {
			ret = tcss_cros_ec_req(info, tcss_requests[i].req_cmd,
					       port, &mux_data.mux_info);
			if (ret)
				break;
		}
	}

	if (ret) {
		dev_err(info->dev, "Error port=%d, error=%d\n", port, ret);
	} else {
		mux_data.mux_info.hpd_irq = 0;
		tcss_info->mux_info = mux_data.mux_info;
		tcss_info->conn_mode = mux_data.conn_mode;
	}

	return ret;
}

static void cros_tcss_bh_work(struct work_struct *work)
{
	struct cros_ec_tcss_info *info =
	    container_of(work, struct cros_ec_tcss_info, bh_work);
	uint8_t port;

	for (port = 0; port < info->num_ports; port++) {
		if (tcss_cros_ec_detect_cable(info, false, port) <=
		     TCSS_ERROR) {
			dev_err(info->dev, "Port %d, Error detecting cable\n",
				port);
		}
	}
}

static int tcss_cros_ec_event(struct notifier_block *nb,
			      unsigned long queued_during_suspend,
			      void *_notify)
{
	struct cros_ec_tcss_info *info =
	    container_of(nb, struct cros_ec_tcss_info, notifier);

	schedule_work(&info->bh_work);

	return NOTIFY_DONE;
}

static int tcss_cros_ec_remove(struct platform_device *pdev)
{
	struct cros_ec_tcss_info *info = platform_get_drvdata(pdev);

	blocking_notifier_chain_unregister(&info->ec->event_notifier,
					   &info->notifier);
	cancel_work_sync(&info->bh_work);

	return 0;
}

static int tcss_cros_ec_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec = dev_get_drvdata(pdev->dev.parent);
	struct device *dev = &pdev->dev;
	struct cros_ec_tcss_info *info;
	uint8_t port;
	int ret;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->ec = ec;
	info->dev = dev;

	info->scu = devm_intel_scu_ipc_dev_get(dev);
	if (!info->scu)
		return -EPROBE_DEFER;

	ret = cros_ec_pd_get_num_ports(info);
	if (ret <= TCSS_ERROR) {
		dev_err(dev, "failed getting number of ports! ret = %d\n", ret);
		return -ENODEV;
	}

	info->tcss = devm_kzalloc(dev, sizeof(*info->tcss) * info->num_ports,
					GFP_KERNEL);
	if (!info->tcss)
		ret = -ENOMEM;

	memset(info->tcss, 0, sizeof(*info->tcss) * info->num_ports);
	platform_set_drvdata(pdev, info);

	/* Get Port detection events from the EC */
	info->notifier.notifier_call = tcss_cros_ec_event;
	ret = blocking_notifier_chain_register(&info->ec->event_notifier,
					       &info->notifier);
	if (ret <= TCSS_ERROR) {
		dev_err(dev, "failed to register notifier\n");
		return ret;
	}

	INIT_WORK(&info->bh_work, cros_tcss_bh_work);

	/* Perform initial detection */
	for (port = 0; port < info->num_ports; port++) {
		ret = cros_ec_pd_get_port_info(info, port);
		if (ret <= TCSS_ERROR) {
			dev_err(dev, "failed getting USB port info! ret = %d\n",
				ret);
			goto remove_tcss;
		}

		ret = tcss_cros_ec_detect_cable(info, true, port);
		if (ret <= TCSS_ERROR) {
			dev_err(dev, "failed to detect initial cable state\n");
			goto remove_tcss;
		}
	}

	return 0;

remove_tcss:
	tcss_cros_ec_remove(pdev);

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int tcss_cros_ec_suspend(struct device *dev)
{
	return 0;
}

static int tcss_cros_ec_resume(struct device *dev)
{
	struct cros_ec_tcss_info *info = dev_get_drvdata(dev);
	uint8_t port;
	int ret;

	for (port = 0; port < info->num_ports; port++) {
		ret = tcss_cros_ec_detect_cable(info, true, port);
		if (ret <= TCSS_ERROR) {
			dev_err(dev, "cable detection failed on resume\n");
			return ret;
		}
	}

	return 0;
}

static const struct dev_pm_ops tcss_cros_ec_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(tcss_cros_ec_suspend, tcss_cros_ec_resume)
};

#define DEV_PM_OPS	(&tcss_cros_ec_dev_pm_ops)
#else
#define DEV_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id tcss_cros_ec_of_match[] = {
	{.compatible = "google,extcon-tcss-cros-ec"},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, tcss_cros_ec_of_match);

static struct platform_driver tcss_cros_ec_driver = {
	.driver = {
		   .name = "extcon-tcss-cros-ec",
		   .of_match_table = tcss_cros_ec_of_match,
		   .pm = DEV_PM_OPS,
	},
	.remove = tcss_cros_ec_remove,
	.probe = tcss_cros_ec_probe,
};

module_platform_driver(tcss_cros_ec_driver);

MODULE_DESCRIPTION("ChromeOS Embedded Controller Intel TCSS driver");
MODULE_AUTHOR("Vijay P Hiremath <vijay.p.hiremath@intel.com>");
MODULE_AUTHOR("Divya Sasidharan <divya.s.sasidharan@intel.com>");
MODULE_AUTHOR("Utkarsh Patel <utkarsh.h.patel@intel.com>");
MODULE_LICENSE("GPL v2");
