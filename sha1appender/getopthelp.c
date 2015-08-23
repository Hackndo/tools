#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "misc.h"
#include "getopthelp.h"


static struct option gnuhelp = {"help", no_argument, NULL, 'h'};
static struct goh_option gohhelp = {"help", 'h', GOH_ARG_REFUSED, 'h',
                                    "Print this help message."};


static void gnu_option_from_goh_option(struct option *go,
                                       const struct goh_option *o) {
	if (o->arg == GOH_ARG_REQUIRED)
		go->has_arg = required_argument;
	else if (o->arg == GOH_ARG_OPTIONAL)
		go->has_arg = optional_argument;
	else
		go->has_arg = no_argument;

	go->name = o->name;
	go->flag = NULL;
	go->val = o->id;
}



static struct option *build_gnu_options(const struct goh_option *opts,
                                        size_t cnt, int autohelp) {

	struct option *gnuopts;
	size_t allocopts_sz;
	const struct goh_option *o;
	struct option *go;

	/* We will copy cnt args + the last NULL one.
	 * +1 if we add the autohelp option. */
	allocopts_sz = sizeof(*gnuopts);
	if (autohelp)
		allocopts_sz *= cnt + 2;
	else
		allocopts_sz *= cnt + 1;

	gnuopts = malloc(allocopts_sz);
	if (gnuopts == NULL)
		system_error("malloc");

	go = gnuopts;
	for (o = opts; o < opts + cnt; o++) {
		gnu_option_from_goh_option(go, o);
		go++;
	}

	if (autohelp) {
		*go = gnuhelp;
		go++;
	}

	/* Zero out the last struct. */
	memset(go, 0, sizeof(*go));

	return gnuopts;
}



static char *build_shortopt(const struct goh_option *opts, size_t cnt,
                            int autohelp) {
	char *shortopt, *ptr;
	size_t shortopt_sz;
	const struct goh_option *o;

	/* At least one char for the \0. */
	shortopt_sz = 1;
	for (o = opts; o < opts + cnt; o++) {
		if (o->abbr != '\0')
			shortopt_sz++;

		if (o->arg == GOH_ARG_REQUIRED)
			shortopt_sz++;
		else if (o->arg == GOH_ARG_OPTIONAL)
			shortopt_sz += 2;
	}

	if (autohelp)
		shortopt_sz++;

	shortopt = malloc(shortopt_sz);
	if (shortopt == NULL)
		system_error("malloc");


	/* Now we have the buffer, fill it! */
	memset(shortopt, 0, shortopt_sz);

	ptr = shortopt;
	for (o = opts; o < opts + cnt; o++) {
		if (o->abbr != '\0')
			*ptr++ = o->abbr;

		/* REQUIRED or OPTIONAL need at least one colon
		 * OPTIONAL needs a second colon. */
		if (o->arg != GOH_ARG_REFUSED)
			*ptr++ = ':';

		if (o->arg == GOH_ARG_OPTIONAL)
			*ptr++ = ':';
	}

	if (autohelp)
		*ptr++ = 'h';


	return shortopt;
}



static void options_check(const struct goh_option *opts, size_t cnt) {
	size_t i, j;

	for (i = 0; i < cnt; i++) {
		const struct goh_option *a = &opts[i];
		for (j = i + 1; j < cnt; j++) {
			const struct goh_option *b = &opts[j];

			if (a->abbr != '\0' && a->abbr == b->abbr)
				custom_error("getopthelp: Short option %c "
				             "defined twice", a->abbr);

			if (a->id == b->id)
				custom_error("getopthelp: Option id %d defined "
				             "twice", a->id);

			if (strcmp(a->name, b->name) == 0)
				custom_error("getopthelp: Long option name `%s'"
				             " defined twice", a->name);
		}
	}
}



void goh_init(struct goh_state *state, const struct goh_option *opts, size_t cnt,
              int argc, char *const *argv, int autohelp) {

	memset(state, 0, sizeof(*state));

	options_check(opts, cnt);

	state->opts = opts;
	state->optcnt = cnt;
	state->argc = argc;
	state->argv = argv;
	state->autohelp = autohelp;

	/* Build the struct option* to used for getopt */
	state->gnuopts = build_gnu_options(opts, cnt, autohelp);

	/* Compute the length of shortopt string before building it */
	state->shortopt = build_shortopt(opts, cnt, autohelp);

	state->usagehelp = "[options] ...";
}



void goh_fini(struct goh_state *state) {
	free(state->gnuopts);
	free(state->shortopt);
}



int goh_nextoption(struct goh_state *state) {
	const struct goh_option *o;
	int idx = -1;
	int val;


	val = getopt_long(state->argc, state->argv, state->shortopt,
	                      state->gnuopts, &idx);

	if (state->autohelp && val == 'h') {
		goh_printhelp(state, stdout);
		goh_fini(state);
		exit(EXIT_SUCCESS);
	}

	state->argidx = optind;
	state->argval = optarg;

	/* getopt_long return -1 at the end of the options. */
	if (val == -1)
		return -1;

	/* When an option is found and idx is set, it's a long option. */
	if (idx != -1)
		return state->opts[idx].id;

	/* When val == -1 and idx != -1, it's a short option */
	for (o = state->opts; o < state->opts + state->optcnt; o++) {
		if (val == o->abbr)
			return o->id;
	}

	if (state->autohelp) {
		goh_printhelp(state, stderr);
		goh_fini(state);
		exit(EXIT_FAILURE);
	}

	return val;
}



/* This function is tightly coupled with printhelp_option.
 * Factor them? */
static size_t printhelp_optwidth(const struct goh_option *o) {
	size_t thiscolwidth = 4;

	if (o->abbr != '\0')
		thiscolwidth += 2;

	if (o->abbr != '\0' && o->name != NULL)
		thiscolwidth += 2;

	if (o->name != NULL)
		thiscolwidth += 2 + strlen(o->name);

	if (o->arg == GOH_ARG_REQUIRED)
		thiscolwidth += 4;
	else if (o->arg == GOH_ARG_OPTIONAL)
		thiscolwidth += 6;

	return thiscolwidth;
}



/* Return the number of char str to put on a line so that it wrap on width
 * characaters and cut on a space. */
static size_t word_wrap_len(const char *str, size_t width) {
	const char *p = str;
	const char *lastspace = NULL;

	while (*p) {
		if (*p == ' ')
			lastspace = p;

		/* This will break on the last space before the limit if it
		 * exists. Or on the first space just after the limit. */
		if (p >= str + width && lastspace != NULL)
			return lastspace - str;

		p++;
	}

	/* Print everything if we reached the \0. */
	return p - str;
}



static void printhelp_option(const struct goh_option *o, size_t width,
                             FILE *stream) {
	size_t colcnt = 0;
	const char *helpstr = o->help;

	/* Print the options */
	colcnt += fprintf(stream, "  ");

	if (o->abbr != '\0')
		colcnt += fprintf(stream, "-%c", o->abbr);

	if (o->abbr != '\0' && o->name != NULL)
		colcnt += fprintf(stream, ", ");

	if (o->name != NULL)
		colcnt += fprintf(stream, "--%s", o->name);

	if (o->arg == GOH_ARG_REQUIRED)
		colcnt += fprintf(stream, " arg");
	else if (o->arg == GOH_ARG_OPTIONAL)
		colcnt += fprintf(stream, " [arg]");

	colcnt += fprintf(stream, "  ");


	/* Wrap the text to print. */
	do {
		size_t helplen;

		/* Print the spaces to align the help text */
		while (colcnt < width)
			colcnt += fprintf(stream, " ");

		helplen = word_wrap_len(helpstr, 80 - width);

		fprintf(stream, "%.*s\n", (int)helplen, helpstr);

		helpstr += helplen;
		if (*helpstr == ' ')
			helpstr++;

		colcnt = 0;
	} while (*helpstr != '\0');
}



void goh_printhelp(const struct goh_state *state, FILE *stream) {
	const struct goh_option *o;
	size_t optcolwidth = 0;

	/* Compute the width of the column to print options */
	for (o = state->opts; o < state->opts + state->optcnt; o++) {
		size_t w = printhelp_optwidth(o);

		if (w > optcolwidth)
			optcolwidth = w;
	}

	if (state->autohelp) {
		size_t w = printhelp_optwidth(&gohhelp);
		if (w > optcolwidth)
			optcolwidth = w;
	}

	/* We'll have output not completely aligned... but it's ok. */
	if (optcolwidth > 30)
		optcolwidth = 30;

	/* Now print everything. */
	fprintf(stream, "usage: %s %s\n", state->argv[0], state->usagehelp);

	for (o = state->opts; o < state->opts + state->optcnt; o++)
		printhelp_option(o, optcolwidth, stream);

	if (state->autohelp)
		printhelp_option(&gohhelp, optcolwidth, stream);
}
