#ifndef REPORT_H
#define REPORT_H

#include <time.h>

// Constants
#define MAX_INSPECTOR_LEN 50
#define MAX_CATEGORY_LEN 20
#define MAX_DESC_LEN 100
#define MAX_PATH_LEN 256
#define MAX_LOG_LEN 512

// Permission constants
#define DISTRICT_DIR_PERM 0750            // rwxr-x---
#define REPORTS_DAT_PERM 0664          // rw-rw-r--
#define LOGGED_DISTRICT_PERM 0644  // rw-r--r--
#define DISTRICT_CFG_PERM 0640          // rw-r-----

// Report structure
typedef struct {
    int id;
    char inspector[MAX_INSPECTOR_LEN];
    double latitude;
    double longitude;
    char category[MAX_CATEGORY_LEN];
    int severity;
    time_t timestamp;
    char description[MAX_DESC_LEN];
} Report;

#endif // REPORT_H
