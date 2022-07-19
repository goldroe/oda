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
} TokenType;

typedef struct {
	TokenType type;
	const char *start;
	const char *end;

	union {
		int64_t int_val;
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

int64_t scan_int(int64_t base) {
	int64_t val = 0;
	int64_t digit = 
	while (isalnum(*stream)) {
		digit = char_to_digit[*stream];
		if (val > (INT_MAX - digit) / 10) {
			syntax_error("integer literal overflow");
			val = 0;
		}
		if (digit >= base) {
			syntax_error("invalid character in decimal literal %c", *stream);
			digit = 0;
		}
		val = val * base + digit;
		stream++;
	}

	return digit;
}

void next_token() {
	token.start = stream;
	switch (*stream) {
	//case ' ': case '\n': case '\r': case '\t': case '\v': case '\f':
		//while (isspace(*stream)) {
		//	*stream++;
		//}
		//break;
	case '0': case '1': case '2': case '3': case '4': case '5': case '6':
	case '7': case '8': case '9':
		token.type = TOKEN_INT;
		int64_t val = 0;
		int64_t base = 10;
		if (*stream == '0') {
			stream++;
			if (tolower(*stream) == 'x') {
				stream++;
				base = 16;
				while (isalnum(*stream)) {
					int64_t digit = char_to_digit[*stream];
					if (val > (INT_MAX - digit) / 10) {
						syntax_error("integer literal overflow");
						val = 0;
					}
					if (digit >= base) {
						syntax_error("invalid character in hexadecimal literal %c", *stream); 
						digit = 0;
					}
					val = val * base + digit;
				}
			} else if (tolower(*stream) == 'b') {
				stream++;
				base = 2;
			} else if (isdigit(*stream)) {
				stream++;
				base = 8;
			} else {
				syntax_error("invalid integer literal prefix, %c", *stream);
				base = 1;
			}
		} else {
			while (isdigit(*stream)) {
				int64_t digit = char_to_digit[*stream];
				if (val > (INT_MAX - digit) / 10) {
					syntax_error("integer literal overflow");
					val = 0;
				}
				if (digit >= base) {
					syntax_error("invalid character in decimal literal %c", *stream);
					val = 0;
				}
				val = val * base + digit;

				stream++;
			}
		}

		token.int_val = val;
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

void lex_test() {
	init_stream("10+20+foo+bizz+bizzbuzz");
	while (token.type) {
		print_token();
		next_token();
	}
}

int main(int argc, char **argv) {
	buf_test();
	intern_str_test();
	lex_test();

	return 0;
}
