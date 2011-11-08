/*
 * Functions related to OMAP3 SDRC.
 *
 * This file has been created after exctracting and consolidating
 * the SDRC related content from mem.c and board.c, also created
 * generic init function (mem_init).
 *
 * Copyright (C) 2004-2010
 * Texas Instruments Incorporated - http://www.ti.com/
 *
 * Author :
 *     Vaibhav Hiremath <hvaibhav@ti.com>
 *
 * Original implementation by (mem.c, board.c) :
 *      Sunil Kumar <sunilsaini05@gmail.com>
 *      Shashi Ranjan <shashiranjanmca05@gmail.com>
 *      Manikandan Pillai <mani.pillai@ti.com>
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
#include <asm/io.h>
#include <asm/arch/mem.h>
#include <asm/arch/sys_proto.h>

extern omap3_sysinfo sysinfo;

static struct sdrc *sdrc_base = (struct sdrc *)OMAP34XX_SDRC_BASE;

/*
 * is_mem_sdr -
 *  - Return 1 if mem type in use is SDR
 */
u32 is_mem_sdr(void)
{
	if (readl(&sdrc_base->cs[CS0].mr) == SDRC_MR_0_SDR)
		return 1;
	return 0;
}

/*
 * make_cs1_contiguous -
 * - When we have CS1 populated we want to have it mapped after cs0 to allow
 *   command line mem=xyz use all memory with out discontinuous support
 *   compiled in.  We could do it in the ATAG, but there really is two banks...
 */
void make_cs1_contiguous(void)
{
	u32 size, a_add_low, a_add_high;

	size = get_sdr_cs_size(CS0);
	size >>= 25;	/* divide by 32 MiB to find size to offset CS1 */
	a_add_high = (size & 3) << 8;	/* set up low field */
	a_add_low = (size & 0x3C) >> 2;	/* set up high field */
	writel((a_add_high | a_add_low), &sdrc_base->cs_cfg);

}


/*
 * get_sdr_cs_size -
 *  - Get size of chip select 0/1
 */
u32 get_sdr_cs_size(u32 cs)
{
	u32 size;

	/* get ram size field */
	size = readl(&sdrc_base->cs[cs].mcfg) >> 8;
	size &= 0x3FF;		/* remove unwanted bits */
	size <<= 21;		/* multiply by 2 MiB to find size in MB */
	return size;
}

/*
 * get_sdr_cs_offset -
 *  - Get offset of cs from cs0 start
 */
u32 get_sdr_cs_offset(u32 cs)
{
	u32 offset;

	if (!cs)
		return 0;

	offset = readl(&sdrc_base->cs_cfg);
	offset = (offset & 15) << 27 | (offset & 0x30) << 17;

	return offset;
}

/*
 * do_sdrc_init -
 *  - Initialize the SDRAM for use.
 *  - Sets up SDRC timings for CS0
 *  - code called once in C-Stack only context for CS0 and a possible 2nd
 *    time depending on memory configuration from stack+global context
 */
void do_sdrc_init(u32 cs, u32 early)
{
	struct sdrc_actim *sdrc_actim_base;

	if (cs)
		sdrc_actim_base = (struct sdrc_actim *)SDRC_ACTIM_CTRL1_BASE;
	else
		sdrc_actim_base = (struct sdrc_actim *)SDRC_ACTIM_CTRL0_BASE;

	if (early) {
		/* reset sdrc controller */
		writel(SOFTRESET, &sdrc_base->sysconfig);
		wait_on_value(RESETDONE, RESETDONE, &sdrc_base->status,
				12000000);
		writel(0, &sdrc_base->sysconfig);

		/* setup sdrc to ball mux */
		writel(SDRC_SHARING, &sdrc_base->sharing);

		/* Disable Power Down of CKE cuz of 1 CKE on combo part */
		writel(WAKEUPPROC | SRFRONRESET | PAGEPOLICY_HIGH,
				&sdrc_base->power);

		writel(ENADLL | DLLPHASE_90, &sdrc_base->dlla_ctrl);
		sdelay(0x20000);
	}

	writel(RASWIDTH_13BITS | CASWIDTH_10BITS | ADDRMUXLEGACY |
			RAMSIZE_128 | BANKALLOCATION | B32NOT16 | B32NOT16 |
			DEEPPD | DDR_SDRAM, &sdrc_base->cs[cs].mcfg);
	writel(ARCV | ARE_ARCV_1, &sdrc_base->cs[cs].rfr_ctrl);
	if (is_cpu_family(CPU_OMAP36XX)) {
		writel(V_ACTIMA_200, &sdrc_actim_base->ctrla);
		writel(V_ACTIMB_200, &sdrc_actim_base->ctrlb);
	} else {
		writel(V_ACTIMA_165, &sdrc_actim_base->ctrla);
		writel(V_ACTIMB_165, &sdrc_actim_base->ctrlb);
	}

	writel(CMD_NOP, &sdrc_base->cs[cs].manual);
	writel(CMD_PRECHARGE, &sdrc_base->cs[cs].manual);
	writel(CMD_AUTOREFRESH, &sdrc_base->cs[cs].manual);
	writel(CMD_AUTOREFRESH, &sdrc_base->cs[cs].manual);

	/*
	 * CAS latency 3, Write Burst = Read Burst, Serial Mode,
	 * Burst length = 4
	 */
	writel(CASL3 | BURSTLENGTH4, &sdrc_base->cs[cs].mr);

	if (!mem_ok(cs))
		writel(0, &sdrc_base->cs[cs].mcfg);
}

/*
 * dram_init -
 *  - Sets uboots idea of sdram size
 */
int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	unsigned int size0 = 0, size1 = 0;

	size0 = get_sdr_cs_size(CS0);
	/*
	 * We always need to have cs_cfg point at where the second
        * bank would be, if present.  Failure to do so can lead to
        * strange situations where memory isn't detected and
        * configured correctly.  CS0 will already have been setup
        * at this point.
	 */
	make_cs1_contiguous();
#ifndef CONFIG_OMAP3_PANTHER
	// Jack_20111108: Disable CS1's initialization.
	/* 
	 * According to "<ROWBOAT>/x-loader/board/omap3evm/omap3evm.c". The initialization of CS1 has been done in function "config_3430sdram_ddr()".
	 * These two initializations(config_3430sdram_ddr() & dram_init()) are basically the same except for the parameters of SDRC. However, in x-loader,
	 * these parameters are specified respectively by each board. But in u-boot, they aren't. That means if we run this code in u-boot, it might configurate 		 * SDRC with inappropriate settings. To fix this issue, I've duplicated omap3evm's process into "panther.c". Then disable function "do_sdrc_init()"
	 * to avoid parameters overwritting.
	 *
	 * Also, there is a patch for omap3's SDRC size issue, but it won't work for pantherboard. Please refer to 
	 * http://www.mail-archive.com/u-boot@lists.denx.de/msg68395.html if interested.
	 */
	do_sdrc_init(CS1, NOT_EARLY);
#endif
	size1 = get_sdr_cs_size(CS1);

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = size0;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_1 + get_sdr_cs_offset(CS1);
	gd->bd->bi_dram[1].size = size1;

	return 0;
}

/*
 * mem_init -
 *  - Init the sdrc chip,
 *  - Selects CS0 and CS1,
 */
void mem_init(void)
{
	/* only init up first bank here */
	do_sdrc_init(CS0, EARLY_INIT);
}
