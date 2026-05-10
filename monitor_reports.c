#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define MONITOR_PID_FILE ".monitor_pid"
#define MAX_PATH_LEN 256

volatile sig_atomic_t got_sigint = 0;
volatile sig_atomic_t got_sigusr1 = 0;

// Signal handler for SIGINT
void handle_sigint(int sig) {
    got_sigint = 1;
}

// Signal handler for SIGUSR1
void handle_sigusr1(int sig) {
    got_sigusr1 = 1;
}

// Write process ID to .monitor_pid file
int write_pid_file(pid_t pid) {
    int fd = open(MONITOR_PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open .monitor_pid");
        return -1;
    }

    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", pid);

    if (write(fd, pid_str, strlen(pid_str)) == -1) {
        perror("write pid to .monitor_pid");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

// Delete .monitor_pid file
int delete_pid_file(void) {
    if (unlink(MONITOR_PID_FILE) == -1) {
        perror("unlink .monitor_pid");
        return -1;
    }
    return 0;
}

// Get current timestamp string
const char *get_timestamp(void) {
    static char timestamp[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    return timestamp;
}

int main(void) {
    pid_t pid = getpid();
    struct sigaction sa;

    // Write PID to .monitor_pid file
    if (write_pid_file(pid) == -1) {
        fprintf(stderr, "Failed to write monitor PID file\n");
        return 1;
    }

    printf("[%s] Monitor started with PID %d\n", get_timestamp(), pid);
    fflush(stdout);

    // Set up signal handlers
    memset(&sa, 0, sizeof(sa));

    // Handler for SIGINT
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        delete_pid_file();
        return 1;
    }

    // Handler for SIGUSR1
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        delete_pid_file();
        return 1;
    }

    // Main loop
    while (!got_sigint) {
        if (got_sigusr1) {
            printf("[%s] Received notification: new report has been added\n", get_timestamp());
            fflush(stdout);
            got_sigusr1 = 0;
        }

        sleep(1);
    }

    printf("[%s] Received SIGINT, shutting down...\n", get_timestamp());
    fflush(stdout);

    // Clean up
    if (delete_pid_file() == -1) {
        fprintf(stderr, "Warning: Failed to delete monitor PID file\n");
    }

    printf("[%s] Monitor stopped\n", get_timestamp());
    fflush(stdout);

    return 0;
}
