#ifndef UTILS_H
#define UTILS_H

#include <sys/stat.h>
#include "report.h"

// Permission utilities
char *permissions_to_string(mode_t mode);
int has_read_permission(const char *path, const char *role);
int has_write_permission(const char *path, const char *role);
int parse_condition(const char *input, char *field, char *op, char *value);
int match_condition(Report *r, const char *field, const char *op, const char *value);

// District management
void ensure_district_exists(const char *district_id);
void create_symlink(const char *district_id);

// Logging
void log_operation(const char *district_id, const char *operation, const char *role, const char *user);

#endif // UTILS_H

