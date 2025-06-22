#ifndef AUTH_H
#define AUTH_H
#include <stdbool.h>

void init_auth_or_exit();
char *generate_jwt(const char *username);
char *extract_bearer_token(const char *headers);
bool validate_token(const char *token);

#endif
