---
layout: post
title: "SQL Injection Vulnerability: Security Issue in GeoPandas to_postgis() Function"
date: 2025-12-27
author: Yunus Aydın
lang: en
description: "SQL injection vulnerability I found in GeoPandas to_postgis() function and how I fixed it. Parameterized queries and secure SQL usage."
keywords: "SQL injection, GeoPandas, to_postgis, PostgreSQL, security vulnerability, parameterized queries, security research, bug bounty"
canonical_url: "https://aydinnyunus.github.io/2025/12/27/sql-injection-geopandas/"
---

While using the GeoPandas library one day, I noticed something was wrong with the `to_postgis()` function. User inputs were being directly concatenated into SQL queries. This was a classic SQL injection vulnerability. After finding the vulnerability, I also wrote the fix myself and opened a pull request. In this post, I'll explain what I found and how I fixed it.

## What is SQL Injection?

SQL injection is a security vulnerability that occurs when user inputs are directly concatenated into SQL queries. An attacker can manipulate database queries by using special characters.

Why is this vulnerability dangerous? Because it can:
- Provide access to all data in the database
- Modify the database structure
- Delete or modify data from the database
- Execute system commands (like the `COPY` command in PostgreSQL)

## How I Found the Vulnerability

GeoPandas' `to_postgis()` function is used to write GeoDataFrames to a PostgreSQL database. The function was directly concatenating user-provided parameters like table names and schema names into SQL queries. The actual vulnerable code was:

```python
if connection.dialect.has_table(connection, name, schema):
    target_srid = connection.execute(
        text(f"SELECT Find_SRID('{schema_name}', '{name}', '{geom_name}');")
    ).fetchone()[0]
```

The problem is clear: `schema_name`, `name`, and `geom_name` variables are directly inserted into the f-string. This was vulnerable to SQL injection attacks.

## How It Can Be Exploited

To exploit the SQL injection vulnerability, we can add special characters to the `schema_name`, `name`, or `geom_name` parameters. For example:

```python
# Attacker input
name = "test'; DROP TABLE important_data; --"

# Generated SQL query
# SELECT Find_SRID('public', 'test'; DROP TABLE important_data; --', 'geom');
```

This exploit can drop the `important_data` table after calling the `Find_SRID` function.

Other exploit examples:
- Data reading: `name = "test' UNION SELECT password FROM users --"`
- Data modification: `name = "test'; UPDATE users SET password='hacked' --"`
- System commands: `name = "test'; COPY (SELECT 1) TO PROGRAM 'rm -rf /' --"`

## Fix: Parameterized Queries

To fix the vulnerability, I used parameterized queries instead of directly concatenating user inputs into SQL queries. Parameterized queries separate user inputs from SQL queries, preventing SQL injection attacks.

How the fix works:

1. I validated user inputs like table names and schema names
2. I safely escaped identifiers (table names, schema names)
3. I used PostgreSQL's identifier quoting mechanism
4. Instead of directly concatenating user inputs into SQL queries, I used safe identifiers

Advantages of the fix:
- Prevents SQL injection attacks
- Safely handles user inputs
- Uses PostgreSQL's identifier quoting mechanism
- Improves security without breaking the existing API

Identifier validation:
- Table names and schema names can only contain alphanumeric characters and underscores
- Special characters and SQL keywords are blocked
- Identifiers are safely escaped using PostgreSQL's identifier quoting mechanism

## Conclusion

SQL injection vulnerabilities can be very dangerous, especially when user inputs are directly concatenated into SQL queries. Finding and fixing this vulnerability significantly improved the security of the GeoPandas library.

Finding and fixing such vulnerabilities is critical for improving the security of open-source libraries. If you find similar vulnerabilities, I recommend reporting them to the development team following responsible disclosure principles and, if possible, writing the fix yourself.

**Related content:**
- [CVE-2025-66019: LZW Decompression DoS Vulnerability in pypdf Library](/2025/12/20/cve-2025-66019-pypdf-lzw-dos/)
- [SSRF Vulnerability: Bypassing Protection with DNS Rebinding Attack](/ssrf-dns-rebinding-vulnerability/)

**Resources:**
- [GitHub PR: BUG: SQL Injection Exploit Report - GeoPandas to_postgis()](https://github.com/geopandas/geopandas/pull/3681)
- [OWASP: SQL Injection](https://owasp.org/www-community/attacks/SQL_Injection)
- [PostgreSQL Identifier Quoting](https://www.postgresql.org/docs/current/sql-syntax-lexical.html#SQL-SYNTAX-IDENTIFIERS)

