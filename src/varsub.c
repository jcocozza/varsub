#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "varsub.h"
#include "parser.h"
#include "string_util.h"


Variables *get_variables(char *input, char *separator, char *assignment, int err_on_empty_var) {
	Tokenizer *tokenizer = new_tokenizer(input, separator, assignment);
	Tokens *tkns = tokenize(tokenizer);
	Parser *p = new_parser(tkns, err_on_empty_var);
	Variables *v = parse(p);
	return v;
}

char *render(Variables *vars, char *template, int err_on_empty) {
	const char *t = template;
	size_t output_size = strlen(template) + 1;
	char *output = malloc(output_size);
	output[0] = '\0';

	while (*t) {
		char *start = strstr(t, "{{");

		if (!start) {
			strcat(output, t);
			break;
		}

		char *end = strstr(t, "}}");
		if (!end) {
			fprintf(stderr, "missing closing }}");
			exit(EXIT_FAILURE);
		}

		strncat(output, t, start - t);

		size_t var_len = end - (start + 2);
		char *var = malloc(var_len + 1);
		strncpy(var, start + 2, var_len);
		var[var_len] = '\0';

		var = trim_space(var);
		char *val = get_var(vars, var);

		if (err_on_empty && val == NULL) {
			fprintf(stderr, "variable %s not found\n", var);
			exit(EXIT_FAILURE);
		} else if (val == NULL) {
			val = "";
		}

		size_t needed = strlen(output) + strlen(val) + strlen(t) + 1;
		output = realloc(output, needed);
		strcat(output, val);
		t = end + 2;
	}
	return output;
}

void print_vars_in_template(char *template) {
	const char *t = template;
	while (*t) {
		char *start = strstr(t, "{{");
		if (!start) { break; }
		char *end = strstr(t, "}}");
		if (!end) {
			fprintf(stderr, "missing closing }}");
			exit(EXIT_FAILURE);
		}
		size_t var_len = end - (start + 2);
		char *var = malloc(var_len + 1);
		strncpy(var, start + 2, var_len);
		var[var_len] = '\0';
		var = trim_space(var);
		fprintf(stdout, "%s\n", var);
		t = end + 2;
	}
}
