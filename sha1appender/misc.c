#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "misc.h"


/* Ugly hack for C89 conformance. May not work everywhere. */
#ifndef va_copy
# ifdef __va_copy
#  define va_copy(a,b) __va_copy((a),(b))
# else
#  define va_copy(a,b) ((a) = (b))
# endif
#endif


void system_error(const char *msg) {
	perror(msg);
	abort();
}



void custom_warn(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}



/* TODO: factor with custom_warn */
void custom_error(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	abort();
}



int asprintf(char **str, const char *format, ...) {
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = avsprintf(str, format, ap);
	va_end(ap);

	return ret;
}



int avsprintf(char **str, const char *format, va_list ap) {
	size_t size;
	FILE *devnull;
	int err;
	va_list aq;

	va_copy(aq, ap);

	devnull = fopen("/dev/null", "w");
	if (devnull == NULL)
		system_error("/dev/null");

	size = vfprintf(devnull, format, ap);

	err = fclose(devnull);
	if (err != 0)
		system_error("close(\"/dev/null\")");

	/* size++ for the final \0. */
	size++;
	*str = malloc(size * sizeof(char));
	if (*str == NULL)
		system_error("malloc");

	size = vsprintf(*str, format, aq);
	return size;
}
