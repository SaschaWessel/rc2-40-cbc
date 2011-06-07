/* rc2-40-cbc brute force tool
 *
 * Copyright (C) 2011 Sascha Wessel <wessel@nefkom.net>
 *
 * Licensed under GPLv2
 */

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define INLINE static inline __attribute__((always_inline))

INLINE void rc2_keyschedule(unsigned short xkey[64], const unsigned char *key, unsigned len, unsigned bits);
INLINE void rc2_encrypt(const unsigned short xkey[64], const unsigned char *plain, unsigned char *cipher);
INLINE void rc2_decrypt(const unsigned short xkey[64], unsigned char *plain, const unsigned char *cipher);

#include "rc2.c"

INLINE void cbc(unsigned char *data, const unsigned char *prev)
{
	int i;

	for (i = 0; i < 8; i++)
		data[i] ^= prev[i];
}

INLINE int isvalid(unsigned char c)
{
	return ((0x09 <= c) && (c <= 0x0d))  /* isspace(c) \t\n\v\f\r */
	    || ((0x20 <= c) && (c <= 0x7e)); /* isprint(c) */
}

INLINE int check(const unsigned char *cipher, const unsigned char *iv, const unsigned char *key)
{
	unsigned short xkey[64];
	unsigned char plain[32];
	int i;

	rc2_keyschedule(xkey, key, 5, 40);

	rc2_decrypt(xkey, plain, cipher);
	cbc(plain, iv);
	for (i = 0; i < 8; i++)
		if (!isvalid(plain[i]))
			return 0;

	rc2_decrypt(xkey, plain + 8, cipher + 8);
	cbc(plain + 8, cipher);
	for (i = 8; i < 16; i++)
		if (!isvalid(plain[i]))
			return 0;

	rc2_decrypt(xkey, plain + 16, cipher + 16);
	cbc(plain + 16, cipher + 8);
	for (i = 16; i < 24; i++)
		if (!isvalid(plain[i]))
			return 0;

	rc2_decrypt(xkey, plain + 24, cipher + 24);
	cbc(plain + 24, cipher + 16);
	for (i = 24; i < 32; i++)
		if (!isvalid(plain[i]))
			return 0;

	printf("%6s: %02x%02x%02x%02x%02x  ", "Key",
			key[0], key[1], key[2], key[3], key[4]);

	for (i = 0; i < 32; i++)
		printf("%02x", plain[i]);

	printf("  |");

	for (i = 0; i < 32; i++)
		printf("%c", isprint(plain[i]) ? plain[i] : '.');

	printf("|\n");

	return 1;
}

static int scanhex(const char *name, unsigned n, const char *str, unsigned char *b)
{
	int i;

	for (i = 0; i < n; i++)
		if (sscanf(str + (i << 1), "%2hhx", b + i) != 1)
			return 1;

	printf("%6s: ", name);

	for (i = 0; i < n; i++)
		printf("%02x", b[i]);

	printf("\n");

	return 0;
}

static void stats(uint64_t min, uint64_t max, uint64_t current)
{
	static time_t begin = 0;
	time_t now;

	now = time(NULL);

	if (begin == 0) {
		begin = now;
	} else {
		uint64_t ela, eta;
		float percent, persec;
		unsigned ela_s, ela_m, ela_h, eta_s, eta_m, eta_h;

		ela = now - begin;
		eta = (ela * (max - min) / (current - min)) - ela;

		persec = (float) (current - min) / ela;
		percent = 100.0 * (float) (current - min) / (float) (max - min);

		ela_s = ela % 60;
		ela /= 60;
		ela_m = ela % 60;
		ela /= 60;
		ela_h = ela;

		eta_s = eta % 60;
		eta /= 60;
		eta_m = eta % 60;
		eta /= 60;
		eta_h = eta;

		printf("%6s: %010"PRIx64"  %3u:%02u:%02u  %7.3f%%  %.0f/s  "
				"-> ETA %3u:%02u:%02u\n", "Status", current,
				ela_h, ela_m, ela_s, percent,
				persec, eta_h, eta_m, eta_s);
	}
}

static void usage(const char *name)
{
	printf("Usage:   %s <iv> <cipher> [<key_min> <key_max>]\n", name);
	printf("Example: %s %s %s %s %s\n", name, "FEDCBA9876543210",
		"8AC497D81B21050DF0E4B4D5BA39DC0C3E8AF82B73FFC14038E465BCC37B0BDA",
		"0001020304", "0001020304");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	unsigned char iv[8], cipher[32], key[5];
	uint64_t k, key_min = 0, key_max = 0xffffffffff, found = 0;

	setbuf(stdout, NULL);

	if (argc != 5 && argc != 3)
		usage(argv[0]);

	if (scanhex("IV", 8, argv[1], iv))
		usage(argv[0]);

	if (scanhex("Cipher", 32, argv[2], cipher))
		usage(argv[0]);

	if (argc == 5) {
		key_min = strtoul(argv[3], NULL, 16);
		key_max = strtoul(argv[4], NULL, 16);
		key_min = MIN(key_min, 0xffffffffff);
		key_max = MIN(key_max, 0xffffffffff);
		key_max = MAX(key_min, key_max);
	}

	printf("%6s: %010"PRIx64" - %010"PRIx64"\n", "Range", key_min, key_max);

	for (k = key_min; k <= key_max; k++) {
		key[0] = 0xff & (k >> 32);
		key[1] = 0xff & (k >> 24);
		key[2] = 0xff & (k >> 16);
		key[3] = 0xff & (k >>  8);
		key[4] = 0xff & (k >>  0);

		if (check(cipher, iv, key))
			found++;

		if ((k & 0xffffff) == 0 || k == key_min || k == key_max)
			stats(key_min, key_max, k);
	}

	printf("%6s: %"PRIu64" keys\n", "Found", found);

	return EXIT_SUCCESS;
}
