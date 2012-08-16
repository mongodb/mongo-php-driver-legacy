#ifndef __HAVE_MCON_STR_H__
#define __HAVE_MCON_STR_H__

#define MCON_STR_PREALLOC 1024
#define mcon_str_ptr_init(str) str = malloc(sizeof(mcon_str)); str->l = 0; str->a = 0; str->d = NULL;
#define mcon_str_ptr_dtor(str) free(str->d); free(str)
#define mcon_str_dtor(str)     free(str.d)

typedef struct mcon_str {
	int   l;
	int   a;
	char *d;
} mcon_str;

void mcon_str_add(mcon_str *xs, char *str, int f);
void mcon_str_addl(mcon_str *xs, char *str, int le, int f);
void mcon_str_free(mcon_str *s);

#endif
