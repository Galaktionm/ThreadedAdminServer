#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "auth.h"
#include "server.h"
#include "signal.h"
#include "service_manager.h"

int main(int argc, char *argv[]) {

    /*
     Placing this before launching the service does not work, because it is set after fork(),
     meaning it is inside a child process. It results in undefined behaviour. It outputs a very
     large number in our case
     */
    server_start_time = time(NULL);

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

    /*
    &array[index] expression gives you the address of the element at index in the array.
    How do we know it is a pointer to an array element and we can iterate over it,
    i.e. follow the next pointer, and we will get the next element? How does it differ from standard pointer?

    Let's say int *p = &arr[0]; C knows that p points to an int, so when you do p + 1,
    it automatically moves the pointer by sizeof(int) (typically 4 bytes). If we just do

    int x = 42;
    int *p = &x;

    p + 1 points to whatever garbage is 4 bytes after x — it's undefined behavior if you access it.

    Is it possible for int *p = &x; to return a valid int? Can another int be stored right next to it?

    The C standard says that pointer arithmetic is only valid within arrays
    (or one past the end of arrays, but even then you can't dereference).
    So while this might "work" on some machines, depending on the compiler,
    it’s illegal in standard C, and anything could happen.
    Only arrays guarantee contiguous memory layout that supports pointer arithmetic.
     */

    char **service_argv = &argv[2];

    /*
     Could we have used char *service_argv[] instead of char **service_argv?

     In C, arrays decay to pointers, so char *[]service_argv would decay to char** service_argv,
     so in function arguments they are essentially the same, therefore we could have used them interchangeably.

     However, inside a function they are different, since one is treated as an array, and one as a pointer.
     For example, using sizeof operations on them would give different results, since one would give us the
     size of an array (char* argv[]) and the other (char** argv) would give us the size of the pointer.
     */

    if (start_monitored_service(service_argv[0], service_argv) != 0) {
        fprintf(stderr, "Failed to launch monitored service, exiting.\n");
        return EXIT_FAILURE;
    }

    //init_auth_or_exit();
    install_signal_handlers(); // handle SIGINT, SIGTERM
    int server_fd = start_server(port); // Bind & listen

    printf("Starting server on port %d\n", port);

    accept_clients(server_fd); // Thread per request here

    close(server_fd);
    return 0;
}
