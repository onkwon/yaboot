MEMORY
{
	ram (rwx)	: ORIGIN = 0x20000000, LENGTH = 20K
	rom (rx)	: ORIGIN = 0x08000000, LENGTH = 128K
}

PROVIDE(_bootopt = 0xC00); /* 3072 */
PROVIDE(_app = 0x1000); /* 4096 */

INCLUDE common.ld
