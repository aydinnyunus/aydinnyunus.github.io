# SSRF DNS Rebinding — LinkedIn & X (English)

**Blog:** https://aydinnyunus.github.io/2026/03/14/ssrf-dns-rebinding-vulnerability/  
**CVE:** CVE-2025-69660 | simstudioai/sim

---

## X — Single post

They were resolving the hostname and blocking private IPs. Still got hit — same domain, public at check time and private by the time the request went out. DNS rebinding. Pin the IP if you fetch user URLs.

---

## X — Thread (2 tweets)

**1/2**  
Found an SSRF in simstudioai/sim. They did check that the URL didn't resolve to a private IP. Problem: they checked once, then fetched later. In between you flip your domain to 192.168 whatever. Game over.

**2/2**  
They fixed it with DNS pinning (resolve once, use that IP for the actual request). Got assigned CVE-2025-69660. Wrote the bypass and the fix here: https://aydinnyunus.github.io/2026/03/14/ssrf-dns-rebinding-vulnerability/

---

## LinkedIn

I was looking at simstudioai/sim's proxy and file-parse APIs. They validate URLs — resolve the hostname, block if it's a private IP. Sounds fine.

Except there's a gap between when they check and when they actually fetch. So you point your domain at a random public IP, pass the check, then after TTL you point it at 169.254.169.254 or localhost. Request goes out, hits your target. No magic, just TOCTOU.

They fixed it by resolving once and pinning that IP for the request, plus blocking the usual bad ranges. Got assigned CVE-2025-69660. If you're fetching URLs users give you, worth thinking about whether someone could rebind in between.

https://aydinnyunus.github.io/2026/03/14/ssrf-dns-rebinding-vulnerability/
