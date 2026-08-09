#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "varsub.h"

// flags
char *var_sep = "\n";
char *assignment_op = "=";
int err_on_empty_var = 0;
int print_passed_vars = 0;
int print_template_vars = 0;

void usage() {
	fprintf(stderr, "usage: varsub [OPTIONS] [TEMPLATE FILE] [VARS]\n");
}

void usage_long() {
	usage();
	fprintf(stderr, "\n");
	fprintf(stderr, "options:\n");
	if (!strcmp(var_sep, "\n")) {
		fprintf(stderr, "  -s: set variable separator (default \"\\n\")\n");
	} else {
		fprintf(stderr, "  -s: set variable separator (default \"%s\")\n", var_sep);
	}
	fprintf(stderr, "  -a: set assignment operator (default \"%s\")\n",
			assignment_op);
	fprintf(stderr, "  -p: print passed variables and exit.\n");
	fprintf(stderr, "  -V: extract variable names in template, print them, and exit.\n");
	fprintf(stderr,
			"  -e: varsub will error when rendering if a variable is empty\n");
	fprintf(stderr, "  -t, --template: pass template as a string\n");
	fprintf(stderr, "  --set: manually set a variable. must use correct"
			" assignment (e.g. --set foo=bar)\n");
	fprintf(stderr, "  --vars: pass in a variable file. file must use correct "
			"separator and assignment\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "example:\n");
	fprintf(stderr, "  varsub my_template.txt < vars.txt\n");
}

char *read_all(FILE *f) {
	size_t cap = 4096; // initial capacity (4 KB)
	size_t len = 0;
	char *buf = malloc(cap);
	if (!buf)
		return NULL;

	int c;
	while ((c = fgetc(f)) != EOF) {
		// grow if needed
		if (len + 1 >= cap) {
			cap *= 2;
			char *tmp = realloc(buf, cap);
			if (!tmp) {
				free(buf);
				return NULL;
			}
			buf = tmp;
		}
		buf[len++] = (char)c;
	}

	buf[len] = '\0';
	return buf;
}

// append v onto s connected by delim
void append(char **s, const char *delim, const char *v) {
	if (*s == NULL) {
		*s = malloc(strlen(v) + 1);
		strcpy(*s, v);
		return;
	}

	size_t len = strlen(*s) + strlen(delim) + strlen(v);
	char *tmp = realloc(*s, len + 1);
	if (!tmp)
		exit(1);
	*s = tmp;

	strcat(*s, delim);
	strcat(*s, v);
}

int main(int argc, char *argv[]) {
	char *template = NULL;
	char *manually_set = NULL;

	char *input = NULL;
	// cli stuff
	for (int i = 1; i < argc; i++) {
		// handle flags
		if (argv[i][0] == '-' && strlen(argv[i]) > 0) {
			if (!strcmp(argv[i], "-h")) {
				usage();
				return 1;
			} else if (!strcmp(argv[i], "--help")) {
				usage_long();
				return 1;
			} else if (!strcmp(argv[i], "-p")) {
				print_passed_vars = 1;
			} else if (!strcmp(argv[i], "-s")) {
				i++;
				var_sep = argv[i];
			} else if (!strcmp(argv[i], "-a")) {
				i++;
				assignment_op = argv[i];
			} else if (!strcmp(argv[i], "-e")) {
				err_on_empty_var = 1;
			} else if (!strcmp(argv[i], "-V")) {
				print_template_vars = 1;
			} else if (!strcmp(argv[i], "--set")) {
				i++;
				char *newvar = argv[i];
				append(&manually_set, var_sep, newvar);
			} else if (!strcmp(argv[i], "--vars")) {
				i++;
				FILE *f = fopen(argv[i], "r");
				if (!f) {
					fprintf(stderr, "failed to open vars file: %s\n", argv[i]);
					exit(EXIT_FAILURE);
				}
				input = read_all(f);
				fclose(f);
			} else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template")) {
				i++;
				template = argv[i];
			} else {
				fprintf(stderr, "unknown flag: %s\n", argv[i]);
				usage_long();
				exit(EXIT_FAILURE);
			}
		} else {
			FILE *f = fopen(argv[i], "r");
			if (!f) {
				fprintf(stderr, "failed to open template file: %s\n", argv[i]);
				exit(EXIT_FAILURE);
			}
			template = read_all(f);
			fclose(f);
		}
	}

	if (template == NULL || strlen(template) == 0) {
		fprintf(stderr, "err: no template found\n\n");
		usage_long();
		exit(EXIT_FAILURE);
	}

	// determine if anything is coming down the pipe
	// if so read it
	if (!isatty(fileno(stdin))) {
		input = read_all(stdin);
	}
	if (manually_set != NULL) {
		append(&input, var_sep, manually_set);
	}
	if (print_template_vars) {
		print_vars_in_template(template);
		return EXIT_SUCCESS;
	}

	if (input == NULL || strlen(input) == 0) {
		fprintf(stderr, "no variables provided\n");
		exit(EXIT_FAILURE);
	}

	Variables *v = get_variables(input, var_sep, assignment_op, err_on_empty_var);

	if (print_passed_vars) {
		for (size_t i = 0; i < v->cnt; i++) {
			printf("%s\t%s\n", v->variables[i].key, v->variables[i].value);
		}
		return EXIT_SUCCESS;
	}

	char *output = render(v, template, err_on_empty_var);
	fprintf(stdout, "%s", output);

	if (output[strlen(output) - 1] != '\n') {
		fprintf(stdout, "\n");
	}
}
