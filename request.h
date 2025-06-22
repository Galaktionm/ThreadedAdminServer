#ifndef REQUEST_H
#define REQUEST_H

void handle_logs_tail(int client_fd);
void handle_admin_rebuild(int client_fd);

// No specific includes are strictly necessary here, as the functions
// take basic types (int, char*). The dependencies for send_file_response
// and send_405 will be handled by response.h.

// Function to handle an incoming HTTP request from a client.
// It reads the request, parses the method and path, and dispatches
// to the appropriate response function (e.g., serving a file for GET,
// or sending a 405 for unsupported methods).
//
// int client_fd: The file descriptor of the connected client socket
//                from which the request is read and to which the
//                response is written.
void handle_request(int client_fd);

#endif // REQUEST_H