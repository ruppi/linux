/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 * Author: Tarek Dakhran <t.dakhran@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Common Clock Framework support for Exynos5410 SoC.
*/

#include <dt-bindings/clock/exynos5410.h>

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "clk.h"

#define APLL_LOCK		0x0
#define APLL_CON0		0x100
#define CPLL_LOCK		0x10020
#define CPLL_CON0		0x10120
#define MPLL_LOCK		0x4000
#define MPLL_CON0		0x4100
#define BPLL_LOCK		0x20010
#define BPLL_CON0		0x20110
#define KPLL_LOCK		0x28000
#define KPLL_CON0		0x28100
#define VPLL_LOCK		0x10040
#define VPLL_CON0		0x10140
#define DPLL_LOCK		0x10030
#define DPLL_CON0		0x10128

#define SRC_CPU			0x200
#define DIV_CPU0		0x500
#define SRC_CPERI1		0x4204
#define DIV_TOP0		0x10510
#define DIV_TOP1		0x10514
#define DIV_FSYS1		0x1054c
#define DIV_FSYS2		0x10550
#define DIV_PERIC0		0x10558
#define DIV_DISP1_0		0x1052c
#define SRC_TOP0		0x10210
#define SRC_TOP1		0x10214
#define SRC_TOP2		0x10218
#define SRC_TOP3		0x1021c
#define SRC_FSYS		0x10244
#define SRC_DISP1_0		0x1022c
#define SRC_PERIC0		0x10250
#define SRC_MASK_DISP1_0	0x1032c
#define SRC_MASK_FSYS		0x10340
#define SRC_MASK_PERIC0		0x10350
#define GATE_BUS_FSYS0		0x10740
#define GATE_TOP_SCLK_DISP1	0x10828
#define GATE_IP_FSYS		0x10944
#define GATE_IP_PERIC		0x10950
#define GATE_IP_PERIS		0x10960
#define GATE_IP_DISP1		0x10928
#define SRC_CDREX		0x20200
#define SRC_KFC			0x28200
#define DIV_KFC0		0x28500

/* list of PLLs */
enum exynos5410_plls {
	apll, cpll, mpll,
	bpll, kpll, vpll,
	dpll,
	nr_plls			/* number of PLLs */
};

/*
 * list of controller registers to be saved and restored during a
 * suspend/resume cycle.
 */
static unsigned long exynos5410_clk_regs[] __initdata = {
	SRC_CPU,
	DIV_CPU0,
	SRC_CPERI1,
	DIV_TOP0,
	DIV_TOP1,
	DIV_FSYS1,
	DIV_FSYS2,
	DIV_PERIC0,
	SRC_TOP0,
	SRC_TOP1,
	SRC_TOP2,
	SRC_FSYS,
	SRC_PERIC0,
	SRC_MASK_FSYS,
	SRC_MASK_PERIC0,
	GATE_BUS_FSYS0,
	GATE_IP_FSYS,
	GATE_IP_PERIC,
	GATE_IP_PERIS,
	SRC_CDREX,
	SRC_KFC,
	DIV_KFC0,
};

/* list of all parent clocks */
PNAME(apll_p)		= { "fin_pll", "fout_apll", };
PNAME(bpll_p)		= { "fin_pll", "fout_bpll", };
PNAME(cpll_p)		= { "fin_pll", "fout_cpll" };
PNAME(mpll_p)		= { "fin_pll", "fout_mpll", };
PNAME(kpll_p)		= { "fin_pll", "fout_kpll", };
PNAME(dpll_p)		= { "fin_pll", "fout_dpll", };

PNAME(mout_cpu_p)	= { "mout_apll", "sclk_mpll", };
PNAME(mout_kfc_p)	= { "mout_kpll", "sclk_mpll", };
PNAME(mout_vpllsrc_p)	= { "fin_pll", "sclk_hdmi27m" };
PNAME(mout_vpll_p)	= { "mout_vpllsrc", "fout_vpll" };
PNAME(mout_hdmi_p)	= { "div_hdmi_pixel", "sclk_hdmiphy" };

PNAME(mpll_user_p)	= { "fin_pll", "sclk_mpll", };
PNAME(bpll_user_p)	= { "fin_pll", "sclk_bpll", };
PNAME(mpll_bpll_p)	= { "sclk_mpll_muxed", "sclk_bpll_muxed", };
PNAME(aclk200_disp1_p) = { "fin_pll", "aclk200", };

PNAME(group2_p)		= { "fin_pll", "fin_pll", "none", "none",
			"none", "none", "sclk_mpll_bpll",
			 "none", "none", "sclk_cpll" };

/* fixed rate clocks generated inside the soc */
static struct samsung_fixed_rate_clock exynos5410_fixed_rate_clks[] __initdata = {
	FRATE(CLK_SCLK_HDMIPHY, "sclk_hdmiphy", NULL, CLK_IS_ROOT, 24000000),
	FRATE(0, "sclk_hdmi27m", NULL, CLK_IS_ROOT, 27000000),
	FRATE(0, "sclk_dptxphy", NULL, CLK_IS_ROOT, 24000000),
	FRATE(0, "sclk_uhostphy", NULL, CLK_IS_ROOT, 48000000),
};

static struct samsung_mux_clock exynos5410_pll_pmux_clks[] __initdata = {
	MUX(0, "mout_vpllsrc", mout_vpllsrc_p, SRC_TOP2, 0, 1),
};

static struct samsung_mux_clock exynos5410_mux_clks[] __initdata = {
	MUX(0, "mout_apll", apll_p, SRC_CPU, 0, 1),
	MUX(0, "mout_cpu", mout_cpu_p, SRC_CPU, 16, 1),

	MUX(0, "mout_kpll", kpll_p, SRC_KFC, 0, 1),
	MUX(0, "mout_kfc", mout_kfc_p, SRC_KFC, 16, 1),

	MUX(0, "sclk_mpll", mpll_p, SRC_CPERI1, 8, 1),
	MUX(0, "sclk_mpll_muxed", mpll_user_p, SRC_TOP2, 20, 1),

	MUX(0, "sclk_bpll", bpll_p, SRC_CDREX, 0, 1),
	MUX(0, "sclk_bpll_muxed", bpll_user_p, SRC_TOP2, 24, 1),
	MUX(0, "sclk_vpll", mout_vpll_p, SRC_TOP2, 16, 1),

	MUX(0, "sclk_cpll", cpll_p, SRC_TOP2, 8, 1),

	MUX(0, "sclk_mpll_bpll", mpll_bpll_p, SRC_TOP1, 20, 1),

	MUX(0, "mout_mmc0", group2_p, SRC_FSYS, 0, 4),
	MUX(0, "mout_mmc1", group2_p, SRC_FSYS, 4, 4),
	MUX(0, "mout_mmc2", group2_p, SRC_FSYS, 8, 4),

	MUX(0, "mout_uart0", group2_p, SRC_PERIC0, 0, 4),
	MUX(0, "mout_uart1", group2_p, SRC_PERIC0, 4, 4),
	MUX(0, "mout_uart2", group2_p, SRC_PERIC0, 8, 4),

	MUX(0, "mout_aclk200", mpll_bpll_p, SRC_TOP0, 12, 1),
	MUX(0, "mout_aclk400", mpll_bpll_p, SRC_TOP0, 20, 1),
	MUX_A(0, "mout_aclk200_disp1", aclk200_disp1_p,
			SRC_TOP3, 4, 1, "aclk200_disp1"),
	MUX(CLK_MOUT_HDMI, "mout_hdmi", mout_hdmi_p, SRC_DISP1_0, 20, 1),
};

static struct samsung_div_clock exynos5410_div_clks[] __initdata = {
	DIV(0, "div_arm", "mout_cpu", DIV_CPU0, 0, 3),
	DIV(0, "div_arm2", "div_arm", DIV_CPU0, 28, 3),

	DIV(0, "div_acp", "div_arm2", DIV_CPU0, 8, 3),
	DIV(0, "div_cpud", "div_arm2", DIV_CPU0, 4, 3),
	DIV(0, "div_atb", "div_arm2", DIV_CPU0, 16, 3),
	DIV(0, "pclk_dbg", "div_arm2", DIV_CPU0, 20, 3),

	DIV(0, "div_kfc", "mout_kfc", DIV_KFC0, 0, 3),
	DIV(0, "div_aclk", "div_kfc", DIV_KFC0, 4, 3),
	DIV(0, "div_pclk", "div_kfc", DIV_KFC0, 20, 3),

	DIV(0, "aclk66_pre", "sclk_mpll_muxed", DIV_TOP1, 24, 3),
	DIV(0, "aclk66", "aclk66_pre", DIV_TOP0, 0, 3),

	DIV(0, "div_mmc0", "mout_mmc0", DIV_FSYS1, 0, 4),
	DIV(0, "div_mmc1", "mout_mmc1", DIV_FSYS1, 16, 4),
	DIV(0, "div_mmc2", "mout_mmc2", DIV_FSYS2, 0, 4),

	DIV_F(0, "div_mmc_pre0", "div_mmc0",
			DIV_FSYS1, 8, 8, CLK_SET_RATE_PARENT, 0),
	DIV_F(0, "div_mmc_pre1", "div_mmc1",
			DIV_FSYS1, 24, 8, CLK_SET_RATE_PARENT, 0),
	DIV_F(0, "div_mmc_pre2", "div_mmc2",
			DIV_FSYS2, 8, 8, CLK_SET_RATE_PARENT, 0),

	DIV(0, "div_uart0", "mout_uart0", DIV_PERIC0, 0, 4),
	DIV(0, "div_uart1", "mout_uart1", DIV_PERIC0, 4, 4),
	DIV(0, "div_uart2", "mout_uart2", DIV_PERIC0, 8, 4),
	DIV(0, "div_uart3", "mout_uart3", DIV_PERIC0, 12, 4),

	DIV(0, "aclk200", "mout_aclk200", DIV_TOP0, 12, 3),
	DIV(0, "aclk400", "mout_aclk400", DIV_TOP0, 24, 3),
	DIV(0, "div_hdmi_pixel", "sclk_vpll", DIV_DISP1_0, 28, 4),
};

static struct samsung_gate_clock exynos5410_gate_clks[] __initdata = {
	GATE(CLK_MCT, "mct", "aclk66", GATE_IP_PERIS, 18, 0, 0),

	GATE(CLK_SCLK_MMC0, "sclk_mmc0", "div_mmc_pre0",
			SRC_MASK_FSYS, 0, CLK_SET_RATE_PARENT, 0),
	GATE(CLK_SCLK_MMC1, "sclk_mmc1", "div_mmc_pre1",
			SRC_MASK_FSYS, 4, CLK_SET_RATE_PARENT, 0),
	GATE(CLK_SCLK_MMC2, "sclk_mmc2", "div_mmc_pre2",
			SRC_MASK_FSYS, 8, CLK_SET_RATE_PARENT, 0),
/*	GATE(CLK_SCLK_HDMI, "sclk_hdmi", "mout_hdmi",
			SRC_MASK_DISP1_0, 20, 0, 0), */
	GATE(CLK_SCLK_HDMI, "sclk_hdmi", "mout_hdmi",
		GATE_TOP_SCLK_DISP1, 9, CLK_SET_RATE_PARENT, 0),
	GATE(CLK_SCLK_PIXEL, "sclk_pixel", "div_hdmi_pixel",
		GATE_TOP_SCLK_DISP1, 10, CLK_SET_RATE_PARENT, 0),
	
	GATE(CLK_MMC0, "sdmmc0", "aclk200", GATE_BUS_FSYS0, 12, 0, 0),
	GATE(CLK_MMC1, "sdmmc1", "aclk200", GATE_BUS_FSYS0, 13, 0, 0),
	GATE(CLK_MMC2, "sdmmc2", "aclk200", GATE_BUS_FSYS0, 14, 0, 0),

	GATE(CLK_UART0, "uart0", "aclk66", GATE_IP_PERIC, 0, 0, 0),
	GATE(CLK_UART1, "uart1", "aclk66", GATE_IP_PERIC, 1, 0, 0),
	GATE(CLK_UART2, "uart2", "aclk66", GATE_IP_PERIC, 2, 0, 0),

	GATE(CLK_I2C0, "i2c0", "aclk66", GATE_IP_PERIC, 6, 0, 0),
	GATE(CLK_I2C1, "i2c1", "aclk66", GATE_IP_PERIC, 7, 0, 0),
	GATE(CLK_I2C2, "i2c2", "aclk66", GATE_IP_PERIC, 8, 0, 0),
	GATE(CLK_I2C3, "i2c3", "aclk66", GATE_IP_PERIC, 9, 0, 0),
	GATE(CLK_I2C4, "i2c4", "aclk66", GATE_IP_PERIC, 10, 0, 0),
	GATE(CLK_I2C5, "i2c5", "aclk66", GATE_IP_PERIC, 11, 0, 0),
	GATE(CLK_I2C6, "i2c6", "aclk66", GATE_IP_PERIC, 12, 0, 0),
	GATE(CLK_I2C7, "i2c7", "aclk66", GATE_IP_PERIC, 13, 0, 0),
	GATE(CLK_I2C_HDMI, "i2c_hdmi", "aclk66", GATE_IP_PERIC, 14, 0, 0),

	GATE(CLK_SCLK_UART0, "sclk_uart0", "div_uart0",
			SRC_MASK_PERIC0, 0, CLK_SET_RATE_PARENT, 0),
	GATE(CLK_SCLK_UART1, "sclk_uart1", "div_uart1",
			SRC_MASK_PERIC0, 4, CLK_SET_RATE_PARENT, 0),
	GATE(CLK_SCLK_UART2, "sclk_uart2", "div_uart2",
			SRC_MASK_PERIC0, 8, CLK_SET_RATE_PARENT, 0),
	GATE(CLK_FIMD1, "fimd1", "aclk200", GATE_IP_DISP1, 0, 0, 0),
	GATE(CLK_MIE1, "mie1", "aclk200", GATE_IP_DISP1, 1, 0, 0),
	GATE(CLK_DSIM0, "dsim0", "aclk200", GATE_IP_DISP1, 3, 0, 0),
	GATE(CLK_DP, "dp", "aclk200", GATE_IP_DISP1, 4, 0, 0),
	GATE(CLK_MIXER, "mixer", "mout_aclk200_disp1", GATE_IP_DISP1, 5, 0, 0),
	GATE(CLK_HDMI, "hdmi", "mout_aclk200_disp1", GATE_IP_DISP1, 6, 0, 0),
};

static struct samsung_pll_clock exynos5410_plls[nr_plls] __initdata = {
	[apll] = PLL(pll_35xx, CLK_FOUT_APLL, "fout_apll", "fin_pll", APLL_LOCK,
		APLL_CON0, NULL),
	[cpll] = PLL(pll_35xx, CLK_FOUT_CPLL, "fout_cpll", "fin_pll", CPLL_LOCK,
		CPLL_CON0, NULL),
	[mpll] = PLL(pll_35xx, CLK_FOUT_MPLL, "fout_mpll", "fin_pll", MPLL_LOCK,
		MPLL_CON0, NULL),
	[bpll] = PLL(pll_35xx, CLK_FOUT_BPLL, "fout_bpll", "fin_pll", BPLL_LOCK,
		BPLL_CON0, NULL),
	[kpll] = PLL(pll_35xx, CLK_FOUT_KPLL, "fout_kpll", "fin_pll", KPLL_LOCK,
		KPLL_CON0, NULL),
	[vpll] = PLL(pll_35xx, CLK_FOUT_VPLL, "fout_vpll", "mout_vpllsrc",
		VPLL_LOCK, VPLL_CON0,  NULL),
	[dpll] = PLL(pll_35xx, CLK_FOUT_DPLL, "fout_dpll", "fin_pll", DPLL_LOCK,
		DPLL_CON0, NULL),
};

/* register exynos5410 clocks */
static void __init exynos5410_clk_init(struct device_node *np)
{
	void __iomem *reg_base;

	reg_base = of_iomap(np, 0);
	if (!reg_base)
		panic("%s: failed to map registers\n", __func__);

	samsung_clk_init(np, reg_base, CLK_NR_CLKS,
			exynos5410_clk_regs, ARRAY_SIZE(exynos5410_clk_regs),
			NULL, 0);
	samsung_clk_register_mux(exynos5410_pll_pmux_clks,
				ARRAY_SIZE(exynos5410_pll_pmux_clks));
	samsung_clk_register_pll(exynos5410_plls, ARRAY_SIZE(exynos5410_plls),
					reg_base);

	samsung_clk_register_fixed_rate(exynos5410_fixed_rate_clks,
			ARRAY_SIZE(exynos5410_fixed_rate_clks));
	samsung_clk_register_mux(exynos5410_mux_clks,
			ARRAY_SIZE(exynos5410_mux_clks));
	samsung_clk_register_div(exynos5410_div_clks,
			ARRAY_SIZE(exynos5410_div_clks));
	samsung_clk_register_gate(exynos5410_gate_clks,
			ARRAY_SIZE(exynos5410_gate_clks));

	pr_debug("Exynos5410: clock setup completed.\n");
}
CLK_OF_DECLARE(exynos5410_clk, "samsung,exynos5410-clock", exynos5410_clk_init);
