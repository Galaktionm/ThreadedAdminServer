// base64url.h
#ifndef BASE64URL_H
#define BASE64URL_H
#include <stddef.h>

int base64url_decode(const char *in, unsigned char *out, size_t out_len);
void base64url_encode(const unsigned char *in, size_t in_len, char *out, size_t out_len);

#endif