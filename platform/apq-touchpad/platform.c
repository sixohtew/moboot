/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * Copyright (c) 2011, James Sullins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <reg.h>
#include <debug.h>
#include <uart_dm.h>
#include <qgic.h>
#include <platform/iomap.h>
#include <mmu.h>
#include <qgic.h>
#include <arch/arm/mmu.h>
#include <dev/fbcon.h>

extern void platform_init_timer(void);
extern uint8_t target_uart_gsbi(void);

static uint32_t ticks_per_sec = 0;

#define MB (1024*1024)
#define MSM_IOMAP_SIZE ((MSM_IOMAP_END - MSM_IOMAP_BASE)/MB)

/* Scratch region - cacheable, write through */
#define CACHEABLE_MEMORY  (MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH   | \
                           MMU_MEMORY_AP_READ_WRITE)

/* Peripherals - non-shared device */
#define IOMAP_MEMORY      (MMU_MEMORY_TYPE_DEVICE_NON_SHARED | \
                           MMU_MEMORY_AP_READ_WRITE)

mmu_section_t mmu_section_table[] = {
/*  Physical addr,    Virtual addr,    Size (in MB),    Flags */
    {MEMBASE,         MEMBASE,         (MEMSIZE/MB),    CACHEABLE_MEMORY},
    {SCRATCH_ADDR,    SCRATCH_ADDR,    SCRATCH_SIZE,    CACHEABLE_MEMORY},
    {MSM_IOMAP_BASE,  MSM_IOMAP_BASE,  MSM_IOMAP_SIZE,  IOMAP_MEMORY},
};

void platform_early_init(void)
{
#if WITH_DEBUG_UART
	apq_gpio_set(58, 1); // enable UART
	uart_init(target_uart_gsbi());
#endif
	qgic_init();
	platform_init_timer();
}

void platform_init(void)
{
	dprintf(SPEW, "platform_init()\n");
	platform_init_display();
}

/* Setup memory for this platform */
void platform_init_mmu_mappings(void)
{
	uint32_t i;
	uint32_t sections;
	uint32_t table_size = ARRAY_SIZE(mmu_section_table);

	for (i = 0; i < table_size; i++)
	{
		sections = mmu_section_table[i].num_of_sections;

		while (sections--)
		{
			arm_mmu_map_section(mmu_section_table[i].paddress + sections*MB,
								mmu_section_table[i].vaddress + sections*MB,
								mmu_section_table[i].flags);
		}
	}
}

/* Initialize DGT timer */
void platform_init_timer(void)
{
	/* disable timer */
	writel(0, DGT_ENABLE);

	/* DGT uses LPXO source which is 27MHz.
	 * Set clock divider to 4.
	 */
	writel(3, DGT_CLK_CTL);

	ticks_per_sec = 6750000; /* (27 MHz / 4) */
}

/* Returns timer ticks per sec */
uint32_t platform_tick_rate(void)
{
	return ticks_per_sec;
}

void display_init(void)
{
	static int runonce = 0;
	static struct fbcon_config fb_cfg = {
		.base = (void *)0x7f600000, 
		.height = 768,
		.width = 1024,
		/* stride, format, bpp NOT USED for DISPLAY_TYPE_TOUCHPAD */
		.stride = 4,                /* not used */
		.format = FB_FORMAT_RGB888, /* not used */
		.bpp = 24,                  /* not used */
		.update_start = NULL,
		.update_done = NULL,
	};


    if (runonce == 1)
        return;
    else
        runonce = 1;

	gfxconsole_start_on_display();
    fbcon_setup(&fb_cfg);
}

