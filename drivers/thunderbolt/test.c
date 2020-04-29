// SPDX-License-Identifier: GPL-2.0
/*
 * KUnit tests
 *
 * Copyright (C) 2020, Intel Corporation
 * Author: Mika Westerberg <mika.westerberg@linux.intel.com>
 */

#include <kunit/test.h>
#include <linux/idr.h>
#include <linux/uuid.h>

#include "tb.h"
#include "tunnel.h"

struct port_expectation {
	u64 route;
	u8 port;
	enum tb_port_type type;
};

static const u32 root_directory[] = {
	0x55584401,	/* "UXD" v1 */
	0x00000018,	/* Root directory length */
	0x76656e64,	/* "vend" */
	0x6f726964,	/* "orid" */
	0x76000001,	/* "v" R 1 */
	0x00000a27,	/* Immediate value, ! Vendor ID */
	0x76656e64,	/* "vend" */
	0x6f726964,	/* "orid" */
	0x74000003,	/* "t" R 3 */
	0x0000001a,	/* Text leaf offset, (“Apple Inc.”) */
	0x64657669,	/* "devi" */
	0x63656964,	/* "ceid" */
	0x76000001,	/* "v" R 1 */
	0x0000000a,	/* Immediate value, ! Device ID */
	0x64657669,	/* "devi" */
	0x63656964,	/* "ceid" */
	0x74000003,	/* "t" R 3 */
	0x0000001d,	/* Text leaf offset, (“Macintosh”) */
	0x64657669,	/* "devi" */
	0x63657276,	/* "cerv" */
	0x76000001,	/* "v" R 1 */
	0x80000100,	/* Immediate value, Device Revision */
	0x6e657477,	/* "netw" */
	0x6f726b00,	/* "ork" */
	0x44000014,	/* "D" R 20 */
	0x00000021,	/* Directory data offset, (Network Directory) */
	0x4170706c,	/* "Appl" */
	0x6520496e,	/* "e In" */
	0x632e0000,	/* "c." ! */
	0x4d616369,	/* "Maci" */
	0x6e746f73,	/* "ntos" */
	0x68000000,	/* "h" */
	0x00000000,	/* padding */
	0xca8961c6,	/* Directory UUID, Network Directory */
	0x9541ce1c,	/* Directory UUID, Network Directory */
	0x5949b8bd,	/* Directory UUID, Network Directory */
	0x4f5a5f2e,	/* Directory UUID, Network Directory */
	0x70727463,	/* "prtc" */
	0x69640000,	/* "id" */
	0x76000001,	/* "v" R 1 */
	0x00000001,	/* Immediate value, Network Protocol ID */
	0x70727463,	/* "prtc" */
	0x76657273,	/* "vers" */
	0x76000001,	/* "v" R 1 */
	0x00000001,	/* Immediate value, Network Protocol Version */
	0x70727463,	/* "prtc" */
	0x72657673,	/* "revs" */
	0x76000001,	/* "v" R 1 */
	0x00000001,	/* Immediate value, Network Protocol Revision */
	0x70727463,	/* "prtc" */
	0x73746e73,	/* "stns" */
	0x76000001,	/* "v" R 1 */
	0x00000000,	/* Immediate value, Network Protocol Settings */
};

static const uuid_t network_dir_uuid =
	UUID_INIT(0xc66189ca, 0x1cce, 0x4195,
		  0xbd, 0xb8, 0x49, 0x59, 0x2e, 0x5f, 0x5a, 0x4f);

static int __ida_init(struct kunit_resource *res, void *context)
{
	struct ida *ida = context;

	ida_init(ida);
	res->allocation = ida;
	return 0;
}

static void __ida_destroy(struct kunit_resource *res)
{
	struct ida *ida = res->allocation;

	ida_destroy(ida);
}

static void kunit_ida_init(struct kunit *test, struct ida *ida)
{
	kunit_alloc_resource(test, __ida_init, __ida_destroy, GFP_KERNEL, ida);
}

static struct tb_switch *alloc_switch(struct kunit *test, u64 route,
				      u8 upstream_port, u8 max_port_number)
{
	struct tb_switch *sw;
	size_t size;
	int i;

	sw = kunit_kzalloc(test, sizeof(*sw), GFP_KERNEL);
	if (!sw)
		return NULL;

	sw->config.upstream_port_number = upstream_port;
	sw->config.depth = tb_route_length(route);
	sw->config.route_hi = upper_32_bits(route);
	sw->config.route_lo = lower_32_bits(route);
	sw->config.enabled = 0;
	sw->config.max_port_number = max_port_number;

	size = (sw->config.max_port_number + 1) * sizeof(*sw->ports);
	sw->ports = kunit_kzalloc(test, size, GFP_KERNEL);
	if (!sw->ports)
		return NULL;

	for (i = 0; i <= sw->config.max_port_number; i++) {
		sw->ports[i].sw = sw;
		sw->ports[i].port = i;
		sw->ports[i].config.port_number = i;
		if (i) {
			kunit_ida_init(test, &sw->ports[i].in_hopids);
			kunit_ida_init(test, &sw->ports[i].out_hopids);
		}
	}

	return sw;
}

static struct tb_switch *alloc_host(struct kunit *test)
{
	struct tb_switch *sw;

	sw = alloc_switch(test, 0, 7, 13);
	if (!sw)
		return NULL;

	sw->config.vendor_id = 0x8086;
	sw->config.device_id = 0x9a1b;

	sw->ports[0].config.type = TB_TYPE_PORT;
	sw->ports[0].config.max_in_hop_id = 7;
	sw->ports[0].config.max_out_hop_id = 7;

	sw->ports[1].config.type = TB_TYPE_PORT;
	sw->ports[1].config.max_in_hop_id = 19;
	sw->ports[1].config.max_out_hop_id = 19;
	sw->ports[1].dual_link_port = &sw->ports[2];

	sw->ports[2].config.type = TB_TYPE_PORT;
	sw->ports[2].config.max_in_hop_id = 19;
	sw->ports[2].config.max_out_hop_id = 19;
	sw->ports[2].dual_link_port = &sw->ports[1];
	sw->ports[2].link_nr = 1;

	sw->ports[3].config.type = TB_TYPE_PORT;
	sw->ports[3].config.max_in_hop_id = 19;
	sw->ports[3].config.max_out_hop_id = 19;
	sw->ports[3].dual_link_port = &sw->ports[4];

	sw->ports[4].config.type = TB_TYPE_PORT;
	sw->ports[4].config.max_in_hop_id = 19;
	sw->ports[4].config.max_out_hop_id = 19;
	sw->ports[4].dual_link_port = &sw->ports[3];
	sw->ports[4].link_nr = 1;

	sw->ports[5].config.type = TB_TYPE_DP_HDMI_IN;
	sw->ports[5].config.max_in_hop_id = 9;
	sw->ports[5].config.max_out_hop_id = 9;
	sw->ports[5].cap_adap = -1;

	sw->ports[6].config.type = TB_TYPE_DP_HDMI_IN;
	sw->ports[6].config.max_in_hop_id = 9;
	sw->ports[6].config.max_out_hop_id = 9;
	sw->ports[6].cap_adap = -1;

	sw->ports[7].config.type = TB_TYPE_NHI;
	sw->ports[7].config.max_in_hop_id = 11;
	sw->ports[7].config.max_out_hop_id = 11;

	sw->ports[8].config.type = TB_TYPE_PCIE_DOWN;
	sw->ports[8].config.max_in_hop_id = 8;
	sw->ports[8].config.max_out_hop_id = 8;

	sw->ports[9].config.type = TB_TYPE_PCIE_DOWN;
	sw->ports[9].config.max_in_hop_id = 8;
	sw->ports[9].config.max_out_hop_id = 8;

	sw->ports[10].disabled = true;
	sw->ports[11].disabled = true;

	sw->ports[12].config.type = TB_TYPE_USB3_DOWN;
	sw->ports[12].config.max_in_hop_id = 8;
	sw->ports[12].config.max_out_hop_id = 8;

	sw->ports[13].config.type = TB_TYPE_USB3_DOWN;
	sw->ports[13].config.max_in_hop_id = 8;
	sw->ports[13].config.max_out_hop_id = 8;

	return sw;
}

static struct tb_switch *alloc_dev_default(struct kunit *test,
					   struct tb_switch *parent,
					   u64 route)
{
	struct tb_port *port, *upstream_port;
	struct tb_switch *sw;

	sw = alloc_switch(test, route, 1, 19);
	if (!sw)
		return NULL;

	sw->config.vendor_id = 0x8086;
	sw->config.device_id = 0x15ef;

	sw->ports[0].config.type = TB_TYPE_PORT;
	sw->ports[0].config.max_in_hop_id = 8;
	sw->ports[0].config.max_out_hop_id = 8;

	sw->ports[1].config.type = TB_TYPE_PORT;
	sw->ports[1].config.max_in_hop_id = 19;
	sw->ports[1].config.max_out_hop_id = 19;
	sw->ports[1].dual_link_port = &sw->ports[2];

	sw->ports[2].config.type = TB_TYPE_PORT;
	sw->ports[2].config.max_in_hop_id = 19;
	sw->ports[2].config.max_out_hop_id = 19;
	sw->ports[2].dual_link_port = &sw->ports[1];
	sw->ports[2].link_nr = 1;

	sw->ports[3].config.type = TB_TYPE_PORT;
	sw->ports[3].config.max_in_hop_id = 19;
	sw->ports[3].config.max_out_hop_id = 19;
	sw->ports[3].dual_link_port = &sw->ports[4];

	sw->ports[4].config.type = TB_TYPE_PORT;
	sw->ports[4].config.max_in_hop_id = 19;
	sw->ports[4].config.max_out_hop_id = 19;
	sw->ports[4].dual_link_port = &sw->ports[3];
	sw->ports[4].link_nr = 1;

	sw->ports[5].config.type = TB_TYPE_PORT;
	sw->ports[5].config.max_in_hop_id = 19;
	sw->ports[5].config.max_out_hop_id = 19;
	sw->ports[5].dual_link_port = &sw->ports[6];

	sw->ports[6].config.type = TB_TYPE_PORT;
	sw->ports[6].config.max_in_hop_id = 19;
	sw->ports[6].config.max_out_hop_id = 19;
	sw->ports[6].dual_link_port = &sw->ports[5];
	sw->ports[6].link_nr = 1;

	sw->ports[7].config.type = TB_TYPE_PORT;
	sw->ports[7].config.max_in_hop_id = 19;
	sw->ports[7].config.max_out_hop_id = 19;
	sw->ports[7].dual_link_port = &sw->ports[8];

	sw->ports[8].config.type = TB_TYPE_PORT;
	sw->ports[8].config.max_in_hop_id = 19;
	sw->ports[8].config.max_out_hop_id = 19;
	sw->ports[8].dual_link_port = &sw->ports[7];
	sw->ports[8].link_nr = 1;

	sw->ports[9].config.type = TB_TYPE_PCIE_UP;
	sw->ports[9].config.max_in_hop_id = 8;
	sw->ports[9].config.max_out_hop_id = 8;

	sw->ports[10].config.type = TB_TYPE_PCIE_DOWN;
	sw->ports[10].config.max_in_hop_id = 8;
	sw->ports[10].config.max_out_hop_id = 8;

	sw->ports[11].config.type = TB_TYPE_PCIE_DOWN;
	sw->ports[11].config.max_in_hop_id = 8;
	sw->ports[11].config.max_out_hop_id = 8;

	sw->ports[12].config.type = TB_TYPE_PCIE_DOWN;
	sw->ports[12].config.max_in_hop_id = 8;
	sw->ports[12].config.max_out_hop_id = 8;

	sw->ports[13].config.type = TB_TYPE_DP_HDMI_OUT;
	sw->ports[13].config.max_in_hop_id = 9;
	sw->ports[13].config.max_out_hop_id = 9;
	sw->ports[13].cap_adap = -1;

	sw->ports[14].config.type = TB_TYPE_DP_HDMI_OUT;
	sw->ports[14].config.max_in_hop_id = 9;
	sw->ports[14].config.max_out_hop_id = 9;
	sw->ports[14].cap_adap = -1;

	sw->ports[15].disabled = true;

	sw->ports[16].config.type = TB_TYPE_USB3_UP;
	sw->ports[16].config.max_in_hop_id = 8;
	sw->ports[16].config.max_out_hop_id = 8;

	sw->ports[17].config.type = TB_TYPE_USB3_DOWN;
	sw->ports[17].config.max_in_hop_id = 8;
	sw->ports[17].config.max_out_hop_id = 8;

	sw->ports[18].config.type = TB_TYPE_USB3_DOWN;
	sw->ports[18].config.max_in_hop_id = 8;
	sw->ports[18].config.max_out_hop_id = 8;

	sw->ports[19].config.type = TB_TYPE_USB3_DOWN;
	sw->ports[19].config.max_in_hop_id = 8;
	sw->ports[19].config.max_out_hop_id = 8;

	if (!parent)
		return sw;

	/* Link them */
	upstream_port = tb_upstream_port(sw);
	port = tb_port_at(route, parent);
	port->remote = upstream_port;
	upstream_port->remote = port;
	if (port->dual_link_port && upstream_port->dual_link_port) {
		port->dual_link_port->remote = upstream_port->dual_link_port;
		upstream_port->dual_link_port->remote = port->dual_link_port;
	}

	/* Bonding is used */
	port->bonded = true;
	port->dual_link_port->bonded = true;
	upstream_port->bonded = true;
	upstream_port->dual_link_port->bonded = true;

	return sw;
}

static struct tb_switch *alloc_dev_with_dpin(struct kunit *test,
					     struct tb_switch *parent,
					     u64 route)
{
	struct tb_switch *sw;

	sw = alloc_dev_default(test, parent, route);
	if (!sw)
		return NULL;

	sw->ports[13].config.type = TB_TYPE_DP_HDMI_IN;
	sw->ports[13].config.max_in_hop_id = 9;
	sw->ports[13].config.max_out_hop_id = 9;

	sw->ports[14].config.type = TB_TYPE_DP_HDMI_IN;
	sw->ports[14].config.max_in_hop_id = 9;
	sw->ports[14].config.max_out_hop_id = 9;

	return sw;
}

static void tb_test_path_basic(struct kunit *test)
{
	struct tb_port *src_port, *dst_port, *p;
	struct tb_switch *host;

	host = alloc_host(test);

	src_port = &host->ports[5];
	dst_port = src_port;

	p = tb_next_port_on_path(src_port, dst_port, NULL);
	KUNIT_EXPECT_PTR_EQ(test, p, dst_port);

	p = tb_next_port_on_path(src_port, dst_port, p);
	KUNIT_EXPECT_TRUE(test, !p);
}

static void tb_test_path_not_connected(struct kunit *test)
{
	struct tb_port *src_port, *dst_port, *p;
	struct tb_switch *host, *dev;

	host = alloc_host(test);
	/* No connection between host and dev */
	dev = alloc_dev_default(test, NULL, 3);

	src_port = &host->ports[12];
	dst_port = &dev->ports[16];

	p = tb_next_port_on_path(src_port, dst_port, NULL);
	KUNIT_EXPECT_PTR_EQ(test, p, src_port);

	p = tb_next_port_on_path(src_port, dst_port, p);
	KUNIT_EXPECT_PTR_EQ(test, p, &host->ports[3]);

	p = tb_next_port_on_path(src_port, dst_port, p);
	KUNIT_EXPECT_TRUE(test, !p);

	/* Other direction */

	p = tb_next_port_on_path(dst_port, src_port, NULL);
	KUNIT_EXPECT_PTR_EQ(test, p, dst_port);

	p = tb_next_port_on_path(dst_port, src_port, p);
	KUNIT_EXPECT_PTR_EQ(test, p, &dev->ports[1]);

	p = tb_next_port_on_path(dst_port, src_port, p);
	KUNIT_EXPECT_TRUE(test, !p);
}

static void tb_test_path_single_hop_walk(struct kunit *test)
{
	/*
	 * Walks from Host PCIe downstream port to Device #1 PCIe
	 * upstream port.
	 *
	 *   [Host]
	 *   1 |
	 *   1 |
	 *  [Device]
	 */
	static const struct port_expectation test_data[] = {
		{ .route = 0x0, .port = 8, .type = TB_TYPE_PCIE_DOWN },
		{ .route = 0x0, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 9, .type = TB_TYPE_PCIE_UP },
	};
	struct tb_port *src_port, *dst_port, *p;
	struct tb_switch *host, *dev;
	int i;

	host = alloc_host(test);
	dev = alloc_dev_default(test, host, 1);

	src_port = &host->ports[8];
	dst_port = &dev->ports[9];

	/* Walk both directions */

	i = 0;
	tb_for_each_port_on_path(src_port, dst_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i++;
	}

	KUNIT_EXPECT_EQ(test, i, (int)ARRAY_SIZE(test_data));

	i = ARRAY_SIZE(test_data) - 1;
	tb_for_each_port_on_path(dst_port, src_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i--;
	}

	KUNIT_EXPECT_EQ(test, i, -1);
}

static void tb_test_path_daisy_chain_walk(struct kunit *test)
{
	/*
	 * Walks from Host DP IN to Device #2 DP OUT.
	 *
	 *           [Host]
	 *            1 |
	 *            1 |
	 *         [Device #1]
	 *       3 /
	 *      1 /
	 * [Device #2]
	 */
	static const struct port_expectation test_data[] = {
		{ .route = 0x0, .port = 5, .type = TB_TYPE_DP_HDMI_IN },
		{ .route = 0x0, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x301, .port = 13, .type = TB_TYPE_DP_HDMI_OUT },
	};
	struct tb_port *src_port, *dst_port, *p;
	struct tb_switch *host, *dev1, *dev2;
	int i;

	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x1);
	dev2 = alloc_dev_default(test, dev1, 0x301);

	src_port = &host->ports[5];
	dst_port = &dev2->ports[13];

	/* Walk both directions */

	i = 0;
	tb_for_each_port_on_path(src_port, dst_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i++;
	}

	KUNIT_EXPECT_EQ(test, i, (int)ARRAY_SIZE(test_data));

	i = ARRAY_SIZE(test_data) - 1;
	tb_for_each_port_on_path(dst_port, src_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i--;
	}

	KUNIT_EXPECT_EQ(test, i, -1);
}

static void tb_test_path_simple_tree_walk(struct kunit *test)
{
	/*
	 * Walks from Host DP IN to Device #3 DP OUT.
	 *
	 *           [Host]
	 *            1 |
	 *            1 |
	 *         [Device #1]
	 *       3 /   | 5  \ 7
	 *      1 /    |     \ 1
	 * [Device #2] |    [Device #4]
	 *             | 1
	 *         [Device #3]
	 */
	static const struct port_expectation test_data[] = {
		{ .route = 0x0, .port = 5, .type = TB_TYPE_DP_HDMI_IN },
		{ .route = 0x0, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 5, .type = TB_TYPE_PORT },
		{ .route = 0x501, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x501, .port = 13, .type = TB_TYPE_DP_HDMI_OUT },
	};
	struct tb_switch *host, *dev1, *dev2, *dev3, *dev4;
	struct tb_port *src_port, *dst_port, *p;
	int i;

	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x1);
	dev2 = alloc_dev_default(test, dev1, 0x301);
	dev3 = alloc_dev_default(test, dev1, 0x501);
	dev4 = alloc_dev_default(test, dev1, 0x701);

	src_port = &host->ports[5];
	dst_port = &dev3->ports[13];

	/* Walk both directions */

	i = 0;
	tb_for_each_port_on_path(src_port, dst_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i++;
	}

	KUNIT_EXPECT_EQ(test, i, (int)ARRAY_SIZE(test_data));

	i = ARRAY_SIZE(test_data) - 1;
	tb_for_each_port_on_path(dst_port, src_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i--;
	}

	KUNIT_EXPECT_EQ(test, i, -1);
}

static void tb_test_path_complex_tree_walk(struct kunit *test)
{
	/*
	 * Walks from Device #3 DP IN to Device #9 DP OUT.
	 *
	 *           [Host]
	 *            1 |
	 *            1 |
	 *         [Device #1]
	 *       3 /   | 5  \ 7
	 *      1 /    |     \ 1
	 * [Device #2] |    [Device #5]
	 *    5 |      | 1         \ 7
	 *    1 |  [Device #4]      \ 1
	 * [Device #3]             [Device #6]
	 *                       3 /
	 *                      1 /
	 *                    [Device #7]
	 *                  3 /      | 5
	 *                 1 /       |
	 *               [Device #8] | 1
	 *                       [Device #9]
	 */
	static const struct port_expectation test_data[] = {
		{ .route = 0x50301, .port = 13, .type = TB_TYPE_DP_HDMI_IN },
		{ .route = 0x50301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x301, .port = 5, .type = TB_TYPE_PORT },
		{ .route = 0x301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 7, .type = TB_TYPE_PORT },
		{ .route = 0x701, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x701, .port = 7, .type = TB_TYPE_PORT },
		{ .route = 0x70701, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x70701, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x3070701, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x3070701, .port = 5, .type = TB_TYPE_PORT },
		{ .route = 0x503070701, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x503070701, .port = 14, .type = TB_TYPE_DP_HDMI_OUT },
	};
	struct tb_switch *host, *dev1, *dev2, *dev3, *dev4, *dev5;
	struct tb_switch *dev6, *dev7, *dev8, *dev9;
	struct tb_port *src_port, *dst_port, *p;
	int i;

	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x1);
	dev2 = alloc_dev_default(test, dev1, 0x301);
	dev3 = alloc_dev_with_dpin(test, dev2, 0x50301);
	dev4 = alloc_dev_default(test, dev1, 0x501);
	dev5 = alloc_dev_default(test, dev1, 0x701);
	dev6 = alloc_dev_default(test, dev5, 0x70701);
	dev7 = alloc_dev_default(test, dev6, 0x3070701);
	dev8 = alloc_dev_default(test, dev7, 0x303070701);
	dev9 = alloc_dev_default(test, dev7, 0x503070701);

	src_port = &dev3->ports[13];
	dst_port = &dev9->ports[14];

	/* Walk both directions */

	i = 0;
	tb_for_each_port_on_path(src_port, dst_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i++;
	}

	KUNIT_EXPECT_EQ(test, i, (int)ARRAY_SIZE(test_data));

	i = ARRAY_SIZE(test_data) - 1;
	tb_for_each_port_on_path(dst_port, src_port, p) {
		KUNIT_EXPECT_TRUE(test, i < ARRAY_SIZE(test_data));
		KUNIT_EXPECT_EQ(test, tb_route(p->sw), test_data[i].route);
		KUNIT_EXPECT_EQ(test, p->port, test_data[i].port);
		KUNIT_EXPECT_EQ(test, (enum tb_port_type)p->config.type,
				test_data[i].type);
		i--;
	}

	KUNIT_EXPECT_EQ(test, i, -1);
}

static void tb_test_tunnel_usb3(struct kunit *test)
{
	struct tb_switch *host, *dev1, *dev2;
	struct tb_tunnel *tunnel1, *tunnel2;
	struct tb_port *down, *up;

	/*
	 * Create USB3 tunnel between host and two devices.
	 *
	 *   [Host]
	 *    1 |
	 *    1 |
	 *  [Device #1]
	 *          \ 7
	 *           \ 1
	 *         [Device #2]
	 */
	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x1);
	dev2 = alloc_dev_default(test, dev1, 0x701);

	down = &host->ports[12];
	up = &dev1->ports[16];
	tunnel1 = tb_tunnel_alloc_usb3(NULL, up, down, 0, 0);
	KUNIT_ASSERT_TRUE(test, tunnel1 != NULL);
	KUNIT_EXPECT_EQ(test, tunnel1->type, (enum tb_tunnel_type)TB_TUNNEL_USB3);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->src_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->dst_port, up);
	KUNIT_ASSERT_EQ(test, tunnel1->npaths, (size_t)2);
	KUNIT_ASSERT_EQ(test, tunnel1->paths[0]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[0]->hops[0].in_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[0]->hops[1].out_port, up);
	KUNIT_ASSERT_EQ(test, tunnel1->paths[1]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[1]->hops[0].in_port, up);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[1]->hops[1].out_port, down);

	down = &dev1->ports[17];
	up = &dev2->ports[16];
	tunnel2 = tb_tunnel_alloc_usb3(NULL, up, down, 0, 0);
	KUNIT_ASSERT_TRUE(test, tunnel2 != NULL);
	KUNIT_EXPECT_EQ(test, tunnel2->type, (enum tb_tunnel_type)TB_TUNNEL_USB3);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->src_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->dst_port, up);
	KUNIT_ASSERT_EQ(test, tunnel2->npaths, (size_t)2);
	KUNIT_ASSERT_EQ(test, tunnel2->paths[0]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[0]->hops[0].in_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[0]->hops[1].out_port, up);
	KUNIT_ASSERT_EQ(test, tunnel2->paths[1]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[1]->hops[0].in_port, up);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[1]->hops[1].out_port, down);

	tb_tunnel_free(tunnel2);
	tb_tunnel_free(tunnel1);
}

static void tb_test_tunnel_pcie(struct kunit *test)
{
	struct tb_switch *host, *dev1, *dev2;
	struct tb_tunnel *tunnel1, *tunnel2;
	struct tb_port *down, *up;

	/*
	 * Create PCIe tunnel between host and two devices.
	 *
	 *   [Host]
	 *    1 |
	 *    1 |
	 *  [Device #1]
	 *    5 |
	 *    1 |
	 *  [Device #2]
	 */
	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 1);
	dev2 = alloc_dev_default(test, dev1, 0x501);

	down = &host->ports[8];
	up = &dev1->ports[9];
	tunnel1 = tb_tunnel_alloc_pci(NULL, up, down);
	KUNIT_ASSERT_TRUE(test, tunnel1 != NULL);
	KUNIT_EXPECT_EQ(test, tunnel1->type, (enum tb_tunnel_type)TB_TUNNEL_PCI);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->src_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->dst_port, up);
	KUNIT_ASSERT_EQ(test, tunnel1->npaths, (size_t)2);
	KUNIT_ASSERT_EQ(test, tunnel1->paths[0]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[0]->hops[0].in_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[0]->hops[1].out_port, up);
	KUNIT_ASSERT_EQ(test, tunnel1->paths[1]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[1]->hops[0].in_port, up);
	KUNIT_EXPECT_PTR_EQ(test, tunnel1->paths[1]->hops[1].out_port, down);

	down = &dev1->ports[10];
	up = &dev2->ports[9];
	tunnel2 = tb_tunnel_alloc_pci(NULL, up, down);
	KUNIT_ASSERT_TRUE(test, tunnel2 != NULL);
	KUNIT_EXPECT_EQ(test, tunnel2->type, (enum tb_tunnel_type)TB_TUNNEL_PCI);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->src_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->dst_port, up);
	KUNIT_ASSERT_EQ(test, tunnel2->npaths, (size_t)2);
	KUNIT_ASSERT_EQ(test, tunnel2->paths[0]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[0]->hops[0].in_port, down);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[0]->hops[1].out_port, up);
	KUNIT_ASSERT_EQ(test, tunnel2->paths[1]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[1]->hops[0].in_port, up);
	KUNIT_EXPECT_PTR_EQ(test, tunnel2->paths[1]->hops[1].out_port, down);

	tb_tunnel_free(tunnel2);
	tb_tunnel_free(tunnel1);
}

static void tb_test_tunnel_dp(struct kunit *test)
{
	struct tb_switch *host, *dev;
	struct tb_port *in, *out;
	struct tb_tunnel *tunnel;

	/*
	 * Create DP tunnel between Host and Device
	 *
	 *   [Host]
	 *   1 |
	 *   1 |
	 *  [Device]
	 */
	host = alloc_host(test);
	dev = alloc_dev_default(test, host, 3);

	in = &host->ports[5];
	out = &dev->ports[13];

	tunnel = tb_tunnel_alloc_dp(NULL, in, out, 0, 0);
	KUNIT_ASSERT_TRUE(test, tunnel != NULL);
	KUNIT_EXPECT_EQ(test, tunnel->type, (enum tb_tunnel_type)TB_TUNNEL_DP);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->src_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->dst_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->npaths, (size_t)3);
	KUNIT_ASSERT_EQ(test, tunnel->paths[0]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[0].in_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[1].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[1]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[0].in_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[1].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[2]->path_length, 2);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[0].in_port, out);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[1].out_port, in);
	tb_tunnel_free(tunnel);
}

static void tb_test_tunnel_dp_chain(struct kunit *test)
{
	struct tb_switch *host, *dev1, *dev2, *dev3, *dev4;
	struct tb_port *in, *out;
	struct tb_tunnel *tunnel;

	/*
	 * Create DP tunnel from Host DP IN to Device #4 DP OUT.
	 *
	 *           [Host]
	 *            1 |
	 *            1 |
	 *         [Device #1]
	 *       3 /   | 5  \ 7
	 *      1 /    |     \ 1
	 * [Device #2] |    [Device #4]
	 *             | 1
	 *         [Device #3]
	 */
	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x1);
	dev2 = alloc_dev_default(test, dev1, 0x301);
	dev3 = alloc_dev_default(test, dev1, 0x501);
	dev4 = alloc_dev_default(test, dev1, 0x701);

	in = &host->ports[5];
	out = &dev4->ports[14];

	tunnel = tb_tunnel_alloc_dp(NULL, in, out, 0, 0);
	KUNIT_ASSERT_TRUE(test, tunnel != NULL);
	KUNIT_EXPECT_EQ(test, tunnel->type, (enum tb_tunnel_type)TB_TUNNEL_DP);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->src_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->dst_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->npaths, (size_t)3);
	KUNIT_ASSERT_EQ(test, tunnel->paths[0]->path_length, 3);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[0].in_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[2].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[1]->path_length, 3);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[0].in_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[2].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[2]->path_length, 3);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[0].in_port, out);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[2].out_port, in);
	tb_tunnel_free(tunnel);
}

static void tb_test_tunnel_dp_tree(struct kunit *test)
{
	struct tb_switch *host, *dev1, *dev2, *dev3, *dev4, *dev5;
	struct tb_port *in, *out;
	struct tb_tunnel *tunnel;

	/*
	 * Create DP tunnel from Device #2 DP IN to Device #5 DP OUT.
	 *
	 *          [Host]
	 *           3 |
	 *           1 |
	 *         [Device #1]
	 *       3 /   | 5  \ 7
	 *      1 /    |     \ 1
	 * [Device #2] |    [Device #4]
	 *             | 1
	 *         [Device #3]
	 *             | 5
	 *             | 1
	 *         [Device #5]
	 */
	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x3);
	dev2 = alloc_dev_with_dpin(test, dev1, 0x303);
	dev3 = alloc_dev_default(test, dev1, 0x503);
	dev4 = alloc_dev_default(test, dev1, 0x703);
	dev5 = alloc_dev_default(test, dev3, 0x50503);

	in = &dev2->ports[13];
	out = &dev5->ports[13];

	tunnel = tb_tunnel_alloc_dp(NULL, in, out, 0, 0);
	KUNIT_ASSERT_TRUE(test, tunnel != NULL);
	KUNIT_EXPECT_EQ(test, tunnel->type, (enum tb_tunnel_type)TB_TUNNEL_DP);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->src_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->dst_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->npaths, (size_t)3);
	KUNIT_ASSERT_EQ(test, tunnel->paths[0]->path_length, 4);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[0].in_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[3].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[1]->path_length, 4);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[0].in_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[3].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[2]->path_length, 4);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[0].in_port, out);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[3].out_port, in);
	tb_tunnel_free(tunnel);
}

static void tb_test_tunnel_dp_max_length(struct kunit *test)
{
	struct tb_switch *host, *dev1, *dev2, *dev3, *dev4, *dev5;
	struct tb_switch *dev6, *dev7, *dev8, *dev9, *dev10, *dev11;
	struct tb_port *in, *out;
	struct tb_tunnel *tunnel;

	/*
	 * Creates DP tunnel from Device #6 to Device #11.
	 *
	 *          [Host]
	 *           1 |
	 *           1 |
	 *       [Device #1]
	 *        3 /    \ 7
	 *       1 /      \ 1
	 * [Device #2]   [Device #7]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #3]   [Device #8]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #4]   [Device #9]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #5]   [Device #10]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #6]   [Device #11]
	 */
	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x1);
	dev2 = alloc_dev_default(test, dev1, 0x301);
	dev3 = alloc_dev_default(test, dev2, 0x30301);
	dev4 = alloc_dev_default(test, dev3, 0x3030301);
	dev5 = alloc_dev_default(test, dev4, 0x303030301);
	dev6 = alloc_dev_with_dpin(test, dev5, 0x30303030301);
	dev7 = alloc_dev_default(test, dev1, 0x701);
	dev8 = alloc_dev_default(test, dev7, 0x30701);
	dev9 = alloc_dev_default(test, dev8,  0x3030701);
        dev10 = alloc_dev_default(test, dev9, 0x303030701);
	dev11 = alloc_dev_default(test, dev10, 0x30303030701);

	in = &dev6->ports[13];
	out = &dev11->ports[13];

	tunnel = tb_tunnel_alloc_dp(NULL, in, out, 0, 0);
	KUNIT_ASSERT_TRUE(test, tunnel != NULL);
	KUNIT_EXPECT_EQ(test, tunnel->type, (enum tb_tunnel_type)TB_TUNNEL_DP);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->src_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->dst_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->npaths, (size_t)3);
	KUNIT_ASSERT_EQ(test, tunnel->paths[0]->path_length, 11);
	/* First hop */
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[0].in_port, in);
	/* Middle */
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[5].in_port,
			    &dev1->ports[3]);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[5].out_port,
			    &dev1->ports[7]);
	/* Last */
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[0]->hops[10].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[1]->path_length, 11);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[0].in_port, in);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[5].in_port,
			    &dev1->ports[3]);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[5].out_port,
			    &dev1->ports[7]);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[1]->hops[10].out_port, out);
	KUNIT_ASSERT_EQ(test, tunnel->paths[2]->path_length, 11);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[0].in_port, out);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[5].in_port,
			    &dev1->ports[7]);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[5].out_port,
			    &dev1->ports[3]);
	KUNIT_EXPECT_PTR_EQ(test, tunnel->paths[2]->hops[10].out_port, in);
	tb_tunnel_free(tunnel);
}

static void tb_test_tunnel_port_on_path(struct kunit *test)
{
	struct tb_switch *host, *dev1, *dev2, *dev3, *dev4, *dev5;
	struct tb_port *in, *out, *port;
	struct tb_tunnel *dp_tunnel;

	/*
	 *          [Host]
	 *           3 |
	 *           1 |
	 *         [Device #1]
	 *       3 /   | 5  \ 7
	 *      1 /    |     \ 1
	 * [Device #2] |    [Device #4]
	 *             | 1
	 *         [Device #3]
	 *             | 5
	 *             | 1
	 *         [Device #5]
	 */
	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x3);
	dev2 = alloc_dev_with_dpin(test, dev1, 0x303);
	dev3 = alloc_dev_default(test, dev1, 0x503);
	dev4 = alloc_dev_default(test, dev1, 0x703);
	dev5 = alloc_dev_default(test, dev3, 0x50503);

	in = &dev2->ports[13];
	out = &dev5->ports[13];

	dp_tunnel = tb_tunnel_alloc_dp(NULL, in, out, 0, 0);
	KUNIT_ASSERT_TRUE(test, dp_tunnel != NULL);

	KUNIT_EXPECT_TRUE(test, tb_tunnel_port_on_path(dp_tunnel, in));
	KUNIT_EXPECT_TRUE(test, tb_tunnel_port_on_path(dp_tunnel, out));

	port = &host->ports[8];
	KUNIT_EXPECT_FALSE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &host->ports[3];
	KUNIT_EXPECT_FALSE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &dev1->ports[1];
	KUNIT_EXPECT_FALSE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &dev1->ports[3];
	KUNIT_EXPECT_TRUE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &dev1->ports[5];
	KUNIT_EXPECT_TRUE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &dev1->ports[7];
	KUNIT_EXPECT_FALSE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &dev3->ports[1];
	KUNIT_EXPECT_TRUE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &dev5->ports[1];
	KUNIT_EXPECT_TRUE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	port = &dev4->ports[1];
	KUNIT_EXPECT_FALSE(test, tb_tunnel_port_on_path(dp_tunnel, port));

	tb_tunnel_free(dp_tunnel);
}

static void tb_test_property_parse(struct kunit *test)
{
	struct tb_property_dir *dir, *network_dir;
	struct tb_property *p;

	dir = tb_property_parse_dir(root_directory, ARRAY_SIZE(root_directory));
	KUNIT_ASSERT_TRUE(test, dir != NULL);

	p = tb_property_find(dir, "foo", TB_PROPERTY_TYPE_TEXT);
	KUNIT_ASSERT_TRUE(test, !p);

	p = tb_property_find(dir, "vendorid", TB_PROPERTY_TYPE_TEXT);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_STREQ(test, p->value.text, "Apple Inc.");

	p = tb_property_find(dir, "vendorid", TB_PROPERTY_TYPE_VALUE);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_EQ(test, p->value.immediate, (u32)0xa27);

	p = tb_property_find(dir, "deviceid", TB_PROPERTY_TYPE_TEXT);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_STREQ(test, p->value.text, "Macintosh");

	p = tb_property_find(dir, "deviceid", TB_PROPERTY_TYPE_VALUE);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_EQ(test, p->value.immediate, (u32)0xa);

	p = tb_property_find(dir, "missing", TB_PROPERTY_TYPE_DIRECTORY);
	KUNIT_ASSERT_TRUE(test, !p);

	p = tb_property_find(dir, "network", TB_PROPERTY_TYPE_DIRECTORY);
	KUNIT_ASSERT_TRUE(test, p != NULL);

	network_dir = p->value.dir;
	KUNIT_EXPECT_TRUE(test, uuid_equal(network_dir->uuid, &network_dir_uuid));

	p = tb_property_find(network_dir, "prtcid", TB_PROPERTY_TYPE_VALUE);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_EQ(test, p->value.immediate, (u32)0x1);

	p = tb_property_find(network_dir, "prtcvers", TB_PROPERTY_TYPE_VALUE);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_EQ(test, p->value.immediate, (u32)0x1);

	p = tb_property_find(network_dir, "prtcrevs", TB_PROPERTY_TYPE_VALUE);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_EQ(test, p->value.immediate, (u32)0x1);

	p = tb_property_find(network_dir, "prtcstns", TB_PROPERTY_TYPE_VALUE);
	KUNIT_ASSERT_TRUE(test, p != NULL);
	KUNIT_EXPECT_EQ(test, p->value.immediate, (u32)0x0);

	p = tb_property_find(network_dir, "deviceid", TB_PROPERTY_TYPE_VALUE);
	KUNIT_EXPECT_TRUE(test, !p);
	p = tb_property_find(network_dir, "deviceid", TB_PROPERTY_TYPE_TEXT);
	KUNIT_EXPECT_TRUE(test, !p);

	tb_property_free_dir(dir);
}

static void tb_test_property_format(struct kunit *test)
{
	struct tb_property_dir *dir;
	ssize_t block_len;
	u32 *block;
	int ret, i;

	dir = tb_property_parse_dir(root_directory, ARRAY_SIZE(root_directory));
	KUNIT_ASSERT_TRUE(test, dir != NULL);

	ret = tb_property_format_dir(dir, NULL, 0);
	KUNIT_ASSERT_EQ(test, ret, (int)ARRAY_SIZE(root_directory));

	block_len = ret;

	block = kunit_kzalloc(test, block_len * sizeof(u32), GFP_KERNEL);
	KUNIT_ASSERT_TRUE(test, block != NULL);

	ret = tb_property_format_dir(dir, block, block_len);
	KUNIT_EXPECT_EQ(test, ret, 0);

	for (i = 0; i < ARRAY_SIZE(root_directory); i++)
		KUNIT_EXPECT_EQ(test, root_directory[i], block[i]);

	tb_property_free_dir(dir);
}

static struct kunit_case tb_test_cases[] = {
	KUNIT_CASE(tb_test_path_basic),
	KUNIT_CASE(tb_test_path_not_connected),
	KUNIT_CASE(tb_test_path_single_hop_walk),
	KUNIT_CASE(tb_test_path_daisy_chain_walk),
	KUNIT_CASE(tb_test_path_simple_tree_walk),
	KUNIT_CASE(tb_test_path_complex_tree_walk),
	KUNIT_CASE(tb_test_tunnel_usb3),
	KUNIT_CASE(tb_test_tunnel_pcie),
	KUNIT_CASE(tb_test_tunnel_dp),
	KUNIT_CASE(tb_test_tunnel_dp_chain),
	KUNIT_CASE(tb_test_tunnel_dp_tree),
	KUNIT_CASE(tb_test_tunnel_dp_max_length),
	KUNIT_CASE(tb_test_tunnel_port_on_path),
	KUNIT_CASE(tb_test_property_parse),
	KUNIT_CASE(tb_test_property_format),
	{ }
};

static struct kunit_suite tb_test_suite = {
	.name = "thunderbolt",
	.test_cases = tb_test_cases,
};
kunit_test_suite(tb_test_suite);
