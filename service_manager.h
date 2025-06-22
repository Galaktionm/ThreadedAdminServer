#ifndef SERVICE_MONITOR_H
#define SERVICE_MONITOR_H

#include <sys/types.h> // For pid_t

extern pid_t monitored_service_pid;
extern time_t server_start_time;

// Function to start a new service as a child process and monitor its execution.
// It uses a pipe to check if the execvp call in the child process was successful.
//
// const char *service_path: The path to the executable of the service.
// char *const service_argv[]: An array of string arguments for the service,
//                             with the last element being NULL.
//
// Returns: 0 on successful start of the service.
//          -1 if fork, pipe, or execvp failed.
int start_monitored_service(const char *service_path, char *const service_argv[]);

#endif // SERVICE_MONITOR_H