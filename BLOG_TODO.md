# Blog Post TODO List

Gelecekte yazılacak blog postlar için todo listesi.

## Yüksek Öncelik

### 1. CVE-2025-29927: Next.js Middleware Bypass Vulnerability
- **Repository:** https://github.com/aydinnyunus/CVE-2025-29927
- **Durum:** ⏳ Planlanıyor
- **Konu:** Next.js middleware bypass zafiyeti, `x-middleware-subrequest` header'ı ile authentication bypass
- **Etkilenen Versiyonlar:**
  - Next.js 15.x < 15.2.3
  - Next.js 14.x < 14.2.25
  - Next.js 13.x < 13.5.9
- **Notlar:**
  - Self-hosted Next.js uygulamaları etkileniyor
  - Vercel ve Netlify'da çalışan uygulamalar etkilenmiyor
  - Proof of concept repository mevcut
  - 96 stars, 28 forks

### 2. GitleaksVerifier: Secret Verification Tool
- **Repository:** https://github.com/aydinnyunus/GitleaksVerifier
- **Durum:** ⏳ Planlanıyor
- **Konu:** Gitleaks tarafından bulunan secret'ları doğrulama aracı
- **Özellikler:**
  - Python tabanlı verification tool
  - 40+ farklı secret tipini destekliyor
  - JSON output
  - Rule-based filtering
- **Notlar:**
  - Gitleaks ile entegre çalışıyor
  - streaak/keyhacks verification metodlarını kullanıyor
  - 29 stars

### 3. CVE-2024-24576 Exploit
- **Repository:** https://github.com/aydinnyunus/CVE-2024-24576-Exploit
- **Durum:** ⏳ Planlanıyor
- **Konu:** CVE-2024-24576 zafiyeti ve exploit detayları
- **Notlar:** Repository detayları kontrol edilmeli

### 4. Akaunting Authenticated RCE
- **Repository:** https://github.com/aydinnyunus/akaunting-authenticated-rce
- **Durum:** ⏳ Planlanıyor
- **Konu:** Akaunting uygulamasında authenticated remote code execution zafiyeti
- **Notlar:** Repository detayları kontrol edilmeli

### 5. VNC Brute Force Tool
- **Repository:** https://github.com/aydinnyunus/VNCBruteForce
- **Durum:** ⏳ Planlanıyor
- **Konu:** VNC brute force aracı ve güvenlik testleri
- **Notlar:** Repository detayları kontrol edilmeli

### 6. CVE-2025-XXXX: Python http.server CRLF Injection Vulnerability
- **Issue:** https://github.com/python/cpython/issues/142533
- **Durum:** ⏳ Planlanıyor
- **Konu:** Python `http.server` modülünde CRLF injection zafiyeti
- **Zafiyet Detayları:**
  - `send_header` metodu user-controlled input'u validate etmeden header'a yazıyor
  - CRLF (`\r\n`) injection ile yeni header'lar inject edilebiliyor
  - Set-Cookie injection (session fixation)
  - Location header injection (malicious redirect)
  - Cache poisoning
- **Saldırı Senaryoları:**
  - Session fixation attacks
  - Malicious redirects
  - Cache poisoning
  - Web cache deception
- **İlgili PR'lar:**
  - [PR #142605](https://github.com/python/cpython/pull/142605): Prevent CRLF injection in HTTP headers
  - [PR #143395](https://github.com/python/cpython/pull/143395): Document CRLF injection vulnerability
- **Notlar:**
  - CPython main branch'de test edildi
  - Documentation issue olarak açıldı
  - Fix PR'ları mevcut

### 7. Pywikibot Remote Code Execution via eval() in Password File
- **Phabricator Tasks:**
  - [T410753](https://phabricator.wikimedia.org/T410753): Security issue
  - [T410755](https://phabricator.wikimedia.org/T410755): Related task
- **Commit:** [a0fea2b](https://github.com/wikimedia/pywikibot/commit/a0fea2b175943c338fb901737a069a837e1402cc)
- **Durum:** ⏳ Planlanıyor
- **Konu:** Pywikibot'da password file parsing'de eval() kullanımı RCE zafiyeti
- **Zafiyet Detayları:**
  - `readPassword()` metodu password file'larını parse ederken `eval()` kullanıyordu
  - User-controlled password file içeriği Python kodu olarak execute edilebiliyordu
  - Remote code execution mümkündü
- **Fix Detayları:**
  - `eval()` yerine `ast.literal_eval()` kullanıldı
  - Line length ve comma count sanity check'leri eklendi
  - Malformed password line'lar için ValueError raise ediliyor
  - Default `PASS_BASENAME` 'user-password.py'den 'user-password.cfg'ye değiştirildi
- **Saldırı Senaryoları:**
  - Password file'ına malicious Python kodu yazılabilir
  - Arbitrary code execution
  - System compromise
- **Notlar:**
  - Version 10.7.1'de fix edildi
  - Unit test ile doğrulandı (test_eval_security)
  - Backport yapıldı

### 8. CVE-2026-21883: Bokeh WebSocket Origin Validation Bypass (CSWSH)
- **Security Advisory:** [GHSA-793v-589g-574v](https://github.com/bokeh/bokeh/security/advisories/GHSA-793v-589g-574v)
- **CVE:** CVE-2026-21883
- **Durum:** ⏳ Planlanıyor
- **Konu:** Bokeh server'da WebSocket origin validation bypass zafiyeti
- **Zafiyet Detayları:**
  - `match_host` fonksiyonu `src/bokeh/server/util.py`'de hostname karşılaştırmasında hata var
  - Python'un `zip()` fonksiyonu kısa iterable bittiğinde duruyor
  - Kod sadece pattern'in host'tan uzun olup olmadığını kontrol ediyor, host'un pattern'den uzun olup olmadığını kontrol etmiyor
  - Host pattern ile başlıyorsa (ama ek segmentler varsa) yanlışlıkla match ediyor
- **Saldırı Senaryosu:**
  - Allowlist `['dashboard.corp']` ise
  - Saldırgan `dashboard.corp.attacker.com` domain'i kaydedebilir
  - Kurban malicious site'e yönlendirilir
  - Malicious site vulnerable Bokeh server'a WebSocket bağlantısı başlatır
  - Origin header (`http://dashboard.corp.attacker.com/`) allowlist'e göre match eder
  - Saldırgan kurban adına Bokeh server ile etkileşime girebilir
- **Etki:**
  - Cross-Site WebSocket Hijacking (CSWSH)
  - Sensitive data'ya erişim
  - Visualization'ları değiştirme
- **Etkilenen Versiyonlar:**
  - Bokeh < 3.8.2
- **Fix:**
  - Version 3.8.2 ve sonrasında patch edildi
- **CWE:** CWE-1385 (Missing Origin Validation in WebSockets)
- **Severity:** Low
- **Notlar:**
  - Sadece deployed Bokeh server instance'ları etkileniyor
  - Static HTML output, standalone embedded plots, Jupyter notebook kullanımı etkilenmiyor
  - Authentication hook'ları olan server'larda authentication bypass yapmıyor

## Zaten Yazılmış (Referans)

### ✅ PackageSpy: Package Repository Secret Scanning
- **Blog Post:** [2026-02-14-package-repository-secret-scanning.md](_posts/2026-02-14-package-repository-secret-scanning.md)
- **Repository:** https://github.com/aydinnyunus/PackageSpy
- **Durum:** ✅ Tamamlandı

### ✅ IP Address Classification Inconsistencies
- **Blog Post:** [2026-02-21-ip-address-classification-inconsistencies.md](_posts/2026-02-21-ip-address-classification-inconsistencies.md)
- **Repository:** https://github.com/aydinnyunus/ipValidator
- **Durum:** ✅ Tamamlandı

## Blog Post Formatı

Her blog post için gerekli bilgiler:

```markdown
---
layout: post
title: "Post Başlığı"
date: YYYY-MM-DD
author: Yunus Aydın
lang: en/tr
description: "150-160 karakter meta description"
keywords: "keyword1, keyword2, keyword3"
canonical_url: "https://aydinnyunus.github.io/YYYY/MM/DD/post-url/"
---
```

## Yazım Kuralları

- AI pattern'lerinden kaçın (blog-writing.mdc kurallarına uy)
- Teknik doğruluk
- Kod örnekleri ve proof of concept'ler
- Gerçek dünya örnekleri
- Internal linkler (2-3 adet)
- SEO optimizasyonu

## Notlar

- Tarihleri yayın planına göre ayarla
- Her post için EN ve TR versiyonları oluştur
- Repository linklerini ekle
- CVE numaralarını doğru kullan
- Disclosure timeline ekle (varsa)
