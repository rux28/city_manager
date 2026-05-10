### Question
How can I implement monitor_reports to respond to SIGUSR1 and SIGINT? What are the best practices?

### Best Practices & Answers

**Use `sigaction()` instead of `signal()`**
- `sigaction()` is more portable and reliable than `signal()`. It provides consistent behavior across different UNIX systems and allows fine-grained control over signal handling.
- Why: `signal()` has undefined behavior in some edge cases; `sigaction()` is POSIX-compliant.

**Use volatile sig_atomic_t for signal flags**
- Store signal state in `volatile sig_atomic_t` variables (e.g., `sigusr1_received`, `sigint_received`) instead of modifying global state directly.
- Why: The compiler won't optimize away reads/writes, and access is atomic—safe even in the presence of asynchronous signals.

**Keep signal handlers minimal**
- Handlers should only set flags, not perform I/O, memory allocation, or system calls (except for a few signal-safe functions like `write()` and `signal()`).
- Why: Most library functions are not async-signal-safe; complex operations in handlers can deadlock or corrupt state.

**SIGUSR1 Implementation**
- Handler sets a flag (e.g., `sigusr1_received = 1`). Main loop checks this flag each iteration and prints a message.
- Why: Avoids blocking in the handler; the signal merely wakes or notifies the main loop.

**SIGINT Implementation**
- Handler sets `sigint_received = 1`. Main loop checks this flag and exits gracefully, cleaning up (e.g., deleting `.monitor_pid`).
- Why: Allows controlled shutdown with proper resource cleanup, not abrupt termination.

**Example Signal Setup**
```c
struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = handle_sigusr1;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sigaction(SIGUSR1, &sa, NULL);
```
- `sigemptyset()` ensures no signals are blocked while the handler runs.
- `sa_flags = 0` means no extra options (e.g., no automatic restart of interrupted syscalls).

**Main Loop Pattern**
- Loop continuously, sleep briefly (`sleep(1)`), and check flags. Exit on SIGINT.
- Why: Simple, responsive, and avoids busy-waiting; the signal handlers interrupt `sleep()` so notifications are processed quickly.


### Question
What are best practices when working with processes? Give examples and explain why.

### Best Practices & Answers

**1. Always check return values from fork(), wait(), and exec*()**
- Example:
```c
pid_t pid = fork();
if (pid == -1) {
    perror("fork");
    exit(1);
}
if (pid == 0) {
    execlp("rm", "rm", "-rf", directory, NULL);
    perror("execlp");
    exit(1);
}
int status;
waitpid(pid, &status, 0);
if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    printf("Success\n");
} else {
    fprintf(stderr, "Child failed\n");
}
```
- Why: Errors indicate resource exhaustion or invalid arguments. Ignoring them causes undefined behavior and hard-to-debug issues.

**2. Always wait for child processes**
- Use `waitpid(pid, &status, 0)` or `wait(&status)` to reap child processes.
- Why: Prevents zombie processes that accumulate in the system and consume resources; allows checking the child's exit status.

**3. Check child exit status using WIFEXITED() and WEXITSTATUS()**
- Example: `if (WIFEXITED(status) && WEXITSTATUS(status) == 0)` confirms normal exit with code 0.
- Why: Child processes can exit abnormally (signal termination), timeout, or exit with non-zero codes indicating failure. WIFEXITED() checks for normal exit; WEXITSTATUS() extracts the code.

**4. Use execlp() or execvp() over hardcoded paths**
- Example: `execlp("rm", "rm", "-rf", dir, NULL)` uses PATH to find the executable.
- Example (bad): `execl("/bin/rm", "rm", "-rf", dir, NULL)` hardcodes the path.
- Why: More portable across systems where binaries may be in different locations; execlp() searches PATH automatically.

**5. Pass argv[0] correctly in exec*()**
- Example: `execlp("rm", "rm", "-rf", dir, NULL)` — second argument is argv[0].
- Why: argv[0] is the program name as seen by the child; tools and shell use it for error messages and debugging.

**6. Avoid shell injection with exec*() family**
- Example (safe): `execlp("rm", "rm", "-rf", directory, NULL)` — arguments are separate.
- Example (unsafe): `system("rm -rf " + user_input)` — concatenates user input into a shell string.
- Why: Direct exec() bypasses the shell and prevents interpretation of metacharacters; shell injection allows arbitrary commands.

**7. Handle SIGCHLD to avoid zombies in long-running daemons**
- Example:
```c
void sigchld_handler(int sig) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}
struct sigaction sa;
sa.sa_handler = sigchld_handler;
sigaction(SIGCHLD, &sa, NULL);
```
- Why: Spawning many children without handling SIGCHLD fills the process table with zombies; WNOHANG prevents blocking.

**8. Preserve important resources before fork() and child isolation after**
- Example: Close file descriptors in child if not needed; open new ones after exec().
- Why: File descriptors are inherited; unclosed descriptors lock resources or allow unintended access. Clean state prevents bugs and security issues.

**9. Set appropriate permissions/umask before creating child processes that write files**
- Example: `umask(0022)` before fork ensures child-created files have predictable permissions.
- Why: Child processes inherit umask; setting it early ensures all operations use the intended permissions.

**10. Use waitpid() with specific options for different scenarios**
- Example: `waitpid(pid, &status, 0)` blocks; `waitpid(pid, &status, WNOHANG)` returns immediately.
- Why: Blocking waits stall the parent; WNOHANG allows polling or handling multiple children concurrently.
