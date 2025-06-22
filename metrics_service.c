#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "service_manager.h"

long get_rss_memory_kb(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    long kb = -1;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%ld", &kb);
            break;
        }
    }

    fclose(f);
    return kb;
}

double get_cpu_time_seconds(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    // Fields: pid (1) ... utime (14), stime (15)
    long utime = 0, stime = 0;
    char dummy[512];
    for (int i = 0; i < 13; ++i) fscanf(f, "%s", dummy); // skip 13 fields
    fscanf(f, "%ld %ld", &utime, &stime);
    fclose(f);

    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    return (utime + stime) / (double)ticks_per_sec;
}

int get_own_thread_count() {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return -1;

    char line[256];
    int threads = -1;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Threads:", 8) == 0) {
            sscanf(line + 8, "%d", &threads);
            break;
        }
    }

    fclose(f);
    return threads;
}

int get_thread_count_for_pid(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    int threads = -1;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Threads:", 8) == 0) {
            sscanf(line + 8, "%d", &threads);
            break;
        }
    }

    fclose(f);
    return threads;
}

void handle_metrics(int client_fd) {
    char body[2048];

    time_t now = time(NULL);
    long uptime = now - server_start_time;
    long rss_kb = get_rss_memory_kb(monitored_service_pid);
    double cpu_time = get_cpu_time_seconds(monitored_service_pid);
    int thread_count = get_thread_count_for_pid(monitored_service_pid);

    int len = snprintf(body, sizeof(body),
        "admin_service_uptime_seconds %ld\n"
        "monitored_service_pid %d\n",
        uptime, monitored_service_pid);

    if (rss_kb >= 0) {
        len += snprintf(body + len, sizeof(body) - len,
            "monitored_service_memory_bytes %ld\n", rss_kb * 1024L);
    }

    if (cpu_time >= 0) {
        len += snprintf(body + len, sizeof(body) - len,
            "monitored_service_cpu_seconds_total %.2f\n", cpu_time);
    }

    if (thread_count >= 0) {
        len += snprintf(body + len, sizeof(body) - len,
            "admin_service_thread_count %d\n", thread_count);
    }

    char header[128];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n",
        len);

    write(client_fd, header, strlen(header));
    write(client_fd, body, len);
}

