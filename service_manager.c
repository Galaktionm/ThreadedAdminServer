#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/prctl.h>

pid_t monitored_service_pid = -1;
time_t server_start_time = 0;

/*
char *const argv[] - Array is const, strings are modifiable
const char *argv[] - Strings are const, array is modifiable
const char *const argv[]	Both array and strings are const
 */


/*
 In the context of Linux/Unix, a pipe is a mechanism for inter-process communication (IPC) that allows
 one-way flow of data between two related processes. It acts like a conduit or a "first-in, first-out"
 (FIFO) buffer. Data written to one end of the pipe can be read from the other end.

 Characteristics:
 1. Data flows in only one direction. If you need two-way communication,
    you'll need two pipes (one for each direction).
 2. Data is treated as a stream of bytes. There's no inherent structure
    or message boundaries imposed by the pipe itself.
 3. Standard pipes are "anonymous" in the sense that they don't have a name in the filesystem.
    They exist only while the processes using them are running and are typically created by a common
    ancestor process (like a shell) that then forks.

Pipes are implemented in the kernel. When a pipe is created, the kernel allocates a small,
in-memory buffer (usually 64KB on modern Linux systems, though it can vary).

Pipes provide implicit synchronization. If a reader tries to read from an empty pipe, it blocks until
data is written. If a writer tries to write to a full pipe, it blocks until space becomes available.

FIFOS are non-anonymous, persistent pipes, which have a name in the filesystem
(they appear as a file, though they don't consume disk space for data). Because they have a name in the
filesystem, FIFOs can be used for communication between unrelated processes.
 */


int start_monitored_service(const char *service_path, char *const service_argv[]) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    else if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close read end, child writes errors if exec fails

        /*
         Why did we close pipefd[0]?

        When fork() is called, the child process inherits copies of all open file descriptors from the parent.
        This means the child process now has access to both pipefd[0] (read end) and pipefd[1] (write end)
        of the pipe.

        This specific pipe is intended for the child to write an error code to the parent if execvp fails.
        It's a one-way communication channel from child to parent for error reporting. The child's role
        is to write an error if execvp fails. It will never read from this pipe.

        If the child keeps pipefd[0] open, the parent's read() call on its end of the pipe might block
        indefinitely if the child successfully executes execvp (because the pipe's write end, pipefd[1],
        would then be closed, but the read end pipefd[0] would still appear open to the kernel in the child,
        preventing EOF from being signaled).
         */

        /*
          FD_CLOEXEC (File Descriptor Close-on-Exec) flag is specifically and solely designed to control
          whether a file descriptor is closed when one of the exec family of functions
          (like execve, execvp, execl, etc.) is successfully called.

          Set FD_CLOEXEC so that if execvp succeeds, pipe is closed automatically
        */
        if (fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) == -1) {   //Cloexec is important, otherwise the parent may block!!
            perror("fcntl FD_CLOEXEC failed");
            // Not fatal, continue anyway, although parent may block!!!!!!!!
        }


        /*
        prctl provides a mechanism for a process to control its own behavior or the behavior of its children
        in ways that are not covered by other system calls. It takes a command and typically one or more arguments.

        PR_SET_PDEATHSIG - This command registers a signal (arg2) that the calling process will receive
        if its parent process terminates. In this part of the code, we are in the CHILD process.
        This termination can be due to exiting normally, being killed, or crashing.
         */

        prctl(PR_SET_PDEATHSIG, SIGTERM);
        execvp(service_path, service_argv);

        // If execvp returns, it failed: write errno to pipe
        int err = errno;
        /*
         Write expects a buffer to write contents, not value. So we pass &err, memory location of err and
         tell it to write err number of bytes, essentially writing err.
         */
        write(pipefd[1], &err, sizeof(err));
        close(pipefd[1]);
        _exit(EXIT_FAILURE); // Use _exit to avoid flushing stdio buffers twice
    }
    else {
        // Parent process
        close(pipefd[1]); // Close write end, parent reads error status

        /*
        If pipes are synchronous by default, why doesn't this block indefinitely if child process
        launches successfully?

        The answer is that read won't block if all the writes are blocked (and read is empty).
        If the child process launches, it will close it's write pipe with CLOEXEC
         */

        int exec_error = 0;
        ssize_t n = read(pipefd[0], &exec_error, sizeof(exec_error));
        close(pipefd[0]);

        if (n == 0) {
            // Pipe closed with no data: exec succeeded, child replaced by service
            monitored_service_pid = pid;
            printf("Started monitored service with PID %d\n", (int)pid);
            return 0;
        }
        else if (n == sizeof(exec_error)) {
            // Exec failed, child wrote errno before exiting
            fprintf(stderr, "execvp failed with errno %d (%s)\n", exec_error, strerror(exec_error));
            waitpid(pid, NULL, 0); // Reap child to avoid zombie
            return -1;
        }
        else {
            fprintf(stderr, "Unexpected read from pipe: %zd bytes\n", n);
            waitpid(pid, NULL, 0);
            return -1;
        }
    }
}