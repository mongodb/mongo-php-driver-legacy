#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

inline static mcon_resize(mcon_str *xs, int extra)
{
}

void mcon_str_add(mcon_str *xs, char *str, int f)
{
	return mcon_str_addl(xs, str, strlen(str), f);
}

void mcon_str_addl(mcon_str *xs, char *str, int le, int f)
{
	if (xs->l + le > xs->a - 1) {
		xs->d = realloc(xs->d, xs->a + le + MCON_STR_PREALLOC);
		xs->a = xs->a + le + MCON_STR_PREALLOC;
	}
	if (!xs->l) {
		xs->d[0] = '\0';
	}
	memcpy(xs->d + xs->l, str, le);
	xs->d[xs->l + le] = '\0';
	xs->l = xs->l + le;

	if (f) {
		free(str);
	}
}

void mcon_str_free(mcon_str *s)
{
	if (s->d) {
		free(s->d);
	}
}
