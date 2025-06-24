#include "server.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "thread_pool.h"

int start_server(int port) {

    int server_fd = -1;
    int optval = 1; // Optval being 1 means the option argument should be "enabled" when used in socket options call

    /*
     * getaddrinfo() is often used to resolve hostnames like "chatgpt.com" to IP addresses.
     * But it does much more than that. In fact, it's the modern, flexible way to set up sockets,
     * both for clients and servers.
     *
     * Even though getaddrinfo() is often associated with DNS lookup (e.g., "google.com"),
     * when you pass NULL as the hostname and set AI_PASSIVE,
     * it instead gives you the addresses you should bind to as a server.
     *
     */

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;      // Support both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP, the other (datagram) being SOCK_DGRAM
    hints.ai_flags = AI_PASSIVE;      // For wildcard IP

    /* If the AI_PASSIVE flag is specified in hints.ai_flags, and node is
       NULL, then the returned socket addresses will be suitable for
       bind(2)ing a socket that will accept(2) connections.  The returned
       socket address will contain the "wildcard address" (INADDR_ANY for
       IPv4 addresses, IN6ADDR_ANY_INIT for IPv6 address).  The wildcard
       address is used by applications (typically servers) that intend to
       accept connections on any of the host's network addresses.  If
       node is not NULL, then the AI_PASSIVE flag is ignored. */

    char port_str[6]; // Max port is 65535, so 5 digits + null terminator
    snprintf(port_str, sizeof(port_str), "%d", port);

    int err = getaddrinfo(NULL, port_str, &hints, &res);
    if (err != 0) {
        // gai_strerror is specifically used for getaddrinfo's erros - Convert error return from getaddrinfo() to a string.
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    for (const struct addrinfo *rp = res; rp != NULL; rp = rp->ai_next) {

        /*
        * SOCK_NONBLOCK
              Sets the O_NONBLOCK file status flag on the open file
              description (see open(2)) referred to by the new file
              descriptor.  Using this flag saves extra calls to fcntl(2)
              to achieve the same result.

              On rare cases, calling fcntl separately on socket's fd
              can cause subtle bugs, because the operation is not atomic,
              while providing SOCK_NONBLOCK flag is atomic

        That means:
              Calls like accept(), recv(), and send() will not block the thread.
              Instead, they will return immediately:
                 recv() returns -1 with errno == EAGAIN or EWOULDBLOCK if no data.
                 accept() returns -1 with EAGAIN if no connection is ready.

        Because the server is using epoll, which is an event-driven, non-blocking I/O mechanism. That means:
        We don't want our thread to hang waiting for a slow client.
        Instead, we register the socket with epoll, and only act when the kernel tells you it’s ready.
         */

        server_fd = socket(rp->ai_family, rp->ai_socktype | SOCK_NONBLOCK, rp->ai_protocol);
        if (server_fd == -1) continue;

        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        if (bind(server_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break; // success
        }

        close(server_fd);
        server_fd = -1;
    }

    freeaddrinfo(res);

    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    if (listen(server_fd, 16) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

void accept_clients(int server_fd) {
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        spawn_thread_for_client(client_fd); // Thread function in thread_pool.c
    }
}
