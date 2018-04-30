MEMORY
{
	ram (rwx)	: ORIGIN = 0x20000000, LENGTH = 64K
	rom (rx)	: ORIGIN = 0x08000000, LENGTH = 512K
}

PROVIDE(_bootopt_offset = _app_offset - 2048); /* sector size = 2048 bytes */
PROVIDE(_app_offset = 0x2000); /* 8192 */

INCLUDE common.ld
