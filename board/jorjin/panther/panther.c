/*
 * (C) Copyright 2004-2008
 * Texas Instruments, <www.ti.com>
 *
 * Author :
 *	Sunil Kumar <sunilsaini05@gmail.com>
 *	Shashi Ranjan <shashiranjanmca05@gmail.com>
 *
 * Derived from Beagle Board and 3430 SDP code by
 *	Richard Woodruff <r-woodruff2@ti.com>
 *	Syed Mohammed Khasim <khasim@ti.com>
 *
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <twl4030.h>
#include <asm/io.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/gpio.h>
#include <asm/mach-types.h>
#include "panther.h"

/*
 * Routine: board_init
 * Description: Early hardware init.
 */
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gpmc_init(); /* in SRAM or SDRAM, finish GPMC */
	/* board id for Linux */
	gd->bd->bi_arch_number = MACH_TYPE_PANTHER;
	/* boot param addr */
	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100);

	return 0;
}

/* T2 Register definitions */
#define T2_BASE			0x48002000
typedef struct t2 {
	unsigned char res1[0x520];
	unsigned int pbias_lite;	/* 0x520 */
	unsigned char res2[0x538];
	unsigned int wkup_ctrl;	/* 0xA5C */
} t2_t;

/*
 * Routine: misc_init_r
 * Description: Configure board specific parts
 */
int misc_init_r(void)
{
	struct gpio *gpio2_base = (struct gpio *)OMAP34XX_GPIO2_BASE;
	struct gpio *gpio5_base = (struct gpio *)OMAP34XX_GPIO5_BASE;
	struct gpio *gpio6_base = (struct gpio *)OMAP34XX_GPIO6_BASE;

	t2_t *t2_base = (t2_t *)T2_BASE;

	/*
	 * Configure drive strength for IO cells
	 */
	*(ulong *)(CONTROL_PROG_IO1) &= ~(PRG_I2C2_PULLUPRESX);

	twl4030_power_init();
	/* Set VAUX3 to 1.5V, VAUX4 to 1.8V */
	twl4030_pmrecv_vsel_cfg(TWL4030_PM_RECEIVER_VAUX3_DEDICATED,
				TWL4030_PM_RECEIVER_VAUX3_VSEL_15,
				TWL4030_PM_RECEIVER_VAUX3_DEV_GRP,
				0x00);
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, TWL4030_PM_RECEIVER_VAUX4_VSEL_18, TWL4030_PM_RECEIVER_VAUX4_DEDICATED);
	twl4030_led_init(TWL4030_LED_LEDEN_LEDAON | TWL4030_LED_LEDEN_LEDBON);
	/* Turn on LEDA */
	twl4030_i2c_write_u8(TWL4030_CHIP_PWMA, 0x7F, TWL4030_BASEADD_PWMA);
	twl4030_i2c_write_u8(TWL4030_CHIP_PWMA, 0x7F, TWL4030_BASEADD_PWMA+1);
	twl4030_led_init(TWL4030_LED_LEDEN_LEDBON);

	printf("Panther Rev A\n");
	/* Set VAUX2 to 1.8V for EHCI PHY */
	twl4030_pmrecv_vsel_cfg(TWL4030_PM_RECEIVER_VAUX2_DEDICATED,
				TWL4030_PM_RECEIVER_VAUX2_VSEL_18,
				TWL4030_PM_RECEIVER_VAUX2_DEV_GRP,
				TWL4030_PM_RECEIVER_DEV_GRP_P1);
	/* Set VSIM to 1.8V for GPIO_126, 127 & 129 */
	twl4030_pmrecv_vsel_cfg(TWL4030_PM_RECEIVER_VSIM_DEDICATED,
				TWL4030_PM_RECEIVER_VSIM_VSEL_18,
				TWL4030_PM_RECEIVER_VSIM_DEV_GRP,
				TWL4030_PM_RECEIVER_DEV_GRP_P1);
	// Switch VSIM's voltage from 3V to 1.8V. For more information, please refer to AMDM37x TRM section 13.5.2.
#define mdelay(n) ({unsigned long msec=(n); while (msec--) udelay(1000);})
#define PBIASLITEVMODE1	(1 << 8)
#define PBIASLITEPWRDNZ1	(1 << 9)
#define GPIO_IO_PWRDNZ	(1 << 6)
	writel(readl(&t2_base->pbias_lite) & ~PBIASLITEPWRDNZ1, &t2_base->pbias_lite);
	writel(readl(&t2_base->wkup_ctrl) & ~GPIO_IO_PWRDNZ, &t2_base->wkup_ctrl);
	writel(readl(&t2_base->pbias_lite) & ~PBIASLITEVMODE1, &t2_base->pbias_lite);
	mdelay(100);
	writel(readl(&t2_base->pbias_lite) | PBIASLITEPWRDNZ1, &t2_base->pbias_lite);
	writel(readl(&t2_base->wkup_ctrl) | GPIO_IO_PWRDNZ, &t2_base->wkup_ctrl);

	/* Configure GPIOs to output */
	// GPIO_39
	writel(~(GPIO7), &gpio2_base->oe);
	// GPIO_183, GPIO_170, GPIO_168, GPIO_162, GPIO_161
	writel(~(GPIO23 | GPIO10 | GPIO8 | GPIO2 | GPIO1), &gpio6_base->oe);
	// GPIO_159, GPIO_158, GPIO_157, GPIO_156, GPIO_150, GPIO_149, GPIO_143, GPIO_142, GPIO_141, GPIO_140
	writel(~(GPIO31 | GPIO30 | GPIO29 | GPIO28 | GPIO22 | GPIO21 |
		GPIO15 | GPIO14 | GPIO13 | GPIO12), &gpio5_base->oe);

	/* Set GPIOs */
	writel(GPIO7, &gpio2_base->setdataout); // set GPIO_39(P8 USB HUB nreset) to high voltage
	writel(GPIO23 | GPIO10 | GPIO8 | GPIO2 | GPIO1,
		&gpio6_base->setdataout);
	writel(GPIO31 | GPIO30 | GPIO29 | GPIO28 | GPIO22 | GPIO21 |
		GPIO15 | GPIO14 | GPIO13 | GPIO12, &gpio5_base->setdataout);

	dieid_num_r();

	return 0;
}

/*
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers specific to the
 *		hardware. Many pins need to be moved from protect to primary
 *		mode.
 */
void set_muxconf_regs(void)
{
	MUX_PANTHER();
}
