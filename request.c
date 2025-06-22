#include "request.h"

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auth.h"
#include "response.h"
#include "metrics_service.h"

static int READ_BUF_SIZE = 4096;

void handle_logs_tail(int client_fd) {
    const char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nHello Logs";
    write(client_fd, msg, 33);
}

void handle_admin_rebuild(int client_fd) {
    const char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nRebuild Done";
    write(client_fd, msg, 36);
}

void handle_auth_token(int client_fd, const char *body) {
    json_error_t error;
    json_t *root = json_loads(body, 0, &error);
    if (!root) {
        send_401(client_fd);  // invalid JSON
        return;
    }

    const char *username = json_string_value(json_object_get(root, "username"));
    const char *password = json_string_value(json_object_get(root, "password"));

    const char *env_username = getenv("AUTH_USERNAME");
    const char *env_password = getenv("AUTH_PASSWORD");

    if (!username || !password || !env_username || !env_password ||
        strcmp(username, env_username) != 0 || strcmp(password, env_password) != 0) {
        json_decref(root);
        send_401(client_fd);
        return;
        }

    char *jwt = generate_jwt(username);
    json_t *response = json_object();
    json_object_set_new(response, "token", json_string(jwt));
    free(jwt);

    char *response_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);

    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n",
             strlen(response_str));
    write(client_fd, header, strlen(header));
    write(client_fd, response_str, strlen(response_str));

    free(response_str);
    json_decref(root);
}

static char *read_http_request(int fd) {
    char *buffer = NULL;
    size_t buffer_size = 0;
    size_t data_len = 0;
    char temp_buf[READ_BUF_SIZE];

    while (1) {
        ssize_t n = read(fd, temp_buf, READ_BUF_SIZE);
        if (n <= 0) {
            // EOF or error
            break;
        }

        char *new_buf = realloc(buffer, data_len + n + 1);
        if (!new_buf) {
            free(buffer);
            return NULL;
        }
        buffer = new_buf;

        memcpy(buffer + data_len, temp_buf, n);
        data_len += n;
        buffer[data_len] = '\0';

        // Check for end of headers
        if (strstr(buffer, "\r\n\r\n") != NULL) {
            break;
        }
    }

    return buffer;
}

void handle_request(int client_fd) {
    char *request = read_http_request(client_fd);
    if (!request) {
        // Could not read request properly
        // Optionally send 400 Bad Request or just close connection
        close(client_fd);
        return;
    }

    // Parse method and path from request start line
    char method[8] = {0};
    char path[1024] = {0};

    if (sscanf(request, "%7s %1023s", method, path) != 2) {
        free(request);
        // Malformed request line
        // Send 400 Bad Request or close
        close(client_fd);
        return;
    }

    /*
    // Always check JWT token from headers
    char *token = extract_bearer_token(request);
    if (!token || !validate_token(token)) {
        free(token);
        send_401(client_fd);
        free(request);
        return;
    }
    free(token);
    */

    // Dispatch by method + path
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/metrics") == 0) {
            handle_metrics(client_fd);
        } else if (strcmp(path, "/logs/tail") == 0) {
            handle_logs_tail(client_fd);
        } else {
            send_404(client_fd);
        }
    } else if (strcmp(method, "POST") == 0) {

        if (strcmp(path, "/admin/rebuild") == 0) {
            handle_admin_rebuild(client_fd);
        } else {
            send_404(client_fd);
        }
    } else {
        send_405(client_fd);
    }

    free(request);
}