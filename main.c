#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// flags
// char *var_sep = "";
// char *assignment_op = "";
// int batch = 0;

void usage() { printf("help message"); }

typedef struct var {
  char *key;
  char *value;
} var_t;

typedef struct vars {
  var_t *variables;
  size_t cnt;
  size_t cap;
} vars_t;

vars_t *new_vars() {
  vars_t *vars = malloc(sizeof(vars_t));
  vars->cnt = 0;
  vars->cap = 10;
  vars->variables = malloc(sizeof(var_t) * vars->cap);
  return vars;
}

void add_variable(vars_t *vars, var_t v) {
  if (vars->cnt + 1 >= vars->cap) {
    vars->cap = vars->cap * 2;
    vars->variables = realloc(vars->variables, vars->cap * sizeof(var_t));
  }
  vars->variables[vars->cnt] = v;
  vars->cnt++;
}

char *lookup(vars_t *vars, char *key) {
  for (size_t i = 0; i < vars->cnt; i++) {
    if (!strcmp(vars->variables[i].key, key)) {
      return vars->variables[i].value;
    }
  }
  return "";
}

typedef enum token_type {
  T_UNKNOWN,
  T_INDENT,
  T_SEP,
  T_ASSIGNMENT,
  T_END,
} token_type_t;

typedef struct token {
  token_type_t type;
  char *txt;
} token_t;

typedef struct tokenizer {
  char *input;
  int loc;

  char *separator;
  char *assignment;
} tokenizer_t;

tokenizer_t *new_tokenizer(char *input, char *separator, char *assignment) {
  tokenizer_t *t = malloc(sizeof(tokenizer_t));
  if (!t) {
    return NULL;
  }
  t->input = input;
  t->loc = 0;

  t->separator = separator;
  t->assignment = assignment;
  return t;
}

char peek(tokenizer_t *t) { return t->input[t->loc]; }

char advance(tokenizer_t *t) {
  t->loc++;
  return t->input[t->loc];
}

int match(tokenizer_t *t, char *s) {
  int len = strlen(s);
  fflush(stdout);
  if (strncmp(&t->input[t->loc], s, len) == 0) {
    return 1;
  }
  return 0;
}

token_t next_token(tokenizer_t *t) {
  token_t tkn = {.type = T_UNKNOWN};
  tkn.txt = malloc(1);
  tkn.txt[0] = '\0';

  char c = peek(t);
  if (c == '\0') {
    tkn.type = T_END;
    tkn.txt = realloc(tkn.txt, 4);
    strcpy(tkn.txt, "EOF");
    return tkn;
  }

  if (match(t, t->separator)) {
    tkn.type = T_SEP;
    tkn.txt = realloc(tkn.txt, strlen(t->separator) + 1);
    strcpy(tkn.txt, t->separator);
    t->loc += strlen(t->separator);
    return tkn;
  }

  if (match(t, t->assignment)) {
    tkn.type = T_ASSIGNMENT;
    tkn.txt = realloc(tkn.txt, strlen(t->assignment) + 1);
    strcpy(tkn.txt, t->assignment);
    t->loc += strlen(t->assignment);
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

typedef struct tokens {
  size_t cnt;
  size_t cap;
  token_t *tokens;
} tokens_t;

tokens_t *new_tokens() {
  tokens_t *tkns = malloc(sizeof(tokens_t));
  if (tkns == NULL) {
    free(tkns);
    return NULL;
  };
  tkns->cnt = 0;
  tkns->cap = 10;
  tkns->tokens = malloc(tkns->cap * sizeof(token_t));
  if (tkns->tokens == NULL) {
    free(tkns->tokens);
    free(tkns);
    return NULL;
  }
  return tkns;
};

void add_token(tokens_t *tkns, token_t tkn) {
  if (tkns->cnt + 1 >= tkns->cap) {
    tkns->cap = tkns->cap * 2;
    tkns->tokens = realloc(tkns->tokens, tkns->cap * sizeof(token_t));
  }
  tkns->tokens[tkns->cnt] = tkn;
  tkns->cnt++;
}

tokens_t *tokenize(tokenizer_t *t) {
  tokens_t *tkns = new_tokens();

  token_t curr_token;
  while (curr_token.type != T_END) {
    curr_token = next_token(t);
    add_token(tkns, curr_token);
  }
  return tkns;
}

typedef struct parser {
  tokens_t *tkns;
  int loc;
} parser_t;

parser_t *new_parser(tokens_t *tkns) {
  parser_t *p = malloc(sizeof(parser_t));
  p->loc = 0;
  p->tkns = tkns;
  return p;
}

token_t p_peek(parser_t *p) { return p->tkns->tokens[p->loc]; }

token_t p_advance(parser_t *p) {
  p->loc++;
  return p->tkns->tokens[p->loc];
}

vars_t *parse(parser_t *p) {
  vars_t *vars = new_vars();

  token_t curr_token = p_peek(p);
  var_t curr_var = {0};

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
      }
      break;
    case T_SEP:
      if (curr_var.key == NULL || curr_var.value == NULL) {
        fprintf(stderr, "unexpected separator\n");
        exit(EXIT_FAILURE);
      }
      // time to flush
      add_variable(vars, curr_var);
      curr_var.key = NULL;
      curr_var.value = NULL;
      break;
    case T_ASSIGNMENT:
      left_of_assignment = 0;
      break;
    case T_END:
      fprintf(stderr, "unexpected end\n");
      exit(EXIT_FAILURE);
    }
    curr_token = p_advance(p);
    //printf("tok type: %d: %s\n", curr_token.type, curr_token.txt);
  }
  // flush last var
  if (curr_var.key == NULL || curr_var.value == NULL) {
    fprintf(stderr, "unexpected end\n");
    exit(EXIT_FAILURE);
  }
  add_variable(vars, curr_var);
  return vars;
}

char *read_stdin_all(void) {
  size_t cap = 4096; // initial capacity (4 KB)
  size_t len = 0;
  char *buf = malloc(cap);
  if (!buf)
    return NULL;

  int c;
  while ((c = fgetc(stdin)) != EOF) {
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

int main(int argc, char *argv[]) {
  // cli stuff
  for (int i = 1; i < argc; i++) {
    // handle flags
    if (argv[i][0] == '-' && strlen(argv[i]) > 0) {
      if (!strcmp(argv[i], "--help")) {
        return 0;
      }
    }
  }

  // char *input = read_stdin_all();

  char *input = "foo=asdf bar=quix ban=bon bozo=fff";
  tokenizer_t *tokenizer = new_tokenizer(input, " ", "=");
  tokens_t *tkns = tokenize(tokenizer);

  parser_t *p = new_parser(tkns);
  vars_t *v = parse(p);

  for (size_t i = 0; i < v->cnt; i++) {
    printf("%s:%s\n", v->variables[i].key, v->variables[i].value);
  }
}
