// SPDX-License-Identifier: GPL-2.0
/*
 * KUnit tests
 *
 * Copyright (C) 2020, Intel Corporation
 * Author: Mika Westerberg <mika.westerberg@linux.intel.com>
 */

#include <kunit/test.h>
#include <linux/idr.h>

#include "tb.h"

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

struct port_expectation {
	u64 route;
	u8 port;
	enum tb_port_type type;
};

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

static void tb_test_path_max_length_walk(struct kunit *test)
{
	struct tb_switch *host, *dev1, *dev2, *dev3, *dev4, *dev5, *dev6;
	struct tb_switch *dev7, *dev8, *dev9, *dev10, *dev11, *dev12;
	struct tb_port *src_port, *dst_port, *p;
	int i;

	/*
	 * Walks from Device #6 DP IN to Device #12 DP OUT.
	 *
	 *          [Host]
	 *         1 /  \ 3
	 *        1 /    \ 1
	 * [Device #1]   [Device #7]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #2]   [Device #8]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #3]   [Device #9]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #4]   [Device #10]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #5]   [Device #11]
	 *     3 |           | 3
	 *     1 |           | 1
	 * [Device #6]   [Device #12]
	 */
	static const struct port_expectation test_data[] = {
		{ .route = 0x30303030301, .port = 13, .type = TB_TYPE_DP_HDMI_IN },
		{ .route = 0x30303030301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x303030301, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x303030301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x3030301, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x3030301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x30301, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x30301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x301, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x301, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x1, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x0, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x0, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x3, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x3, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x303, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x303, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x30303, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x30303, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x3030303, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x3030303, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x303030303, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x303030303, .port = 3, .type = TB_TYPE_PORT },
		{ .route = 0x30303030303, .port = 1, .type = TB_TYPE_PORT },
		{ .route = 0x30303030303, .port = 13, .type = TB_TYPE_DP_HDMI_OUT },
	};

	host = alloc_host(test);
	dev1 = alloc_dev_default(test, host, 0x1);
	dev2 = alloc_dev_default(test, dev1, 0x301);
	dev3 = alloc_dev_default(test, dev2, 0x30301);
	dev4 = alloc_dev_default(test, dev3, 0x3030301);
	dev5 = alloc_dev_default(test, dev4, 0x303030301);
	dev6 = alloc_dev_with_dpin(test, dev5, 0x30303030301);
	dev7 = alloc_dev_default(test, host, 0x3);
	dev8 = alloc_dev_default(test, dev7, 0x303);
	dev9 = alloc_dev_default(test, dev8, 0x30303);
	dev10 = alloc_dev_default(test, dev9, 0x3030303);
	dev11 = alloc_dev_default(test, dev10, 0x303030303);
	dev12 = alloc_dev_default(test, dev11, 0x30303030303);

	src_port = &dev6->ports[13];
	dst_port = &dev12->ports[13];

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

static struct kunit_case tb_test_cases[] = {
	KUNIT_CASE(tb_test_path_basic),
	KUNIT_CASE(tb_test_path_not_connected),
	KUNIT_CASE(tb_test_path_single_hop_walk),
	KUNIT_CASE(tb_test_path_daisy_chain_walk),
	KUNIT_CASE(tb_test_path_simple_tree_walk),
	KUNIT_CASE(tb_test_path_complex_tree_walk),
	KUNIT_CASE(tb_test_path_max_length_walk),
	{ }
};

static struct kunit_suite tb_test_suite = {
	.name = "thunderbolt",
	.test_cases = tb_test_cases,
};
kunit_test_suite(tb_test_suite);
