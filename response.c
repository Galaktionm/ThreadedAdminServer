#include "response.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void send_file_response(int client_fd, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        send_404(client_fd);
        return;
    }

    dprintf(client_fd, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");

    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        write(client_fd, buf, n);
    }

    fclose(fp);
}

void send_401(int client_fd) {
    dprintf(client_fd, "HTTP/1.1 401 Unauthorized\r\n\r\n");
}

void send_404(int client_fd) {
    dprintf(client_fd, "HTTP/1.1 404 Not Found\r\n\r\nFile Not Found");
}

void send_405(int client_fd) {
    dprintf(client_fd, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
}
