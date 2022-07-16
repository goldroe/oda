/* ----------------------------------------------------------------------
 * Author: Carlos Rivera
 * Date: 07/15/2022
 * File: oda.c
 * --------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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
#define buf_free(b) ((b) ? free(buf__hdr(b)) : 0)

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
}

typedef enum {
	TOKEN_IDENT = 128,
	TOKEN_INT,
} TokenType;

typedef struct {
	TokenType type;
	const char *start;
	const char *end;

	typedef union {
		int val;
		const char *identifier;
	};
} Token;

const char *stream;
Token token;

void next_token() {
	token.start = stream;
	switch (*stream) {
		case '0': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9':
			token.type = TOKEN_INT;
			token.val = 0;
			while (isdigit(*stream)) {
				token.val *= 10;
				token.val += *stream - '0';
				stream++;
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
			break;
		default:
			token.type = *stream++;
			token.end = stream;
	}
}

void print_token() {
	switch (token.type) {
		case TOKEN_INT:
			printf("TOKEN_INT: %d\n", token.val);
			break;
		case TOKEN_IDENT:
			printf("TOKEN_IDENT: %s\n", token.identifier);
			break;
		default:
			if (token.type < 128) {
				printf("<ASCII: %c>\n", token.type);
			}	else {
				printf("<NON ASCII: %c>\n", token.type);
			}
	}
}

void lex_test() {
	printf("%s\n", stream);
	stream = "10+foo+bizz";
	next_token();
	while (token.type) {
		print_token();
		next_token();
	}
}

int main(int argc, char **argv) {
	buf_test();
	lex_test();

	return 0;
}