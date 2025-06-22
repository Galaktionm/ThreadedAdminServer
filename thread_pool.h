#ifndef THREAD_POOL_H
#define THREAD_POOL_H

// Forward declaration of functions from other modules that are used here.
// This prevents circular dependencies if request.h also includes thread_pool.h.
// It's generally better to include the full header if many things are used,
// but for a single function call, a forward declaration is sufficient
// and can reduce compilation time for large projects.
#include <unistd.h> // For close()

// Although not strictly necessary for the public interface,
// it's good practice to ensure pthread_t is defined if the header
// might be used in contexts where <pthread.h> isn't already included.
// However, since spawn_thread_for_client directly creates pthreads,
// including it here makes sense.
#include <pthread.h> // For pthread_t and other pthread types (though only pthread_t is exposed directly)


// Function to spawn a new thread to handle a client connection.
// The created thread will be detached, meaning its resources are
// automatically reclaimed upon termination.
//
// int client_fd: The file descriptor of the connected client socket.
//                Ownership of this file descriptor is passed to the
//                newly spawned thread, which will eventually close it.
void spawn_thread_for_client(int client_fd);

#endif // THREAD_POOL_H