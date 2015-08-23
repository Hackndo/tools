#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/sha.h>

#include "getopthelp.h"
#include "misc.h"



enum decode {
	DECODE_RAW,
	DECODE_HEX,
	DECODE_ECHO
};



static struct goh_option opt_desc[] = {
	{"min-length", 'm', GOH_ARG_REQUIRED, 'm',
		"Minimum data length tried."},
	{"max-length", 'M', GOH_ARG_REQUIRED, 'M',
		"Maximum data length tried."},
	{"append-decode", 'd', GOH_ARG_REQUIRED, 'd',
		"How the append string is decoded. Valid values are raw, hex and echo (the default)."}
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



size_t decode_append(char *append, enum decode appenddecode) {
	size_t inlen;
	size_t i, o;
	unsigned x;

	inlen = strlen(append);

	if (appenddecode == DECODE_RAW)
		return inlen;

	/* Additional requirement for DECODE_HEX */
	if (appenddecode == DECODE_HEX && inlen % 2 != 0)
		custom_error("The append string must have an even length to hex-decode it.");

	for (i = 0, o = 0; i < inlen; o++) {
		int n = 0;

		/* Special case for DECODE_HEX */
		if (appenddecode == DECODE_HEX) {
			sscanf(&append[i], "%02x%n", &x, &n);
			if (n != 2)
				custom_error("Error while hex-decoding %s", &append[i]);
			append[o] = x;
			i += 2;
			continue;
		}

		if (append[i] != '\\') {
			append[o] = append[i];
			i++;
			continue;
		}

		switch (append[i + 1]) {
		case 'a':
			append[o] = '\x07';
			i += 2;
			break;
		case 'b':
			append[o] = '\x08';
			i += 2;
			break;
		case 't':
			append[o] = '\x09';
			i += 2;
			break;
		case 'n':
			append[o] = '\x0a';
			i += 2;
			break;
		case 'v':
			append[o] = '\x0b';
			i += 2;
			break;
		case 'f':
			append[o] = '\x0c';
			i += 2;
			break;
		case 'r':
			append[o] = '\x0d';
			i += 2;
			break;
		case 'e':
			append[o] = '\x1b';
			i += 2;
			break;
		case 'x':
			sscanf(&append[i + 2], "%02x%n", &x, &n);
			append[o] = x;
			i += 2 + n;
			break;
		case '0':
			sscanf(&append[i + 2], "%03o%n", &x, &n);
			append[o] = x;
			i += 2 + n;
			break;

		case '\\':
		case '\0':
		default:
			append[o] = '\\';
			i += 2;
			break;
		}
	}

	append[o] = '\0';

	return o;
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
	enum decode appenddecode = DECODE_ECHO;
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

		case 'd':
			if (strcmp(st.argval, "raw") == 0)
				appenddecode = DECODE_RAW;
			else if (strcmp(st.argval, "hex") == 0)
				appenddecode = DECODE_HEX;
			else if (strcmp(st.argval, "echo") == 0)
				appenddecode = DECODE_ECHO;
			else
				custom_error("%s not a valid value for --append-decode", st.argval);
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


	/* Command line checking */
	if (strlen(prefixhash) != 2 * SHA_DIGEST_LENGTH)
		custom_error("sha1(prefix) must be %d hex characters long", 2 * SHA_DIGEST_LENGTH);

	/* Decode the append string */
	appendlen = decode_append(append, appenddecode);

	sha1append(prefixhash, append, appendlen, minlen, maxlen);

	return EXIT_SUCCESS;
}
