#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "auth.h"
#include "server.h"
#include "signal.h"

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <port> <service> [service args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse port
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    char **service_argv = &argv[2];

    if (start_monitored_service(service_argv[0], service_argv) != 0) {
        fprintf(stderr, "Failed to launch monitored service, exiting.\n");
        return EXIT_FAILURE;
    }

    init_auth_or_exit();
    install_signal_handlers(); // handle SIGINT, SIGTERM
    int server_fd = start_server(port); // Bind & listen

    printf("Starting server on port %d\n", port);

    accept_clients(server_fd); // Thread per request here

    close(server_fd);
    return 0;
}
