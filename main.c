#include "bsp.h"
#include "flash.h"
#include "aes.h"
#include "sha256.h"
#include "uart.h"

#include <string.h>
#include <stdlib.h>

#define MAGIC1		0xDEC0ADDE
#define MAGIC2		0x23016745
#define MAGIC3		0xAB89EFCD

#define BUFSIZE		1024

struct appimg_t {
	const uint32_t magic[3];
	const uint32_t len;
	const uint8_t iv[16];
	const uint8_t hash[32];
	const uint8_t data[];
} __attribute__((packed, aligned(4)));

void reboot()
{
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

static void program()
{
	extern char _aes_key;
	struct AES_ctx ctx;
	const uint8_t *key = (const uint8_t *)&_aes_key;
	struct appimg_t *img = (struct appimg_t *)0x08020000;
	uint8_t buf[BUFSIZE];

	verify_hash(img->hash, img->data, img->len);

	AES_init_ctx_iv(&ctx, key, img->iv);

	memcpy(buf, img->data, BUFSIZE);
	AES_CTR_xcrypt_buffer(&ctx, buf, BUFSIZE);

#if 0
	char t[10];

uart_puts("-----------------------");
uart_put('\r');
uart_put('\n');
	for (int i = 0; i < BUFSIZE; i++) {
		if (!(i % 16)) {
			uart_put('\r');
			uart_put('\n');
		}

		itoa(buf[i], t, 16);
		for (int j = 0; t[j]; j++)
			uart_put(t[j]);
		uart_put(' ');
	}
	///////////////////
	memcpy(buf, &img->data[BUFSIZE], BUFSIZE);
	AES_CTR_xcrypt_buffer(&ctx, buf, BUFSIZE);
uart_put('\r');
uart_put('\n');
	for (int i = 0; i < BUFSIZE; i++) {
		if (!(i % 16)) {
			uart_put('\r');
			uart_put('\n');
		}

		itoa(buf[i], t, 16);
		for (int j = 0; t[j]; j++)
			uart_put(t[j]);
		uart_put(' ');
	}
#endif
}

void main()
{
	extern char _app, _bootopt, _rom_start, _rom_size;
	struct appimg_t *img;
	unsigned int *app = (unsigned int *)&_app;
	unsigned int *bootopt = (unsigned int *)&_bootopt;
	unsigned int addr, len, start, end;

	//uart_init();

	start = (unsigned int)&_rom_start;
	end = start + (unsigned int)&_rom_size;
	addr = (unsigned int)bootopt[0];
	len = (unsigned int)bootopt[1];

addr = 0x08020000; // test
(void)len;
	if (addr == (unsigned int)app) {
		// check len and hash
		// if not match retrieve it
	} else if (addr >= start && addr < end) {
		img = (struct appimg_t *)addr;
		// if p[0] >= ram_start && p[0] < ram_end then it's stack pointer
		// run &_app after checking if current app is valid
		if (img->magic[0] == MAGIC1 &&
				img->magic[1] == MAGIC2 &&
				img->magic[2] == MAGIC3) {
			//verify_image();
			//  - verify_hash();
			program();
			//update_bootopt();
		}
	} else {
		// reload bootopt from the current app
		// don't reboot here just run the app after reloading
	}

#if 0
	flash_program((void * const)0x08018000, (const void * const)p, 1024);

if (*(unsigned int *)0x08018000 == 0xffffffff)
	reboot();
#endif

	//setsp(app[0]);
	((void (*)())app[1])();
}
