// base64url.c
#include "base64.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <string.h>

void base64url_encode(const unsigned char *in, size_t in_len, char *out, size_t out_len) {
    BIO *b64, *bmem;
    BUF_MEM *bptr;
    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, in, in_len);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    size_t len = bptr->length;
    if (len >= out_len) len = out_len - 1;

    strncpy(out, bptr->data, len);
    out[len] = '\0';

    // Convert base64 → base64url
    for (size_t i = 0; i < len; ++i) {
        if (out[i] == '+') out[i] = '-';
        else if (out[i] == '/') out[i] = '_';
    }
    while (len > 0 && out[len - 1] == '=') len--;
    out[len] = '\0';

    BIO_free_all(b64);
}

int base64url_decode(const char *in, unsigned char *out, size_t out_len) {
    char buf[1024];
    size_t len = strlen(in);
    strncpy(buf, in, sizeof(buf));
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '-') buf[i] = '+';
        else if (buf[i] == '_') buf[i] = '/';
    }
    while (len % 4) buf[len++] = '=';

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new_mem_buf(buf, len);
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    int decoded = BIO_read(b64, out, out_len);
    BIO_free_all(b64);
    return decoded;
}
