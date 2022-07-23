/* ----------------------------------------------------------------------
 * Author: Carlos Rivera
 * Date: 07/22/2022
 * File: lex.c
 * --------------------------------------------------------------------*/

typedef enum {
	TOKEN_IDENT = 128,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_CHAR,
	TOKEN_STR,

	TOKEN_LSHIFT,
	TOKEN_RSHIFT,
	TOKEN_EQ,
	TOKEN_NOTEQ,
	TOKEN_LTEQ,
	TOKEN_GTEQ,
	TOKEN_AND,
	TOKEN_OR,
	TOKEN_INC,
	TOKEN_DEC,

	TOKEN_ASSIGN,
} TokenType;

typedef enum {
	TOKENMOD_NONE,
	TOKENMOD_COLON,
	TOKENMOD_ADD,
	TOKENMOD_SUB,
	TOKENMOD_MUL,
	TOKENMOD_DIV,
	TOKENMOD_MOD,
	TOKENMOD_AND,
	TOKENMOD_OR,
	TOKENMOD_LSHIFT,
	TOKENMOD_RSHIFT,
} TokenMod;

typedef struct {
	TokenType type;
	TokenMod mod;
	const char *start;
	const char *end;

	union {
		uint64_t int_val;
		double float_val;
		const char *str_val;
		char char_val;
		const char *identifier;
	};
} Token;

const char *stream;
Token token;

void fatal(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("FATAL: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
	exit(1);
}

void syntax_error(const char *fmt, ...) {
	va_list(args);
	va_start(args, fmt);
	printf("Syntax Error: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}


void print_token() {
	switch (token.type) {
	case TOKEN_INT:
		printf("TOKEN_INT: %lld\n", token.int_val);
        break;
	case TOKEN_IDENT:
		printf("TOKEN_IDENT: %s\n", token.identifier);
		break;
	default:
		if (token.type < 128) {
			printf("%c\n", token.type);
		}	else {
			printf("<ASCII>%c\n", token.type);
		}
	}
}

size_t copy_token_type_str(char *dest, size_t dest_size, TokenType type) {
	size_t n = 0;
	switch (type) {
	case TOKEN_INT:
		n = snprintf(dest, dest_size, "integer");
		break;
	case TOKEN_IDENT:
		n = snprintf(dest, dest_size, "identifer");
		break;
	default:
		n = snprintf(dest, dest_size, "%c", type);
	}

	return n;
}

// @note: Returns static buffer
const char *token_type_str(TokenType type) {
	static char buf[256];
	size_t n = copy_token_type_str(buf, sizeof(buf), type);
	assert(n + 1 <= sizeof(buf));
	return buf;
}

const int char_to_digit[128] = {
	['0'] = 0, ['5'] = 5, ['a'] = 10, ['f'] = 15, ['E'] = 14,
	['1'] = 1, ['6'] = 6, ['b'] = 11, ['A'] = 10, ['F'] = 15,
	['2'] = 2, ['7'] = 7, ['c'] = 12, ['B'] = 11,
	['3'] = 3, ['8'] = 8, ['d'] = 13, ['C'] = 12,
	['4'] = 4, ['9'] = 9, ['e'] = 14, ['D'] = 13,
};

double scan_float() {
	const char *start = stream;
	while (isdigit(*stream)) {
		stream++;
	}
	if (*stream != '.') {
		syntax_error("Expected '.' in float literal, found %c", *stream);
	}
	stream++;
	while (isdigit(*stream)) {
		stream++;
	}
	if (tolower(*stream) == 'e') {
		stream++;
		if (*stream == '+' || *stream == '-') {
			stream++;
		}
		if (!isdigit(*stream)) {
			syntax_error("expected digit after float literal exponent, found %c", *stream);
		}
		while (isdigit(*stream)) {
			stream++;
		}
	}

	double result = strtod(start, NULL);
	if (result == HUGE_VAL || result == -HUGE_VAL) {
		syntax_error("float literal overflow");
	}
	return result;
}

static uint64_t scan_int() {
	uint64_t result = 0;
	uint64_t base = 10;
	if (*stream == '0') {
		stream++;
		if (tolower(*stream) == 'x') {
			base = 16;
			stream++;
		} else if (tolower(*stream) == 'b') {
			base = 2;
			stream++;
		} else if (isdigit(*stream)) {
			base = 8;
		} else {
			syntax_error("invalid integer literal prefix, %c", *stream);
			base = 1;
			stream++;
		}
	}

	for (;;) {
		uint64_t digit = char_to_digit[*stream];
		if (digit == 0 && *stream != '0') {
			break;
		}
		if (result > (INT_MAX - digit) / 10) {
			syntax_error("integer literal overflow");
			result = 0;
		}
		if (digit >= base) {
			syntax_error("digit %c, out of range for base %lld", *stream, base);
			digit = 0;
		}
		result = result * base + digit;
		stream++;
	}

	return result;
}

static char escape_to_char[256] = {
	['n'] = '\n', ['r'] = '\r', ['t'] = '\t', ['v'] = '\v', ['f'] = '\f',
};

void next_token() {
top:
	token.start = stream;
	token.mod = TOKENMOD_NONE;
	switch (*stream) {
	case ' ': case '\n': case '\r': case '\t': case '\v': case '\f':
		while (isspace(*stream)) {
			*stream++;
		}
		goto top;
		break;
	case '<':
		stream++;
		if (*stream == '<') {
			stream++;
			if (*stream == '=') {
				stream++;
				token.type = TOKEN_ASSIGN;
				token.mod = TOKENMOD_LSHIFT;
			} else {
				token.type = TOKEN_LSHIFT;
			}
		} else if (*stream == '=') {
			stream++;
			token.type = TOKEN_LTEQ;
		} else {
			token.type = *stream;
		}

		token.end = stream;
		break;
	case '>':
		stream++;
		if (*stream == '>') {
			stream++;
			if (*stream == '=') {
				stream++;
				token.type = TOKEN_ASSIGN;
				token.mod = TOKENMOD_RSHIFT;
			} else {
				token.type = TOKEN_RSHIFT;
			}
		} else if (*stream == '=') {
			stream++;
			token.type = TOKEN_GTEQ;
		} else {
			token.type = *stream;
		}

		token.end = stream;
		break;
	case '=':
		stream++;
		if (*stream == '=') {
			stream++;
			token.type = TOKEN_EQ;
		} else {
			token.type = TOKEN_ASSIGN;
		}

		token.end = stream;
		break;
	case '!':
		stream++;
		if (*stream == '=') {
			stream++;
			token.type = TOKEN_NOTEQ;
		}
		
		token.end = stream;
		break;
	case '|':
		stream++;
		if (*stream == '|') {
			stream++;
			token.type = TOKEN_OR;
		} else if (*stream == '=') {
			stream++;
			token.type = TOKEN_ASSIGN;
			token.mod = TOKENMOD_OR;
		} else {
			token.type = '|';
		}

		token.end = stream;
		break;
	case '&':
		stream++;
		if (*stream == '&') {
			stream++;
			token.type = TOKEN_AND;
		} else if (*stream == '=') {
			stream++;
			token.type = TOKEN_ASSIGN;
			token.mod = TOKENMOD_AND;
		} else {
			token.type = '&';
		}
		
		token.end = stream;
		break;
	case '+':
		stream++;
		if (*stream == '+') {
			stream++;
			token.type = TOKEN_INC;
		} else if (*stream == '=') {
			stream++;
			token.type = TOKEN_ASSIGN;
			token.mod = TOKENMOD_ADD;
		} else {
			token.type = '+';
		}

		token.end = stream;
		break;
	case '-':
		stream++;
		if (*stream == '-') {
			stream++;
			token.type = TOKEN_DEC;
		} else if (*stream == '=') {
			stream++;
			token.type = TOKEN_ASSIGN;
			token.mod = TOKENMOD_SUB;
		} else {
			token.type = '-';
		}

		token.end = stream;
		break;
	case '*':
		stream++;
		if (*stream == '=') {
			stream++;
			token.type = TOKEN_ASSIGN;
			token.mod = TOKENMOD_MUL;
		} else {
			token.type = '*';
		}

		token.end = stream;
		break;
	case '/':
		stream++;
		// commments
		if (*stream == '=') {
			stream++;
			token.type = TOKEN_ASSIGN;
			token.mod = TOKENMOD_DIV;
		} else {
			token.type = '/';
		}

		token.end = stream;
		break;
	
	case '"':
		token.type = TOKEN_INT;
		stream++;
		char *str = NULL;
		while (*stream != '"' && *stream != 0) {
			char ch;
			if (*stream == '\\') {
				stream++;
				ch = escape_to_char[*stream];
				if (ch == 0) {
					syntax_error("unexpected character in char literal, %c", *stream);
				}
			} else {
				ch = *stream;
			}

			buf_push(str, ch);
			stream++;
		}
		if (*stream == '"') {
			stream++;
		} else {
			syntax_error("unexpected end of file in string literal");
		}

		token.type = TOKEN_STR;
		token.str_val = str;
		token.end = stream;
		break;
	case '\'':
		char val;
		stream++;
		if (*stream == '\\') {
			stream++;
			val = escape_to_char[*stream];
		} else {
			val = *stream;
			if (isspace(val)) {
				val = 0;
			}
		}
		if (val == 0) {
			syntax_error("unexpected character in char literal, %c", *stream);
		}
		stream++;
		if (*stream != '\'') {
			syntax_error("expected single quote, got %c", *stream);
		}
		stream++;

		token.type = TOKEN_CHAR;
		token.char_val = val;
		token.end = stream;
		break;
	case '.':
		token.type = TOKEN_FLOAT;
		token.float_val = scan_float();
		token.end = stream;
		break;
	case '0': case '1': case '2': case '3': case '4': case '5': case '6':
	case '7': case '8': case '9':
		while (isdigit(*stream)) {
			stream++;
		}
		if (*stream == '.') {
			token.type = TOKEN_FLOAT;
			stream = token.start;
			token.float_val = scan_float();
		} else {
			token.type = TOKEN_INT;
			stream = token.start;
			token.int_val = scan_int();
		}
		token.end = stream;
		break;
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
	case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
	case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
	case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
	case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
	case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
	case 'v': case 'w': case 'x': case 'y': case 'z':
	case '_':
		token.type = TOKEN_IDENT;
		while (isalnum(*stream) || *stream == '_') {
			stream++;
		}
		token.end = stream;
		token.identifier = intern_str_range(token.start, token.end);
		break;
	default:
		token.type = *stream++;
		token.end = stream;
	}
}

inline bool is_token(TokenType type) {
	return token.type == type;
}

inline bool match_token(TokenType type) {
	if (is_token(type)) {
		next_token();
		return true;
	}
	return false;
}

inline bool expect_token(TokenType type) {
	if (is_token(type)) {
		return true;
	}
	fatal("Expected %s, got %s", token_type_str(type), token_type_str(token.type));
	return false;
}

static void init_stream(const char *str) {
	stream = str;
	next_token();
}	

#define assert_token(token) (assert(match_token(token)))
#define assert_token_int(x) (assert(token.int_val == x && match_token(TOKEN_INT)))
#define assert_token_float(f) (assert(token.float_val == f && match_token(TOKEN_FLOAT)))
#define assert_token_ident(str) (assert(token.identifier == intern_str(str) && match_token(TOKEN_IDENT)))
#define assert_token_char(ch) (assert(token.char_val == ch && match_token(TOKEN_CHAR)))
#define assert_token_str(str) (assert(strcmp(str, token.str_val) && match_token(TOKEN_STR)))
#define assert_token_eof() (assert(is_token(0)))
#define assert_token_assign(t) (assert(token.mod == t && match_token(TOKEN_ASSIGN)))

void lex_test() {
	// operators
	init_stream("<< >> <= >= ++ -- && &= += -= <<= >>= /= *=");
	assert_token(TOKEN_LSHIFT);
	assert_token(TOKEN_RSHIFT);
	assert_token(TOKEN_LTEQ);
	assert_token(TOKEN_GTEQ);
	assert_token(TOKEN_INC);
	assert_token(TOKEN_DEC);
	assert_token(TOKEN_AND);
	assert_token_assign(TOKENMOD_AND);
	assert_token_assign(TOKENMOD_ADD);
	assert_token_assign(TOKENMOD_SUB);
	assert_token_assign(TOKENMOD_LSHIFT);
	assert_token_assign(TOKENMOD_RSHIFT);
	assert_token_assign(TOKENMOD_DIV);
	assert_token_assign(TOKENMOD_MUL);
    assert_token_eof();

	// float literals
	init_stream("10.0 0.034 .190 0.2E2 9.0E-2 .9e+10");
	assert_token_float(10.0);
	assert_token_float(0.034);
	assert_token_float(.190);
	assert_token_float(0.2E2);
	assert_token_float(9.0E-2);
	assert_token_float(.9e+10);
    assert_token_eof();

	// integer literals
	init_stream("0x32 + 0b1010 + 036");
	assert_token_int(0x32);
	assert_token('+');
	assert_token_int(10);
	assert_token('+');
	assert_token_int(036);
	assert_token_eof();
	
	// strings/char literals
	init_stream("\'\\n\' \"foo \\n\"");
	assert_token_char('\n');
	assert_token_str("foo \n");
	assert_token_eof();

	// basic test
	init_stream("bizzbuzz * (0x84 + 29)");
	assert_token_ident("bizzbuzz");
	assert_token('*');
	assert_token('(');
	assert_token_int(0x84);
	assert_token('+');
	assert_token_int(29);
	assert_token(')');
	assert_token_eof();
}


