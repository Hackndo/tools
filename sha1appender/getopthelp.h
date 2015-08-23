#ifndef GETOPTHELP__
#define GETOPTHELP__

#include <stdio.h>
#include <getopt.h>
#include <stdint.h>

/*
 * This module is basically a getopt_long wrapper that:
 * - can generate an output for --help automatically;
 * - handle the short and long options at once;
 * - has a less broken API than getopt (no global variable in the API),
 *   although it uses getopt as backend.
 */



enum has_arg {
	GOH_ARG_REQUIRED,
	GOH_ARG_OPTIONAL,
	GOH_ARG_REFUSED
};

struct goh_option {
	char *name;       /* Long option name. May be NULL. */
	char abbr;        /* Short option name. May be \0. */
	enum has_arg arg; /* Indicate if this option takes an argument. */
	int id;           /* Id that is returned by goh_nextoption to identify
	                     this option whenever encountered. */
	char *help;       /* help text. */
};

struct goh_state {
	const struct goh_option *opts;
	size_t optcnt;
	char *const *argv;
	int argc;
	int autohelp;
	char *usagehelp; /* First line of help. May be customized. */
	int argidx;      /* Filled with the first non-option argument index. */
	char *argval;    /* Filled with the value of the current option. */

	/*
	 * These are automatically computed from the above and used for backend
	 * calls to getopt.
	 */
	struct option *gnuopts;
	char *shortopt;
};



/*
 * state is a parsing state to initialize
 * opts is the list of options to recognize
 * cnt is the number of options
 * autohelp indicate whether add automatically options -h and --help that,
 * whenever encountered, call goh_printhelp and exit(EXIT_FAILURE).
 */
void goh_init(struct goh_state *state, const struct goh_option *opts, size_t cnt,
              int argc, char *const *argv, int autohelp);

void goh_fini(struct goh_state *state);

/*
 * Return the value specified by goh_option.id or -1 at the end.
 * Help is printed when --help or -h or an unknown option is encountered.
 */
int goh_nextoption(struct goh_state *state);

/* Only format the help text and print it. */
void goh_printhelp(const struct goh_state *state, FILE *stream);

#endif
