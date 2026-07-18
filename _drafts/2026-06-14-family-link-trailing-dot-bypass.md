---
layout: post
title: "Bypassing Family Link's website blocklist with a trailing dot"
author: Yunus Aydın
date: 2026-06-14
lang: en
description: "Chrome's Family Link URL filter compares the URL host to the parent's blocklist with strict string equality. Appending a single trailing dot bypasses every blocked site."
keywords: "Chrome, Family Link, parental controls, URL filter, host comparison, trailing dot, FQDN bypass, supervised user, Chrome VRP, security research"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/family-link-trailing-dot-bypass/"
---

I reported this to the Chrome VRP on May 24, 2026. A Family Link parent blocks `example.com`. The child types `example.com.` (note the trailing dot) and the page loads. No interstitial, no "Ask in person" prompt, nothing. The same site, reached by appending one character.

The root cause is a single `==` comparison in `FamilyLinkUrlFilter::HostMatchesPattern` that does not normalize the trailing dot. `url::DomainIs()` in the same codebase already does this correctly. The URL filter just doesn't use it.

## The setup

This is reproducible on a real device, no Chromium build required.

1. Parent device: install Family Link, open the child account → **Controls** → **Content restrictions** → **Google Chrome** → set to *Try to block explicit sites* or *Only allow approved sites*.
2. **Manage sites** → **Blocked sites** → add `example.com`.
3. Child device: sign in to Chrome with the supervised account. Wait for the blocklist to sync (about 10 minutes, or trigger sync manually).

Then:

**Baseline.** Navigate to `https://example.com/`. The block works. Chrome shows the Turkish parental approval interstitial ("Anne veya babanıza sorun", which translates to "Ask your mom or dad").

![Family Link parental block interstitial on the bare host example.com, works as designed]({{ site.baseurl }}/assets/images/family-link-trailing-dot/baseline-blocked.jpeg)

**Bypass.** Navigate to `https://example.com./` (trailing dot after `.com`). The page loads. No interstitial. No block.

![Same supervised account, trailing-dot variant example.com. Page loads, no parental block]({{ site.baseurl }}/assets/images/family-link-trailing-dot/bypass-loaded.jpeg)

DNS resolves the trailing-dot variant identically to the bare host. Modern CAs sign certificates that match the FQDN form. The HTTPS handshake succeeds. The page content is identical. The only difference is the dot, and that dot is enough to skip the block.

I verified the bare host is still blocked after the bypass. So the filter is otherwise active. The issue is in host matching, not in a globally broken filter.

## The vulnerable function

`components/supervised_user/core/browser/family_link_url_filter.cc:365-418`:

```cpp
bool FamilyLinkUrlFilter::HostMatchesPattern(const std::string& canonical_host,
                                             const std::string& pattern) {
  std::string trimmed_host = canonical_host;
  std::string trimmed_pattern = TrimHttpOrHttpsProtocol(pattern);

  // ... www. prefix handling ...
  // ... *.example.com wildcard handling ...
  // ... *.google.* registry wildcard handling ...

  return trimmed_host == trimmed_pattern;  // line 417
}
```

The function handles `www.` trimming, `*.example.com` subdomain wildcards, `*.google.*` registry wildcards, and malformed pattern rejection. It does **not** handle the trailing dot on the canonical host.

Line 417 is strict string equality. `"example.com." == "example.com"` is `false`. The function returns `false`, no entry matches, and the URL is treated as not-blocked.

The caller, at line 493-503, feeds it `url.GetHost()` directly:

```cpp
const std::string host = url.GetHost();   // preserves trailing dot
if (result != FilteringBehavior::kBlock) {
  auto it = std::ranges::find_if(
      blocked_host_list_, [&host](const std::string& host_entry) {
        return HostMatchesPattern(host, host_entry);
      });
  if (it != blocked_host_list_.end()) {
    result = FilteringBehavior::kBlock;
  }
}
```

`GURL::GetHost()` preserves the trailing dot. It is documented to do so. The unit test at `url/gurl_unittest.cc:1066-1083` exists specifically to enforce that behavior. The URL filter feeds the raw host into a strict-equality check. That is the entire bug.

## What Chromium already knew

The trailing-dot bypass class is not new in Chromium. `url::DomainIs()` (`url/url_util.cc:733-754`) explicitly normalizes:

```cpp
// If the host name ends with a dot but the input domain doesn't, then we
// ignore the dot in the host name.
size_t host_len = canonical_host.length();
if (canonical_host.back() == '.' && canonical_domain.back() != '.')
  --host_len;
```

The process isolation layer has an explicit test for it. From `content/browser/site_instance_impl_unittest.cc:1696-1698`:

```cpp
// Appending a trailing dot to a URL should not bypass process isolation.
```

DevTools had the same bug. [Derin Eryilmaz](https://x.com/deryilz/status/1753394956956295488) found that `chrome.devtools.inspectedWindow.eval` checked `parsedURL.hostname === "chrome.google.com"` and was bypassed by `chrome.google.com.`, which allowed arbitrary extension code execution against the Chrome Web Store. Tracked as [crbug.com/1472898](https://crbug.com/1472898), rewarded $5,000, fixed in DevTools, never audited across the rest of the codebase.

The Family Link URL filter is another instance of the same class. The infrastructure to fix this correctly is sitting one function call away, and the filter just doesn't use it.

## The blocklist isn't normalized either

`components/supervised_user/core/browser/family_link_settings_service.cc:667-672`:

```cpp
for (const auto&& [host, value] : *manual_behavior_hosts) {
  if (value.GetIfBool().value_or(false)) {
    host_exceptions.allowed_hosts.insert(host);
  } else {
    host_exceptions.blocked_hosts.insert(host);   // raw key, no canonicalization
  }
}
```

Whatever the parent typed in the Family Link UI is inserted verbatim. There is no canonicalization step. So even fixing the comparison alone is incomplete. The inverse case (parent stores `example.com.`, child types `example.com`) still fails. The fix needs both sides.

## The unit-test gap

`family_link_url_filter_unittest.cc:98-170` covers:

- `www.google.com` vs `google.com`
- `*.google.com` subdomain wildcard
- `*.google.*` registry wildcard
- Empty / `.` / `*` malformed patterns
- `notgoogle.com` (substring negative match)
- `www.googleplex.com` (longest-prefix negative match)
- `mail.google.com` (subdomain negative match)

Trailing-dot variants are missing from the matrix. `google.com.` and `www.google.com.` are not tested. The bug class is invisible to CI.

## The fix

Normalize both sides at the top of `HostMatchesPattern`:

```cpp
bool FamilyLinkUrlFilter::HostMatchesPattern(const std::string& canonical_host,
                                             const std::string& pattern) {
  std::string trimmed_host = canonical_host;
  if (!trimmed_host.empty() && trimmed_host.back() == '.') {
    trimmed_host.pop_back();
  }

  std::string trimmed_pattern = TrimHttpOrHttpsProtocol(pattern);
  if (!trimmed_pattern.empty() && trimmed_pattern.back() == '.') {
    trimmed_pattern.pop_back();
  }

  // ... rest of function unchanged
}
```

Alternative: replace the final `trimmed_host == trimmed_pattern` with `url::DomainIs(trimmed_host, trimmed_pattern)` when no wildcard is in play. That delegates the canonicalization to existing, tested code.

Defense in depth: strip trailing dots in `family_link_settings_service.cc:667-672` before insertion, so the stored blocklist is always in canonical form.

The unit tests to add are obvious:

```cpp
TEST_P(FamilyLinkUrlFilterTest, HostMatchesPatternTrailingDot) {
  EXPECT_TRUE(FamilyLinkUrlFilter::HostMatchesPattern("example.com.", "example.com"));
  EXPECT_TRUE(FamilyLinkUrlFilter::HostMatchesPattern("example.com", "example.com."));
  EXPECT_TRUE(FamilyLinkUrlFilter::HostMatchesPattern("www.example.com.", "*.example.com"));
  EXPECT_FALSE(FamilyLinkUrlFilter::HostMatchesPattern("example.com..", "example.com"));
}

TEST_F(FamilyLinkUrlFilterTest, TrailingDotDoesNotBypassBlockList) {
  HostExceptions exceptions;
  exceptions.blocked_hosts = {"example.com"};
  filter_->UpdateManualHosts(std::move(exceptions));

  EXPECT_EQ(filter_->GetFilteringBehaviorForURL(GURL("https://example.com/")),
            FilteringBehavior::kBlock);
  EXPECT_EQ(filter_->GetFilteringBehaviorForURL(GURL("https://example.com./")),
            FilteringBehavior::kBlock);
}
```

## Impact

CVSS 3.1 base is **3.1 (Low)**: `AV:N/AC:L/PR:L/UI:N/S:U/C:N/I:L/A:N`. No data is leaked, no system is destabilized. The child only reaches sites that already exist publicly.

The product impact is the real story. Family Link is on tens of millions of supervised accounts. The threat model isn't a remote attacker. It's the legitimate device user (the child) bypassing a control set by another party (the parent). Chrome VRP has historically accepted this class as in-scope for supervised user / enterprise URL filtering.

Concrete consequences:

- **Adult-content blocklists, gambling, social media restrictions.** All bypassed by a single keystroke. Works for any domain on the parent's manual blocklist.
- **School-managed Chromebooks.** Many K-12 districts run content filters on managed devices. If the underlying enforcement uses the same code path (this needs separate verification for the enterprise path), entire student fleets are exposed.
- **Spread vector.** One TikTok video, one classroom whisper, and the technique is everywhere. Family Link's value proposition collapses for that cohort. Parents have no signal the bypass happened; activity reports may or may not flag the trailing-dot host (depends on upstream URL handling).
- **Reproducibility.** Trivial. No tools, no extensions, no developer mode. Just type a dot.

The CVSS score reflects the per-incident technical impact. The deployment scale and the broken-contract framing are what make this worth filing.

## Affected platforms

| Platform | Family Link web filter | Affected |
|----------|------------------------|----------|
| Chrome Android (supervised child account) | Yes | Yes (verified) |
| ChromeOS (managed child Chromebook) | Yes | Yes (same code path) |
| Chrome iOS | Limited (Apple Family Sharing only) | Likely not (different code path) |
| Chrome desktop (Windows/macOS/Linux) | Removed in 2021 | Not affected |

Chromium-based browsers like Edge and Brave don't ship Family Link, so they are irrelevant here.

## Why this pattern keeps appearing

Trailing-dot bypass is a textbook host-identity mismatch. The browser canonicalizes the URL for network, certificate, and process-isolation purposes upstream, but downstream security checks that compare strings directly will diverge.

The Chromium codebase already paid the cost of fixing this for `url::DomainIs()`, for SiteInstance, and for DevTools. Each fix was scoped to the bug at hand. None of them resulted in a sweep across every place that calls `url.host()` and then does string equality on a security-relevant comparison. The class survives in any newly written code that doesn't reach for `DomainIs()`.

This is CWE-178 (Improper Handling of Case Sensitivity) / CWE-20 (Improper Input Validation) at the host-canonicalization layer. The same class shows up across Host header validation, cookie domain scoping, CORS, SAML audience checks, and OAuth redirect URI validation. Anywhere a host is compared as a string, the trailing dot, uppercase form, IDN form, and punycode form all need to canonicalize to the same shape before the comparison. The right fix is a lint rule, not another scoped patch.

## Disclosure timeline

- **May 24, 2026.** Vulnerability discovered and verified on Chrome Stable with a managed Family Link child account.
- **May 24, 2026.** Reported to the Chrome VRP, classified as *Permissions Bypass*.
- **June 14, 2026.** Public write-up.

## References

- [Chromium source: `family_link_url_filter.cc`](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/components/supervised_user/core/browser/family_link_url_filter.cc)
- [Chromium source: `family_link_settings_service.cc`](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/components/supervised_user/core/browser/family_link_settings_service.cc)
- [Chromium source: `url::DomainIs` in `url_util.cc`](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/url/url_util.cc)
- [crbug.com/1472898: DevTools trailing-dot bypass in `chrome.devtools.inspectedWindow.eval` (same class, found by Derin Eryilmaz, $5,000 reward)](https://crbug.com/1472898)
- [Derin Eryilmaz on X: original write-up of the DevTools trailing-dot bypass](https://x.com/deryilz/status/1753394956956295488)
- [CWE-178: Improper Handling of Case Sensitivity](https://cwe.mitre.org/data/definitions/178.html)
- [CWE-20: Improper Input Validation](https://cwe.mitre.org/data/definitions/20.html)
- [Chrome Vulnerability Reward Program](https://bughunters.google.com/about/rules/chrome-friends)

## Related content

- [SSRF via DNS rebinding]({{ site.baseurl }}{% post_url 2026-03-14-ssrf-dns-rebinding-vulnerability %}): another host-identity mismatch, but at resolution time.
- [IP address classification inconsistencies]({{ site.baseurl }}{% post_url 2026-03-21-ip-address-classification-inconsistencies %}): same shape, different canonicalization surface.
- [Content-Type spoofing in lollms]({{ site.baseurl }}{% post_url 2026-05-03-content-type-spoofing-lollms-chat-image-cve-2026-5728 %}): string-equality checks on attacker-influenced input.
