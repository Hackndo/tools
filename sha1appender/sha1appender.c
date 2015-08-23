#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/sha.h>

#include "getopthelp.h"
#include "misc.h"




static struct goh_option opt_desc[] = {
	{"min-length", 'm', GOH_ARG_REQUIRED, 'm',
		"Minimum data length tried."},
	{"max-length", 'M', GOH_ARG_REQUIRED, 'M',
		"Maximum data length tried."},
	{"append-hex", 'x', GOH_ARG_REFUSED, 'x',
		"The string to append is given in hexadecimal."},
};



void printhash(unsigned char *md) {
	size_t i;
	for (i = 0; i < SHA_DIGEST_LENGTH; i++)
		printf("%02x", md[i]);
}



void printhashnl(unsigned char *md) {
	printhash(md);
	printf("\n");
}


void decode_append(char *append, size_t *appendlen, int appendhex) {
	size_t i;

	/* Hex decoding "4142" is "AB" */
	if (appendhex) {
		*appendlen /= 2;
		for (i = 0; i < *appendlen; i++) {
			unsigned x;
			sscanf(&append[2 * i], "%02x", &x);
			append[i] = x;
		}
	}
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
	struct goh_state st;
	int opt;
	const char *prefixhash;
	char *append;
	size_t appendlen;
	int appendhex = 0;
	size_t minlen = 0, maxlen = SHA_CBLOCK;


	/* Options parsing */
	goh_init(&st, opt_desc, ARRAY_LENGTH(opt_desc), argc, argv, 1);
	st.usagehelp = "[options] sha1(prefix) hex(append)\n";

	while ((opt = goh_nextoption(&st)) >= 0) {
		switch (opt) {
		case 'm':
			minlen = strtol(st.argval, NULL, 0);
			break;

		case 'M':
			maxlen = strtol(st.argval, NULL, 0);
			break;

		case 'x':
			appendhex = 1;
			break;

		default:
			custom_error("Option declared but not handled");
		}
	}

	/* Common command line error */
	if (st.argidx + 2 != argc) {
		custom_warn("Missing mandatory arguments");
		goh_printhelp(&st, stderr);
		return EXIT_FAILURE;
	}

	prefixhash = argv[st.argidx];
	append = argv[st.argidx + 1];

	goh_fini(&st);


	/* Command line checking and conversion */

	if (strlen(prefixhash) != 2 * SHA_DIGEST_LENGTH)
		custom_error("sha1(prefix) must be %d hex characters long", 2 * SHA_DIGEST_LENGTH);

	appendlen = strlen(append);

	if (appendlen % 2 != 0)
		custom_error("hex(append) must have an even length");

	/* Decode the append string */
	decode_append(append, &appendlen, appendhex);

	sha1append(prefixhash, append, appendlen, minlen, maxlen);

	return EXIT_SUCCESS;
}
