#include "bsp.h"
#include "flash.h"
#include "aes.h"
#include "tinycrypt/sha256.h"
#include "tinycrypt/ecc_dsa.h"
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
	union {
		struct {
			const uint8_t r[32];
			const uint8_t s[32];
		} ecdsa;
		const uint8_t hash[64];
	};
	const uint8_t data[];
} __attribute__((packed, aligned(4)));

struct bootopt_t {
	const uint32_t addr;
	const uint32_t len;
	union {
		struct {
			const uint8_t r[32];
			const uint8_t s[32];
		} ecdsa;
		const uint8_t hash[64];
	};
	const uint8_t iv[16];
} __attribute__((packed, aligned(4)));

extern char _pubkey, _aeskey, _sector_size, _bootopt, _app, _rom_start, _rom_size;

void reboot()
{
	dsb();
	isb();

#define VECTKEY		0x5fa
	SCB_AIRCR = (VECTKEY << 16)
		| (SCB_AIRCR & (7 << 8)) /* keep priority group unchanged */
		| (1 << 2); /* system reset request */
}

static int verify(const uint8_t *signature, const uint8_t *data, uint32_t len)
{
	const uint8_t *pubkey = (const uint8_t *)&_pubkey;
	struct tc_sha256_state_struct sha256_ctx;
	uint8_t digest[TC_SHA256_DIGEST_SIZE];

	notice("Verify");

	tc_sha256_init(&sha256_ctx);
	tc_sha256_update(&sha256_ctx, data, len);
	tc_sha256_final(digest, &sha256_ctx);

#ifdef DEBUG
	char t[10];
	uart_puts("SHA256: ");
	for (int i = 0; i < TC_SHA256_DIGEST_SIZE; i++) {
		itoa(digest[i], t, 16);
		uart_puts(t);
	}
	uart_puts("\r\n");
#endif

	if (uECC_valid_public_key(pubkey, uECC_secp256r1()) != 0)
		warn("Public key is not valid");
	if (!uECC_verify(pubkey, digest, sizeof(digest), signature, uECC_secp256r1())) {
		warn("Verify failed");
		return -1;
	}

	return 0;
}

static int verify_enc(const uint8_t *signature, const uint8_t *data, uint32_t len)
{
	struct AES_ctx ctx;
	uint8_t buf[(int)&_sector_size];
	int size;
	const uint8_t *pubkey = (const uint8_t *)&_pubkey;
	const uint8_t *key = (const uint8_t *)&_aeskey;
	const struct bootopt_t *bootopt = (struct bootopt_t *)&_bootopt;

	struct tc_sha256_state_struct sha256_ctx;
	uint8_t digest[TC_SHA256_DIGEST_SIZE];

	notice("Verify");

	AES_init_ctx_iv(&ctx, key, bootopt->iv);
	tc_sha256_init(&sha256_ctx);

	for (uint32_t i = 0; i < len; i += (uint32_t)&_sector_size) {
		size = ((len - i) < (uint32_t)&_sector_size)?
			len - i : (uint32_t)&_sector_size;
		memcpy(buf, &data[i], size);
		AES_CTR_xcrypt_buffer(&ctx, buf, size);

		tc_sha256_update(&sha256_ctx, buf, size);
	}

	tc_sha256_final(digest, &sha256_ctx);

	if (uECC_valid_public_key(pubkey, uECC_secp256r1()) != 0)
		warn("Public key is not valid");
	if (!uECC_verify(pubkey, digest, sizeof(digest), signature, uECC_secp256r1())) {
		warn("Verify failed");
		return -1;
	}

	return 0;
}

#if 0
static int verify_hash(const uint8_t *hash, const uint8_t *data, size_t len)
{
	struct tc_sha256_state_struct sha256_ctx;
	uint8_t result[TC_SHA256_DIGEST_SIZE];
	int q, r, i;

	q = len / TC_SHA256_DIGEST_SIZE;
	r = len % TC_SHA256_DIGEST_SIZE;

	tc_sha256_init(&sha256_ctx);
	for (i = 0; i < q; i++)
		tc_sha256_update(&sha256_ctx, &data[i * TC_SHA256_DIGEST_SIZE], TC_SHA256_DIGEST_SIZE);
	if (r)
		tc_sha256_update(&sha256_ctx, &data[i * TC_SHA256_DIGEST_SIZE], r);
	tc_sha256_final(result, &sha256_ctx);

#ifdef DEBUG
	char t[10];
	uart_puts("SHA256: ");
	for (i = 0; i < TC_SHA256_DIGEST_SIZE; i++) {
		itoa(result[i], t, 16);
		uart_puts(t);
	}
	uart_puts("\r\n");
#endif

	return memcmp(hash, result, 32);
}
#endif

static void program(void *addr, const struct appimg_t *img)
{
	struct AES_ctx ctx;
	uint8_t buf[(int)&_sector_size];
	int size;
	uint8_t *d = (uint8_t *)addr;
	const uint8_t *key = (const uint8_t *)&_aeskey;

	AES_init_ctx_iv(&ctx, key, img->iv);

	for (uint32_t i = 0; i < img->len; i += (uint32_t)&_sector_size) {
		size = ((img->len - i) < (uint32_t)&_sector_size)?
			img->len - i : (uint32_t)&_sector_size;
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
	flash_program(d, (const void * const)img, (int)img->data - (int)img);
}

static void update_bootopt(void *addr, const struct appimg_t *img)
{
	unsigned int buf[22];

	buf[0] = (unsigned int)addr;
	buf[1] = img->len;
	memcpy(&buf[2], img->hash, 64);
	memcpy(&buf[18], img->iv, 16);

	flash_program((void *)&_bootopt, buf, 22 * 4);
}

int __attribute__((weak)) default_CSPRNG(uint8_t *dest, unsigned int size)
{
	return 0;
	(void)dest;
	(void)size;
}

void main()
{
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
	uart_puts("APP     0x");
	uart_puts(t);
	uart_puts("\r\n");
	itoa(bootopt->len, t, 10);
	uart_puts("Len         ");
	uart_puts(t);
	uart_puts("\r\n");
#endif

	rom_start = (unsigned int)&_rom_start;
	rom_end = rom_start + (unsigned int)&_rom_size;

	if (bootopt->addr == (unsigned int)app) {
		// check len and hash
		// if not match retrieve it
	} else if (bootopt->addr >= rom_start && bootopt->addr < rom_end) {
		const struct appimg_t *img = (struct appimg_t *)bootopt->addr;

		if (img->magic[0] == MAGIC1 &&
				img->magic[1] == MAGIC2 &&
				img->magic[2] == MAGIC3 &&
				!memcmp(img->hash, bootopt->hash, 64) &&
				!verify(img->hash, img->data, img->len)) {
			notice("Program new image");
			program(app, img);
			dsb();
			isb();
			update_bootopt(app, img);
			reboot();
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

	if (verify_enc(bootopt->hash, (const uint8_t *)bootopt->addr, bootopt->len)) {
		warn("bootopt does not match to the current app!");
		while (1);
	}

#ifdef DEBUG
	uart_puts("Run     0x");
	itoa((int)app, t, 16);
	uart_puts(t);
	uart_puts("\r\n\r\n");
#endif
	((void (*)())app[1])();
}
