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

	.keys _rom_start + _bootopt_offset - 16 - 64 :
	{
		_aeskey = .;
		LONG(0x933ADA7F);
		LONG(0x485F0153);
		LONG(0x874F637C);
		LONG(0x520202A2);
		_pubkey = .;
		LONG(0x6CC0085F);
		LONG(0x6A1E9B21);
		LONG(0xEEFCB6FB);
		LONG(0x17B8ECEC);
		LONG(0x3BBF86F1);
		LONG(0x403DDBC9);
		LONG(0xE501F3BE);
		LONG(0x922F342D);
		LONG(0x7D7A84F3);
		LONG(0x707434ED);
		LONG(0x4FB7D4D0);
		LONG(0x7BB8F21F);
		LONG(0xAA135FE1);
		LONG(0xB9DED991);
		LONG(0x165A04D0);
		LONG(0xA740506E);
	} > rom

	.bootopt _rom_start + _bootopt_offset :
	{
		LONG(_rom_start + _app_offset); /* ADDR */
		LONG(0xffffffff); /* LEN */
		LONG(0xffffffff); /* HASH */
	} > rom
}
