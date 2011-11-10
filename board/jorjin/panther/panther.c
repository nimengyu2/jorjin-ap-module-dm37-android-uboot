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
#include <fastboot.h>
#include "panther.h"

#ifdef	CONFIG_CMD_FASTBOOT
#ifdef	FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING
/* Initialize the name of fastboot flash name mappings */
fastboot_ptentry ptn[6] = {
	{
		.name   = "xloader",
		.start  = 0x0000000,
		.length = 0x0020000,
		/* Written into the first 4 0x20000 blocks
		   Use HW ECC */
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I |
		          FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
			  FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_2 |
			  FASTBOOT_PTENTRY_FLAGS_REPEAT_4,
	},
		{
		.name   = "bootloader",
		.start  = 0x0080000,
		.length = 0x01C0000,
		/* Skip bad blocks on write
		   Use HW ECC */
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_I |
		          FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
			  FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_2,
	},
	{
		.name   = "environment",
		.start  = SMNAND_ENV_OFFSET,  /* set in config file */
		.length = 0x0040000,
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
			  FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_1 |
			  FASTBOOT_PTENTRY_FLAGS_WRITE_ENV,
	},
	{
		.name   = "boot",
		/* Test with start close to bad block
		   The is dependent on the individual board.
		   Change to what is required */
		/* .start  = 0x0a00000, */
			/* The real start */
		.start  = 0x0280000,
		.length = 0x0500000,
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
		          FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_1 |
			  FASTBOOT_PTENTRY_FLAGS_WRITE_I,
	},
	{
		.name   = "system",
		.start  = 0x00780000,
		.length = 0x1F880000,
		.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC |
		          FASTBOOT_PTENTRY_FLAGS_HW_ECC_LAYOUT_1 |
			  FASTBOOT_PTENTRY_FLAGS_WRITE_I,
	},
};
#endif /* FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING */
#endif /* CONFIG_FASTBOOT */

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

#ifdef	CONFIG_CMD_FASTBOOT
#ifdef	FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING
	int i;

	for (i = 0; i < 6; i++)
		fastboot_flash_add_ptn (&ptn[i]);
#endif /* FASTBOOT_PORT_OMAPZOOM_NAND_FLASHING */
#endif /* CONFIG_FASTBOOT */

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
	writel(~(GPIO8 | GPIO7), &gpio2_base->oe);
	writel(~(GPIO1), &gpio5_base->oe);

	/* Set GPIOs */
	writel(GPIO8, &gpio2_base->cleardataout);	// set GPIO_40(USB HUB reset) to low
	writel(GPIO7, &gpio2_base->setdataout);	// set GPIO_39(P8 USB HUB nreset) to high
	writel(GPIO1, &gpio5_base->setdataout);	// set GPIO_129(DVI enable) to high

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
