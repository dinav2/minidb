# minidb

A minimal relational database engine written from scratch in C. It implements page-based storage, a table catalog, schema definitions, row insertion, cursor-based sequential scans, and row deletion -- all backed by a single flat file on disk.

## Architecture

```
 ┌───────────────────────────────────┐
 │           database.c              │  High-level API
 │  open, close, create_table,       │  (what users call)
 │  insert_row, delete_row, scan     │
 ├───────────────────────────────────┤
 │   table.c          page.c         │  Table catalog /    Page layout &
 │   Schema records,   page init,    │  schema mgmt        record storage
 │   catalog entries   add_record    │
 ├───────────────────────────────────┤
 │            pager.c                │  File I/O layer
 │  read_page, write_page,           │  (single file, seek-based)
 │  create_page, open, close         │
 └───────────────────────────────────┘
         ↕  stdio (fread/fwrite)
    ┌──────────────┐
    │  database.db  │  Flat file on disk
    └──────────────┘
```

### Modules

| File | Role |
|------|------|
| `types.h` | Fixed-width type aliases (`u8`, `u32`, `i32`, `f64`, etc.) |
| `page.h/c` | 4 KB page layout, header accessors, record packing |
| `pager.h/c` | File I/O -- reads, writes, and allocates pages by index |
| `table.h/c` | Table creation: writes catalog entries and column schemas |
| `database.h/c` | Public API that ties everything together |
| `main.c` | Demo program exercising the API |

## Storage Format

The database is a sequence of 4096-byte pages stored contiguously in a single file.

### Page Layout

```
Offset  Field          Size
─────────────────────────────
  0     page_id          4
  4     free_offset       4
  8     record_count      4
 12     next_page_id      4
 16     page_type         1
 17     (reserved)      111
─────────────────────────────
128     payload        3968
```

### Page Types

| Type | ID | Purpose |
|------|----|---------|
| Header  | `0x01` | Page 0 -- stores database version and page size |
| Data    | `0x02` | Holds table rows; linked together via `next_page_id` |
| Catalog | `0x03` | Page 1 -- one entry per table (name, row size, page range, schema pointer) |
| Schema  | `0x04` | Column definitions for a table (type, size, name) |

### On-disk Organization

```
File: [ Page 0 ][ Page 1 ][ Page 2 ][ Page 3 ][ Page 4 ] ...
        Header    Catalog   Schema A   Data A₁   Data A₂
```

The catalog (page 1) stores one entry per table. Each entry points to:
- A **schema page** holding the table's column definitions
- A **data page range** (`page_start` / `page_end`) where rows live

```
Catalog Entry "Table A"
  ├── schema_page ──→ Page 2 (column definitions)
  ├── page_start  ──→ Page 3 (first data page)
  └── page_end    ──→ Page 4 (last data page)

Data pages form a linked list:
  Page 3 ──next──→ Page 4 ──next──→ 0 (end)
```

When the last data page fills up, a new page is allocated and linked to the end of the chain.

### Row Deletion

Data pages include a 1-byte record header per row. Deleting a row sets this byte to `0x01` (tombstone). Scans skip tombstoned rows automatically.

## API

```c
int  db_open(Database *db, const char *filename);
int  db_create_table(Database *db, char *table_name, Column *schema, u32 column_count);
int  db_insert_row(Database *db, char *table_name, const void *buf, u32 length);
int  db_delete_row(Database *db, Cursor *cursor);
int  db_scan_table(Database *db, char *table_name, Cursor *cursor);
int  db_scan_next(Database *db, Cursor *cursor, void *buf);
void db_close(Database *db);
```

All functions return `0` on success, non-zero on error. Scanning uses a cursor: call `db_scan_table` to initialize, then `db_scan_next` in a loop until it returns `SCAN_END`.

## Build & Run

```sh
make        # compiles with gcc -std=c11, warnings enabled
./main      # runs the demo (creates/uses database.db)
```

Requires: GCC (or any C11 compiler) and Make. No external dependencies.
