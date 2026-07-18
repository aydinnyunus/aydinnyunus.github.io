---
layout: post
title: "Firecrawl logged BULL_AUTH_KEY in plain text on startup"
author: Yunus Aydın
date: 2026-06-14
lang: en
description: "Firecrawl printed the Bull Board admin secret to application logs on every startup. Anyone with log access could open the queue dashboard. Reported March 2025, fixed in three days."
keywords: "Firecrawl, BULL_AUTH_KEY, Bull Board, log injection, CWE-532, sensitive information in logs, queue admin, secret leakage, security research"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/firecrawl-bull-auth-key-logged-plaintext/"
---

Firecrawl's API server printed the Bull Board admin secret to its logs on every startup. I reported it as issue #1321 on March 10, 2025. The maintainer fixed it three days later by deleting three lines. No CVE was assigned. The fix was small. The underlying bug class is the kind that quietly leaks production secrets into a hundred places at once.

## Background

[Firecrawl](https://github.com/firecrawl/firecrawl) is a web scraping and crawling API by Mendable. It uses [Bull](https://github.com/OptimalBits/bull) (a Redis-backed job queue) to schedule and run scrape jobs, and [Bull Board](https://github.com/felixmosh/bull-board) to expose a web UI for inspecting and managing those jobs.

Bull Board has no built-in authentication. The standard pattern is to mount it behind a hard-to-guess URL prefix and treat that prefix as a shared secret. Firecrawl uses `BULL_AUTH_KEY` for exactly that purpose. The dashboard lives at `/admin/<BULL_AUTH_KEY>/queues`.

If you leak the key, you leak the dashboard.

## The vulnerable code

`apps/api/src/index.ts` had this in the server startup block:

```typescript
logger.info(
  `For the Queue UI, open: http://${HOST}:${port}/admin/${process.env.BULL_AUTH_KEY}/queues`,
);
```

Local dev convenience: print a clickable URL when the server starts. The problem is that `logger.info()` does not care whether you're a developer running on localhost or a production container shipping logs to a SIEM. The URL goes wherever logs go.

On a typical production deployment that includes:

- Container stdout, which is captured by the orchestrator (Docker, Kubernetes, ECS)
- Cloud log aggregators (CloudWatch, Stackdriver, Datadog, Loki, Grafana Cloud)
- Whatever Filebeat / Fluentd / Vector pipeline forwards logs downstream
- Long-term log archives in S3 or equivalent
- Support tickets where ops teams paste startup output
- Screenshots in Slack threads when someone asks "is the server up?"

Each of those is a separate place where the secret now lives, with separate access controls, separate retention policies, and separate audit trails.

## Root cause

The secret is interpolated directly into a log message, the logger doesn't redact it, and no environment check gates the line. The combination means the log fires on every restart, in every environment, and writes the secret in clear text.

## Impact

For anyone with read access to logs the attack is one line: `grep -oE '/admin/[^/]+/queues'` finds the URL, then `curl` opens the Bull Board dashboard. From there the queue UI exposes scrape targets, request headers, response payloads, and any API keys passed through job metadata. Bull Board isn't read-only either; retry, delete, and re-queue actions are all surfaced.

"Log access" sounds restrictive but rarely is. Junior ops, contractors, SaaS log vendors, and anyone who has ever been added to a debugging session typically have it. A leaked secret in logs has a much larger blast radius than the same secret in a code file, because logs propagate by design and code does not.

## Proof of concept

There's no exploit script. The "PoC" is reading the logs:

```bash
# On the host running Firecrawl, or in any aggregator the logs ship to:
grep -oE '/admin/[^/]+/queues' /var/log/firecrawl/*.log | head -1
# /admin/SuperSecretKey123/queues

# Then visit:
curl https://firecrawl.example.com/admin/SuperSecretKey123/queues
```

That's the whole attack. The secret was printed for you on startup; you just had to know where to look.

## The fix

Commit [`f87e1171`](https://github.com/firecrawl/firecrawl/commit/f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2) by Gergő Móricz, message `fix: don't log bull secret`, landed on March 13, 2025. It deletes the three lines:

```diff
-    logger.info(
-      `For the Queue UI, open: http://${HOST}:${port}/admin/${process.env.BULL_AUTH_KEY}/queues`,
-    );
```

No replacement. The startup log no longer mentions the queue UI at all. Developers who want the URL can construct it from their own `.env` file.

That's the right call. There is no "log it but redacted" version of this that's better than just not logging it. If you need a startup helper for local dev, gate it behind `NODE_ENV === "development"` and even then prefer printing it to stdout outside the structured logger so it never reaches the aggregator.

## Why this pattern keeps showing up

The rule: anything in `process.env` that ends in `_KEY`, `_SECRET`, `_TOKEN`, `_PASSWORD`, or `_AUTH` is a secret, and secrets do not go in log messages. Not in URLs, not in error messages, not in "for your convenience" startup banners. The logger's destinations are not under your control.

A stricter version of the same rule: use a logger that scrubs known secret patterns at the framework level (e.g., Pino's `redact` option, Winston transports with redaction middleware). That way a careless `logger.info({ config })` doesn't leak everything from the environment in one shot.

## Disclosure timeline

- **March 10, 2025**: Reported to Firecrawl as issue [#1321](https://github.com/firecrawl/firecrawl/issues/1321)
- **March 13, 2025**: Fixed in commit [`f87e1171`](https://github.com/firecrawl/firecrawl/commit/f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2) by maintainer Gergő Móricz
- No CVE assigned

## References

- **CWE-532**: [Insertion of Sensitive Information into Log File](https://cwe.mitre.org/data/definitions/532.html)
- **CWE-200**: [Exposure of Sensitive Information to an Unauthorized Actor](https://cwe.mitre.org/data/definitions/200.html)
- **OWASP**: [Logging Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Logging_Cheat_Sheet.html)
- **Firecrawl issue #1321**: [https://github.com/firecrawl/firecrawl/issues/1321](https://github.com/firecrawl/firecrawl/issues/1321)
- **Fix commit**: [`f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2`](https://github.com/firecrawl/firecrawl/commit/f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2)
- **Bull Board**: [https://github.com/felixmosh/bull-board](https://github.com/felixmosh/bull-board)

## Related content

- [Command injection in NLTK collocations](https://aydinnyunus.github.io/2026/06/07/command-injection-nltk-collocations-eval/)
- [Package Repository Secret Scanning](https://aydinnyunus.github.io/2026/03/07/package-repository-secret-scanning/)
- [Finding security fixes without a CVE: A changelog analyzer](https://aydinnyunus.github.io/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer/)
