OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

PROVIDE(_rom_start   = ORIGIN(rom));
PROVIDE(_rom_size    = LENGTH(rom));
PROVIDE(_ram_start   = ORIGIN(ram));
PROVIDE(_ram_size    = LENGTH(ram));
PROVIDE(_ram_end     = ORIGIN(ram) + LENGTH(ram));
PROVIDE(_vector_size = 0x200); /* The minimum alignment is 128 words. */

SECTIONS
{
	.text :
	{
		*(.vector)
		. = _vector_size;
		LONG(0); /* null sentinel */

		*(.text)
		*(.text.*)

		. = ALIGN(4);
		*(.rodata)
		*(.rodata.*)

		_etext = .;
	} > rom

	.data :
	{
		. = ALIGN(4);
		_data = .;

		*(.data .data.*)
		*(.iap)

		. = ALIGN(4);
		_edata = .;
	} > ram AT > rom

	.bss (NOLOAD) :
	{
		. = ALIGN(4);
		_bss = .;

		*(.bss .bss.*)
		*(COMMON)

		. = ALIGN(4);
		_ebss = .;
	} > ram AT > rom

	.bootopt _rom_start + _bootopt :
	{
		LONG(_rom_start + _app); /* ADDR */
		LONG(0xffffffff); /* LEN */
		LONG(0xffffffff); /* HASH */
	} > rom
}
