#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "report.h"
#include "utils.h"
#include "commands.h"



int main(int argc, char *argv[]) {
    char *command = NULL;
    char *district_id = NULL;
    char *arg1 = NULL;
    char *role = NULL;
    char *user = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            role = argv[++i];
        } else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            user = argv[++i];
        } else if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            command = "add";
            district_id = argv[++i];
        } else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            command = "list";
            district_id = argv[++i];
        } else if (strcmp(argv[i], "--view") == 0 && i + 1 < argc) {
            command = "view";
            district_id = argv[++i];
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                arg1 = argv[++i];
            }
        } else if (strcmp(argv[i], "--remove_report") == 0 && i + 1 < argc) {
            command = "remove_report";
            district_id = argv[++i];
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                arg1 = argv[++i];
            }
        } else if (strcmp(argv[i], "--update_threshold") == 0 && i + 1 < argc) {
            command = "update_threshold";
            district_id = argv[++i];
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                arg1 = argv[++i];
            }
        } else if (strcmp(argv[i], "--filter") == 0 && i + 1 < argc) {
            command = "filter";
            district_id = argv[++i];
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                arg1 = argv[++i];
            }
        }
    }

    // Validate required arguments
    if (!role || !user) {
        fprintf(stderr, "Error: --role and --user are required.\n");
        return 1;
    }

    if (!command || !district_id) {
        fprintf(stderr, "Error: No command or district specified.\n");
        return 1;
    }

    if (strcmp(role, "manager") != 0 && strcmp(role, "inspector") != 0) {
        fprintf(stderr, "Error: Role must be 'manager' or 'inspector'.\n");
        return 1;
    }

    // Ensure district exists before executing command
    ensure_district_exists(district_id);

    // Dispatch command
    if (strcmp(command, "add") == 0) {
        cmd_add(district_id, role, user);
        create_symlink(district_id);
    } else if (strcmp(command, "list") == 0) {
        cmd_list(district_id, role);
    } else if (strcmp(command, "view") == 0) {
        cmd_view(district_id, arg1, role);
    } else if (strcmp(command, "remove_report") == 0) {
        cmd_remove_report(district_id, arg1, role, user);
        create_symlink(district_id);
    } else if (strcmp(command, "update_threshold") == 0) {
        cmd_update_threshold(district_id, arg1, role, user);
    } else if (strcmp(command, "filter") == 0) {
        cmd_filter(district_id, argc, argv, role);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'.\n", command);
        return 1;
    }

    return 0;
}
