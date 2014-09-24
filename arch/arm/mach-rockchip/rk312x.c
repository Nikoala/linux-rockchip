/*
 * Device Tree support for Rockchip RK3288
 *
 * Copyright (C) 2014 ROCKCHIP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#include <linux/cpuidle.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/rockchip/common.h>
#include <linux/rockchip/cpu.h>
#include <linux/rockchip/cru.h>
#include <linux/rockchip/dvfs.h>
#include <linux/rockchip/grf.h>
#include <linux/rockchip/iomap.h>
#include <linux/rockchip/pmu.h>
#include <asm/cpuidle.h>
#include <asm/cputype.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include "cpu_axi.h"
#include "loader.h"
#define CPU 312x
#include "sram.h"
#include "pm.h"

#define RK312X_DEVICE(name) \
	{ \
		.virtual	= (unsigned long) RK_##name##_VIRT, \
		.pfn		= __phys_to_pfn(RK312X_##name##_PHYS), \
		.length		= RK312X_##name##_SIZE, \
		.type		= MT_DEVICE, \
	}

static const char * const rk3126_dt_compat[] __initconst = {
	"rockchip,rk3126",
	NULL,
};

static const char * const rk3128_dt_compat[] __initconst = {
	"rockchip,rk3128",
	NULL,
};

#define RK312X_IMEM_VIRT (RK_BOOTRAM_VIRT + SZ_32K)
#define RK312X_TIMER5_VIRT (RK_TIMER_VIRT + 0xa0)

static struct map_desc rk312x_io_desc[] __initdata = {
	RK312X_DEVICE(CRU),
	RK312X_DEVICE(GRF),
	RK312X_DEVICE(ROM),
	RK312X_DEVICE(PMU),
	RK312X_DEVICE(EFUSE),
	RK312X_DEVICE(TIMER),
	RK312X_DEVICE(CPU_AXI_BUS),
	RK_DEVICE(RK_DEBUG_UART_VIRT, RK312X_UART2_PHYS, RK312X_UART_SIZE),
	RK_DEVICE(RK_DDR_VIRT, RK312X_DDR_PCTL_PHYS, RK312X_DDR_PCTL_SIZE),
	RK_DEVICE(RK_DDR_VIRT + RK312X_DDR_PCTL_SIZE, RK312X_DDR_PHY_PHYS, RK312X_DDR_PHY_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(0), RK312X_GPIO0_PHYS, RK312X_GPIO_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(1), RK312X_GPIO1_PHYS, RK312X_GPIO_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(2), RK312X_GPIO2_PHYS, RK312X_GPIO_SIZE),
	RK_DEVICE(RK_GPIO_VIRT(3), RK312X_GPIO3_PHYS, RK312X_GPIO_SIZE),
	RK_DEVICE(RK_GIC_VIRT, RK312X_GIC_DIST_PHYS, RK312X_GIC_DIST_SIZE),
	RK_DEVICE(RK_GIC_VIRT + RK312X_GIC_DIST_SIZE, RK312X_GIC_CPU_PHYS, RK312X_GIC_CPU_SIZE),
	RK_DEVICE(RK312X_IMEM_VIRT, RK312X_IMEM_PHYS, RK312X_IMEM_SIZE),
};
static void usb_uart_init(void)
{
#ifdef CONFIG_RK_USB_UART
	u32 soc_status0 = readl_relaxed(RK_GRF_VIRT + RK312X_GRF_SOC_STATUS0);
#endif
	writel_relaxed(0x34000000, RK_GRF_VIRT + RK312X_GRF_UOC1_CON4);
#ifdef CONFIG_RK_USB_UART
	if (!(soc_status0 & (1 << 5)) && (soc_status0 & (1 << 8))) {
		/* software control usb phy enable */
		writel_relaxed(0x007f0055, RK_GRF_VIRT + RK312X_GRF_UOC0_CON0);
		writel_relaxed(0x34003000, RK_GRF_VIRT + RK312X_GRF_UOC1_CON4);
	}
#endif

	writel_relaxed(0x07, RK_DEBUG_UART_VIRT + 0x88);
	writel_relaxed(0x00, RK_DEBUG_UART_VIRT + 0x04);
	writel_relaxed(0x83, RK_DEBUG_UART_VIRT + 0x0c);
	writel_relaxed(0x0d, RK_DEBUG_UART_VIRT + 0x00);
	writel_relaxed(0x00, RK_DEBUG_UART_VIRT + 0x04);
	writel_relaxed(0x03, RK_DEBUG_UART_VIRT + 0x0c);
}

static void __init rk312x_dt_map_io(void)
{
	iotable_init(rk312x_io_desc, ARRAY_SIZE(rk312x_io_desc));
	debug_ll_io_init();
	usb_uart_init();

	/* enable timer5 for core */
	writel_relaxed(0, RK312X_TIMER5_VIRT + 0x10);
	dsb();
	writel_relaxed(0xFFFFFFFF, RK312X_TIMER5_VIRT + 0x00);
	writel_relaxed(0xFFFFFFFF, RK312X_TIMER5_VIRT + 0x04);
	dsb();
	writel_relaxed(1, RK312X_TIMER5_VIRT + 0x10);
	dsb();
}

static void __init rk3126_dt_map_io(void)
{
	rockchip_soc_id = ROCKCHIP_SOC_RK3126;

	rk312x_dt_map_io();
}

static void __init rk3128_dt_map_io(void)
{
	rockchip_soc_id = ROCKCHIP_SOC_RK3128;

	rk312x_dt_map_io();
}

extern void secondary_startup(void);
static int rk312x_sys_set_power_domain(enum pmu_power_domain pd, bool on)
{
	if (on) {
#ifdef CONFIG_SMP
		if (pd >= PD_CPU_1 && pd <= PD_CPU_3) {
			writel_relaxed(0x20000 << (pd - PD_CPU_1),
				       RK_CRU_VIRT + RK312X_CRU_SOFTRSTS_CON(0));
			dsb();
			udelay(10);
			writel_relaxed(virt_to_phys(secondary_startup),
				       RK312X_IMEM_VIRT + 8);
			writel_relaxed(0xDEADBEAF, RK312X_IMEM_VIRT + 4);
			dsb_sev();
		}
#endif
	} else {
#ifdef CONFIG_SMP
		if (pd >= PD_CPU_1 && pd <= PD_CPU_3) {
			writel_relaxed(0x20002 << (pd - PD_CPU_1),
				       RK_CRU_VIRT + RK312X_CRU_SOFTRSTS_CON(0));
			dsb();
		}
#endif
	}

	return 0;
}

static bool rk312x_pmu_power_domain_is_on(enum pmu_power_domain pd)
{
	return 1;
}

static int rk312x_pmu_set_idle_request(enum pmu_idle_req req, bool idle)
{
	return 0;
}

static void __init rk312x_dt_init_timer(void)
{
	rockchip_pmu_ops.set_power_domain = rk312x_sys_set_power_domain;
	rockchip_pmu_ops.power_domain_is_on = rk312x_pmu_power_domain_is_on;
	rockchip_pmu_ops.set_idle_request = rk312x_pmu_set_idle_request;
	of_clk_init(NULL);
	clocksource_of_init();
	of_dvfs_init();
}

static void __init rk312x_reserve(void)
{
	/* reserve memory for ION */
	rockchip_ion_reserve();
}

static void __init rk312x_init_late(void)
{
}

static void rk312x_restart(char mode, const char *cmd)
{
	u32 boot_flag, boot_mode;

	rockchip_restart_get_boot_mode(cmd, &boot_flag, &boot_mode);

	writel_relaxed(boot_flag, RK_GRF_VIRT + RK312X_GRF_OS_REG4);	// for loader
	writel_relaxed(boot_mode, RK_GRF_VIRT + RK312X_GRF_OS_REG5);	// for linux
	dsb();

	/* pll enter slow mode */
	writel_relaxed(0x30110000, RK_CRU_VIRT + RK312X_CRU_MODE_CON);
	dsb();
	writel_relaxed(0xeca8, RK_CRU_VIRT + RK312X_CRU_GLB_SRST_SND_VALUE);
	dsb();
}

DT_MACHINE_START(RK3126_DT, "Rockchip RK3126")
	.smp		= smp_ops(rockchip_smp_ops),
	.map_io		= rk3126_dt_map_io,
	.init_time	= rk312x_dt_init_timer,
	.dt_compat	= rk3126_dt_compat,
	.init_late	= rk312x_init_late,
	.reserve	= rk312x_reserve,
	.restart	= rk312x_restart,
MACHINE_END

DT_MACHINE_START(RK3128_DT, "Rockchip RK3128")
	.smp		= smp_ops(rockchip_smp_ops),
	.map_io		= rk3128_dt_map_io,
	.init_time	= rk312x_dt_init_timer,
	.dt_compat	= rk3128_dt_compat,
	.init_late	= rk312x_init_late,
	.reserve	= rk312x_reserve,
	.restart	= rk312x_restart,
MACHINE_END


char PIE_DATA(sram_stack)[1024];
EXPORT_PIE_SYMBOL(DATA(sram_stack));

static int __init rk312x_pie_init(void)
{
	int err;

	if (!cpu_is_rk312x())
		return 0;

	err = rockchip_pie_init();
	if (err)
		return err;

	rockchip_pie_chunk = pie_load_sections(rockchip_sram_pool, rk312x);
	if (IS_ERR(rockchip_pie_chunk)) {
		err = PTR_ERR(rockchip_pie_chunk);
		pr_err("%s: failed to load section %d\n", __func__, err);
		rockchip_pie_chunk = NULL;
		return err;
	}

	rockchip_sram_virt = kern_to_pie(rockchip_pie_chunk, &__pie_common_start[0]);
	rockchip_sram_stack = kern_to_pie(rockchip_pie_chunk, (char *)DATA(sram_stack) + sizeof(DATA(sram_stack)));

	return 0;
}
arch_initcall(rk312x_pie_init);
#include "ddr_rk3126.c"
static int __init rk312x_ddr_init(void)
{
	if (cpu_is_rk312x()) {
		ddr_change_freq = _ddr_change_freq;
		ddr_round_rate = _ddr_round_rate;
		ddr_set_auto_self_refresh = _ddr_set_auto_self_refresh;
		ddr_init(DDR3_DEFAULT, 300);
		}
	return 0;
}
arch_initcall_sync(rk312x_ddr_init);
