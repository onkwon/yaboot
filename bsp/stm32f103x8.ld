MEMORY
{
	ram (rwx)	: ORIGIN = 0x20000000, LENGTH = 20K
	rom (rx)	: ORIGIN = 0x08000000, LENGTH = 128K
}

PROVIDE(_bootopt_offset = _app_offset - 1024); /* sector size = 1024 bytes */
PROVIDE(_app_offset = 0x2000); /* 8192 */

INCLUDE common.ld
