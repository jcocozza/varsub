typedef struct var {
	char *key;
	char *value;
} Var;


typedef struct variables {
	Var *variables;
	size_t cnt;
	size_t cap;
} Variables;


Variables *new_variables(int cnt, int cap);
void add_variable(Variables *vars, Var v);
char *get_var(Variables *vars, char *key);

Variables *get_variables(char *input, char *separator, char *assignment, int err_on_empty_var);

char *render(Variables *vars, char *template, int err_on_empty);
void print_vars_in_template(char *template);
