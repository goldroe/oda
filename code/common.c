/* ----------------------------------------------------------------------
 * Author: Carlos Rivera
 * Date: 07/22/2022
 * File: common.c
 * --------------------------------------------------------------------*/

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void *xcalloc(size_t num_items, size_t item_size) {
	void *result = calloc(num_items, item_size);
	if (result == NULL) {
		perror("xcalloc failed");
		exit(1);
	}

	return result;
}
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
		new_hdr = xmalloc(new_size);
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
	char *new_str = xmalloc(len + 1);
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
