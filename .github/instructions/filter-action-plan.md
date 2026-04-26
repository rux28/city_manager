# Filter Action Plan

1. Update `utils.h`
- Add declarations for:
```c
int parse_condition(const char *input, char *field, char *op, char *value);
int match_condition(Report *r, const char *field, const char *op, const char *value);
```

2. Implement `parse_condition()` in `utils.c`
- Parse exactly one `field:operator:value` string.
- Split it into `field`, `op`, and `value`.
- Return `0` for `NULL` inputs, invalid format, empty parts, unknown fields, unknown operators, or invalid field/operator combinations.
- Copy parsed parts into the output buffers and return `1` on success.

3. Implement `match_condition()` in `utils.c`
- Read the correct field from `Report`.
- Compare `severity` and `timestamp` numerically.
- Compare `category` and `inspector` using exact, case-sensitive string matching.
- Return `0` for `NULL` inputs, invalid fields/operators, or non-numeric values used with numeric fields.
- Return `1` only when the report matches the condition.

4. Add small private helpers in `utils.c` if needed
- Use helpers only for field validation, operator validation, numeric-field checks, or numeric conversion.

5. Verify expected cases
- Valid: `severity:>=:2`, `category:==:road`, `inspector:!=:alice`, `timestamp:<:1710000000`
- Invalid: `severity`, `category:>:road`, `foo:==:bar`, `severity:==:abc`

6. Keep the task scoped
- Do not modify `commands.c`
- Do not implement `cmd_filter()`
- Do not add file I/O, permissions, logging, or output formatting
