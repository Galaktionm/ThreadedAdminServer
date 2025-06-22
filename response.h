#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h> // For size_t
#include <stdio.h>  // For FILE (though not directly in function signatures, common for dprintf/fopen context)

// Function to send an HTTP 200 OK response and the content of a file.
// If the file cannot be opened, it calls send_404.
//
// int client_fd: The file descriptor of the client socket.
// const char *path: The path to the file to be served.
void send_file_response(int client_fd, const char *path);


void send_401(int client_fd);

// Function to send an HTTP 404 Not Found response.
//
// int client_fd: The file descriptor of the client socket.
void send_404(int client_fd);

// Function to send an HTTP 405 Method Not Allowed response.
//
// int client_fd: The file descriptor of the client socket.
void send_405(int client_fd);

#endif // RESPONSE_H