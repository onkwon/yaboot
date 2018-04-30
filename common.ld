OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

PROVIDE(_rom_start   = ORIGIN(rom));
PROVIDE(_rom_size    = LENGTH(rom));
PROVIDE(_ram_start   = ORIGIN(ram));
PROVIDE(_ram_size    = LENGTH(ram));
PROVIDE(_ram_end     = ORIGIN(ram) + LENGTH(ram));
PROVIDE(_vector_size = 0x200); /* The minimum alignment is 128 words. */

PROVIDE(_bootopt = _rom_start + _bootopt_offset);
PROVIDE(_app = _rom_start + _app_offset);

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

	.aes128 _rom_start + _bootopt_offset - 16 :
	{
		_aes_key = .;
		LONG(0x933ADA7F);
		LONG(0x485F0153);
		LONG(0x874F637C);
		LONG(0x520202A2);
	} > rom

	.bootopt _rom_start + _bootopt_offset :
	{
		LONG(_rom_start + _app_offset); /* ADDR */
		LONG(0xffffffff); /* LEN */
		LONG(0xffffffff); /* HASH */
	} > rom
}
