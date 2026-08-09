typedef enum token_type {
	T_UNKNOWN,
	T_INDENT,
	T_SEP,
	T_ASSIGNMENT,
	T_END,
} TokenType;

typedef struct token {
	TokenType type;
	char *txt;

	int col;
	int row;
} Token;

typedef struct tokenizer {
	char *input;
	int loc;

	int curr_col;
	int curr_row;

	char *separator;
	char *assignment;
} Tokenizer;

typedef struct tokens {
	size_t cnt;
	size_t cap;
	Token *tokens;
} Tokens;


typedef struct parser {
	Tokens *tkns;
	int loc;
	int err_on_empty_var;
} Parser;

Tokenizer *new_tokenizer(char *input, char *separator, char *assignment);
Tokens *tokenize(Tokenizer *t);
Parser *new_parser(Tokens *tkns, int err_on_empty_var);
Variables *parse(Parser *p);
