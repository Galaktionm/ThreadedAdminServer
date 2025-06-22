#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h> // For socket-related types like int and struct sockaddr_in
#include <netinet/in.h> // For internet addresses (htons, INADDR_ANY)

// Function to start the server and return its file descriptor.
// int port: The port number on which the server should listen.
// Returns: The file descriptor of the listening server socket on success,
//          or an error code (though the current implementation uses perror and returns the fd)
//          Consider returning -1 on error for better error handling in the caller.
int start_server(int port);

// Function to continuously accept incoming client connections.
// int server_fd: The file descriptor of the listening server socket.
// This function enters an infinite loop, accepting connections and
// spawning threads for each client.
void accept_clients(int server_fd);

#endif // SERVER_H