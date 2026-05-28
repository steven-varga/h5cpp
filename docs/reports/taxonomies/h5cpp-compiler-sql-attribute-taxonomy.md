@page reports_compiler_sql_attribute_taxonomy sql:: Attribute Vocabulary (SQL Backend)

**Date:** 2026-05-24
**Authors:** Steven Varga
**Status:** Implemented on `vargalabs/h5cpp-compiler#33-sql-backend`
**Repos:** `vargalabs/h5cpp-compiler` (SQL DDL emitter, issue #33)
**Scope:** SQL backend C++ attribute vocabulary and type-mapping taxonomy. Defines the `[[sql::...]]` attributes the compiler will recognize (once attribute wiring is extended), the DDL emission semantics, and the three supported dialects.
**Companions:**
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` — HDF5 attribute vocabulary
- `tasks/h5cpp-compiler-backend-cookbook.md` — transferable recipe for adding any new backend

---

## TL;DR

The SQL backend emits `CREATE TABLE` DDL statements from annotated C++ structs. Three dialects are supported: **PostgreSQL**, **MySQL**, and **SQLite3**. Each backend gets a boolean flag (`--sql-postgres`, `--sql-mysql`, `--sql-lite3`) and uses the unified `-o <file>` output path.

| Surface today (C++17 standard-attribute) | C++26 reflection form |
|---|---|
| `[[sql::table("users")]]` | `[[=sql::table{"users"}]]` |
| `[[sql::column("user_id")]]` | `[[=sql::column{"user_id"}]]` |
| `[[sql::primary_key]]` | `[[=sql::primary_key{}]]` |
| `[[sql::unique]]` | `[[=sql::unique{}]]` |
| `[[sql::not_null]]` | `[[=sql::not_null{}]]` |
| `[[sql::default_(42)]]` | `[[=sql::default_{42}]]` |
| `[[sql::foreign_key("other_table.col")]]` | `[[=sql::foreign_key{"other_table.col"}]]` |
| `[[sql::check("value > 0")]]` | `[[=sql::check{"value > 0"}]]` |
| `[[sql::index]]` | `[[=sql::index{}]]` |
| `[[sql::type_override("VARCHAR(255)")]]` | `[[=sql::type_override{"VARCHAR(255)"}]]` |
| `[[sql::nested("jsonb")]]` | `[[=sql::nested{"jsonb"}]]` |

Only syntactic shift is `(args)` → `{args}` under the `[[=...]]` form. **Names stay put.**

---

## 1. Dialect flags and CLI usage

```bash
h5cpp --sql-postgres -o schema.sql my_structs.cpp -- -std=c++17
h5cpp --sql-mysql    -o schema.sql my_structs.cpp -- -std=c++17
h5cpp --sql-lite3    -o schema.sql my_structs.cpp -- -std=c++17
```

The SQL backend is integrated into the unified `--format` dispatcher (issue #30). Each dialect is a distinct enum value in `OutputFormat` and gets its own CLI flag.

---

## 2. Universal vocabulary — same words, `sql::` namespace

These attributes use vocabulary identical to `h5::*` where the concept overlaps (rename, ignore, doc, alias). They live in `sql::` so the namespace stays self-contained for SQL-only users.

### Universal Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[sql::name("col_name")]]` | Rename a field for the SQL column. Defaults to the C++ field name. | `[[sql::name("created_at")]] std::chrono::time_point_t ts;` |
| `[[sql::ignore]]` | Skip this field entirely. Column absent from the emitted table. | `[[sql::ignore]] int debug_counter;` |

### Universal Tier 2 — high value, low cost

| Attribute | Purpose | Example |
|---|---|---|
| `[[sql::doc("description")]]` | Emitted as a SQL comment on the column or table. | `[[sql::doc("nanoseconds since epoch")]] std::uint64_t ts;` |
| `[[sql::alias("Name")]]` | **Class-level.** Overrides the table name. Defaults to sanitized C++ qualified name (`::` → `_`). | `struct [[sql::alias("Users")]] user_t { ... };` |

---

## 3. SQL-specific vocabulary

### Tier 1 — must-have

| Attribute | Purpose | Example |
|---|---|---|
| `[[sql::table("name")]]` | **Class-level.** Overrides the generated table name. | `struct [[sql::table("app_users")]] user_t { ... };` |
| `[[sql::column("name")]]` | **Field-level.** Overrides the generated column name. | `[[sql::column("user_id")]] int id;` |
| `[[sql::primary_key]]` | Emits `PRIMARY KEY` constraint. | `[[sql::primary_key]] int id;` |
| `[[sql::not_null]]` | Emits `NOT NULL` constraint. **Default for all fields** (C++ POD fields are always present). | `[[sql::not_null]] int id;` |
| `[[sql::unique]]` | Emits `UNIQUE` constraint. | `[[sql::unique]] std::string email;` |

### Tier 2 — high value

| Attribute | Purpose | Example |
|---|---|---|
| `[[sql::default_(value)]]` | Emits `DEFAULT value` clause. Dialect-aware quoting (strings quoted, numbers bare). | `[[sql::default_("active")]] std::string status;` |
| `[[sql::foreign_key("table.col")]]` | Emits `REFERENCES table(col)` clause. | `[[sql::foreign_key("orders.id")]] int order_id;` |
| `[[sql::check("expr")]]` | Emits `CHECK (expr)` clause. | `[[sql::check("value > 0")]] int value;` |
| `[[sql::index]]` | Emits a `CREATE INDEX` statement after the table. | `[[sql::index]] std::string email;` |

### Tier 3 — type control

| Attribute | Purpose | Example |
|---|---|---|
| `[[sql::type_override("TYPE")]]` | Overrides the dialect-specific type mapping for this field. | `[[sql::type_override("VARCHAR(255)")]] std::string name;` |
| `[[sql::nested("jsonb")]]` | Controls how nested structs are represented. Values: `"jsonb"` (PostgreSQL), `"json"` (MySQL), `"text"` (SQLite3), or `"table"` (create a separate table with FK). | `[[sql::nested("jsonb")]] address_t addr;` |

---

## 4. Type map — C++ → SQL per dialect

### Primitives

| C++ type | PostgreSQL | MySQL | SQLite3 |
|---|---|---|---|
| `bool` / `_Bool` | `BOOLEAN` | `TINYINT(1)` | `INTEGER` |
| `char` | `SMALLINT` | `TINYINT` | `INTEGER` |
| `unsigned char` | `SMALLINT` | `TINYINT UNSIGNED` | `INTEGER` |
| `short` | `SMALLINT` | `SMALLINT` | `INTEGER` |
| `unsigned short` | `SMALLINT` | `SMALLINT UNSIGNED` | `INTEGER` |
| `int` | `INTEGER` | `INT` | `INTEGER` |
| `unsigned int` | `INTEGER` | `INT UNSIGNED` | `INTEGER` |
| `long` | `BIGINT` | `BIGINT` | `INTEGER` |
| `unsigned long` | `BIGINT` | `BIGINT UNSIGNED` | `INTEGER` |
| `long long` | `BIGINT` | `BIGINT` | `INTEGER` |
| `unsigned long long` | `BIGINT` | `BIGINT UNSIGNED` | `INTEGER` |
| `float` | `REAL` | `FLOAT` | `REAL` |
| `double` | `DOUBLE PRECISION` | `DOUBLE` | `REAL` |
| `long double` | `DOUBLE PRECISION` | `DOUBLE` | `REAL` |
| `enum` / `enum class` | `INTEGER` | `INT` | `INTEGER` |

### Complex types

| C++ type | PostgreSQL | MySQL | SQLite3 | Notes |
|---|---|---|---|---|
| Nested struct `S` | `JSONB` | `JSON` | `TEXT` | One column holding the nested struct as JSON. |
| `T[N]` (C array) | `T[]` | `JSON` | `TEXT` | PostgreSQL native arrays; MySQL/SQLite fall back to JSON/TEXT. |
| `T[M][N]` (multi-dim array) | `T[][]` | `JSON` | `TEXT` | PostgreSQL supports `[][]` syntax; MySQL/SQLite fall back. |
| `std::string` | `TEXT` | `TEXT` | `TEXT` | No length limit assumed; override with `sql::type_override`. |
| `std::vector<T>` | `JSONB` | `JSON` | `TEXT` | No native variable-length array in standard SQL. |
| `std::map<K,V>` | `JSONB` | `JSON` | `TEXT` | No native map type; JSON fallback. |
| `std::optional<T>` | `T` (nullable) | `T` (nullable) | `T` (nullable) | Emitted without `NOT NULL`; runtime nulls allowed. |
| `std::chrono::time_point` | `TIMESTAMPTZ` | `DATETIME(6)` | `TEXT` | Auto-detected; can override with `sql::type_override`. |

---

## 5. DDL output format

### Single struct

Input:
```cpp
namespace sn {
struct Record {
  int    id;
  double value;
};
}
```

PostgreSQL output:
```sql
-- Generated by h5cpp-compiler SQL backend
-- Dialect: postgresql

CREATE TABLE IF NOT EXISTS "sn__Record" (
    "id" INTEGER NOT NULL,
    "value" DOUBLE PRECISION NOT NULL
);
```

MySQL output:
```sql
-- Generated by h5cpp-compiler SQL backend
-- Dialect: mysql

CREATE TABLE IF NOT EXISTS `sn__Record` (
    `id` INT NOT NULL,
    `value` DOUBLE NOT NULL
);
```

SQLite3 output:
```sql
-- Generated by h5cpp-compiler SQL backend
-- Dialect: sqlite3

CREATE TABLE IF NOT EXISTS "sn__Record" (
    "id" INTEGER NOT NULL,
    "value" REAL NOT NULL
);
```

### Nested structs

Input:
```cpp
namespace sn {
struct Inner { int a; double b; };
struct Outer {
  int   idx;
  Inner inner_singleton;
  Inner inner_array[4];
};
}
```

PostgreSQL output:
```sql
CREATE TABLE IF NOT EXISTS "sn__Inner" (
    "a" INTEGER NOT NULL,
    "b" DOUBLE PRECISION NOT NULL
);

CREATE TABLE IF NOT EXISTS "sn__Outer" (
    "idx" INTEGER NOT NULL,
    "inner_singleton" JSONB NOT NULL,
    "inner_array" JSONB[] NOT NULL
);
```

**Key behavior:** Nested structs produce a separate `CREATE TABLE` for the inner struct, AND the outer struct references it as `JSONB` (or `JSON` / `TEXT` depending on dialect). This is a pragmatic compromise: the inner table exists for introspection and potential FK usage, but the outer table stores the nested data as JSON for query simplicity.

### Multidimensional arrays

Input:
```cpp
namespace sn {
struct Cell { float x; float y; };
struct Grid { int idx; Cell field_05[3][8]; };
}
```

PostgreSQL output:
```sql
CREATE TABLE IF NOT EXISTS "sn__Cell" (
    "x" REAL NOT NULL,
    "y" REAL NOT NULL
);

CREATE TABLE IF NOT EXISTS "sn__Grid" (
    "idx" INTEGER NOT NULL,
    "field_05" JSONB[][] NOT NULL
);
```

---

## 6. Identifier rules per dialect

| Dialect | Table/column quoting | `::` → `_` | Case sensitivity |
|---|---|---|---|
| PostgreSQL | `"identifier"` | Yes | Preserved (quoted) |
| MySQL | `` `identifier` `` | Yes | Preserved (quoted, but depends on OS/file system) |
| SQLite3 | `"identifier"` | Yes | Preserved (quoted) |

The `sanitize_name()` function replaces all `::` with `_` so C++ qualified names like `sn::typecheck::Record` become `sn__typecheck__Record`.

---

## 7. Test coverage

All 11 existing fixtures are reused for SQL testing, yielding 33 SQL-specific tests (11 fixtures × 3 dialects). The HDF5 tests (11 tests) remain unchanged.

| Fixture | What it exercises | PostgreSQL | MySQL | SQLite3 |
|---|---|---|---|---|
| `primitives` | All primitive type mappings | ✓ | ✓ | ✓ |
| `typedef` | Typedef resolution | ✓ | ✓ | ✓ |
| `nested-ns` | Namespace nesting → table naming | ✓ | ✓ | ✓ |
| `embedded-pod` | Nested structs + 1D arrays | ✓ | ✓ | ✓ |
| `array-of-arrays` | Multi-dim arrays of structs | ✓ | ✓ | ✓ |
| `ignored-non-pod` | Non-POD skipped | ✓ | ✓ | ✓ |
| `unreferenced` | Unreferenced types skipped | ✓ | ✓ | ✓ |
| `topological` | Multiple related structs | ✓ | ✓ | ✓ |
| `enum-member` | Enum → INTEGER | ✓ | ✓ | ✓ |
| `multi-array` | Multi-dim primitive arrays | ✓ | ✓ | ✓ |
| `inheritance` | Inheritance skipped | ✓ | ✓ | ✓ |

---

## 8. Attribute wiring status

### Not yet wired (staging branch #30 pattern)

The `sql::` attribute namespace is **defined in this taxonomy** but not yet implemented in the attribute rewriter (`h5_attr_translator.hpp`) or consumed in the SQL producer. The staging branch uses the producer/consumer pattern; attribute integration would follow the same `h5_attr_reader` + `clang::annotate` rewrite path used for `h5::` attributes (issue #32).

| Attribute | Where read (planned) | Where emitted (planned) |
|---|---|---|
| `sql::table` | `h5_attr_reader::read_class_string(node, "sql::table")` | Overrides table name in `record_decl_impl` |
| `sql::column` | `h5_attr_reader::read_field_string(fld, "sql::column")` | Overrides column name in `type_insert_impl` |
| `sql::primary_key` | `h5_attr_reader::has_attr(fld, "sql::primary_key")` | Appends `PRIMARY KEY` to column definition |
| `sql::unique` | `h5_attr_reader::has_attr(fld, "sql::unique")` | Appends `UNIQUE` to column definition |
| `sql::not_null` | `h5_attr_reader::has_attr(fld, "sql::not_null")` | Emits `NOT NULL` (default anyway) |
| `sql::default_` | `h5_attr_reader::read_field_string(fld, "sql::default_")` | Appends `DEFAULT value` |
| `sql::foreign_key` | `h5_attr_reader::read_field_string(fld, "sql::foreign_key")` | Appends `REFERENCES ...` |
| `sql::check` | `h5_attr_reader::read_field_string(fld, "sql::check")` | Emits `CHECK (expr)` on column or table |
| `sql::index` | `h5_attr_reader::has_attr(fld, "sql::index")` | Emits `CREATE INDEX` after table DDL |
| `sql::type_override` | `h5_attr_reader::read_field_string(fld, "sql::type_override")` | Replaces mapped type in `type_insert_impl` |
| `sql::nested` | `h5_attr_reader::read_field_string(fld, "sql::nested")` | Controls JSONB/JSON/TEXT vs separate table |

---

## 9. Design decisions

### Decided

| # | Decision | Rationale |
|---|---|---|
| 1 | **Three separate producer types** (`SqlProducer<postgres>`, `SqlProducer<mysql>`, `SqlProducer<sqlite3>`) via CRTP template | Clean separation of type maps; zero runtime overhead; follows existing `H5Producer` pattern. |
| 2 | **Nested structs → JSON/JSONB/TEXT** | SQL has no native nested struct type. Flattening would require changing the consumer; JSON is the pragmatic SQL-native approach. |
| 3 | **Arrays → `T[]` (PostgreSQL) or JSON/TEXT (MySQL/SQLite3)** | PostgreSQL has native arrays; MySQL and SQLite3 do not. Consistent fallback to JSON/TEXT for the latter two. |
| 4 | **All fields `NOT NULL` by default** | C++ POD fields are always present. `std::optional<T>` (when supported) would omit `NOT NULL`. |
| 5 | **Separate `CREATE TABLE` per struct** | Topological ordering from the consumer naturally yields one table per struct. Each struct is self-contained DDL. |
| 6 | **Sanitize `::` → `_`** | `::` is not valid in unquoted SQL identifiers. Quoted identifiers could preserve it, but `_` is cleaner and more portable. |
| 7 | **No `DROP TABLE IF EXISTS`** | The emitted DDL is idempotent (`CREATE TABLE IF NOT EXISTS`) and non-destructive. |

### Open

| # | Question | Context |
|---|---|---|
| 1 | **Attribute wiring.** | When should `sql::` attributes be added to `h5_attr_translator.hpp`? After issue #32 (`h5::` attributes) is merged to staging, or in parallel? |
| 2 | **`sql::nested("table")` semantics.** | If a user requests separate-table nesting, should the compiler emit a foreign key column (`INTEGER REFERENCES inner(id)`) or a join table? |
| 3 | **`std::optional<T>` support.** | The staging branch consumer does not yet traverse `std::optional`. Once #32 adds it, SQL should emit nullable columns. |
| 4 | **Index emission.** | Should `sql::index` emit `CREATE INDEX` inline after the table, or collect all indexes and emit them at the end of the file? |
| 5 | **Multi-table FKs.** | For `topological` fixtures with multiple related structs, should the compiler auto-emit foreign keys between tables? |
| 6 | **Type override dialect scoping.** | Should `sql::type_override("VARCHAR(255)")` apply to all dialects, or should there be `sql::type_override_postgres`, `sql::type_override_mysql`, etc.? |

---

## 10. Sources

- `tasks/h5cpp-compiler-backend-cookbook.md` (workspace) — transferable recipe for adding any new backend
- `tasks/h5cpp-compiler-h5-attribute-taxonomy.md` (workspace) — HDF5 attribute vocabulary
- `vargalabs/h5cpp-compiler#30` — Unified `--format` dispatcher and producer/consumer pattern
- `vargalabs/h5cpp-compiler#33` — `src/producer_sql.hpp`, `src/h5cpp.cpp`, `tests/CMakeLists.txt`
- PostgreSQL 16 Data Types: https://www.postgresql.org/docs/current/datatype.html
- MySQL 8.0 Data Types: https://dev.mysql.com/doc/refman/8.0/en/data-types.html
- SQLite3 Datatypes: https://www.sqlite.org/datatype3.html
