#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/sha.h>



void printhash(unsigned char *md) {
	size_t i;
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		printf("%02x", md[i]);
}



void printhashnl(unsigned char *md) {
	printhash(md);
	printf("\n");
}



void sha1append(const char *prefixhash, const char *append, size_t appendlen,
		size_t minlen, size_t maxlen) {

	SHA_CTX ctx;
	unsigned char md[SHA_DIGEST_LENGTH];
	size_t i;
	uint64_t datasize;


	/* Prepare (part of) the SHA context */
	SHA1_Init(&ctx);

	sscanf(prefixhash + 0, "%08x", &ctx.h0);
	sscanf(prefixhash + 8, "%08x", &ctx.h1);
	sscanf(prefixhash + 16, "%08x", &ctx.h2);
	sscanf(prefixhash + 24, "%08x", &ctx.h3);
	sscanf(prefixhash + 32, "%08x", &ctx.h4);

	for (datasize = minlen; datasize <= maxlen; datasize++) {
		SHA_CTX c = ctx;
		uint64_t nblock = (datasize + 8 + 1 + SHA_CBLOCK - 1) / SHA_CBLOCK;
		uint64_t datasizebits = datasize * 8;
		uint64_t datapadsize = nblock * SHA_CBLOCK;
		uint64_t datapadsizebits = datapadsize * 8;
		uint64_t padzerosize = datapadsize - datasize - 8 - 1;
		unsigned char *p;

		/* Compute the final hash if the prefix data is i bytes long */
		c.Nh = datapadsizebits >> 32;
		c.Nl = datapadsizebits & 0xffffffffUL;

		SHA1_Update(&c, append, appendlen);
		SHA1_Final(md, &c);


		printf("%llu: ", datasize);
		printhash(md);
		printf(" ");


		/* Print the padding */
		printf("\\x80");
		for (i = 0; i < padzerosize; i++)
			printf("\\x00");

		/* Big endian datasize */
		p = (unsigned char *)&datasizebits;
		for (i = 0; i < sizeof(datasizebits); i++)
			printf("\\x%02x", p[sizeof(datasizebits) - 1 - i]);

		for (i = 0; i < appendlen; i++)
			printf("\\x%02x", append[i]);
		printf("\n");
	}
}



int main(int argc, char **argv) {
	const char *prefixhash;
	char *append;
	size_t appendlen;
	size_t minlen, maxlen;
	size_t i;

	if (argc < 3 || argc > 5) {
		fprintf(stderr, "usage: %s sha1(prefix) hex(append) [[minlen] maxlen]\n", argv[0]);
		return EXIT_FAILURE;
	}

	prefixhash = argv[1];
	append = argv[2];

	if (argc == 3) {
		minlen = 0;
		maxlen = SHA_CBLOCK;
	} else if (argc == 4) {
		minlen = 0;
		maxlen = strtol(argv[3], NULL, 0);
	} else if (argc == 5) {
		minlen = strtol(argv[3], NULL, 0);
		maxlen = strtol(argv[4], NULL, 0);
	}

	if (strlen(prefixhash) != 2 * SHA_DIGEST_LENGTH) {
		fprintf(stderr, "sha1(prefix) must be 40 hex chars\n");
		return EXIT_FAILURE;
	}

	appendlen = strlen(append);

	if (appendlen % 2 != 0) {
		fprintf(stderr, "hex(append) must have an even length\n");
		return EXIT_FAILURE;
	}

	/* Decode the hex of append */
	appendlen /= 2;
	for (i = 0; i < appendlen; i++) {
		unsigned x;
		sscanf(&append[2 * i], "%02x", &x);
		append[i] = x;
	}

	sha1append(prefixhash, append, appendlen, minlen, maxlen);

	return EXIT_SUCCESS;
}
