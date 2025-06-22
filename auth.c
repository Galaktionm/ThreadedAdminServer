#include "auth.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <jansson.h>
#include <signal.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

static const char *HMAC_SECRET = NULL;

void init_auth_or_exit() {
    HMAC_SECRET = getenv("JWT_SECRET");
    if (!HMAC_SECRET) {
        fprintf(stderr, "[FATAL] JWT_SECRET not set. Shutting down.\n");
        raise(SIGTERM);
    }
}


/// Extracts the Bearer token value from the HTTP headers.
/// Returns a heap-allocated string containing the token (must be freed by caller),
/// or NULL if the token is not found or is malformed.
char *extract_bearer_token(const char *headers) {
    const char *auth_prefix = "Authorization: Bearer ";
    const char *auth_start = strstr(headers, auth_prefix);
    if (!auth_start) return NULL;

    auth_start += strlen(auth_prefix);
    const char *line_end = strstr(auth_start, "\r\n");
    if (!line_end) return NULL;

    size_t token_len = line_end - auth_start;
    if (token_len == 0) return NULL;

    char *token = malloc(token_len + 1);
    if (!token) return NULL;

    strncpy(token, auth_start, token_len);
    token[token_len] = '\0';
    return token;
}

/// Validates the given token against an expected value.
/// Returns true if the token is valid; false otherwise.
bool validate_token(const char *token) {
    if (!token) return false;

    // Split token into parts
    char *token_copy = strdup(token);
    if (!token_copy) return false;

    char *header_b64 = strtok(token_copy, ".");
    char *payload_b64 = strtok(NULL, ".");
    char *signature_b64 = strtok(NULL, ".");

    if (!header_b64 || !payload_b64 || !signature_b64) {
        free(token_copy);
        return false;
    }

    // Reconstruct signing input: header.payload
    size_t signing_input_len = strlen(header_b64) + 1 + strlen(payload_b64);
    char *signing_input = malloc(signing_input_len + 1);
    if (!signing_input) {
        free(token_copy);
        return false;
    }
    snprintf(signing_input, signing_input_len + 1, "%s.%s", header_b64, payload_b64);

    // Compute HMAC SHA256
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    HMAC(EVP_sha256(), HMAC_SECRET, strlen(HMAC_SECRET),
         (unsigned char *)signing_input, signing_input_len,
         digest, &digest_len);

    // Base64url encode the digest
    char encoded_sig[256];
    base64url_encode(digest, digest_len, encoded_sig, sizeof(encoded_sig));

    // Compare with signature from token
    bool valid = strcmp(encoded_sig, signature_b64) == 0;

    // Optionally check payload claims
    if (valid) {
        unsigned char decoded_payload[2048];
        if (base64url_decode(payload_b64, decoded_payload, sizeof(decoded_payload)) > 0) {
            decoded_payload[sizeof(decoded_payload) - 1] = '\0';
            json_error_t err;
            json_t *json = json_loads((const char *)decoded_payload, 0, &err);
            if (json) {
                json_t *exp = json_object_get(json, "exp");
                if (exp && json_is_integer(exp)) {
                    time_t now = time(NULL);
                    if (now > (time_t)json_integer_value(exp)) {
                        valid = false;  // Token expired
                    }
                }
                json_decref(json);
            }
        }
    }

    free(signing_input);
    free(token_copy);
    return valid;
}
