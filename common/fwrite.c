#include "md5.h"
#include "defs.h"
#include "config.h"
#include "fwrite.h"

MD5_CTX hash;

void init_hash(const void *header, const unsigned int length)
{
    md5_init(&hash);
    md5_update(&hash, header, length);
}

void save_hash(unsigned char *h)
{
    md5_final(h, &hash);
}

void fwrite_double(FILE *f, const double t)
{
    fwrite(&t, sizeof(double), 1, f);

    md5_update(&hash, &t, sizeof(double));
}

void fwrite_float(FILE *f, const float t)
{
    fwrite(&t, sizeof(float), 1, f);

    md5_update(&hash, &t, sizeof(float));
}

void fwrite_int(FILE *f, const int t)
{
#if !WORDS_BIGENDIAN
    int q = swapint(t);

    fwrite(&q, sizeof(int), 1, f);
    md5_update(&hash, &q, sizeof(int));
#else
    fwrite(&t, sizeof(int), 1, f);
    md5_update(&hash, &t, sizeof(int));
#endif
}


void fwrite_unsigned(FILE *f, const unsigned int t)
{
#if !WORDS_BIGENDIAN
    int q = swapint(t);

    fwrite(&q, sizeof(int), 1, f);
    md5_update(&hash, &q, sizeof(unsigned int));
#else
    fwrite(&t, sizeof(int), 1, f);
    md5_update(&hash, &t, sizeof(unsigned int));
#endif
}

void fwrite_lydia_bool(FILE *f, const lydia_bool t)
{
#if !WORDS_BIGENDIAN
    lydia_bool q = swapint(t);

    fwrite(&q, sizeof(lydia_bool), 1, f);
    md5_update(&hash, &q, sizeof(lydia_bool));
#else
    fwrite(&t, sizeof(lydia_bool), 1, f);
    md5_update(&hash, &t, sizeof(lydia_bool));
#endif
}

void fwrite_lydia_symbol(FILE *f, const lydia_symbol t)
{
    unsigned int length;

    if (lydia_symbolNIL == t) {
	fwrite_int(f, -1);
	return;
    }
    length = (int)strlen(t->name);
    fwrite_unsigned(f, length);
    fwrite(t->name, sizeof(char), length, f);
    md5_update(&hash, t->name, sizeof(char) * length);
}

/*
 * Local variables:
 * mode: c
 * tab-width: 8
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=8 fdm=marker
 * vim<600: sw=4 ts=8
 */
