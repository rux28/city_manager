#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include "utils.h"
#include "report.h"

static int is_supported_field(const char *field) {
    return strcmp(field, "severity") == 0 ||
           strcmp(field, "category") == 0 ||
           strcmp(field, "inspector") == 0 ||
           strcmp(field, "timestamp") == 0;
}

static int is_supported_operator(const char *op) {
    return strcmp(op, "==") == 0 ||
           strcmp(op, "!=") == 0 ||
           strcmp(op, "<") == 0 ||
           strcmp(op, "<=") == 0 ||
           strcmp(op, ">") == 0 ||
           strcmp(op, ">=") == 0;
}

static int is_numeric_field(const char *field) {
    return strcmp(field, "severity") == 0 || strcmp(field, "timestamp") == 0;
}

static int is_ordering_operator(const char *op) {
    return strcmp(op, "<") == 0 ||
           strcmp(op, "<=") == 0 ||
           strcmp(op, ">") == 0 ||
           strcmp(op, ">=") == 0;
}

static int parse_long_long_value(const char *value, long long *result) {
    char *endptr;
    long long parsed;

    if (!value || !result || *value == '\0') {
        return 0;
    }

    errno = 0;
    parsed = strtoll(value, &endptr, 10);
    if (errno == ERANGE || *endptr != '\0') {
        return 0;
    }

    *result = parsed;
    return 1;
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    const char *first_colon;
    const char *second_colon;
    const char *third_colon;
    size_t field_len;
    size_t op_len;
    size_t value_len;

    if (!input || !field || !op || !value) {
        return 0;
    }

    first_colon = strchr(input, ':');
    if (!first_colon) {
        return 0;
    }

    second_colon = strchr(first_colon + 1, ':');
    if (!second_colon) {
        return 0;
    }

    third_colon = strchr(second_colon + 1, ':');
    if (third_colon) {
        return 0;
    }

    field_len = (size_t) (first_colon - input);
    op_len = (size_t) (second_colon - first_colon - 1);
    value_len = strlen(second_colon + 1);

    if (field_len == 0 || op_len == 0 || value_len == 0) {
        return 0;
    }

    memcpy(field, input, field_len);
    field[field_len] = '\0';

    memcpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    memcpy(value, second_colon + 1, value_len + 1);

    if (!is_supported_field(field) || !is_supported_operator(op)) {
        return 0;
    }

    if (!is_numeric_field(field) && is_ordering_operator(op)) {
        return 0;
    }

    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    long long numeric_value;
    long long report_numeric;
    int cmp;

    if (!r || !field || !op || !value) {
        return 0;
    }

    if (!is_supported_field(field) || !is_supported_operator(op)) {
        return 0;
    }

    if (is_numeric_field(field)) {
        if (!parse_long_long_value(value, &numeric_value)) {
            return 0;
        }

        if (strcmp(field, "severity") == 0) {
            report_numeric = r->severity;
        } else {
            report_numeric = (long long) r->timestamp;
        }

        if (strcmp(op, "==") == 0) {
            return report_numeric == numeric_value;
        }
        if (strcmp(op, "!=") == 0) {
            return report_numeric != numeric_value;
        }
        if (strcmp(op, "<") == 0) {
            return report_numeric < numeric_value;
        }
        if (strcmp(op, "<=") == 0) {
            return report_numeric <= numeric_value;
        }
        if (strcmp(op, ">") == 0) {
            return report_numeric > numeric_value;
        }
        return report_numeric >= numeric_value;
    }

    if (is_ordering_operator(op)) {
        return 0;
    }

    if (strcmp(field, "category") == 0) {
        cmp = strcmp(r->category, value);
    } else {
        cmp = strcmp(r->inspector, value);
    }

    if (strcmp(op, "==") == 0) {
        return cmp == 0;
    }

    return strcmp(op, "!=") == 0 && cmp != 0;
}

// Convert permission bits (st_mode) to symbolic string representation
char *permissions_to_string(mode_t mode) {
    static char str[10];
    str[0] = mode & S_IRUSR ? 'r' : '-';
    str[1] = mode & S_IWUSR ? 'w' : '-';
    str[2] = mode & S_IXUSR ? 'x' : '-';
    str[3] = mode & S_IRGRP ? 'r' : '-';
    str[4] = mode & S_IWGRP ? 'w' : '-';
    str[5] = mode & S_IXGRP ? 'x' : '-';
    str[6] = mode & S_IROTH ? 'r' : '-';
    str[7] = mode & S_IWOTH ? 'w' : '-';
    str[8] = mode & S_IXOTH ? 'x' : '-';
    str[9] = '\0';
    return str;
}

// Helper: Check if current role can read the file
int has_read_permission(const char *path, const char *role) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("stat");
        return 0;
    }
    if (strcmp(role, "manager") == 0) {
        return (st.st_mode & S_IRUSR) != 0;
    }
    return (st.st_mode & S_IRGRP) != 0;
}

// Helper: Check if current role can write the file
int has_write_permission(const char *path, const char *role) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("stat");
        return 0;
    }
    if (strcmp(role, "manager") == 0) {
        return (st.st_mode & S_IWUSR) != 0;
    }
    return (st.st_mode & S_IWGRP) != 0;
}

/*
 * Log an operation to the logged_district file for a given district
 * Format: timestamp | role | user | operation
 */
void log_operation(const char *district_id, const char *operation, const char *role, const char *user) {
    char log_path[MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s/logged_district", district_id);

    // Check if inspector is trying to write (should fail)
    if (strcmp(role, "inspector") == 0) {
        struct stat st;
        if (stat(log_path, &st) != -1) {
            // Check that inspector cannot write
            if (!(st.st_mode & S_IWOTH)) {
                fprintf(stderr, "Error: Inspector role cannot write to logged_district.\n");
                return;
            }
        }
    }

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, LOGGED_DISTRICT_PERM);
    if (fd == -1) {
        perror("open logged_district");
        return;
    }

    chmod(log_path, LOGGED_DISTRICT_PERM);

    time_t now = time(NULL);
    char log_entry[MAX_LOG_LEN];
    snprintf(log_entry, sizeof(log_entry), "%ld | %s | %s | %s\n", now, role, user, operation);

    write(fd, log_entry, strlen(log_entry));
    close(fd);
}

// Ensure a district directory exists with correct permissions and files
void ensure_district_exists(const char *district_id) {
    char dir_path[MAX_PATH_LEN];
    char reports_path[MAX_PATH_LEN];
    char cfg_path[MAX_PATH_LEN];

    snprintf(dir_path, sizeof(dir_path), "%s", district_id);
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);
    snprintf(cfg_path, sizeof(cfg_path), "%s/district.cfg", district_id);

    // Create directory
    if (mkdir(dir_path, DISTRICT_DIR_PERM) == -1 && errno != EEXIST) {
        perror("mkdir");
        return;
    }
    chmod(dir_path, DISTRICT_DIR_PERM);

    // Create reports.dat if it doesn't exist
    int fd = open(reports_path, O_RDWR | O_CREAT, REPORTS_DAT_PERM);
    if (fd == -1) {
        perror("open reports.dat");
        return;
    }
    close(fd);
    chmod(reports_path, REPORTS_DAT_PERM);

    // Create district.cfg if it doesn't exist
    fd = open(cfg_path, O_RDWR | O_CREAT, DISTRICT_CFG_PERM);
    if (fd == -1) {
        perror("open district.cfg");
        return;
    }

    // Write default threshold if file is empty
    struct stat st;
    fstat(fd, &st);
    if (st.st_size == 0) {
        const char *default_threshold = "2\n";
        write(fd, default_threshold, strlen(default_threshold));
    }
    close(fd);

    // Ensure permissions are exactly 640, even if file already existed
    chmod(cfg_path, DISTRICT_CFG_PERM);

    // Verify permissions were set correctly
    if (stat(cfg_path, &st) == 0) {
        mode_t expected_perm = S_IRUSR | S_IWUSR | S_IRGRP;
        if ((st.st_mode & 0777) != expected_perm) {
            // Force correct permissions if they're wrong
            chmod(cfg_path, DISTRICT_CFG_PERM);
        }
    }

    // Create logged_district if it doesn't exist
    char log_path[MAX_PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s/logged_district", district_id);
    fd = open(log_path, O_RDWR | O_CREAT, LOGGED_DISTRICT_PERM);
    if (fd == -1) {
        perror("open logged_district");
        return;
    }
    close(fd);
    chmod(log_path, LOGGED_DISTRICT_PERM);
}

// Create or update symlink for active_reports-<district_id>
void create_symlink(const char *district_id) {
    char symlink_name[MAX_PATH_LEN];
    char target_path[MAX_PATH_LEN];

    snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district_id);
    snprintf(target_path, sizeof(target_path), "%s/reports.dat", district_id);

    // Remove existing symlink if present
    unlink(symlink_name);

    // Create new symlink
    if (symlink(target_path, symlink_name) == -1) {
        perror("symlink");
    }
}

