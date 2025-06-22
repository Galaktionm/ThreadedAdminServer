#include "thread_pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "request.h"

static void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    handle_request(client_fd); // Parse and respond
    close(client_fd);
    return NULL;
}

void spawn_thread_for_client(int client_fd) {
    pthread_t tid;
    int *fd_ptr = malloc(sizeof(int));
    *fd_ptr = client_fd;

    if (pthread_create(&tid, NULL, handle_client, fd_ptr) != 0) {
        perror("pthread_create");
        close(client_fd);
        free(fd_ptr);
    }
    pthread_detach(tid); // Fire and forget
}