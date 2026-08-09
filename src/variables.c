#include <stdlib.h>
#include <string.h>
#include "varsub.h"
#include "parser.h"

Variables *new_variables(int cnt, int cap) {
	if (cnt > cap) {
		return NULL;
	}
	Variables *vars = malloc(sizeof(Variables)); 
	if (vars == NULL) {
		return NULL;
	}
	vars->cnt = cnt;
	vars->cap = cap;
	vars->variables = malloc(sizeof(Var) * vars->cap);
	if (vars == NULL) {
		free(vars);
		return NULL;
	}
	return vars;
}

char *get_var(Variables *vars, char *key) {
	for (size_t i = 0; i < vars->cnt; i++) {
		if (!strcmp(vars->variables[i].key, key)) {
			return vars->variables[i].value;
		}
	}
	return NULL;
}

int contains(Variables *vars, char *key) {
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
void add_variable(Variables *vars, Var v) {
  if (vars->cnt + 1 >= vars->cap) {
    vars->cap = vars->cap * 2;
    vars->variables = realloc(vars->variables, vars->cap * sizeof(Var));
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
