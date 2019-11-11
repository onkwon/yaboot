#include "bsp.h"
#include "flash.h"
#include "tinycrypt/sha256.h"
#include "tinycrypt/ecc_dsa.h"
#include "tinycrypt/ctr_mode.h"
#include "tinycrypt/aes.h"
#include "uart.h"

#include <string.h>
#include <stdlib.h>

#define MAGIC1				0xDEC0ADDE
#define MAGIC2				0xDEC1ADDE
#define MAGIC3				0xDEC2ADDE

#define HASH_SIZE			64
#define INITIAL_VECTOR_SIZE		16

#define error(msg)			uart_puts("ERROR : "msg"\r\n")
#define warn(msg)			uart_puts("WARN  : "msg"\r\n")
#define notice(msg)			uart_puts("NOTICE: "msg"\r\n")

struct appimg_t {
	const uint32_t magic[3];
	const size_t len;
	const uint8_t iv[INITIAL_VECTOR_SIZE];
	union {
		struct {
			const uint8_t r[32];
			const uint8_t s[32];
		} ecdsa;
		const uint8_t hash[HASH_SIZE];
	};
	const uint8_t data[];
} __attribute__((packed, aligned(4)));

struct bootopt_t {
	const uintptr_t addr;
	const size_t len;
	union {
		struct {
			const uint8_t r[32];
			const uint8_t s[32];
		} ecdsa;
		const uint8_t hash[HASH_SIZE];
	};
	const uint8_t iv[INITIAL_VECTOR_SIZE];
} __attribute__((packed, aligned(4)));

extern char _sector_size;

static void reboot(void)
{
	dsb();
	isb();

#define VECTKEY		0x5fa
	SCB_AIRCR = (VECTKEY << 16)
		| (SCB_AIRCR & (7 << 8)) /* keep priority group unchanged */
		| (1 << 2); /* system reset request */
}

static int verify(const uint8_t *signature, const uint8_t *data, uint32_t len,
		const void *eckey)
{
	const uint8_t *pubkey = eckey;
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
		error("Public key is not valid");
	if (!uECC_verify(pubkey, digest, sizeof(digest), signature, uECC_secp256r1())) {
		error("Verify failed");
		return -1;
	}

	return 0;
}

static int verify_enc(const uint8_t *signature, const uint8_t *data, uint32_t len,
		const void *eckey, const void *aeskey, const void *aesiv)
{
	struct tc_aes_key_sched_struct ctx;
	uint8_t buf[(int)&_sector_size], iv[INITIAL_VECTOR_SIZE];
	int size;
	const uint8_t *pubkey = eckey;
	const uint8_t *key = aeskey;

	struct tc_sha256_state_struct sha256_ctx;
	uint8_t digest[TC_SHA256_DIGEST_SIZE];

	notice("Verify(E)");

	tc_aes128_set_encrypt_key(&ctx, key);
	memcpy(iv, aesiv, sizeof(iv));
	tc_sha256_init(&sha256_ctx);

	for (uint32_t i = 0; i < len; i += (uint32_t)&_sector_size) {
		size = ((len - i) < (uint32_t)&_sector_size)?
			len - i : (uint32_t)&_sector_size;
		tc_ctr_mode(buf, size, &data[i], size, iv, &ctx);

		tc_sha256_update(&sha256_ctx, buf, size);
	}

	tc_sha256_final(digest, &sha256_ctx);

	if (uECC_valid_public_key(pubkey, uECC_secp256r1()) != 0)
		error("Public key is not valid");
	if (!uECC_verify(pubkey, digest, sizeof(digest), signature, uECC_secp256r1())) {
		error("Verify failed");
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

static void program(void *addr, const struct appimg_t *img, const void *aeskey)
{
	struct tc_aes_key_sched_struct ctx;
	uint8_t buf[(int)&_sector_size], iv[INITIAL_VECTOR_SIZE];
	int size;
	uint8_t *d = (uint8_t *)addr;
	const uint8_t *key = (const uint8_t *)aeskey;

	tc_aes128_set_encrypt_key(&ctx, key);
	memcpy(iv, img->iv, sizeof(img->iv));

	for (uint32_t i = 0; i < img->len; i += (uint32_t)&_sector_size) {
		size = ((img->len - i) < (uint32_t)&_sector_size)?
			img->len - i : (uint32_t)&_sector_size;
		tc_ctr_mode(buf, size, &img->data[i], size, iv, &ctx);
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

static void update_bootopt(void *dest, void *addr, const struct appimg_t *img)
{
	unsigned int buf[22];

	buf[0] = (unsigned int)addr;
	buf[1] = img->len;
	memcpy(&buf[2], img->hash, HASH_SIZE);
	memcpy(&buf[18], img->iv, INITIAL_VECTOR_SIZE);

	flash_program(dest, buf, 22 * 4);
}

static inline struct appimg_t *get_app_header(const struct bootopt_t *bootopt,
		unsigned int rom_end)
{
	unsigned int *p = (unsigned int *)bootopt->addr;

	for (; (unsigned int)p < rom_end; p++) {
		if (p[0] == MAGIC1 && p[1] == MAGIC2 && p[2] == MAGIC3)
			return (struct appimg_t *)p;
	}

	return NULL;
}

static inline void freeze(void)
{
	error("Freeze");
	while (1);
}

void main(void)
{
	extern char _pubkey, _aeskey, _rom_start, _rom_size;
	extern struct bootopt_t _bootopt;
	extern uintptr_t _app;

	const struct bootopt_t *bootopt;
	const struct appimg_t *img;
	uintptr_t *app;
	uintptr_t rom_start, rom_end;

	bootopt = (struct bootopt_t *)&_bootopt;
	app = (uintptr_t *)&_app;
	img = NULL;

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

	if (bootopt->addr != (uintptr_t)app &&
			bootopt->addr >= rom_start && bootopt->addr < rom_end) {
		img = (struct appimg_t *)bootopt->addr;

		if (img->magic[0] == MAGIC1 &&
				img->magic[1] == MAGIC2 &&
				img->magic[2] == MAGIC3 &&
				!memcmp(img->hash, bootopt->hash, HASH_SIZE) &&
				!verify(img->hash, img->data, img->len, &_pubkey) &&
				/* FIXME: Include meta and align by sector size */
				(unsigned int)app + img->len < (unsigned int)img) {
			notice("Program new image");
			program(app, img, &_aeskey);
			dsb();
			isb();
			update_bootopt(&_bootopt, app, img);
			reboot();
		} else {
			/* Here means new image may have been written but
			 * bootopt's not updated properly due to power lost
			 * during updating. So, run the current app after
			 * checking if valid, and let user do update process
			 * all over again. */
			warn("Updating suspended");
		}
	}

	if ((img = get_app_header(bootopt, rom_end)) == NULL)
		freeze();

	if (img->len == bootopt->len &&
			!memcmp(bootopt->hash, img->hash, HASH_SIZE) &&
			!memcmp(bootopt->iv, img->iv, INITIAL_VECTOR_SIZE))
		goto out;

	warn("bootopt does not match to the current app!");
	if (verify_enc(img->hash, (const uint8_t *)app, img->len,
				&_pubkey, &_aeskey, img->iv))
		freeze();
	update_bootopt(&_bootopt, app, img);
	/* NOTE: Do not reboot here but just run the app after updating
	 * bootopt. Otherwise infinite rebooting may occur when it
	 * reaches flash write endurance */
	//reboot();

out:
#ifndef QUICKBOOT
	if (verify_enc(bootopt->hash, (const uint8_t *)bootopt->addr,
				bootopt->len, &_pubkey, &_aeskey, bootopt->iv)) {
		warn("program may be modified");
		freeze();
	}
#endif

#ifdef DEBUG
	uart_puts("Run     0x");
	itoa((int)app, t, 16);
	uart_puts(t);
	uart_puts("\r\n\r\n");
#endif
	((void (*)())app[1])();
}
