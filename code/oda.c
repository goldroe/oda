/* ----------------------------------------------------------------------
 * Author: Carlos Rivera
 * Date: 07/15/2022
 * File: oda.c
 * --------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void *xrealloc(void *ptr, size_t size) {
	void *result = realloc(ptr, size);
	if (result == NULL) {
		perror("xrealloc failed");
		exit(1);
	}

	return result;
}

void *xmalloc(size_t size) {
	void *result = malloc(size);
	if (result == NULL) {
		perror("xmalloc failed");
		exit(1);
	}
	return result;
}
#define malloc(x) xmalloc(x)

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

// Stretchy Buffer

typedef struct {
	size_t len;
	size_t cap;
	char buf[0];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)(b) - offsetof(BufHdr, buf)))
#define buf__fits(b, n) (buf_len(b) + n <= buf_cap(b))
#define buf__fit(b, n) (buf__fits(b, n) ? 0 : ((b) = buf__grow(b, buf_len(b) + n, sizeof(*(b)))))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((void *)((char *)(b) + sizeof(*(b)) * buf_len((b))))
#define buf_push(b, ...) (buf__fit((b), 1), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_free(b) ((b) ? free(buf__hdr(b)) : 0, (b) = NULL)

void *buf__grow(const void *buf, size_t new_len, size_t element_size) {
	size_t new_cap = MAX(1 + 2 * buf_cap(buf), new_len);
	assert(new_len <= new_cap);
	size_t new_size = new_cap * element_size + offsetof(BufHdr, buf);
	BufHdr *new_hdr = NULL;
	if (buf) {
		new_hdr = realloc(buf__hdr(buf), new_size);

	} else {
		new_hdr = malloc(new_size);
		new_hdr->len = 0;
	}
	new_hdr->cap = new_cap;

	return new_hdr->buf;
}

void buf_test() {
	int *foo = NULL;

	for (int i = 0; i < 1024; i++) {
		buf_push(foo, i);
	}
	assert(buf_len(foo) == 1024);

	for (int i = 0; i < 1024; i++) {
		assert(foo[i] == i);
	}

	buf_free(foo);
	assert(foo == NULL);
}

// String Interning

typedef struct {
	size_t len;
	const char *str;
} InternStr;

InternStr *interns;

const char *intern_str_range(const char *start, const char *end) {
	size_t len = end - start;
	for (InternStr *it = interns; it != buf_end(interns); it++) {
		if (it->len == len && strncmp(it->str, start, len) == 0) {
			return it->str;
		}
	}
	char *new_str = malloc(len + 1);
	memcpy(new_str, start, len);
	new_str[len] = '\0';
	buf_push(interns, (InternStr){len, new_str});
	return new_str;
}

const char *intern_str(const char *str) {
	return intern_str_range(str, str + strlen(str));
}

void intern_str_test() {
	const char *x = intern_str("foo");
	const char *y = intern_str("foo");
	assert(x == y);

	const char *dx = intern_str("bizz");
	const char *dy = intern_str("bizzbuzz");
	assert(dx != dy);
}
// Lexing

typedef enum {
	TOKEN_IDENT = 128,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_CHAR,
	TOKEN_STR,
} TokenType;

typedef struct {
	TokenType type;
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
	switch (*stream) {
	case ' ': case '\n': case '\r': case '\t': case '\v': case '\f':
		while (isspace(*stream)) {
			*stream++;
		}
		goto top;
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

void lex_test() {
	init_stream("10.0 0.034 .190 0.2E2 9.0E-2 .9e+10");
	assert_token_float(10.0);
	assert_token_float(0.034);
	assert_token_float(.190);
	assert_token_float(0.2E2);
	assert_token_float(9.0E-2);
	assert_token_float(.9e+10);

	init_stream("\'\\n\' \"foo \\n\"");
	assert_token_char('\n');
	assert_token_str("foo \n");
	assert_token_eof();

	init_stream("0x32 + 0b1010 + 036");
	assert_token_int(0x32);
	assert_token('+');
	assert_token_int(10);
	assert_token('+');
	assert_token_int(036);
	assert_token_eof();

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

int main(int argc, char **argv) {
	buf_test();
	intern_str_test();
	lex_test();

	return 0;
}
