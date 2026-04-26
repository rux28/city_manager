#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "commands.h"
#include "report.h"
#include "utils.h"

/*
 * add command: Append a new report to reports.dat
 * Usage: city_manager --role <role> --user <user> --add <district_id>
 */
void cmd_add(const char *district_id, const char *role, const char *user) {
    char reports_path[MAX_PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    if (!has_write_permission(reports_path, role)) {
        fprintf(stderr, "Error: %s cannot write to reports.dat.\n", role);
        return;
    }

    // Create a new report structure
    Report report;

    // Generate the next report ID by finding the maximum existing ID
    int fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat for reading");
        return;
    }

    int max_id = 0;
    Report temp_report;
    while (read(fd, &temp_report, sizeof(Report)) == sizeof(Report)) {
        if (temp_report.id > max_id) {
            max_id = temp_report.id;
        }
    }
    report.id = max_id + 1;
    close(fd);

    // Set inspector name from user parameter
    strncpy(report.inspector, user, MAX_INSPECTOR_LEN - 1);
    report.inspector[MAX_INSPECTOR_LEN - 1] = '\0';

    // Get user input for the report details
    printf("Enter latitude: ");
    scanf("%lf", &report.latitude);

    printf("Enter longitude: ");
    scanf("%lf", &report.longitude);

    printf("Enter category (road/lighting/flooding/etc): ");
    scanf("%49s", report.category);

    printf("Enter severity (1=minor, 2=moderate, 3=critical): ");
    scanf("%d", &report.severity);

    // Consume newline before reading description
    getchar();

    printf("Enter description: ");
    fgets(report.description, MAX_DESC_LEN, stdin);
    // Remove trailing newline if present
    size_t len = strlen(report.description);
    if (len > 0 && report.description[len - 1] == '\n') {
        report.description[len - 1] = '\0';
    }

    // Set the current timestamp
    report.timestamp = time(NULL);

    // Open reports.dat for appending
    fd = open(reports_path, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("open reports.dat for appending");
        return;
    }

    // Write the binary report record to the file
    if (write(fd, &report, sizeof(Report)) == -1) {
        perror("write report");
        close(fd);
        return;
    }

    close(fd);

    // Ensure permissions are set correctly
    chmod(reports_path, REPORTS_DAT_PERM);

    printf("Report %d added successfully to district %s\n", report.id, district_id);

    // Log the operation
    log_operation(district_id, "add", role, user);
}

/*
 * list command: List all reports in a district with file info
 * Usage: city_manager --role <role> --user <user> --list <district_id>
 */
void cmd_list(const char *district_id, const char *role) {
    char reports_path[MAX_PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    if (!has_read_permission(reports_path, role)) {
        fprintf(stderr, "Error: %s cannot read reports.dat.\n", role);
        return;
    }

    struct stat st;
    stat(reports_path, &st);

    // Print file information header
    printf("\n=== District: %s ===\n", district_id);
    printf("File: %s\n", reports_path);
    printf("Permissions: %s | Size: %ld bytes | Modified: %s",
           permissions_to_string(st.st_mode),
           (long) st.st_size,
           ctime(&st.st_mtime));
    printf("\n\n");

    // Open reports.dat for reading
    int fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat");
        return;
    }

    // Read and display all reports
    Report report;
    int count = 0;
    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        count++;
        printf("Report ID: %d\n", report.id);
        printf("  Inspector: %s\n", report.inspector);
        printf("  Location: (%.6f, %.6f)\n", report.latitude, report.longitude);
        printf("  Category: %s\n", report.category);
        printf("  Severity: %d\n", report.severity);
        printf("  Timestamp: %s", ctime(&report.timestamp));
        printf("  Description: %s\n", report.description);
        printf("\n");
    }

    close(fd);

    if (count == 0) {
        printf("No reports in this district.\n\n");
    } else {
        printf("Total reports: %d\n\n", count);
    }
}

/*
 * view command: Print full details of a specific report
 * Usage: city_manager --role <role> --user <user> --view <district_id> <report_id>
 */
void cmd_view(const char *district_id, const char *report_id_str, const char *role) {
    if (!report_id_str) {
        fprintf(stderr, "Error: Report ID required for view command.\n");
        return;
    }

    int target_id = atoi(report_id_str);
    char reports_path[MAX_PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    if (!has_read_permission(reports_path, role)) {
        fprintf(stderr, "Error: %s cannot read reports.dat.\n", role);
        return;
    }

    int fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat");
        return;
    }

    Report report;
    int found = 0;
    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        if (report.id == target_id) {
            found = 1;
            printf("\n=== Report Details ===\n");
            printf("Report ID: %d\n", report.id);
            printf("Inspector: %s\n", report.inspector);
            printf("Location: (%.6f, %.6f)\n", report.latitude, report.longitude);
            printf("Category: %s\n", report.category);
            printf("Severity: %d\n", report.severity);
            printf("Timestamp: %s", ctime(&report.timestamp));
            printf("Description: %s\n", report.description);
            printf("\n");
            break;
        }
    }

    close(fd);

    if (!found) {
        fprintf(stderr, "Error: Report ID %d not found in district %s.\n", target_id, district_id);
    }
}

/*
 * remove_report command: Remove a single report (manager only)
 * Usage: city_manager --role manager --user <user> --remove_report <district_id> <report_id>
 */
void cmd_remove_report(const char *district_id, const char *report_id_str, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Error: Only managers can remove reports.\n");
        return;
    }

    if (!report_id_str) {
        fprintf(stderr, "Error: Report ID required for remove_report command.\n");
        return;
    }

    int target_id = atoi(report_id_str);
    char reports_path[MAX_PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    if (!has_write_permission(reports_path, role)) {
        fprintf(stderr, "Error: Manager cannot write to reports.dat.\n");
        return;
    }

    int fd = open(reports_path, O_RDWR);
    if (fd == -1) {
        perror("open reports.dat");
        return;
    }

    Report report;
    off_t report_pos = -1;
    int report_index = 0;

    // Find the report to delete
    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        if (report.id == target_id) {
            report_pos = (off_t) report_index * sizeof(Report);
            break;
        }
        report_index++;
    }

    if (report_pos == -1) {
        fprintf(stderr, "Error: Report ID %d not found.\n", target_id);
        close(fd);
        return;
    }

    // Get total file size
    struct stat file_stat;
    fstat(fd, &file_stat);
    off_t file_size = file_stat.st_size;
    int total_records = file_size / sizeof(Report);

    // Shift all subsequent records one position earlier
    for (int i = report_index + 1; i < total_records; i++) {
        off_t read_pos = (off_t) i * sizeof(Report);
        off_t write_pos = (off_t) (i - 1) * sizeof(Report);

        lseek(fd, read_pos, SEEK_SET);
        if (read(fd, &report, sizeof(Report)) != sizeof(Report)) {
            perror("read record for shift");
            close(fd);
            return;
        }

        lseek(fd, write_pos, SEEK_SET);
        if (write(fd, &report, sizeof(Report)) != sizeof(Report)) {
            perror("write record for shift");
            close(fd);
            return;
        }
    }

    // Truncate file to remove the last record
    if (ftruncate(fd, file_size - sizeof(Report)) == -1) {
        perror("ftruncate");
        close(fd);
        return;
    }

    close(fd);

    printf("Report %d removed successfully from district %s\n", target_id, district_id);

    // Log the operation
    log_operation(district_id, "remove_report", role, user);
}

/*
 * update_threshold command: Update severity threshold (manager only)
 * Usage: city_manager --role manager --user <user> --update_threshold <district_id> <value>
 */
void cmd_update_threshold(const char *district_id, const char *value_str, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Error: Only managers can update thresholds.\n");
        return;
    }

    if (!value_str) {
        fprintf(stderr, "Error: Threshold value required for update_threshold command.\n");
        return;
    }

    char cfg_path[MAX_PATH_LEN];
    snprintf(cfg_path, sizeof(cfg_path), "%s/district.cfg", district_id);

    // Check and verify permissions
    struct stat st;
    if (stat(cfg_path, &st) == -1) {
        perror("stat district.cfg");
        return;
    }

    // Verify and fix permissions if needed (should be 640: rw-r-----)
    mode_t expected_perm = S_IRUSR | S_IWUSR | S_IRGRP;
    if ((st.st_mode & 0777) != expected_perm) {
        fprintf(stderr, "Warning: Correcting district.cfg permissions from %s to rw-r-----\n",
                permissions_to_string(st.st_mode));
        chmod(cfg_path, DISTRICT_CFG_PERM);
    }

    // Open and write the new threshold
    int fd = open(cfg_path, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("open district.cfg");
        return;
    }

    char threshold_str[32];
    snprintf(threshold_str, sizeof(threshold_str), "%s\n", value_str);

    if (write(fd, threshold_str, strlen(threshold_str)) == -1) {
        perror("write threshold");
        close(fd);
        return;
    }

    close(fd);

    // Ensure permissions are preserved
    chmod(cfg_path, DISTRICT_CFG_PERM);

    printf("Threshold updated to %s for district %s\n", value_str, district_id);

    // Log the operation
    log_operation(district_id, "update_threshold", role, user);
}

/*
 * filter command: Filter and display reports matching conditions
 * Usage: city_manager --role <role> --user <user> --filter <district_id> <condition> [condition ...]
 */
void cmd_filter(const char *district_id, int argc, char *argv[], const char *role) {
    char reports_path[MAX_PATH_LEN];
    int filter_index = -1;
    int condition_count = 0;
    int fd;
    Report report;
    char (*fields)[32];
    char (*ops)[8];
    char (*values)[128];

    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    fields = malloc((size_t) argc * sizeof(*fields));
    ops = malloc((size_t) argc * sizeof(*ops));
    values = malloc((size_t) argc * sizeof(*values));
    if (!fields || !ops || !values) {
        fprintf(stderr, "Error: Unable to allocate filter buffers.\n");
        free(fields);
        free(ops);
        free(values);
        return;
    }

    if (!has_read_permission(reports_path, role)) {
        fprintf(stderr, "Error: %s cannot read reports.dat.\n", role);
        free(fields);
        free(ops);
        free(values);
        return;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--filter") == 0) {
            filter_index = i;
            break;
        }
    }

    if (filter_index == -1 || filter_index + 2 >= argc) {
        fprintf(stderr, "Error: At least one filter condition is required.\n");
        free(fields);
        free(ops);
        free(values);
        return;
    }

    for (int i = filter_index + 2; i < argc; i++) {
        if (argv[i][0] == '-') {
            break;
        }

        if (!parse_condition(argv[i], fields[condition_count], ops[condition_count], values[condition_count])) {
            fprintf(stderr, "Error: Invalid filter condition '%s'.\n", argv[i]);
            free(fields);
            free(ops);
            free(values);
            return;
        }

        condition_count++;
    }

    if (condition_count == 0) {
        fprintf(stderr, "Error: At least one filter condition is required.\n");
        free(fields);
        free(ops);
        free(values);
        return;
    }

    fd = open(reports_path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat");
        free(fields);
        free(ops);
        free(values);
        return;
    }

    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        int matches_all = 1;

        for (int i = 0; i < condition_count; i++) {
            if (!match_condition(&report, fields[i], ops[i], values[i])) {
                matches_all = 0;
                break;
            }
        }

        if (matches_all) {
            printf("Report ID: %d\n", report.id);
            printf("  Inspector: %s\n", report.inspector);
            printf("  Location: (%.6f, %.6f)\n", report.latitude, report.longitude);
            printf("  Category: %s\n", report.category);
            printf("  Severity: %d\n", report.severity);
            printf("  Timestamp: %s", ctime(&report.timestamp));
            printf("  Description: %s\n", report.description);
            printf("\n");
        }
    }

    close(fd);
    free(fields);
    free(ops);
    free(values);
}

