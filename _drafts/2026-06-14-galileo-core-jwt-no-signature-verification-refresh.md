---
layout: post
title: "Galileo-Core: JWT Refresh Without Signature Verification"
date: 2026-06-14
author: Yunus Aydın
lang: en
description: "I reported a JWT misconfiguration in galileo-core's refresh token flow on April 9, 2024. The client decoded JWTs with verify_signature=False and trusted whatever exp claim came back."
keywords: "galileo-core, Galileo Labs, JWT, refresh token, signature verification, PyJWT, security research, AI platform security, responsible disclosure"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/galileo-core-jwt-no-signature-verification-refresh/"
---

I reported a JWT signature verification issue in [galileo-core](https://pypi.org/project/galileo-core/) on April 9, 2024 directly to the Galileo Labs team. The SDK's refresh token flow decoded JWTs with `options={"verify_signature": False}` and used the `exp` claim from that unverified blob to decide whether to refresh. Trusting an unverified token to drive a security decision is exactly the kind of pattern static analyzers flag, and it deserved to be flagged here too.

I was reading through SDK clients that Galileo's AI evaluation customers were pulling into their backends. Anything that ships with `pyjwt` in its dependency tree gets a quick grep for `verify_signature=False`, because that's where the foot-guns live in Python JWT code. The first hit landed in `galileo_core/schemas/base_config.py`, inside the function that decides whether to refresh the access token.

## The vulnerable code

Here's the function as it existed when I reported it. The same shape is still visible in current builds:

```python
def refresh_jwt_token(self) -> None:
    """Refresh token if not present or expired."""
    if self.jwt_token:
        claims = jwt_decode(
            self.jwt_token.get_secret_value(),
            options={"verify_signature": False},
        )
        if claims.get("exp", 0) <= (time() + 300):
            logger.debug("JWT token is invalid, refreshing.")
            self.jwt_token, self.refresh_token = self.get_jwt_token(...)
        else:
            logger.debug("JWT token is still valid, not refreshing.")
```

The decode call has two problems baked into one line:

1. `verify_signature=False` disables the only cryptographic check PyJWT performs.
2. The result of that decode is used to make a control-flow decision (refresh or not), which means the client treats the claim payload as authoritative.

The refresh flow itself sends the `refresh_token` cookie to the server, so the server still gets to weigh in. But on the client side, anything that touches `self.jwt_token` after this check is operating on the assumption that the claim came from a verified token. It didn't.

## Root cause

There is no verification at all. None.

1. PyJWT's default is to verify; this code explicitly turned that off.
2. There is no allowlist of expected `iss`, `aud`, or signing algorithm.
3. The function never resolves a JWKS or any public key to verify against, so the client has no way to know if the access token it cached came from Galileo's auth server or from a file an attacker dropped on disk.
4. The `exp` claim it trusts is the one a tampered token would carry, so the local "is this token still valid?" check is meaningless against any adversary who can write to the config file or env.

## Impact

- An attacker who modifies a stored JWT (config file, env var, leaked cookie) can extend the `exp` claim with jwt.io and the client will treat the token as valid forever, never triggering a refresh. The server eventually rejects it, but until that round-trip happens the SDK proceeds as if it had a valid session.
- Local trust decisions ("don't refresh yet, exp is still ten minutes out") are made on attacker-controlled data.
- Anyone auditing the codebase reads `verify_signature=False` as a red flag, regardless of whether the server compensates. In a regulated environment that turns into a finding in every external review until it's gone.

## Proof of concept

```python
# Step 1: take an existing access token (cached config, leaked env)
stolen = "eyJhbGciOi...payload...sig"

# Step 2: tamper with exp using jwt.io or any base64 encoder, no key required
import base64, json, time
header, payload, _sig = stolen.split(".")
new_payload = base64.urlsafe_b64encode(
    json.dumps({"exp": int(time.time()) + 60 * 60 * 24 * 365}).encode()
).rstrip(b"=").decode()
tampered = f"{header}.{new_payload}.{_sig}"

# Step 3: drop tampered token into the SDK's config
from galileo_core.schemas.base_config import GalileoConfig
GalileoConfig.set_current(jwt_token=tampered, ...)

# Step 4: SDK decodes with verify_signature=False, sees exp > now + 300,
# and never calls the refresh endpoint. The local state is "happy."
```

The server still rejects requests carrying the tampered token, so you don't get free API access. What you get is a client that lies to itself about its own auth state until the first request fails.

## The fix

The right shape is to do server-side verification and to remove the local decode entirely, or to call the refresh endpoint on every request and let the server be the source of truth. If a local check is unavoidable, decode with the public key:

```python
import jwt

claims = jwt.decode(
    self.jwt_token.get_secret_value(),
    key=galileo_public_key,
    algorithms=["RS256"],
    audience="galileo-api",
    issuer="https://auth.galileo.ai/",
    options={"verify_signature": True, "require": ["exp", "iss", "aud"]},
)
```

For a pure client SDK with no access to the signing key, the cleaner answer is: don't read `exp` locally at all. Refresh on every 401, refresh proactively on a timer, or call `/me` once a minute. Anything except trusting the payload of an unverified token to decide what your client does next.

## Why this pattern keeps showing up

Every PyJWT tutorial that says "just check the expiry" produces this code. The author wants to avoid the JWKS dance, the test environment doesn't have a real signing key handy, and `verify_signature=False` makes the snippet compile and the tests pass. The code review either misses the flag or accepts the comment that says "client-side, we're only reading claims, the server verifies." Both of those statements can be technically true and still produce a misleading audit trail and a runtime that trusts attacker-controlled bytes for routing decisions.

The boundary that matters is not "client vs server." It is "is this claim load-bearing for any decision I'm about to make?" If the answer is yes, you have to verify. If the answer is no, you don't need to decode it in the first place. The galileo-core code chose the third path, which is to decode without verifying and then make a decision anyway. That is the unsafe combination.

## Disclosure timeline

- **April 9, 2024**: I reported the finding directly to the Galileo team via email. No response received.
- **June 14, 2026**: This writeup.

## References

- [galileo-core on PyPI](https://pypi.org/project/galileo-core/)
- [PyJWT documentation on signature verification](https://pyjwt.readthedocs.io/en/stable/usage.html#requiring-presence-of-claims)
- [CWE-347: Improper Verification of Cryptographic Signature](https://cwe.mitre.org/data/definitions/347.html)
- [OWASP JSON Web Token Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/JSON_Web_Token_for_Java_Cheat_Sheet.html)

## Related content

- [Langflow SQL Injection in the Monitor Service](/2026/06/14/langflow-sql-injection-monitor-service/)
- [Command Injection in Mage-AI Git Utils](/2026/06/14/command-injection-mage-ai-git-utils/)
- [Finding Security Fixes Without CVE: Changelog Analyzer](/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer/)
