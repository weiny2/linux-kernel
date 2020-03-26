/*
 * Intel Thunderbay SoC GPIO driver
 *
 * Copyright (C) 2020 Intel Corporation
 * Author: S, Kiran Kumar1 <kiran.kumar1.s@intel.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include "core.h"
#include "pinconf.h"
#include "pinmux.h"
#include "pinctrl-utils.h"

#include "pinctrl-thunderbay.h"


const struct pinctrl_pin_desc thunderbay_pins[] = {
	THUNDERBAY_PIN_DESC(0, "GPIO0",
		THUNDERBAY_MUX(0x0, "I2C0_SCL"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(1, "GPIO1",
		THUNDERBAY_MUX(0x0, "I2C0_SDA"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(2, "GPIO2",
		THUNDERBAY_MUX(0x0, "I2C1_SCL"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(3, "GPIO3",
		THUNDERBAY_MUX(0x0, "I2C1_SDA"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(4, "GPIO4",
		THUNDERBAY_MUX(0x0, "I2C2_SCL"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(5, "GPIO5",
		THUNDERBAY_MUX(0x0, "I2C2_SDA"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(6, "GPIO6",
		THUNDERBAY_MUX(0x0, "I2C3_SCL"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(7, "GPIO7",
		THUNDERBAY_MUX(0x0, "I2C3_SDA"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(8, "GPIO8",
		THUNDERBAY_MUX(0x0, "I2C4_SCL"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(9, "GPIO9",
		THUNDERBAY_MUX(0x0, "I2C4_SDA"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(10, "GPIO10",
		THUNDERBAY_MUX(0x0, "UART0_SOUT"),
		THUNDERBAY_MUX(0x1, "rt0_dsu_active"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(11, "GPIO11",
		THUNDERBAY_MUX(0x0, "UART0_SIN"),
		THUNDERBAY_MUX(0x1, "rt0_dsu_tstop"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(12, "GPIO12",
		THUNDERBAY_MUX(0x0, "uart0_cts_n"),
		THUNDERBAY_MUX(0x1, "rt1_dsu_active"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(13, "GPIO13",
		THUNDERBAY_MUX(0x0, "uart0_rts_n"),
		THUNDERBAY_MUX(0x1, "rt1_dsu_tstop"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(14, "GPIO14",
		THUNDERBAY_MUX(0x0, "uart1_sout"),
		THUNDERBAY_MUX(0x1, "rt2_dsu_active"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "trigger_out"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(15, "GPIO15",
		THUNDERBAY_MUX(0x0, "uart1_sin"),
		THUNDERBAY_MUX(0x1, "rt2_dsu_tstop"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "trigger_in"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(16, "GPIO16",
		THUNDERBAY_MUX(0x0, "uart1_cts_n"),
		THUNDERBAY_MUX(0x1, "rt3_dsu_active"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(17, "GPIO17",
		THUNDERBAY_MUX(0x0, "uart1_rts_n"),
		THUNDERBAY_MUX(0x1, "rt3_dsu_tstop"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(18, "GPIO18",
		THUNDERBAY_MUX(0x0, "spi0_sclk"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(19, "GPIO19",
		THUNDERBAY_MUX(0x0, "spi0_ss_0"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(20, "GPIO20",
		THUNDERBAY_MUX(0x0, "spi0_dio_0_mosi"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "tpiu_traceclk"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(21, "GPIO21",
		THUNDERBAY_MUX(0x0, "spi0_dio_1_miso"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "tpiu_tracectl"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(22, "GPIO22",
		THUNDERBAY_MUX(0x0, "spi1_sclk"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(23, "GPIO23",
		THUNDERBAY_MUX(0x0, "spi1_ss_0"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(24, "GPIO24",
		THUNDERBAY_MUX(0x0, "spi1_dio_0_mosi"),
		THUNDERBAY_MUX(0x1, "tpiu_traceclk"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(25, "GPIO25",
		THUNDERBAY_MUX(0x0, "spi1_dio_1_miso"),
		THUNDERBAY_MUX(0x1, "tpiu_tracectl"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(26, "GPIO26",
		THUNDERBAY_MUX(0x0, "ether0_phy_txen"),
		THUNDERBAY_MUX(0x1, "tpiu_data0"),
		THUNDERBAY_MUX(0x2, "tpiu_data16"),
		THUNDERBAY_MUX(0x3, "debug_0"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(27, "GPIO27",
		THUNDERBAY_MUX(0x0, "ether0_gmii_clk_tx"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data17"),
		THUNDERBAY_MUX(0x3, "debug_1"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(28, "GPIO28",
		THUNDERBAY_MUX(0x0, "ether0_phy_txd_0"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data18"),
		THUNDERBAY_MUX(0x3, "debug_2"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(29, "GPIO29",
		THUNDERBAY_MUX(0x0, "ether0_phy_txd_1"),
		THUNDERBAY_MUX(0x1, "tpiu_data3"),
		THUNDERBAY_MUX(0x2, "tpiu_data19"),
		THUNDERBAY_MUX(0x3, "debug_3"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(30, "GPIO30",
		THUNDERBAY_MUX(0x0, "ether0_phy_txd_2"),
		THUNDERBAY_MUX(0x1, "tpiu_data4"),
		THUNDERBAY_MUX(0x2, "tpiu_data20"),
		THUNDERBAY_MUX(0x3, "debug_4"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(31, "GPIO31",
		THUNDERBAY_MUX(0x0, "ether0_phy_txd_3"),
		THUNDERBAY_MUX(0x1, "tpiu_data5"),
		THUNDERBAY_MUX(0x2, "tpiu_data21"),
		THUNDERBAY_MUX(0x3, "debug_5"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(32, "GPIO32",
		THUNDERBAY_MUX(0x0, "ether0_phy_rxdv"),
		THUNDERBAY_MUX(0x1, "tpiu_data6"),
		THUNDERBAY_MUX(0x2, "tpiu_data22"),
		THUNDERBAY_MUX(0x3, "debug_6"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(33, "GPIO33",
		THUNDERBAY_MUX(0x0, "ether0_gmii_clk_rx"),
		THUNDERBAY_MUX(0x1, "tpiu_data7"),
		THUNDERBAY_MUX(0x2, "tpiu_data23"),
		THUNDERBAY_MUX(0x3, "debug_7"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(34, "GPIO34",
		THUNDERBAY_MUX(0x0, "ether0_phy_rxd_0"),
		THUNDERBAY_MUX(0x1, "tpiu_data8"),
		THUNDERBAY_MUX(0x2, "tpiu_data24"),
		THUNDERBAY_MUX(0x3, "dig_view_0"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(35, "GPIO35",
		THUNDERBAY_MUX(0x0, "ether0_phy_rxd_1"),
		THUNDERBAY_MUX(0x1, "tpiu_data9"),
		THUNDERBAY_MUX(0x2, "tpiu_data25"),
		THUNDERBAY_MUX(0x3, "dig_view_1"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(36, "GPIO36",
		THUNDERBAY_MUX(0x0, "ether0_phy_rxd_2"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data26"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_0"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(37, "GPIO37",
		THUNDERBAY_MUX(0x0, "ether0_phy_rxd_3"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data27"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_1"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(38, "GPIO38",
		THUNDERBAY_MUX(0x0, "ether0_gmii_mdc"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data28"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_2"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(39, "GPIO39",
		THUNDERBAY_MUX(0x0, "ether0_gmii_mdio"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data29"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_3"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(40, "GPIO40",
		THUNDERBAY_MUX(0x0, "ether0_phy_intr"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data30"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(41, "GPIO41",
		THUNDERBAY_MUX(0x0, "power_interrupt_max_platform_power"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data31"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(42, "GPIO42",
		THUNDERBAY_MUX(0x0, "ether1_phy_txen"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data0"),
		THUNDERBAY_MUX(0x3, "debug_0"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(43, "GPIO43",
		THUNDERBAY_MUX(0x0, "ether1_gmii_clk_tx"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data1"),
		THUNDERBAY_MUX(0x3, "debug_1"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(44, "GPIO44",
		THUNDERBAY_MUX(0x0, "ether1_phy_txd_0"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data2"),
		THUNDERBAY_MUX(0x3, "debug_2"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(45, "GPIO45",
		THUNDERBAY_MUX(0x0, "ether1_phy_txd_1"),
		THUNDERBAY_MUX(0x1, "tpiu_data1"),
		THUNDERBAY_MUX(0x2, "tpiu_data3"),
		THUNDERBAY_MUX(0x3, "debug_3"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(46, "GPIO46",
		THUNDERBAY_MUX(0x0, "ether1_phy_txd_2"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data4"),
		THUNDERBAY_MUX(0x3, "debug_4"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(47, "GPIO47",
		THUNDERBAY_MUX(0x0, "ether1_phy_txd_3"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data5"),
		THUNDERBAY_MUX(0x3, "debug_5"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(48, "GPIO48",
		THUNDERBAY_MUX(0x0, "ether1_phy_rxdv"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data6"),
		THUNDERBAY_MUX(0x3, "debug_6"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(49, "GPIO49",
		THUNDERBAY_MUX(0x0, "ether1_gmii_clk_rx"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data7"),
		THUNDERBAY_MUX(0x3, "debug_7"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(50, "GPIO50",
		THUNDERBAY_MUX(0x0, "ether1_phy_rxd_0"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data8"),
		THUNDERBAY_MUX(0x3, "dig_view_0"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(51, "GPIO51",
		THUNDERBAY_MUX(0x0, "ether1_phy_rxd_1"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data9"),
		THUNDERBAY_MUX(0x3, "dig_view_1"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(52, "GPIO52",
		THUNDERBAY_MUX(0x0, "ether1_phy_rxd_2"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data10"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_0"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(53, "GPIO53",
		THUNDERBAY_MUX(0x0, "ether1_phy_rxd_3"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data11"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_1"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(54, "GPIO54",
		THUNDERBAY_MUX(0x0, "ether1_gmii_mdc"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data12"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_2"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(55, "GPIO55",
		THUNDERBAY_MUX(0x0, "ether1_gmii_mdio"),
		THUNDERBAY_MUX(0x1, "tpiu_data2"),
		THUNDERBAY_MUX(0x2, "tpiu_data13"),
		THUNDERBAY_MUX(0x3, "cpr_io_out_clk_3"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(56, "GPIO56",
		THUNDERBAY_MUX(0x0, "ether1_phy_intr"),
		THUNDERBAY_MUX(0x1, "tpiu_data3"),
		THUNDERBAY_MUX(0x2, "tpiu_data14"),
		THUNDERBAY_MUX(0x3, "power_interrupt_iccmax_vddd"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(57, "GPIO57",
		THUNDERBAY_MUX(0x0, "power_interrupt_iccmax_vpu"),
		THUNDERBAY_MUX(0x1, "tpiu_data3"),
		THUNDERBAY_MUX(0x2, "tpiu_data15"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(58, "GPIO58",
		THUNDERBAY_MUX(0x0, "thermtrip_in"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(59, "GPIO59",
		THUNDERBAY_MUX(0x0, "thermtrip_out"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(60, "GPIO60",
		THUNDERBAY_MUX(0x0, "smbus_scl"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(61, "GPIO61",
		THUNDERBAY_MUX(0x0, "smbus_sda"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "power_interrupt_iccmax_vddd"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(62, "GPIO62",
		THUNDERBAY_MUX(0x0, "platform_reset_in"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(63, "GPIO63",
		THUNDERBAY_MUX(0x0, "platform_reset_out"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(64, "GPIO64",
		THUNDERBAY_MUX(0x0, "platform_shutdown_in"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(65, "GPIO65",
		THUNDERBAY_MUX(0x0, "platform_shutdown_out"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
	THUNDERBAY_PIN_DESC(66, "GPIO66",
		THUNDERBAY_MUX(0x0, "power_interrupt_iccmax_media"),
		THUNDERBAY_MUX(0x1, "EMPTY"),
		THUNDERBAY_MUX(0x2, "EMPTY"),
		THUNDERBAY_MUX(0x3, "EMPTY"),
		THUNDERBAY_MUX(0x4, "CPU_DIRECT_CONTROL")),
};

static const struct thunderbay_pin_soc thunderbay_data = {
	.pins    = thunderbay_pins,
	.npins   = ARRAY_SIZE(thunderbay_pins),
};


/*******************************************************************************/

/* TODO : do remap for complete address range in probe function else do unmap */
static u32 thb_gpio_read_reg (struct gpio_chip *chip, unsigned int pinnr)
{
	struct thunderbay_pinctrl *kpc = gpiochip_get_data(chip);

//	void __iomem *regs = ioremap((kpc->base0 + THB_GPIO_PIN_OFFSET(pinnr)), 4);

//	return (readl(regs));

	return (readl(kpc->base0 + THB_GPIO_PIN_OFFSET(pinnr)));
}

static u32 thb_gpio_write_reg (struct gpio_chip *chip, unsigned int pinnr, u32 value)
{
	struct thunderbay_pinctrl *kpc = gpiochip_get_data(chip);

/*	void __iomem *regs = ioremap((kpc->base0 + THB_GPIO_PIN_OFFSET(pinnr)), 4);

	writel(value, regs);
*/
	writel(value, (kpc->base0 + THB_GPIO_PIN_OFFSET(pinnr)));

	return 0;
}

/*
 * Read GPIO DATA registers:
 * GPIO_DATA_OUT, GPIO_DATA_IN,
 * GPIO_DATA_LOW, GPIO_DATA_HIGH,
 * pad_dir: 1-in; 0-out
 */
static int thb_read_gpio_data(struct gpio_chip *chip, unsigned int offset, unsigned int pad_dir)
{
	int ret_val = -EINVAL;
	int data_offset = 0x2000u;
	u32 data_reg;

	data_offset = (pad_dir > 0) ? (data_offset + 10 + (offset/32)) : (data_offset + (offset/32));

	data_reg = thb_gpio_read_reg(chip, data_offset);

	ret_val = (((data_reg & BIT(offset % 32))>0) ? 1 : 0);

	return (ret_val);
}

/* Write GPIO DATA Registers:
 * GPIO_DATA_OUT, GPIO_DATA_IN,
 * GPIO_DATA_LOW, GPIO_DATA_HIGH,
 */

static int thb_write_gpio_data(struct gpio_chip *chip, unsigned int offset, unsigned int value)
{
	int ret_val = -EINVAL;
	int data_offset = 0x2000u;
	u32 data_reg;

	data_offset += (offset/32);

	data_reg = thb_gpio_read_reg(chip, data_offset);

	data_reg = ((1==value) ? (data_reg|BIT(offset % 32)) : (data_reg & (~(BIT(offset % 32)))));

	ret_val = thb_gpio_write_reg(chip, data_offset, data_reg);

	return (ret_val);
}

/*******************************************************************************/



/*
 * Enable GPIO request for a pin if it is in default GPIO mode (1).
 * Request fails if the pin has been muxed into any other modes (0).
 */
static int thunderbay_request_gpio(struct pinctrl_dev *pctldev,
				struct pinctrl_gpio_range *range,
				unsigned int pin)
{
#if 0
	int ret_val = -EINVAL;

	u32 reg_val = 0;

	struct thunderbay_pinctrl *kpc = pinctrl_dev_get_drvdata(pctldev);

//	void __iomem *regs = ioremap((kpc->base0 + THB_GPIO_PIN_OFFSET(pin)), 4);

//	reg_val = readl(regs);

/* TODO: update below code with required condition check
   By default all pins are in port mode*/
	/* Check pin is GPIO else error */
#if 0
	if (reg_val & THB_GPIO_PORT_SELECT_MASK)
	{
		if (reg == (reg | GPIO_MODE_SELECT_7))
		return 0;
		else
		return -EBUSY;
	}
#endif
#endif
	return 0;
}


/****************************************************************************************************************/

static int thunderbay_pinconf_set_tristate(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale Tri-state */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (config > 0) ? (reg | THB_GPIO_ENAQ_MASK) : (reg & (~(THB_GPIO_ENAQ_MASK)));

	ret_val = thb_gpio_write_reg (chip, pin, reg);

	return (ret_val);
}

static int thunderbay_pinconf_get_tristate(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	*config = ((reg & THB_GPIO_ENAQ_MASK) > 0) ? 1 : 0 ;

	ret_val = 0;

	return (ret_val);
}

static int thunderbay_pinconf_set_pulldown(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale pulldown */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (config > 0) ? (reg | THB_GPIO_PULL_DOWN_MASK) : (reg & (~(THB_GPIO_PULL_DOWN_MASK)));

	ret_val = thb_gpio_write_reg (chip, pin, reg);

	return (ret_val);
}

static int thunderbay_pinconf_get_pulldown(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	*config = ((reg & THB_GPIO_PULL_DOWN_MASK) > 0) ? 1 : 0 ;

	ret_val = 0;

	return (ret_val);
}

static int thunderbay_pinconf_set_pullup(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale pullup */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (config > 0) ? (reg & (~(THB_GPIO_PULL_UP_MASK))) : (reg | THB_GPIO_PULL_UP_MASK);

	ret_val = thb_gpio_write_reg (chip, pin, reg);

	return (ret_val);
}

static int thunderbay_pinconf_get_pullup(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	*config = ((reg & THB_GPIO_PULL_UP_MASK) == 0) ? 1 : 0 ;

	ret_val = 0;

	return (ret_val);
}

static int thunderbay_pinconf_set_opendrain(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale opendrain */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (config > 0) ? (reg & (~(THB_GPIO_PULL_ENABLE_MASK))) : (reg | THB_GPIO_PULL_ENABLE_MASK);

	ret_val = thb_gpio_write_reg (chip, pin, reg);

	return (ret_val);
}

static int thunderbay_pinconf_get_opendrain(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	*config = ((reg & THB_GPIO_PULL_ENABLE_MASK) == 0) ? 1 : 0 ;

	ret_val = 0;

	return (ret_val);
}


static int thunderbay_pinconf_set_pushpull(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale pushpull */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (config > 0) ? (reg | THB_GPIO_PULL_ENABLE_MASK) : (reg & (~(THB_GPIO_PULL_ENABLE_MASK)));

	ret_val = thb_gpio_write_reg (chip, pin, reg);

	return (ret_val);
}

static int thunderbay_pinconf_get_pushpull(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	*config = ((reg & THB_GPIO_PULL_ENABLE_MASK) > 0) ? 1 : 0 ;

	ret_val = 0;

	return (ret_val);
}

static int thunderbay_pinconf_set_drivestrength(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale drivestrength */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	if (config <= 0xF)
	{
		reg = (reg | config);

		ret_val = thb_gpio_write_reg (chip, pin, reg);
	}

	return (ret_val);
}

static int thunderbay_pinconf_get_drivestrength(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (reg & THB_GPIO_DRIVE_STRENGTH_MASK) >> 16;

	*config = (reg > 0) ? reg : 0 ;

	ret_val = 0;

	return (ret_val);
}

static int thunderbay_pinconf_set_schmitt(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale schmitt trigger */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (config > 0) ? (reg | THB_GPIO_SCHMITT_TRIGGER_MASK) : (reg & (~(THB_GPIO_SCHMITT_TRIGGER_MASK)));

	ret_val = thb_gpio_write_reg (chip, pin, reg);

	return (ret_val);
}

static int thunderbay_pinconf_get_schmitt(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	*config = ((reg & THB_GPIO_SCHMITT_TRIGGER_MASK) > 0) ? 1 : 0 ;

	ret_val = 0;

	return (ret_val);
}


static int thunderbay_pinconf_set_slew_rate(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 config)
{
	/* TODO: update with pre-Conditions to be checked
	 * How/when to disbale slew_rate */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	reg = (config > 0) ? (reg | THB_GPIO_SLEW_RATE_MASK) : (reg & (~(THB_GPIO_SLEW_RATE_MASK)));

	ret_val = thb_gpio_write_reg (chip, pin, reg);

	return (ret_val);
}

static int thunderbay_pinconf_get_slew_rate(struct thunderbay_pinctrl *kpc, unsigned int pin, u32 *config)
{
	/* TODO: update with pre-Conditions to be checked */
	int ret_val = -EINVAL;

	struct gpio_chip *chip = &kpc->chip;

	u32 reg = 0;

	reg = thb_gpio_read_reg(chip, pin);

	*config = ((reg & THB_GPIO_SLEW_RATE_MASK) > 0) ? 1 : 0 ;

	ret_val = 0;

	return (ret_val);
}


static int thunderbay_pinconf_get(struct pinctrl_dev *pctldev, unsigned int pin, unsigned long *config)
{
	u32 arg;
	int ret;
	struct thunderbay_pinctrl *kpc = pinctrl_dev_get_drvdata(pctldev);
	enum pin_config_param param = pinconf_to_config_param(*config);

	switch (param)
	{
		case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
			ret=thunderbay_pinconf_get_tristate(kpc, pin, &arg);
			break;

		case PIN_CONFIG_BIAS_PULL_DOWN:
			ret=thunderbay_pinconf_get_pulldown(kpc, pin, &arg);
			break;

		case PIN_CONFIG_BIAS_PULL_UP:
			ret=thunderbay_pinconf_get_pullup(kpc, pin, &arg);
			break;

		case PIN_CONFIG_DRIVE_OPEN_DRAIN:
			ret=thunderbay_pinconf_get_opendrain(kpc, pin, &arg);
			break;

		case PIN_CONFIG_DRIVE_PUSH_PULL:
			ret=thunderbay_pinconf_get_pushpull(kpc, pin, &arg);
			break;

		case PIN_CONFIG_DRIVE_STRENGTH:
			ret=thunderbay_pinconf_get_drivestrength(kpc, pin, &arg);
			break;

		case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
			ret = thunderbay_pinconf_get_schmitt(kpc, pin, &arg);
			break;

		case PIN_CONFIG_SLEW_RATE:
			ret = thunderbay_pinconf_get_slew_rate(kpc, pin, &arg);
			break;

		default:
			return -ENOTSUPP;

	}

	*config = pinconf_to_config_packed(param, arg);

	return 0;
}


static int thunderbay_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin, unsigned long *configs, unsigned int num_configs)
{
	u32 arg;
	enum pin_config_param param;
	unsigned int pinconf;
	int ret = 0;
	struct thunderbay_pinctrl *kpc = pinctrl_dev_get_drvdata(pctldev);

	for (pinconf = 0; pinconf < num_configs; pinconf++)
	{
		param = pinconf_to_config_param(configs[pinconf]);
		arg = pinconf_to_config_argument(configs[pinconf]);

		switch (param)
		{
			case PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
				ret = thunderbay_pinconf_set_tristate(kpc, pin, arg);
				break;

			case PIN_CONFIG_BIAS_PULL_DOWN:
				ret = thunderbay_pinconf_set_pulldown(kpc, pin, arg);
				break;

			case PIN_CONFIG_BIAS_PULL_UP:
				ret = thunderbay_pinconf_set_pullup(kpc, pin, arg);
				break;

			case PIN_CONFIG_DRIVE_OPEN_DRAIN:
				ret = thunderbay_pinconf_set_opendrain(kpc, pin, arg);
				break;

			case PIN_CONFIG_DRIVE_PUSH_PULL:
				ret = thunderbay_pinconf_set_pushpull(kpc, pin, arg);
				break;

			case PIN_CONFIG_DRIVE_STRENGTH:
				ret = thunderbay_pinconf_set_drivestrength(kpc, pin, arg);
				break;

			case PIN_CONFIG_INPUT_SCHMITT_ENABLE:
				ret = thunderbay_pinconf_set_schmitt(kpc, pin, arg);
				break;

			case PIN_CONFIG_SLEW_RATE:
				ret = thunderbay_pinconf_set_slew_rate(kpc, pin, arg);
				break;

			default:
				return -ENOTSUPP;
		}
	}

	return ret;
}
/***********************************************************************************************************/

static const unsigned i2c_0_pins[] = { 0, 1 };
static const unsigned i2c_1_pins[] = { 2, 3 };
static const unsigned i2c_2_pins[] = { 4, 5 };
static const unsigned i2c_3_pins[] = { 6, 7 };
static const unsigned i2c_4_pins[] = { 8, 9 };

static const unsigned uart_0_pins[] = { 10, 11, 12, 13 };
static const unsigned uart_1_pins[] = { 14, 15, 16, 17 };

static const unsigned spi_0_pins[] = { 18, 19, 20, 21 };
static const unsigned spi_1_pins[] = { 22, 23, 24, 25 };

static const unsigned ethernet_0_pins[] = { 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40 };
static const unsigned ethernet_1_pins[] = { 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56 };

static const unsigned power_interrupt_0_pins[] = { 41, 57, 66 };

static const unsigned thermtrip_0_pins[] = { 58, 59 };

static const unsigned smbus_0_pins[] = { 60, 61 };

static const unsigned reset_0_pins[] = { 62, 63 };

static const unsigned shutdown_0_pins[] = { 64, 65 };


#define DEFINE_THB_PINCTRL_GRP(gname) \
	{\
		.name = #gname "_grp", \
		.pins = gname ## _pins, \
		.npins = ARRAY_SIZE(gname ## _pins), \
	}


static const struct thb_pinctrl_group thb_pinctrl_groups[] = {
	DEFINE_THB_PINCTRL_GRP(i2c_0),
	DEFINE_THB_PINCTRL_GRP(i2c_1),
	DEFINE_THB_PINCTRL_GRP(i2c_2),
	DEFINE_THB_PINCTRL_GRP(i2c_3),
	DEFINE_THB_PINCTRL_GRP(i2c_4),
	DEFINE_THB_PINCTRL_GRP(uart_0),
	DEFINE_THB_PINCTRL_GRP(uart_1),
	DEFINE_THB_PINCTRL_GRP(spi_0),
	DEFINE_THB_PINCTRL_GRP(spi_1),
	DEFINE_THB_PINCTRL_GRP(ethernet_0),
	DEFINE_THB_PINCTRL_GRP(ethernet_1),
	DEFINE_THB_PINCTRL_GRP(power_interrupt_0),
	DEFINE_THB_PINCTRL_GRP(thermtrip_0),
	DEFINE_THB_PINCTRL_GRP(smbus_0),
	DEFINE_THB_PINCTRL_GRP(reset_0),
	DEFINE_THB_PINCTRL_GRP(shutdown_0),
};

/* function groups */
static const char * const i2c_groups[] = { "i2c_0_grp", "i2c_1_grp", "i2c_2_grp", "i2c_3_grp", "i2c_4_grp" };
static const char * const uart_groups[] = { "uart_0_grp", "uart_1_grp" };
static const char * const spi_groups[] = { "spi_0_grp", "spi_1_grp" };
static const char * const ethernet_groups[] = { "ethernet_0_grp", "ethernet_1_grp" };
static const char * const power_interrupt_groups[] = { "power_interrupt_0_grp" };
static const char * const thermtrip_groups[] = { "thermtrip_0_grp" };
static const char * const smbus_groups[] = { "smbus_0_grp" };
static const char * const shutdown_groups[] = { "shutdown_0_grp" };



static int thb_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct thunderbay_pinctrl *tpc = pinctrl_dev_get_drvdata(pctldev);

	return (tpc->ngroups);
}

static const char *thb_pinctrl_get_group_name(struct pinctrl_dev *pctldev, unsigned selector)
{
	struct thunderbay_pinctrl *tpc = pinctrl_dev_get_drvdata(pctldev);

        return (tpc->groups[selector].name);
}

static int thb_pinctrl_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector, const unsigned ** pins, unsigned * num_pins)
{
	struct thunderbay_pinctrl *tpc = pinctrl_dev_get_drvdata(pctldev);

        *pins = tpc->groups[selector].pins;
        *num_pins = tpc->groups[selector].npins;
        return 0;
}

/* pinmux */

#define DEFINE_THB_PINMUX_FUNCTION(fname) \
{\
	.name = #fname, \
	.groups = fname##_groups, \
	.ngroups = ARRAY_SIZE(fname##_groups), \
}

static const struct thb_pinmux_function thb_pinctrl_pmx_funcs[] = {
	DEFINE_THB_PINMUX_FUNCTION(i2c),
	DEFINE_THB_PINMUX_FUNCTION(uart),
	DEFINE_THB_PINMUX_FUNCTION(spi),
	DEFINE_THB_PINMUX_FUNCTION(ethernet),
	DEFINE_THB_PINMUX_FUNCTION(power_interrupt),
	DEFINE_THB_PINMUX_FUNCTION(thermtrip),
	DEFINE_THB_PINMUX_FUNCTION(smbus),
	DEFINE_THB_PINMUX_FUNCTION(shutdown),
};

static int thb_pinctrl_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct thunderbay_pinctrl *tpc = pinctrl_dev_get_drvdata(pctldev);

        return (tpc->nfuncs);
}

static const char *thb_pinctrl_get_fname(struct pinctrl_dev *pctldev, unsigned selector)
{
	struct thunderbay_pinctrl *tpc = pinctrl_dev_get_drvdata(pctldev);

        return (tpc->funcs[selector].name);
}

static int thb_pinctrl_get_groups(struct pinctrl_dev *pctldev, unsigned selector,
                        const char * const **groups,
                        unsigned * const num_groups)
{
	struct thunderbay_pinctrl *tpc = pinctrl_dev_get_drvdata(pctldev);

        *groups = tpc->funcs[selector].groups;
        *num_groups = tpc->funcs[selector].ngroups;
        return 0;
}

static int thb_pinctrl_set_mux(struct pinctrl_dev *pctldev, unsigned selector, unsigned group)
{
	/* Check and update precondition check required before setting as mux
	 this as to be updated as per the THB register configuration */

	u32 reg=0;
	int i, modex=0, ret_val = -EINVAL;

	struct thunderbay_pinctrl *tpc = pinctrl_dev_get_drvdata(pctldev);
	struct gpio_chip *chip = &tpc->chip;
	const struct thb_pinctrl_group *pgrp = &tpc->groups[group];
//	const struct thb_pinmux_function *func = &tpc->funcs[selector];

	for(i=0; i < pgrp->npins; i++)
	{
		unsigned int pin = pgrp->pins[i];

		reg = thb_gpio_read_reg(chip,pin);

		reg |= thb_modex_pinval[pin][modex];

		ret_val = thb_gpio_write_reg (chip, pin, reg);
	}

	return (ret_val);
}


/******************************************************************************************************/

struct pinctrl_ops thunderbay_pctlops = {
	.get_groups_count = thb_pinctrl_get_groups_count,
	.get_group_name   = thb_pinctrl_get_group_name,
	.get_group_pins   = thb_pinctrl_get_group_pins,
	.dt_node_to_map   = pinconf_generic_dt_node_to_map_all,
	.dt_free_map      = pinconf_generic_dt_free_map,
};

struct pinmux_ops thunderbay_pmxops = {
	.get_functions_count = thb_pinctrl_get_functions_count,
	.get_function_name   = thb_pinctrl_get_fname,
	.get_function_groups = thb_pinctrl_get_groups,
	.set_mux             = thb_pinctrl_set_mux,
	.gpio_request_enable = thunderbay_request_gpio,
};

static const struct pinconf_ops thunderbay_confops = {
	.is_generic	= true,
	.pin_config_get	= thunderbay_pinconf_get,
	.pin_config_set	= thunderbay_pinconf_set,
};

static struct pinctrl_desc thunderbay_pinctrl_desc = {
	.name    = "thunderbay-pinmux",
	.pctlops = &thunderbay_pctlops,
	.pmxops  = &thunderbay_pmxops,
	.confops = &thunderbay_confops,
	.owner   = THIS_MODULE,
};

/******************************************************************************************************/

// returns direction for signal “offset”, 0=out, 1=in, (same as GPIOF_DIR_XXX), or negative error
static int thunderbay_gpio_get_direction (struct gpio_chip *chip, unsigned int offset)
{
	int ret_val = -EINVAL;

	u32 reg = thb_gpio_read_reg(chip, offset);

	/* Return direction only if configured as GPIO else negative error */
	if (reg & THB_GPIO_PORT_SELECT_MASK)
	{
		ret_val = ((reg & THB_GPIO_PAD_DIRECTION_MASK) > 0) ? 1 : 0;
	}

	return (ret_val);
}

// configures signal “offset” as input, or returns error
static int thunderbay_gpio_set_direction_input (struct gpio_chip *chip, unsigned int offset)
{
	int ret_val = -EINVAL;

	u32 reg = thb_gpio_read_reg(chip, offset);

	/* set pin as input only if it is GPIO else error */
	if (reg & THB_GPIO_PORT_SELECT_MASK)
	{
		reg |= THB_GPIO_PAD_DIRECTION_MASK ;

		thb_gpio_write_reg(chip, offset, reg);

		ret_val = 0;
	}

	return (ret_val);
}

// assigns output value for signal “offset”
static void thunderbay_gpio_set_value (struct gpio_chip *chip, unsigned int offset, int value)
{
	int ret_val = -EINVAL;

	u32 reg = thb_gpio_read_reg(chip, offset);

	/* update pin value only if it is GPIO-output else error */
	if ((reg & THB_GPIO_PORT_SELECT_MASK) && ((reg & THB_GPIO_PAD_DIRECTION_MASK)==0))
	{
		ret_val=thb_write_gpio_data(chip, offset, value);

	}
}


// configures signal “offset” as output, or returns error
static int thunderbay_gpio_set_direction_output (struct gpio_chip *chip, unsigned int offset, int value)
{
	int ret_val = -EINVAL;

	u32 reg = thb_gpio_read_reg(chip, offset);

	/* set pin as output only if it is GPIO else error */
	if (reg & THB_GPIO_PORT_SELECT_MASK)
	{
		reg &= (~(THB_GPIO_PAD_DIRECTION_MASK)) ;

		thb_gpio_write_reg(chip, offset, reg);

		thunderbay_gpio_set_value (chip, offset, value);

		ret_val = 0;
	}

	return (ret_val);
}



// returns value for signal “offset”, 0=low, 1=high, or negative error
static int thunderbay_gpio_get_value (struct gpio_chip *chip, unsigned int offset)
{
	int ret_val = -EINVAL;
	int gpio_dir = 0;

	u32 reg = thb_gpio_read_reg(chip, offset);

	/* Read pin value only if it is GPIO else error */
	if (reg & THB_GPIO_PORT_SELECT_MASK)
	{
		gpio_dir = ((reg & THB_GPIO_PAD_DIRECTION_MASK) > 0) ? 1 : 0;

		ret_val = thb_read_gpio_data (chip, offset, gpio_dir);
	}

	return (ret_val);
}

/******************************************************************************************
 * thunderbay_gpio_probe - Initialization method for a thb_gpio device
 * @pdev:	platform device instance
 *
 * This function allocates memory resources for the gpio device and register the GPIO chip
 * It will also set up interrupts for the gpio pins -> Yet to be implemnetd
 * Note: Interrupts are disabled/not handled temporarily.
 *
 * Return: 0 on success, negative error otherwise.
 ******************************************************************************************/
static int thunderbay_gpiochip_probe(struct thunderbay_pinctrl *kpc)
{
	struct gpio_chip *chip = &kpc->chip;
	int ret, i,temp;

	chip->label		= dev_name(kpc->dev);
	chip->parent		= kpc->dev;
	chip->request		= gpiochip_generic_request;
	chip->free		= gpiochip_generic_free;
	chip->get_direction	= thunderbay_gpio_get_direction;
	chip->direction_input	= thunderbay_gpio_set_direction_input;
	chip->direction_output  = thunderbay_gpio_set_direction_output;
	chip->get		= thunderbay_gpio_get_value;
	chip->set               = thunderbay_gpio_set_value;
/* identifies the first GPIO number handled by this chip; or, if negative during registration, requests dynamic ID allocation.
 DEPRECATION: providing anything non-negative and nailing the base offset of GPIO chips is deprecated.
 Please pass -1 as base to let gpiolib select the chip base in all possible cases.
 We want to get rid of the static GPIO number space in the long run. */
	chip->base		= -1;
/* the number of GPIOs handled by this controller; the last GPIO handled is (base + ngpio - 1). */
	chip->ngpio		= kpc->soc->npins;

	ret = gpiochip_add_data(chip, kpc);
	if (ret) {
		dev_err(kpc->dev, "Failed to add gpiochip\n");
		return ret;
	}

	ret = gpiochip_add_pin_range(chip, dev_name(kpc->dev), 0, 0,
					chip->ngpio);
	if (ret) {
		dev_err(kpc->dev, "Failed to add gpiochip pin range\n");
		return ret;
	}

	for(i=0;i<67;i++)
	{
		temp = thb_gpio_read_reg(chip, i);
		temp = (temp | THB_GPIO_PORT_SELECT_MASK) ;
		thb_gpio_write_reg(chip, i, temp);
	}

	return 0;
}

/*******************************************************************************************************************/

static const struct of_device_id thunderbay_pinctrl_match[] = {
{	.compatible = "intel,thunderbay-pinctrl",
	.data = &thunderbay_data},
	{},
};



/* THB-PinCtrl: Probe/Init function */
int thunderbay_pinctrl_probe (struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct thunderbay_pinctrl *kpc;
	struct resource *iomem;
	const struct of_device_id *of_id;
	int ret;

	of_id = of_match_node(thunderbay_pinctrl_match, pdev->dev.of_node);
	if (!of_id)
		return -ENODEV;

	kpc = devm_kzalloc(dev, sizeof(*kpc), GFP_KERNEL);
	if (!kpc)
		return -ENOMEM;

	kpc->dev = dev;
	kpc->soc = of_id->data;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem)
		return -ENXIO;

	kpc->base0 =  devm_ioremap_resource(dev, iomem);
	if (IS_ERR(kpc->base0))
		return PTR_ERR(kpc->base0);

	thunderbay_pinctrl_desc.pins = kpc->soc->pins;
	thunderbay_pinctrl_desc.npins = kpc->soc->npins;

	/* Register pinctrl */
	kpc->pctrl = devm_pinctrl_register(dev, &thunderbay_pinctrl_desc, kpc);
	if (IS_ERR(kpc->pctrl))
		return PTR_ERR(kpc->pctrl);
#if 0
	/* Setup groups and functions */
	ret = thunderbay_build_groups(kpc);
	if (ret)
		return ret;

	ret = thunderbay_build_functions(kpc);
	if (ret)
		return ret;
#endif

	/* Setup GPIO */
	ret = thunderbay_gpiochip_probe(kpc);
	if (ret < 0)
		return ret;

	platform_set_drvdata(pdev, kpc);

	return 0;
}

/**********************************************************************
 * Function: thunderbay_pinctrl_remove
 * Arg-0: pdev - Platform device instance
 * Arg-1: NA
 * Return: 0 Always
 * Description: Thunderbay pinctrl driver removal/exit.
 **********************************************************************/
static int thunderbay_pinctrl_remove(struct platform_device *pdev)
{
	/* TODO: Update thunderbay_pinctrl_remove function to clear the memory */
	return 0;
}


static struct platform_driver thunderbay_pinctrl_driver = {
	.driver = {
		.name = "thunderbay-pinctrl",
		.of_match_table = thunderbay_pinctrl_match,
	},
	.probe = thunderbay_pinctrl_probe,
	.remove = thunderbay_pinctrl_remove,
};

builtin_platform_driver(thunderbay_pinctrl_driver);
