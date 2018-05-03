#include "bsp.h"
#include "flash.h"
#include "aes.h"
#include "sha256.h"
#include "uart.h"

#include <string.h>
#include <stdlib.h>

#define MAGIC1		0xDEC0ADDE
#define MAGIC2		0xDEC1ADDE
#define MAGIC3		0xDEC2ADDE

#define warn(msg)	uart_puts("WARN: "msg"\r\n")
#define notice(msg)	uart_puts(msg"\r\n")

struct appimg_t {
	const uint32_t magic[3];
	const uint32_t len;
	const uint8_t iv[16];
	const uint8_t hash[32];
	const uint8_t data[];
} __attribute__((packed, aligned(4)));

struct bootopt_t {
	const uint32_t addr;
	const uint32_t len;
	const uint8_t hash[32];
} __attribute__((packed, aligned(4)));

void reboot()
{
	dsb();
	isb();

#define VECTKEY		0x5fa
	SCB_AIRCR = (VECTKEY << 16)
		| (SCB_AIRCR & (7 << 8)) /* keep priority group unchanged */
		| (1 << 2); /* system reset request */
}

static int verify_hash(const uint8_t *hash, const uint8_t *data, size_t len)
{
	SHA256_CTX ctx;
	uint8_t result[SHA256_BLOCK_SIZE];
	int q, r, i;

	q = len / SHA256_BLOCK_SIZE;
	r = len % SHA256_BLOCK_SIZE;

	sha256_init(&ctx);
	for (i = 0; i < q; i++)
		sha256_update(&ctx, &data[i * SHA256_BLOCK_SIZE], SHA256_BLOCK_SIZE);
	if (r)
		sha256_update(&ctx, &data[i * SHA256_BLOCK_SIZE], r);
	sha256_final(&ctx, result);

#ifdef DEBUG
	char t[10];
	uart_puts("SHA256: ");
	for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
		itoa(result[i], t, 16);
		uart_puts(t);
	}
	uart_puts("\r\n");
#endif

	return memcmp(hash, result, 32);
}

static void program(void *addr, const struct appimg_t *img)
{
	extern char _aes_key, _sector_size;
	struct AES_ctx ctx;
	uint8_t buf[(int)&_sector_size];
	int size;
	uint8_t *d = (uint8_t *)addr;
	const uint8_t *key = (const uint8_t *)&_aes_key;

	AES_init_ctx_iv(&ctx, key, img->iv);

	for (int i = 0; i < img->len; i += (int)&_sector_size) {
		size = ((img->len - i) < (int)&_sector_size)?
			img->len - i : (int)&_sector_size;
		memcpy(buf, &img->data[i], size);
		AES_CTR_xcrypt_buffer(&ctx, buf, size);
		flash_program(d, (const void * const)buf, size);
		d += size;
#ifdef DEBUG
		char t[10];
		itoa((i+size) * 100 / img->len, t, 10);
		uart_put('\r');
		uart_puts("Flashing : ");
		uart_puts(t);
		uart_put('%');
#endif
	}

	/* Flash meta data, MAGIC, len, IV, Hash */
	d = (uint8_t *)(((unsigned int)d + 3UL) & ~3UL); /* 4-byte alignement */
#ifdef DEBUG
	char t[10];
	uart_puts("\r\n");
	itoa((int)d, t, 16);
	uart_puts("Meta 0x");
	uart_puts(t);
#endif
	flash_program(d, (const void * const)img, (int)img->data - (int)img);

#ifdef DEBUG
	uart_puts("\r\n");
	itoa((int)addr, t, 16);
	uart_puts("written at 0x");
	uart_puts(t);
	uart_puts("\r\n");
#endif
}

static void update_bootopt(void *addr, const struct appimg_t *img)
{
	extern const char _bootopt;
	unsigned int buf[14];

	buf[0] = (unsigned int)addr;
	buf[1] = img->len;
	memcpy(&buf[2], img->hash, 32);
	memcpy(&buf[10], img->iv, 16);

	flash_program((void *)&_bootopt, buf, 14 * 4);
}

void main()
{
	extern const char _app, _bootopt, _rom_start, _rom_size;
	const struct appimg_t *img;
	const struct bootopt_t *bootopt = (struct bootopt_t *)&_bootopt;
	unsigned int *app = (unsigned int *)&_app;
	unsigned int rom_start, rom_end;

	uart_init();
#ifdef DEBUG
	char t[10];
	itoa((int)bootopt, t, 16);
	uart_puts("BootOpt 0x");
	uart_puts(t);
	uart_puts("\r\n");
	itoa(bootopt->addr, t, 16);
	uart_puts("APP 0x");
	uart_puts(t);
	uart_puts("\r\n");
	itoa(bootopt->len, t, 10);
	uart_puts("Len ");
	uart_puts(t);
	uart_puts("\r\n");
#endif

	rom_start = (unsigned int)&_rom_start;
	rom_end = rom_start + (unsigned int)&_rom_size;

	if (bootopt->addr == (unsigned int)app) {
		// check len and hash
		// if not match retrieve it
		warn("bootopt does not match to the current app!");
	} else if (bootopt->addr >= rom_start && bootopt->addr < rom_end) {
		img = (struct appimg_t *)bootopt->addr;

		if (img->magic[0] == MAGIC1 &&
				img->magic[1] == MAGIC2 &&
				img->magic[2] == MAGIC3) {// &&
				//!memcmp(img->hash, bootopt->hash, 32)) {
			// RSApub(Hash) first before verify_hash()
			if (verify_hash(img->hash, img->data, img->len) == 0) {
				notice("Program new image");
				program(app, img);
				dsb();
				isb();
				update_bootopt(app, img);
				reboot();
			}
		} else {
			/* Here means new image may have been written but
			 * bootopt's not updated properly due to power lost
			 * during updating. So, run the current app after
			 * checking if valid, and let user do update process
			 * all over again. */
			warn("Updating suspended");
		}
	} else {
		/* reload bootopt from the current app
		 * don't reboot here just run the app after reloading
		 * otherwise infinite rebooting may occur when it reaches flash
		 * write endurance */
		warn("Invalid BootOpt. Trying to boot from old one");
		//verify_hash(hash(enc(app)));
	}

#ifdef DEBUG
	uart_puts("Run ");
	itoa((int)app, t, 16);
	uart_puts(t);
	uart_puts("\r\n\r\n");
#endif

	// check hash if modified
	((void (*)())app[1])();
	while (1);
}
