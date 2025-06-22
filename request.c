#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "auth.h"
#include "response.h"

static int READ_BUF_SIZE = 4096;

void handle_metrics(int client_fd) {
    const char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello Metrics";
    write(client_fd, msg, 38);
}

void handle_logs_tail(int client_fd) {
    const char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nHello Logs";
    write(client_fd, msg, 33);
}

void handle_admin_rebuild(int client_fd) {
    const char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nRebuild Done";
    write(client_fd, msg, 36);
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

    // Always check JWT token from headers
    char *token = extract_bearer_token(request);
    if (!token || !validate_token(token)) {
        free(token);
        send_401(client_fd);
        free(request);
        return;
    }
    free(token);

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