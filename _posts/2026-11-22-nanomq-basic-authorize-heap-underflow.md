---
layout: post
title: "Heap Buffer Underflow in NanoMQ rest_api.c basic_authorize()"
date: 2026-11-22
author: Yunus Aydın
lang: en
description: "An unauthenticated heap buffer underflow in NanoMQ rest_api.c basic_authorize(): a 6-byte Authorization header crashes the HTTP API before any credential check."
keywords: "NanoMQ, heap buffer underflow, out-of-bounds write, CWE-787, CWE-191, basic_authorize, rest_api.c, libFuzzer, ASAN, UBSAN, DoS, MQTT broker, security research"
canonical_url: "https://aydinnyunus.github.io/2026/11/22/nanomq-basic-authorize-heap-underflow/"
---

```bash
$ printf 'Basicc' | ./auth_harness -
# rest_api.c:828:5: runtime error: addition of unsigned offset to 0x... overflowed to 0x...
# SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior rest_api.c:828:5
```

Six bytes, no credentials, and the NanoMQ HTTP API takes an out-of-bounds write before it ever looks at your username. I found this while fuzzing NanoMQ's REST surface with libFuzzer and I reported it to the maintainers on **May 2, 2026**, against the latest tag at the time (**0.24.13**). It is a pre-authentication crash: the write lands before a single credential is validated, so there is nothing to guess and nothing to bypass. You only need a socket and the word "Basic".

## Background

NanoMQ is an edge MQTT broker that ships an HTTP management API on top of its broker, usually on port **8081**, with endpoints under `/api/v4/`. Several of those endpoints sit behind HTTP Basic auth, which is parsed by a small helper in `nanomq/rest_api.c`: `basic_authorize(http_msg *msg)`. The helper does three things in sequence: strip the `Basic ` prefix off the `Authorization` header, allocate a buffer for the Base64 payload, and zero its trailing bytes to make room for `=` padding during decode.

That third step is where it goes wrong, and it goes wrong for a reason that has nothing to do with credentials: the size of the buffer is computed from the length of the token, and nothing checks that the token is long enough for the zero-fill to fit.

## Vulnerable code

Here is the function as it looked before the fix, in the block around lines **824-829**, taken from tag **0.24.8**:

```c
static enum result_code
basic_authorize(http_msg *msg)
{
    // msg->token holds the raw Authorization header value
    // strip the "Basic " prefix in place
    sscanf(msg->token, "Basic %s", msg->token);

    size_t token_len = strlen(msg->token);

    size_t   decode_len = token_len * 6 / 8 + 1;
    uint8_t *decode     = nng_alloc(decode_len);

    // intended to clear space for Base64 '=' padding
    decode[decode_len - 1] = '\0';   // fine when decode_len >= 3
    decode[decode_len - 2] = '\0';   // underflow when decode_len < 2
    decode[decode_len - 3] = '\0';   // underflow when decode_len < 3

    // ... base64 decode into 'decode', compare against configured credentials
}
```

There is no length check on `token_len` before the three writes. There is no check that `decode_len` is at least 3. The writes are unconditional. That is the whole bug.

## Root cause

`decode_len` is derived from an attacker-controlled length, and the zero-fill indices are answered with unsigned arithmetic:

1. `sscanf(msg->token, "Basic %s", msg->token)` leaves whatever follows the `Basic ` prefix in the same buffer, with `token_len` now equal to the length of that remainder. A one-character remainder gives `token_len = 1`, so `decode_len = 1 * 6 / 8 + 1 = 1`.
2. `decode_len` is a `size_t`, and `decode_len - 2` / `decode_len - 3` are therefore computed in unsigned space. For `decode_len = 1`, those expressions do not go negative; they wrap to `SIZE_MAX` and `SIZE_MAX - 1`.
3. `decode[SIZE_MAX]` and `decode[SIZE_MAX - 1]` are not "a little before the buffer". On a 64-bit machine the address wraps around the top of the address space and lands in low memory the process does not own, so the null byte either faults immediately or silently corrupts memory the allocator cares about, depending on the layout the sanitizer lets through.

The pattern is CWE-787 (out-of-bounds write) stacked on CWE-191 (integer underflow). The second one is the interesting half: the index *is* negative in the author's head, but the type erases the sign before the pointer arithmetic.

Here is the arithmetic laid out:

```text
token_len | decode_len | decode[decode_len - 2]   | decode[decode_len - 3]
----------|------------|--------------------------|-------------------------
    1     |     1      | wraps to SIZE_MAX        | wraps to SIZE_MAX - 1
    2     |     2      | index 0 (in bounds)      | wraps to SIZE_MAX
    3     |     3      | index 1 (in bounds)      | index 0 (in bounds)
    4+    |     4+     | in bounds                | in bounds
```

Only the first two rows are reachable from a request, and only the first row is the clean underflow.

## Proof of concept

The minimal input is six bytes: the word `Basic` with a single character glued on, no space. Hand that string to the authorization path and it does the rest.

```bash
# One byte after the "Basic " prefix
$ printf 'Basicc' > /tmp/poc-basicc
$ ./auth_harness /tmp/poc-basicc
rest_api.c:828:5: runtime error: addition of unsigned offset to 0x60d000000380 overflowed to 0x60d0000001ff
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior rest_api.c:828:5
```

Over the network, the same thing is one `curl` against the management API:

```bash
# token after "Basic " is a single character here, so decode_len collapses to 1
curl -s -o /dev/null \
  -H 'Authorization: Basic Yg==' \
  http://<nanomq-host>:8081/api/v4/brokers
```

That request reaches `basic_authorize()` over the network, and `Yg==` is the Base64 of a single character, but the four-character remainder it leaves is not itself the trigger: this specific curl hits the crash only because `sscanf(msg->token, "Basic %s", msg->token)` reads and writes the same buffer, so what the parser sees after the prefix is not always what the client sent. The byte-exact trigger is the six-byte `Basicc` above; treat the curl as proof the path is network reachable, not as the minimal reproducer.

Credit for the trigger shape: the fuzzer. The harness was a tiny `auth_harness.c` that `#include`s `rest_api.c` and feeds a file into `basic_authorize()` directly, built with ASAN + UBSAN under clang-14. It reached the crash in **34 executions** from a cold start, which tells you how little of the search space this bug sits behind.

## Impact

- **Denial of service.** Any unauthenticated client that can reach the HTTP API crashes the process with one request. No credentials, no timing, no state.
- **Memory corruption.** The write is a single null byte, but it lands at a wild address near the base of the heap, where glibc keeps `malloc` chunk metadata.
- **No code execution.** The written value is fixed at `\0`, the allocation is short-lived, and modern glibc (2.29 and newer) runs heap integrity checks that turn metadata tampering into an abort rather than a controlled write. I am not claiming RCE here, and nobody should ship a fix that assumes there is one.

The advisory lists this as **moderate** severity. A DoS-only bug is still a real bill when the broker is the thing your devices are talking to.

## The fix

NanoMQ shipped the fix in **0.24.14**, in commit [`24cedc08`](https://github.com/nanomq/nanomq/commit/24cedc08a291e7787acd52108b9161c790aed03b) (`* FIX [rest_api] fix a memory corruption bug in basic authorization API`). It does three things, and only the first one is the arithmetic I described above:

```c
enum result_code result = SUCCEED;

// 1. drop sscanf entirely and refuse anything that is not "Basic " prefixed
if (msg->token_len <= 6 || strncmp(msg->token, "Basic ", 6) != 0) {
    return EMPTY_USERNAME_OR_PASSWORD;
}

char  *b64_token = msg->token + 6;
size_t token_len = strlen(b64_token);

// 2. a real credential is "user:pass", so at least 4 Base64 chars
if (token_len < 4) {
    return WRONG_USERNAME_OR_PASSWORD;
}

size_t   token_alloc_len = token_len + 1;
uint8_t *token           = nng_zalloc(token_alloc_len);
if (token == NULL) {
    return UNKNOWN_MISTAKE;
}
memcpy(token, b64_token, token_len);
// ...
size_t   decode_len = token_len * 6 / 8 + 1;

// 3. zero-fill the whole buffer at allocation; the manual padding writes are gone
uint8_t *decode     = nng_zalloc(decode_len);
if (decode == NULL) {
    nng_free(auth, auth_len);
    nng_free(token, token_alloc_len);
    return UNKNOWN_MISTAKE;
}
```

The third change is the one that removes the bug outright: `nng_zalloc` hands back an already-zeroed buffer, so the three unconditional padding writes that computed `decode[decode_len - 2]` and `decode[decode_len - 3]` simply do not exist anymore. The length floor in change two means `decode_len` can never be small enough to matter even if a future edit reintroduces a manual write, and change one retires the in-place `sscanf` whose overlapping read and write on `msg->token` was its own undefined behavior.

If you are patching a fork that cannot take the upstream commit, the length floor alone is the smallest safe change: a real credential is `user:pass`, which is at least four Base64 characters once the header is parsed, so anything shorter is garbage you can drop at the door.

```c
size_t token_len = strlen(msg->token);
if (token_len < 4) {
    return WRONG_USERNAME_OR_PASSWORD;
}
```

The upstream fix goes further, and the extra steps are worth copying: rejecting the input instead of decoding it, being explicit about the prefix instead of letting `sscanf` rewrite the buffer, and letting the allocator do the zeroing instead of doing it by hand at a computed offset.

## Why this bug is easy to miss

The code was written for a valid token. Whoever wrote it knew the input was a Base64
credential, computed the decode length from the header, allocated that many bytes, and
wrote three bytes at fixed offsets from the end. A fuzzer assumes no such token, and
neither does an attacker.

The second assumption is quieter: that subtracting from a length is safe because a
length is a number. In C it is an unsigned number, so `len - 2` on a length of 1 is not
minus one, it is `SIZE_MAX`. I have written that line myself, and so has most anyone who
has spent an afternoon on C string handling. That is also why a sanitizer beats a careful
read here. UBSAN found this in 34 executions, while the same three lines are easy to wave
through by eye: the intent (clear the padding) is obvious and the boundary (what if there
is no padding) is not. If you maintain a parser, put `-fsanitize=address,undefined` in CI
and point a fuzzer at the entry point. It will catch this class of bug long before a
reviewer does.

## Who is affected

Vulnerable in **0.24.13** and earlier, for any deployment where the HTTP management API is reachable. That is the default `nanomq.conf` if you enable the REST server on 8081. The fix landed in **0.24.14**, so if your version string sorts at or below 0.24.13 you are on the vulnerable code and a curl at the brokers endpoint is enough to know whether 8081 answers at all.

## Disclosure timeline

- **May 2, 2026**: I found the underflow with libFuzzer + ASAN/UBSAN (34 executions) and reported it to the NanoMQ maintainers with the reproducer and the sanitizer trace.
- **May 20, 2026**: Maintainer JaylinYu fixed `basic_authorize` in [`24cedc08`](https://github.com/nanomq/nanomq/commit/24cedc08a291e7787acd52108b9161c790aed03b), released in 0.24.14.
- **November 22, 2026**: I published this write-up.

## References

- [CWE-787: Out-of-bounds Write](https://cwe.mitre.org/data/definitions/787.html)
- [CWE-191: Integer Underflow (Wrap or Wraparound)](https://cwe.mitre.org/data/definitions/191.html)
- [NanoMQ repository](https://github.com/nanomq/nanomq)
- [GitHub Security Advisory GHSA-cgxg-fqq7-hqf9](https://github.com/nanomq/nanomq/security/advisories/GHSA-cgxg-fqq7-hqf9)
- [Fix commit 24cedc08](https://github.com/nanomq/nanomq/commit/24cedc08a291e7787acd52108b9161c790aed03b)
- [NanoMQ 0.24.14 release](https://github.com/nanomq/nanomq/releases/tag/0.24.14)
- [NanoMQ HTTP API documentation](https://nanomq.io/docs/en/latest/rest-api.html)

## Related content

- [Finding security fixes without a CVE: changelog and commit pipeline](/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer/)
- [CRLF injection in CPython's http.server and wsgiref](/2026/04/24/crlf-injection-cpython-http-server-wsgiref/)
