#include "signal.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

static void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    exit(0);
}

void install_signal_handlers() {
    struct sigaction sa = {
        .sa_handler = handle_signal
    };
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}
