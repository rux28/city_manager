# city_manager

Run this project in an Ubuntu terminal.

Build:

```bash
gcc utils.c commands.c main.c -o city_manager
```

Run:

```bash
./city_manager --role <role> --user <user> <command> ...
```

Roles:
- `manager`
- `inspector`

Common requirements:
- `--role <role>` and `--user <user>` are required for every command.
- The program creates the district directory and required files automatically if they do not exist.

Commands:

1. `--add <district_id>`
Adds a new report to `<district_id>/reports.dat`.
Usage:
```bash
./city_manager --role <role> --user <user> --add <district_id>
```
Permissions needed:
- Requires write permission on `reports.dat`
- Works for roles that can write to `reports.dat`

2. `--list <district_id>`
Lists all reports from the district and shows file information.
Usage:
```bash
./city_manager --role <role> --user <user> --list <district_id>
```
Permissions needed:
- Requires read permission on `reports.dat`

3. `--view <district_id> <report_id>`
Shows full details for a single report.
Usage:
```bash
./city_manager --role <role> --user <user> --view <district_id> <report_id>
```
Permissions needed:
- Requires read permission on `reports.dat`

4. `--remove_report <district_id> <report_id>`
Removes one report from the district.
Usage:
```bash
./city_manager --role manager --user <user> --remove_report <district_id> <report_id>
```
Permissions needed:
- Manager only
- Requires write permission on `reports.dat`

5. `--update_threshold <district_id> <value>`
Updates the severity threshold stored in `<district_id>/district.cfg`.
Usage:
```bash
./city_manager --role manager --user <user> --update_threshold <district_id> <value>
```
Permissions needed:
- Manager only
- Requires write permission on `district.cfg`

6. `--filter <district_id> <condition> [condition ...]`
Prints only reports that match all given conditions.
Usage:
```bash
./city_manager --role <role> --user <user> --filter <district_id> <condition> [condition ...]
```
Permissions needed:
- Requires read permission on `reports.dat`

Condition format:
```text
field:operator:value
```

Supported fields:
- `severity`
- `category`
- `inspector`
- `timestamp`

Supported operators:
- `==`
- `!=`
- `<`
- `<=`
- `>`
- `>=`

Examples:
```bash
./city_manager --role manager --user alice --filter downtown severity:>=:2
./city_manager --role inspector --user bob --filter downtown category:==:road inspector:!=:alice
```

Windows note:
- In Windows terminals, especially PowerShell, filter conditions that contain operators should be wrapped in quotes.
- Example:

```powershell
.\city_manager --role manager --user alice --filter downtown "severity:>=:2" "category:==:road"
```

Created files and permissions:
- District directory: `0750`
- `reports.dat`: `0664`
- `district.cfg`: `0640`
- `logged_district`: `0644`
