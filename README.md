# city_manager

Run this project in an Ubuntu terminal.

## Build

```bash
# Build city_manager
gcc utils.c commands.c main.c -o city_manager

# Build monitor_reports
gcc monitor_reports.c -o monitor_reports
```

This produces two executables:
- `city_manager` - Main city infrastructure management program
- `monitor_reports` - Monitoring daemon that listens for report notifications


## Run

### City Manager

```bash
./city_manager --role <role> --user <user> <command> ...
```

### Monitor Reports Daemon

```bash
./monitor_reports
```

The monitor_reports program:
- Starts a monitoring daemon that watches for new report notifications
- Creates a `.monitor_pid` file containing its process ID
- Listens for SIGUSR1 signals (new report notifications) and prints messages
- Exits cleanly on SIGINT (Ctrl+C) and removes the `.monitor_pid` file

## Running Apps in Parallel (Multiple Terminals)

To use the full monitoring system with signal-based notifications:

**Terminal 1 - Start the monitor:**
```bash
./monitor_reports
# Output: [2024-05-10 14:30:45] Monitor started with PID 12345
```

**Terminal 2 - Run city_manager commands:**
```bash
./city_manager --role manager --user alice --add downtown
# This will notify the monitor_reports running in Terminal 1
# Terminal 1 output: [2024-05-10 14:30:50] Received notification: new report has been added
```

**To stop the monitor (from Terminal 1):**
```bash
# Press Ctrl+C
# Output: [2024-05-10 14:31:00] Received SIGINT, shutting down...
#         [2024-05-10 14:31:00] Monitor stopped
```

### How It Works

1. When `monitor_reports` starts, it writes its PID to `.monitor_pid`
2. When `city_manager --add` is executed, it reads the PID from `.monitor_pid`
3. `city_manager` sends a SIGUSR1 signal to the monitor process
4. `monitor_reports` receives the signal and prints a notification message
5. The operation is logged in the district's `logged_district` file

## Roles

- `manager` - Can add reports, remove reports, remove districts, update thresholds
- `inspector` - Can add reports, list reports, view reports, filter reports

## Common Requirements

- `--role <role>` and `--user <user>` are required for every command
- The program creates the district directory and required files automatically if they do not exist
- When adding a report, if monitor_reports is running, it will be notified automatically

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

6. `--remove_district <district_id>`
Deletes an entire district directory and all its contents, including the corresponding symlink.
Usage:
```bash
./city_manager --role manager --user <user> --remove_district <district_id>
```
Permissions needed:
- Manager only
- Uses a child process with `rm -rf` to remove the directory

7. `--filter <district_id> <condition> [condition ...]`
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
./city_manager --role manager --user alice --remove_district downtown
```

Windows note:
- In Windows terminals, especially PowerShell, filter conditions that contain operators should be wrapped in quotes.
- Example:

```powershell
.\city_manager --role manager --user alice --filter downtown "severity:>=:2" "category:==:road"
```

## Monitoring and Notifications

When a new report is added via `city_manager --add`:

1. **If monitor_reports is running:**
   - The program reads the PID from `.monitor_pid`
   - Sends a SIGUSR1 signal to the monitor process
   - Logs success in `logged_district`: `"Monitor notified"`

2. **If monitor_reports is not running:**
   - The add operation completes normally
   - Logs failure in `logged_district`: `"Monitor notification failed"`

The logged_district file tracks all operations with timestamps:
```
1715425800 | manager | alice | add | Monitor notified
1715425805 | manager | bob | add | Monitor notification failed
```

## Signal Handling Implementation

**monitor_reports uses:**
- `sigaction()` for reliable signal handling (not deprecated `signal()`)
- `volatile sig_atomic_t` for safe signal flags
- Minimal signal handlers that only set flags
- Main loop that checks flags and sleeps, allowing responsive signal processing

**city_manager process management:**
- Uses `fork()` to create child processes for system commands
- Checks all return values from `fork()`, `waitpid()`, and `exec*()`
- Validates child exit status with `WIFEXITED()` and `WEXITSTATUS()`
- Uses `execlp()` to search PATH for executables (security, portability)
- Properly waits for child processes to prevent zombies

## Created Files and Permissions

- District directory: `0750` (rwxr-x---)
- `reports.dat`: `0664` (rw-rw-r--)
- `district.cfg`: `0640` (rw-r-----)
- `logged_district`: `0644` (rw-r--r--)
- `.monitor_pid`: `0644` (rw-r--r--) - created by monitor_reports
