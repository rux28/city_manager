### Context:

The filter command accepts one condition, or optionally multiple conditions. If multiple conditions are supported, they are passed as distinct arguments separated by spaces. The command prints all reports that satisfy every condition, with conditions implicitly joined by AND. Each condition is a single string in the following form:

```text
field:operator:value
```

Supported fields: `severity`, `category`, `inspector`, `timestamp`. Supported operators: `==`, `!=`, `<`, `<=`, `>`, `>=`.

### Acceptance criteria:

1. Generate a function

```c
int parse_condition(const char *input, char *field, char *op, char *value);
```

which splits a `field:operator:value` string into its three parts.

2. Generate a function

```c
int match_condition(Report *r, const char *field, const char *op, const char *value);
```

which returns `1` if the record satisfies the condition and `0` otherwise.

### Implementation notes:

- The functions that need to be implemented are based on the report structure found in `report.h`
- The functions should be implemented in `utils.h`, as they are helper functions
- You should not implement `cmd_filter()`, only these helper functions
