/* Bagian Adrian - log identik melalui stdio dan system call. */
#include "logging.h"

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

void get_local_timestamp(char *buffer, size_t buffer_size)
{
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", local_time);
}

void make_logger_file_name(char *buffer, size_t buffer_size, const char *timestamp, const char *method, const char *sys_or_std)
{
    snprintf(buffer, buffer_size, "08-%s_%s-%s.txt", method, sys_or_std, timestamp);
}

void std_logger(const char *record, const char *file_name) {
    FILE *file = fopen(file_name, "a");
    if (file) {
        fprintf(file, "%s\n", record);
        fclose(file);
    } else {
        fprintf(stderr, "Error opening log file: %s\n", file_name);
    }
}

void sys_logger(const char *record, const char *file_name) {
    int file = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file != -1) {
        write(file, record, strlen(record));
        write(file, "\n", 1);
        close(file);
    } else {
        fprintf(stderr, "Error opening log file: %s\n", file_name);
    }
}

int write_logs(const char *method, const char *record)
{
    (void)method;
    (void)record;

    int buffer_size = 256;
    char timestamp[20];
    char sys_log_file_name[buffer_size];
    char std_log_file_name[buffer_size];

    get_local_timestamp(timestamp, sizeof(timestamp));
    make_logger_file_name(sys_log_file_name, sizeof(sys_log_file_name), timestamp, method, "sys");
    make_logger_file_name(std_log_file_name, sizeof(std_log_file_name), timestamp, method, "std");

    sys_logger(record, sys_log_file_name);
    std_logger(record, std_log_file_name);

    return 0;
}
