#ifndef SIGNAL_H
#define SIGNAL_H

// No specific includes are strictly necessary here, as the functions
// take basic types (void). The dependencies for sigaction and exit
// are handled by signal.c.

// Function to install signal handlers for graceful shutdown.
// It sets up handlers for SIGINT (Ctrl+C) and SIGTERM (termination signal)
// to print a message and exit the program.
void install_signal_handlers();

#endif // SIGNAL_H