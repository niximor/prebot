#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <ctype.h>
#include "tb_string.h"

void strtolower(char *str) {
	for (size_t i = 0; i < strlen(str); i++) {
		str[i] = tolower(str[i]);
	}
}

