#ifndef COMMANDS_H
#define COMMANDS_H

// Command functions
void cmd_add(const char *district_id, const char *role, const char *user);
void cmd_list(const char *district_id, const char *role);
void cmd_view(const char *district_id, const char *report_id_str, const char *role);
void cmd_remove_report(const char *district_id, const char *report_id_str, const char *role, const char *user);
void cmd_update_threshold(const char *district_id, const char *value_str, const char *role, const char *user);
void cmd_filter(const char *district_id, int argc, char *argv[], const char *role);

#endif // COMMANDS_H

