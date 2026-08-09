#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "varsub.h"
#include "parser.h"
#include "string_util.h"


Tokenizer *new_tokenizer(char *input, char *separator, char *assignment) {
	Tokenizer *t = malloc(sizeof(Tokenizer));
	if (!t) {
		return NULL;
	}
	t->input = input;
	t->loc = 0;
	t->curr_col = 0;
	t->curr_row = 0;

	t->separator = separator;
	t->assignment = assignment;
	return t;
}

char peek(Tokenizer *t) { return t->input[t->loc]; }

char advance(Tokenizer *t) {
	t->loc++;
	t->curr_col++;
	return t->input[t->loc];
}

int match(Tokenizer *t, char *s) {
	int len = strlen(s);
	fflush(stdout);
	if (strncmp(&t->input[t->loc], s, len) == 0) {
		return 1;
	}
	return 0;
}

Token next_token(Tokenizer *t) {
	Token tkn = {.type = T_UNKNOWN, .col = 0, .row = 0};
	tkn.txt = malloc(1);
	tkn.txt[0] = '\0';

	char c = peek(t);
	if (c == '\0') {
		tkn.type = T_END;
		tkn.txt = realloc(tkn.txt, 4);
		strcpy(tkn.txt, "EOF");
		return tkn;
	}

	if (c == '\n' && !strcmp(t->separator, "\n")) {
		t->curr_col = 0;
		t->curr_row++;
	}

	if (match(t, t->separator)) {
		tkn.type = T_SEP;
		tkn.txt = realloc(tkn.txt, strlen(t->separator) + 1);
		strcpy(tkn.txt, t->separator);

		int l = strlen(t->separator);
		t->loc += l;
		t->curr_col += l;
		return tkn;
	}

	if (match(t, t->assignment)) {
		tkn.type = T_ASSIGNMENT;
		tkn.txt = realloc(tkn.txt, strlen(t->assignment) + 1);
		strcpy(tkn.txt, t->assignment);

		int l = strlen(t->assignment);
		t->loc += l;
		t->curr_col += l;
		return tkn;
	}

	tkn.type = T_INDENT;
	while (!match(t, t->separator) && !match(t, t->assignment) && c != '\0') {
		size_t len = strlen(tkn.txt);
		tkn.txt = realloc(tkn.txt, len + 2); // one for new char, one for end
		tkn.txt[len] = c;
		tkn.txt[len + 1] = '\0';
		c = advance(t);
	}
	return tkn;
}


Tokens *new_tokens() {
	Tokens *tkns = malloc(sizeof(Tokens));
	if (tkns == NULL) {
		free(tkns);
		return NULL;
	};
	tkns->cnt = 0;
	tkns->cap = 10;
	tkns->tokens = malloc(tkns->cap * sizeof(Token));
	if (tkns->tokens == NULL) {
		free(tkns->tokens);
		free(tkns);
		return NULL;
	}
	return tkns;
};

void add_token(Tokens *tkns, Token tkn) {
	if (tkns->cnt + 1 >= tkns->cap) {
		tkns->cap = tkns->cap * 2;
		tkns->tokens = realloc(tkns->tokens, tkns->cap * sizeof(Token));
	}
	tkns->tokens[tkns->cnt] = tkn;
	tkns->cnt++;
}

Tokens *tokenize(Tokenizer *t) {
	Tokens *tkns = new_tokens();

	Token curr_token = {0};
	while (curr_token.type != T_END) {
		curr_token = next_token(t);
		curr_token.col = t->curr_col;
		curr_token.row = t->curr_row;
		add_token(tkns, curr_token);
	}
	return tkns;
}

Parser *new_parser(Tokens *tkns, int err_on_empty_var) {
	Parser *p = malloc(sizeof(Parser));
	p->loc = 0;
	p->tkns = tkns;
	p->err_on_empty_var = err_on_empty_var;
	return p;
}

Token p_peek(Parser *p) { return p->tkns->tokens[p->loc]; }

Token p_advance(Parser *p) {
	p->loc++;
	return p->tkns->tokens[p->loc];
}


Variables *parse(Parser *p) {
	Variables *vars = new_variables(0, 10);

	Token curr_token = p_peek(p);
	Var curr_var = {0};

	int left_of_assignment = 1;
	while (curr_token.type != T_END) {
		switch (curr_token.type) {
			case T_UNKNOWN:
				fprintf(stderr, "unknown token\n");
				exit(EXIT_FAILURE);
			case T_INDENT:
				if (left_of_assignment) {
					curr_var.key = curr_token.txt;
				} else {
					curr_var.value = curr_token.txt;
					left_of_assignment = 1;

					if (p->err_on_empty_var && strlen(trim_space(curr_var.value)) == 0) {
						fprintf(stderr, "ERROR[%d:%d]: variable %s is empty.\n",
								curr_token.row, curr_token.col, curr_var.key);
						exit(EXIT_FAILURE);
					}
					// time to flush
					add_variable(vars, curr_var);
					curr_var.key = NULL;
					curr_var.value = NULL;
				}
				break;
			case T_SEP:
				// if we hit the sep, but haven't assigned a value then error
				if (!left_of_assignment) {
					if (p->err_on_empty_var) {
						fprintf(stderr, "ERROR[%d:%d]: variable %s is empty.\n",
								curr_token.row, curr_token.col, curr_var.key);
						exit(EXIT_FAILURE);
					} else {
						// time to flush - use empty string
						curr_var.value = "";
						left_of_assignment = 1;
						add_variable(vars, curr_var);
						curr_var.key = NULL;
						curr_var.value = NULL;
					}
				}
				break;
			case T_ASSIGNMENT:
				// if we hit assignment but don't have a key then the assignment operator
				// was used twice
				if (curr_var.key == NULL) {
					fprintf(stderr,
							"ERROR[%d:%d]: unparseable section. expected ident not "
							"assignment: %s\n",
							curr_token.row, curr_token.col, curr_token.txt);
					exit(EXIT_FAILURE);
				}
				left_of_assignment = 0;
				break;
			case T_END:
				fprintf(stderr, "ERROR[%d:%d] unexpected end. tok: %s\n", curr_token.row,
						curr_token.col, curr_token.txt);
				exit(EXIT_FAILURE);
		}
		curr_token = p_advance(p);
	}

	// for (size_t i = 0; i < vars->cnt; i++) {
	//   printf("%s:%s\n", vars->variables[i].key, vars->variables[i].value);
	// }

	// flush last var
	// if (curr_var.key == NULL || curr_var.value == NULL) {
	//  fprintf(stderr, "unexpected remaining var. key: %s, val: %s\n",
	//          curr_var.key, curr_var.value);
	//  exit(EXIT_FAILURE);
	//}
	// add_variable(vars, curr_var);
	return vars;
}
