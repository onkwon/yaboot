/*
 * "[...] Sincerity (comprising truth-to-experience, honesty towards the self,
 * and the capacity for human empathy and compassion) is a quality which
 * resides within the laguage of literature. It isn't a fact or an intention
 * behind the work [...]"
 *
 *             - An introduction to Literary and Cultural Theory, Peter Barry
 *
 *
 *                                                   o8o
 *                                                   `"'
 *     oooo    ooo  .oooo.    .ooooo.   .oooo.o     oooo   .ooooo.
 *      `88.  .8'  `P  )88b  d88' `88b d88(  "8     `888  d88' `88b
 *       `88..8'    .oP"888  888   888 `"Y88b.       888  888   888
 *        `888'    d8(  888  888   888 o.  )88b .o.  888  888   888
 *         .8'     `Y888""8o `Y8bod8P' 8""888P' Y8P o888o `Y8bod8P'
 *     .o..P'
 *     `Y8P'                   Kyunghwan Kwon <kwon@yaos.io>
 *
 *  Welcome aboard!
 */

#include "bsp.h"
#include "flash.h"

extern char _ram_end;

static void ISR_null()
{
}

static inline void mem_init()
{
	unsigned int i;

	/* copy .data section from flash to sram */
	extern char _etext, _data, _edata;
	for (i = 0; (((unsigned int *)&_data) + i) < (unsigned int *)&_edata;
			i++)
		((unsigned int *)&_data)[i] = ((unsigned int *)&_etext)[i];

	/* clear .bss section */
	extern char _bss, _ebss;
	for (i = 0; (((unsigned int *)&_bss) + i) < (unsigned int *)&_ebss; i++)
		((unsigned int *)&_bss)[i] = 0;

	dsb();
}

static void __attribute__((naked, used)) ISR_reset()
{
	cli();

	SCB_CCR |= 0x00000008; /* enable unaligned access traps */
	SCB_CCR |= 0x00000200; /* 8-byte stack alignment */

	dsb();
	isb();

	mem_init();

	extern void main();
	main();
}

static void *vectors[]
__attribute__((section(".vector"), aligned(4), used)) = {
			/* nVEC   : ADDR  - DESC */
			/* -------------------- */
	&_ram_end,	/* 00     : 0x00  - Stack pointer */
	ISR_reset,	/* 01     : 0x04  - Reset */
	ISR_null,	/* 02     : 0x08  - NMI */
	ISR_null,	/* 03     : 0x0c  - HardFault */
	ISR_null,	/* 04     : 0x10  - MemManage */
	ISR_null,	/* 05     : 0x14  - BusFault */
	ISR_null,	/* 06     : 0x18  - UsageFault */
	ISR_null,	/* 07     : 0x1c  - Reserved */
	ISR_null,	/* 08     : 0x20  - Reserved */
	ISR_null,	/* 09     : 0x24  - Reserved */
	ISR_null,	/* 10     : 0x28  - Reserved */
	ISR_null,	/* 11     : 0x2c  - SVCall */
	ISR_null,	/* 12     : 0x30  - Debug Monitor */
	ISR_null,	/* 13     : 0x34  - Reserved */
	ISR_null,	/* 14     : 0x38  - PendSV */
	ISR_null,	/* 15     : 0x3c  - SysTick */
};
