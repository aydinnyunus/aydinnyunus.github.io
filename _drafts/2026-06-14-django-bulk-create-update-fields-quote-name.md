---
layout: post
title: "Django bulk_create() update_fields: when quote_name() forgets to escape"
date: 2026-06-14
author: Yunus Aydın
lang: en
description: "Django's quote_name() wraps SQL identifiers in double quotes but never escapes embedded quotes. Pass a malformed field name to bulk_create()'s update_fields and the ON CONFLICT clause breaks."
keywords: "Django, SQL injection, bulk_create, update_fields, quote_name, ORM, identifier escaping, on_conflict_suffix_sql"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/django-bulk-create-update-fields-quote-name/"
---

I reported a possible SQL injection in Django's `bulk_create()` to the Django security team. The bug is in `quote_name()`: it wraps SQL identifiers in outer double quotes but never escapes embedded double quotes inside the identifier. Pass a field whose name contains a `"` to `update_fields`, and the generated `ON CONFLICT ... DO UPDATE SET` clause comes out broken (or injectable, depending on what you can get into the column name).

I was reading the `bulk_create()` code path because I wanted to understand how Django composes `ON CONFLICT` SQL on PostgreSQL. The PoC is small enough to fit in a blog post.

Reproduced on Django 4.2.x and 5.x against PostgreSQL 15 and SQLite 3.45.

## The model

Django lets you set `db_column` to almost any string at model definition time. Including one with an embedded quote:

```python
class Experiment(models.Model):
    start_datetime = models.DateTimeField()
    end_datetime = models.DateTimeField(null=True, blank=True)
    test = models.TextField(null=True, blank=True, db_column='test"')
```

That `db_column='test"'` is the lever. Most developers will never do that on purpose, but the value flows into SQL identifier generation without any sanitization.

## The vulnerable view

Here is a contrived view that exposes the issue. The `field` query parameter goes straight into `update_fields`:

```python
def vuln_bulk_create(request):
    payload = request.GET.get('field')

    start = datetime(2015, 6, 15)
    end = datetime(2015, 7, 2)
    objects = [
        Experiment(start_datetime=start, end_datetime=start),
        Experiment(start_datetime=end, end_datetime=end),
    ]

    Experiment.objects.bulk_create(
        objects,
        update_conflicts=True,
        update_fields=[payload, 'end_datetime'],
        unique_fields=['start_datetime', 'end_datetime'],
    )
```

Passing user input straight to `update_fields` is a developer mistake. Hold that thought.

## Root cause: quote_name() doesn't escape

Here's `quote_name()` for PostgreSQL (`django/db/backends/postgresql/operations.py`):

```python
def quote_name(self, name):
    if name.startswith('"') and name.endswith('"'):
        return name  # Quoting once is enough.
    return '"%s"' % name
```

Two problems live in those four lines.

First, **no escaping of internal `"`.** If `name` is `te"st`, the function returns `"te"st"`. That's three quotes, not two. The SQL identifier closes early at the middle quote, and whatever follows is parsed as the next SQL token.

Second, **the "already quoted" check is too lenient.** It fires on `startswith('"') AND endswith('"')`. Supply `"foo"` and the function returns it unchanged, which is fine. Supply `"foo` (only the leading quote) and the check fails, so the function wraps it: `""foo"`. Same identifier-closes-early problem from a different angle.

The `on_conflict_suffix_sql()` builder then feeds these "quoted" strings into a `%`-format template:

```python
def on_conflict_suffix_sql(self, fields, on_conflict, update_fields, unique_fields):
    if on_conflict == OnConflict.UPDATE:
        return "ON CONFLICT(%s) DO UPDATE SET %s" % (
            ", ".join(map(self.quote_name, unique_fields)),
            ", ".join(
                [
                    f"{field} = EXCLUDED.{field}"
                    for field in map(self.quote_name, update_fields)
                ]
            ),
        )
```

With `db_column='test"'`, here is the SQL Django emits for the `INSERT ... ON CONFLICT ... DO UPDATE`:

```sql
INSERT INTO "vuln_experiment" (..., "test"") VALUES (...)
ON CONFLICT ("start_datetime", "end_datetime")
DO UPDATE SET "test"" = EXCLUDED."test"", "end_datetime" = EXCLUDED."end_datetime"
```

The `"test""` token is broken. The opening quote starts an identifier, the second quote closes it, the third quote opens a new identifier that never closes cleanly. PostgreSQL throws a syntax error here, but the same code path with a more carefully chosen column name keeps the statement parseable. Consider `db_column='id" = 1, "x'`. After `quote_name()` wraps it, the `DO UPDATE SET` fragment becomes `"id" = 1, "x" = EXCLUDED."id" = 1, "x"`. The closing quote in the middle terminates Django's intended identifier, the `= 1, "x"` slips into the SET list as an attacker-controlled assignment, and the trailing `EXCLUDED.` reference now points somewhere Django never planned for. That is the shape of the injection: not classical `; DROP TABLE`, but the ability to rewrite the `DO UPDATE SET` clause.

## Why I call it "possible" and not "guaranteed"

Two gates sit between the bug and exploitation.

The first gate is getting a malicious identifier into the field list. Two ways to do it:

1. A developer passes `request.GET` / `request.POST` data straight to `update_fields`, `only`, `defer`, `order_by`, or any other ORM kwarg that takes identifier strings. Django's [security policy](https://docs.djangoproject.com/en/dev/topics/security/#sql-injection-protection) explicitly calls this out as documented misuse: identifiers must come from trusted sources.
2. The developer registers a `db_column` whose name contains a `"`. Unusual, but legal at model definition time and no validation rejects it.

The second gate is `_check_bulk_create_options` and `model._meta.get_field()`. For `update_fields`, Django resolves names via `get_field()`, which means the value must match an actual field name on the model. A model field literally named `te"st` is the only way through. That is why `db_column` is the more realistic angle: an attacker who can influence model definitions (migrations from untrusted sources, generated models, plugin metadata, schema introspection feeding into model definitions) can get a malformed identifier through.

## Impact

If the malformed identifier survives to SQL execution, three outcomes are on the table:

- The `ON CONFLICT ... DO UPDATE SET` clause is malformed and PostgreSQL refuses the statement. Denial of service for that code path.
- An attacker who fully controls the column name appends SQL after the closing quote. Classic identifier-based SQL injection.
- SQLite's `on_conflict_suffix_sql()` uses the same `quote_name()` and the same `%`-format assembly, so a `db_column='test"'` produces an identical broken token there.

## The fix

`quote_name()` should escape embedded `"` by doubling them. That is the SQL standard identifier escape rule. Something like:

```python
def quote_name(self, name):
    if name.startswith('"') and name.endswith('"'):
        return name
    return '"%s"' % name.replace('"', '""')
```

The "already quoted" shortcut should be retired, or tightened so it only fires on a balanced pair of unescaped quotes at the edges. The current `startswith and endswith` check is the kind of validation that looks safe and isn't.

## Why this pattern keeps showing up

Identifier quoting feels like a solved problem because string quoting is solved. Django parametrizes all values via the DB driver, so any value passed through `Model.objects.filter(...)` flows through prepared statements. But identifiers go through a completely different path: ORMs assemble them with `%` formatting and trust the inputs. The implicit contract is "identifiers come from the developer, not from the request." Nothing in the type system or the API surface communicates that contract clearly.

So every few years a developer writes `update_fields=[request.GET.get('field')]` because the API accepts it, the test passes, and the failure mode is invisible until someone with a `"` in mind shows up. [CWE-89](https://cwe.mitre.org/data/definitions/89.html) captures the injection class, but the deeper lesson is that identifier-taking APIs should either reject untrusted input loudly or escape it defensively. Quietly accepting a `"` and pasting it into SQL is the worst of both worlds.

## Disclosure timeline

- **June 14, 2026**: I reported the issue and PoC to `security@djangoproject.com`.
- **Status**: Response pending at the time of writing. I will update this post once Django's security team weighs in. If they classify it as documented misuse (which their security policy strongly hints at), the patch belongs in your application code: never pass user input to `update_fields`. If they accept it as a hardening fix, `quote_name()` gets the proper double-escape.

## References

- Django source: [`postgresql/operations.py` quote_name](https://github.com/django/django/blob/main/django/db/backends/postgresql/operations.py)
- Django source: [`sqlite3/operations.py` quote_name](https://github.com/django/django/blob/main/django/db/backends/sqlite3/operations.py)
- Django security policy: [SQL injection protection](https://docs.djangoproject.com/en/dev/topics/security/#sql-injection-protection)
- [CWE-89: SQL Injection](https://cwe.mitre.org/data/definitions/89.html)
- PostgreSQL docs: [Identifier quoting rules](https://www.postgresql.org/docs/current/sql-syntax-lexical.html#SQL-SYNTAX-IDENTIFIERS)

## Related content

- [CRLF Injection in CPython http.server and wsgiref](/2026/04/24/crlf-injection-cpython-http-server-wsgiref/)
- [Finding security fixes without a CVE: a changelog analyzer](/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer/)
- [SSRF via DNS rebinding](/2026/03/14/ssrf-dns-rebinding-vulnerability/)
