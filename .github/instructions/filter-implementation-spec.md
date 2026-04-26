# Filter Helper Implementation Specification

## 1. Overview

This document defines the implementation scope for the filter helper functions required by `filter-requirements.md`.

The goal is to prepare the helper logic that a future `filter` command implementation can call. This specification does **not** include implementing `cmd_filter()`.

### Supported condition syntax

```c
field:operator:value
```

### Supported fields

- `severity`
- `category`
- `inspector`
- `timestamp`

### Supported operators

- `==`
- `!=`
- `<`
- `<=`
- `>`
- `>=`

If multiple conditions are later passed to the filter command, they are separate arguments and are implicitly joined with AND logic. That command-level behavior is context only; the work in scope here is limited to the helper functions below.

---

## 2. Scope

### In scope

- Add a condition parser helper
- Add a condition matcher helper
- Base both helpers on the existing `Report` structure from `report.h`
- Expose these helpers through the utility layer

### Out of scope

- Implementing `cmd_filter()`
- Reading `reports.dat`
- Printing filtered results
- Permission checks
- Logging
- CLI argument validation beyond parsing a single condition string

---

## 3. Data Model Context

The helpers operate on the existing `Report` model:

```c
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
```

Only these fields participate in filtering:

| Field | Type | Comparison style |
|---|---|---|
| `severity` | `int` | numeric |
| `category` | string | exact string comparison |
| `inspector` | string | exact string comparison |
| `timestamp` | `time_t` | numeric |

---

## 4. Required Functions

### 4.1 `parse_condition`

```c
int parse_condition(const char *input, char *field, char *op, char *value);
```

#### Purpose

Split a single `field:operator:value` string into its three components.

#### Success behavior

- Returns `1` when parsing succeeds
- Writes the parsed field name into `field`
- Writes the parsed operator into `op`
- Writes the parsed comparison value into `value`

#### Failure behavior

- Returns `0` when parsing fails

#### Minimum expectations

- Accept input in the exact three-part format `field:operator:value`
- Split only into those three parts
- Reject malformed inputs
- Preserve the value portion as a string for later interpretation by `match_condition`

#### Validation expectations

- Validate that `field` is one of:
  - `severity`
  - `category`
  - `inspector`
  - `timestamp`
- Validate that `op` is one of:
  - `==`
  - `!=`
  - `<`
  - `<=`
  - `>`
  - `>=`
- Reject numeric ordering operators on string fields:
  - `category`
  - `inspector`

#### Edge cases to handle

- `NULL` input pointers
- Missing field/operator/value segments
- Empty field/operator/value segments
- Unknown field names
- Unknown operators
- Too few separators
- Extra separators that break the three-part format

### 4.2 `match_condition`

```c
int match_condition(Report *r, const char *field, const char *op, const char *value);
```

#### Purpose

Return whether a `Report` satisfies one already-parsed condition.

#### Success behavior

- Returns `1` if the report matches the condition
- Returns `0` if the report does not match the condition

#### Matching rules

- `severity`: compare `r->severity` numerically
- `timestamp`: compare `r->timestamp` numerically
- `category`: compare `r->category` as an exact string
- `inspector`: compare `r->inspector` as an exact string

#### Comparison expectations

- `==` and `!=` must work for all supported fields
- `<`, `<=`, `>`, `>=` must work only for numeric fields
- String comparisons are exact and case-sensitive

#### Value handling

- `value` remains a string input to this function
- For numeric fields, convert `value` to a numeric type before comparison
- For string fields, compare directly to the corresponding report field

#### Edge cases to handle

- `NULL` pointers
- Unsupported field names
- Unsupported operators
- Non-numeric values for numeric fields

On invalid input, the safe behavior is to return `0`.

---

## 5. Utility Layer Placement

`filter-requirements.md` places these functions in the utility/helper layer.

### Required interface change

- Add declarations for `parse_condition()` and `match_condition()` to `utils.h`

### Implementation placement

- Treat these as utility helpers rather than command handlers
- In this codebase, the practical implementation target is the utility module (`utils.c`) with declarations in `utils.h`
- If the team intentionally keeps helper implementations in headers, that can be decided during implementation, but the planning assumption should be: declare in `utils.h`, implement in the utility layer

This replaces the earlier assumption that they belong in `commands.c`.

---

## 6. Constraints and Assumptions

- The helpers must use the existing `Report` definition from `report.h`
- No new external dependencies are required
- Standard library usage such as `<string.h>`, `<stdlib.h>`, and `<time.h>` is sufficient
- The helpers should be reusable by a later `cmd_filter()` implementation

---

## 7. Acceptance Criteria

Implementation is complete when all of the following are true:

1. `parse_condition()` exists with the required signature.
2. `parse_condition()` splits a valid `field:operator:value` string into three outputs.
3. `parse_condition()` returns `0` for malformed or unsupported conditions.
4. `match_condition()` exists with the required signature.
5. `match_condition()` returns `1` when a `Report` satisfies the condition.
6. `match_condition()` returns `0` otherwise, including invalid input.
7. Both helpers are exposed through `utils.h`.
8. The implementation scope does not include `cmd_filter()`.

---

## 8. Planning Notes

For implementation planning, the work should be broken into these tasks:

1. Add helper declarations to `utils.h`.
2. Implement condition parsing in the utility layer.
3. Implement field-aware matching against `Report`.
4. Add focused tests for valid parsing, invalid parsing, string comparisons, and numeric comparisons.

The earlier sections in the old document about `cmd_filter()` integration, permissions, output formatting, and command-level file handling should not be used as planning requirements for this feature slice.
