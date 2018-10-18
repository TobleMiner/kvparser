#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "util.h"
#include "kvparser.h"

ssize_t query_string_decode_value(char* value, size_t len) {
    char* search_pos = value, *limit = value + len;

    strntr(value, len, '+', ' ');

    while(search_pos < limit) {
        if(*search_pos != '%') {
            search_pos++;
            continue;
        }

        if(limit - search_pos < 2) {
            return -EINVAL;
        }

        *search_pos = hex_to_byte(search_pos + 1);
        search_pos++;
        limit -= 2;
        memmove(search_pos, search_pos + 2, limit - search_pos);
    }

    return limit - value;
}

ssize_t decode_uri_component(char** retval, char* str, size_t len) {
	ssize_t ret = query_string_decode_value(str, len);
	if(ret < 0) {
		return ret;
	}
	str[ret] = 0;
	*retval = str;
	return ret;
}

struct kv_str_processor_def uri_decode_proc = {
	.cb = decode_uri_component,
	.flags = { 0 },
};

int main() {
	int err;
	struct kvparser parser;
	kvlist kvpairs, *cursor, *next;

	char* str = strdup("a=b&b=c&asdf&last=%2fvalue&");

	INIT_LIST_HEAD(kvpairs);
	if((err = kvparser_init_processors(&parser, "&", "=", kv_get_zerocopy_str_proc(), &uri_decode_proc))) {
		printf("Failed to initialize kvparser: %s(%d)\n", strerror(err), err);
		abort();
	}

	if((err = kvparser_parse_string(&parser, &kvpairs, str, strlen(str)))) {
		printf("Failed to initialize kvparser: %s(%d)\n", strerror(err), err);
		abort();
	}

	LIST_FOR_EACH_SAFE(cursor, next, &kvpairs) {
		struct kvpair* pair = LIST_GET_ENTRY(cursor, struct kvpair, list);
		printf("KVPAIR: key='%.*s', value='%.*s'\n", pair->key_len, pair->key, pair->value_len, pair->value);
		kvparser_free_kvpair(&parser, pair);
	}

	return 0;
}
