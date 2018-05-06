MEMORY
{
	ram (rwx)	: ORIGIN = 0x20000000, LENGTH = 64K
	rom (rx)	: ORIGIN = 0x08000000, LENGTH = 512K
}

PROVIDE(_sector_size = 2048);
PROVIDE(_bootopt_offset = _app_offset - _sector_size);
PROVIDE(_app_offset = 0x5000); /* 20480 */

INCLUDE common.ld
