#ifdef _WIN32
#include <io.h>
#define fileno _fileno
#else
#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

char *lookup(vars_t *vars, char *key) {
  for (size_t i = 0; i < vars->cnt; i++) {
    if (!strcmp(vars->variables[i].key, key)) {
      return vars->variables[i].value;
    }
  }
  return "";
}

// return idx of var, otherwise return -1
int contains(vars_t *vars, char *key) {
  for (size_t i = 0; i < vars->cnt; i++) {
    if (!strcmp(vars->variables[i].key, key)) {
      return i;
    }
  }
  return -1;
}

// add v to vars
//
// if v is in vars update its value
void add_variable(vars_t *vars, var_t v) {
  if (vars->cnt + 1 >= vars->cap) {
    vars->cap = vars->cap * 2;
    vars->variables = realloc(vars->variables, vars->cap * sizeof(var_t));
  }
  int idx = contains(vars, v.key);
  if (idx == -1) {
    vars->variables[vars->cnt] = v;
    vars->cnt++;
    return;
  }

  free(vars->variables[idx].key);
  free(vars->variables[idx].value);
  vars->variables[idx] = v;
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

  token_t curr_token = {0};
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
        // time to flush
        add_variable(vars, curr_var);
        curr_var.key = NULL;
        curr_var.value = NULL;
      }
      break;
    case T_SEP:
      break;
    case T_ASSIGNMENT:
      left_of_assignment = 0;
      break;
    case T_END:
      fprintf(stderr, "unexpected end. tok: %s\n", curr_token.txt);
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

char *trim_space(char *s) {
  size_t size;
  char *end;

  size = strlen(s);

  if (!size)
    return s;

  end = s + size - 1;
  while (end >= s && isspace(*end))
    end--;
  *(end + 1) = '\0';

  while (*s && isspace(*s))
    s++;

  return s;
}

char *render(vars_t *vars, char *template) {
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
    char *val = lookup(vars, var);

    output_size = strlen(output) + strlen(val) + 1;
    output = realloc(output, output_size);
    strcat(output, val);
    t = end + 2;
  }
  return output;
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

// flags
char *var_sep = "\n";
char *assignment_op = "=";

void usage() { fprintf(stderr, "usage: varsub [options] [FILE] [vars]"); }

int main(int argc, char *argv[]) {
  char *template = NULL;
  char *manually_set = NULL;

  char *input = NULL;
  // cli stuff
  for (int i = 1; i < argc; i++) {
    // handle flags
    if (argv[i][0] == '-' && strlen(argv[i]) > 0) {
      if (!strcmp(argv[i], "--help")) {
        return 0;
      } else if (!strcmp(argv[i], "-s")) {
        i++;
        var_sep = argv[i];
      } else if (!strcmp(argv[i], "-a")) {
        i++;
        assignment_op = argv[i];
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
    fprintf(stderr, "no template found\n");
    exit(EXIT_FAILURE);
  }

  // if (!read_input) {
  //   input = read_all(stdin);
  // }

  // determine if anything is coming down the pipe
  // if so read it
  if (!isatty(fileno(stdin))) {
    input = read_all(stdin);
  }

  if (manually_set != NULL) {
    append(&input, var_sep, manually_set);
  }

  tokenizer_t *tokenizer = new_tokenizer(input, var_sep, assignment_op);
  tokens_t *tkns = tokenize(tokenizer);

  parser_t *p = new_parser(tkns);
  vars_t *v = parse(p);
  char *output = render(v, template);
  fprintf(stdout, "%s", output);
}
