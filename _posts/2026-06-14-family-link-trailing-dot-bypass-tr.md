---
layout: post
title: "Family Link site engelini sonuna nokta koyarak atlatmak"
author: Yunus Aydın
date: 2026-06-14
lang: tr
description: "Chrome'un Family Link URL filtresi, URL host'unu velinin engel listesiyle strict string eşitliği ile karşılaştırıyor. Sona tek bir nokta eklemek tüm engelli siteleri atlatıyor."
keywords: "Chrome, Family Link, ebeveyn kontrolü, URL filtresi, host karşılaştırma, trailing dot, FQDN bypass, supervised user, Chrome VRP, güvenlik araştırması"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/family-link-trailing-dot-bypass-tr/"
---

Bu zafiyeti 24 Mayıs 2026'da Chrome VRP'ye raporladım. Family Link kullanan bir veli `example.com`'u engelliyor. Çocuk `example.com.` yazıyor (sonundaki noktaya dikkat) ve sayfa açılıyor. Engel interstitial'ı yok, "Şahsen sor" ekranı yok, hiçbir şey yok. Aynı site, tek bir karakter eklenerek erişiliyor.

Root cause, `FamilyLinkUrlFilter::HostMatchesPattern` içindeki trailing dot'u normalize etmeyen tek bir `==` karşılaştırması. Aynı codebase'deki `url::DomainIs()` bu işi zaten doğru yapıyor. URL filtresi onu çağırmıyor sadece.

## Setup

Bu zafiyet gerçek bir cihazda reproduce edilebilir, Chromium build etmeye gerek yok.

1. Veli cihazı: Family Link kur, çocuk hesabını aç → **Controls** → **Content restrictions** → **Google Chrome** → *Try to block explicit sites* veya *Only allow approved sites* seç.
2. **Manage sites** → **Blocked sites** → `example.com` ekle.
3. Çocuk cihazı: Chrome'a supervised hesapla gir. Blocklist sync olsun (~10 dakika, veya sync'i manuel tetikle).

Sonra:

**Baseline.** `https://example.com/` adresine git. Engel çalışıyor. Chrome Türkçe ebeveyn onay interstitial'ını gösteriyor ("Anne veya babanıza sorun").

![example.com için Family Link engel ekranı, beklenildiği gibi çalışıyor]({{ site.baseurl }}/assets/images/family-link-trailing-dot/baseline-blocked.jpeg)

**Bypass.** `https://example.com./` adresine git (`.com`'un sonunda nokta). Sayfa açılıyor. Interstitial yok. Engel yok.

![Aynı supervised hesap, trailing dot'lu example.com. Sayfa açılıyor, engel yok]({{ site.baseurl }}/assets/images/family-link-trailing-dot/bypass-loaded.jpeg)

DNS trailing dot'lu varyantı normal host ile aynı şekilde resolve ediyor. Modern CA'lar FQDN formunu da kapsayan sertifikalar imzalıyor. HTTPS handshake geçiyor. Sayfa içeriği birebir aynı. Tek fark nokta, ve o nokta engeli atlatmaya yetiyor.

Bypass'tan sonra normal host'un hâlâ engelli olduğunu doğruladım. Yani filtre genel olarak bozuk değil. Sorun host matching mantığında.

## Savunmasız fonksiyon

`components/supervised_user/core/browser/family_link_url_filter.cc:365-418`:

```cpp
bool FamilyLinkUrlFilter::HostMatchesPattern(const std::string& canonical_host,
                                             const std::string& pattern) {
  std::string trimmed_host = canonical_host;
  std::string trimmed_pattern = TrimHttpOrHttpsProtocol(pattern);

  // ... www. prefix handling ...
  // ... *.example.com wildcard handling ...
  // ... *.google.* registry wildcard handling ...

  return trimmed_host == trimmed_pattern;  // satır 417
}
```

Fonksiyon `www.` trimming'i, `*.example.com` subdomain wildcard'ını, `*.google.*` registry wildcard'ını ve bozuk pattern'leri reddetmeyi handle ediyor. Canonical host'taki trailing dot'u **handle etmiyor**.

Satır 417 strict string eşitliği. `"example.com." == "example.com"` `false` dönüyor. Fonksiyon `false` dönüyor, hiçbir entry match etmiyor, URL engelli değil olarak işleniyor.

Çağıran kod satır 493-503'te, doğrudan `url.GetHost()` veriyor:

```cpp
const std::string host = url.GetHost();   // trailing dot'u koruyor
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

`GURL::GetHost()` trailing dot'u koruyor. Bu dokümante edilmiş bir davranış. `url/gurl_unittest.cc:1066-1083`'teki unit test tam olarak bu davranışı zorunlu kılmak için var. URL filtresi raw host'u alıp strict-equality check'e veriyor. Bütün bug bundan ibaret.

## Chromium bu sınıfı zaten biliyordu

Trailing dot bypass sınıfı Chromium'da yeni değil. `url::DomainIs()` (`url/url_util.cc:733-754`) açıkça normalize ediyor:

```cpp
// If the host name ends with a dot but the input domain doesn't, then we
// ignore the dot in the host name.
size_t host_len = canonical_host.length();
if (canonical_host.back() == '.' && canonical_domain.back() != '.')
  --host_len;
```

Process isolation katmanının da bunun için açık bir testi var. `content/browser/site_instance_impl_unittest.cc:1696-1698`:

```cpp
// Appending a trailing dot to a URL should not bypass process isolation.
```

DevTools'da da aynı bug vardı. [Derin Eryılmaz](https://x.com/deryilz/status/1753394956956295488) `chrome.devtools.inspectedWindow.eval`'in `parsedURL.hostname === "chrome.google.com"` check ettiğini ve `chrome.google.com.` ile bypass edilerek Chrome Web Store üzerinde extension kodu çalıştırılabildiğini buldu. [crbug.com/1472898](https://crbug.com/1472898) olarak takip edildi, $5,000 ödüllendirildi, DevTools'da fixlendi, ama codebase'in geri kalanı hiç audit edilmedi.

Family Link URL filtresi aynı sınıfın başka bir instance'ı. Doğru fix için altyapı bir fonksiyon çağrısı ötede duruyor, filtre sadece kullanmıyor.

## Blocklist tarafı da normalize edilmiyor

`components/supervised_user/core/browser/family_link_settings_service.cc:667-672`:

```cpp
for (const auto&& [host, value] : *manual_behavior_hosts) {
  if (value.GetIfBool().value_or(false)) {
    host_exceptions.allowed_hosts.insert(host);
  } else {
    host_exceptions.blocked_hosts.insert(host);   // raw key, canonicalization yok
  }
}
```

Velinin Family Link UI'sına yazdığı şey aynen insert ediliyor. Canonicalization adımı yok. Yani sadece karşılaştırmayı fixlemek eksik kalıyor. Ters durum (veli `example.com.` saklıyor, çocuk `example.com` yazıyor) hâlâ patlar. Fix iki tarafa da gerekli.

## Unit-test boşluğu

`family_link_url_filter_unittest.cc:98-170` şunları kapsıyor:

- `www.google.com` vs `google.com`
- `*.google.com` subdomain wildcard
- `*.google.*` registry wildcard
- Boş / `.` / `*` bozuk pattern'ler
- `notgoogle.com` (substring negative match)
- `www.googleplex.com` (longest-prefix negative match)
- `mail.google.com` (subdomain negative match)

Trailing dot varyantları matrix'te yok. `google.com.` ve `www.google.com.` test edilmiyor. Bug sınıfı CI'a görünmez.

## Fix

Her iki tarafı `HostMatchesPattern`'in en başında normalize et:

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

  // ... fonksiyonun geri kalanı değişmez
}
```

Alternatif: wildcard yokken sondaki `trimmed_host == trimmed_pattern`'i `url::DomainIs(trimmed_host, trimmed_pattern)` ile değiştir. Bu canonicalization'ı zaten var olan, test edilmiş koda delege ediyor.

Defense in depth: `family_link_settings_service.cc:667-672`'de insert öncesi trailing dot'ları strip et. Böylece saklanan blocklist her zaman canonical formda olur.

Eklenmesi gereken unit testler bariz:

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

## Etki

CVSS 3.1 base skoru **3.1 (Low)**: `AV:N/AC:L/PR:L/UI:N/S:U/C:N/I:L/A:N`. Veri sızıntısı yok, sistem destabilize olmuyor. Çocuk sadece zaten public olan sitelere erişiyor.

Asıl hikâye ürün etkisi. Family Link on milyonlarca supervised hesapta deploy edilmiş. Threat model burada remote attacker değil. Legitimate cihaz kullanıcısı (çocuk) bir başka tarafın (velinin) koyduğu kontrolü atlatıyor. Chrome VRP supervised user / enterprise URL filtering bypass'larını historik olarak in-scope kabul ediyor.

Somut sonuçlar:

- **Adult-content blocklist'leri, kumar, sosyal medya kısıtlamaları.** Tek tuş ile bypass. Velinin manuel blocklist'indeki her domain için çalışıyor.
- **Okul-managed Chromebook'lar.** Birçok K-12 distrikti managed cihazlarda content filter çalıştırıyor. Eğer altta yatan enforcement aynı code path'i kullanıyorsa (enterprise path için ayrıca verify gerek), tüm öğrenci cihaz fleet'leri exposed.
- **Yayılma vektörü.** Bir TikTok videosu, bir sınıf fısıltısı yeter, teknik her yere yayılır. Family Link'in value proposition'ı o yaş grubu için çöker. Velilerin bypass olduğuna dair sinyali yok; activity report trailing dot'lu host'u flag eder mi etmez mi belirsiz (upstream URL handling'e bağlı).
- **Reproducibility.** Trivial. Tool yok, extension yok, developer mode yok. Sadece nokta yazıyorsun.

CVSS skoru per-incident teknik etkiyi yansıtıyor. Deployment ölçeği ve "kırılan kontrat" framing'i bunu raporlamaya değer kılıyor.

## Etkilenen platformlar

| Platform | Family Link web filtresi | Etkilenen mi |
|----------|--------------------------|---------------|
| Chrome Android (supervised çocuk hesabı) | Var | Evet (verified) |
| ChromeOS (managed çocuk Chromebook) | Var | Evet (aynı code path) |
| Chrome iOS | Limited (sadece Apple Family Sharing) | Muhtemelen değil (farklı code path) |
| Chrome desktop (Windows/macOS/Linux) | 2021'de kaldırıldı | Etkilenmiyor |

Edge, Brave gibi Chromium-based browser'lar Family Link ship etmiyor, dolayısıyla alakasız.

## Bu pattern neden tekrar tekrar çıkıyor

Trailing dot bypass tipik bir host-identity mismatch. Browser URL'i network, sertifika ve process isolation için upstream'de canonicalize ediyor, ama downstream'de string karşılaştıran security check'ler ayrı kalıyor.

Chromium codebase bu fix'in maliyetini zaten ödedi: `url::DomainIs()` için, SiteInstance için, DevTools için. Her fix kendi bug'ına scope'landı. Hiçbiri sweep haline gelmedi (yani "her `url.host()` çağırıp security-relevant karşılaştırmada string equality yapan yer" şeklinde bir tarama yapılmadı). Sınıf yeni yazılan, `DomainIs()`'i kullanmayan kodda hayatta kalmaya devam ediyor.

Bu, host canonicalization katmanında CWE-178 (Improper Handling of Case Sensitivity) / CWE-20 (Improper Input Validation) sınıfı. Aynı sınıf birçok yerde çıkıyor: Host header validation, cookie domain scoping, CORS, SAML audience check'leri, OAuth redirect URI validation. Host'un string olarak karşılaştırıldığı her yerde trailing dot, uppercase, IDN ve punycode formları aynı canonical şekle normalize edilmeli. Doğru çözüm bir lint kuralı, başka bir scoped patch değil.

## Raporlama zaman çizelgesi

- **24 Mayıs 2026.** Zafiyet keşfedildi, managed Family Link çocuk hesabı ile Chrome Stable'da verify edildi.
- **24 Mayıs 2026.** Chrome VRP'ye *Permissions Bypass* kategorisinde raporlandı.
- **14 Haziran 2026.** Bu yazı.

## Referanslar

- [Chromium kaynak: `family_link_url_filter.cc`](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/components/supervised_user/core/browser/family_link_url_filter.cc)
- [Chromium kaynak: `family_link_settings_service.cc`](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/components/supervised_user/core/browser/family_link_settings_service.cc)
- [Chromium kaynak: `url::DomainIs` (`url_util.cc`)](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/url/url_util.cc)
- [crbug.com/1472898: `chrome.devtools.inspectedWindow.eval`'de DevTools trailing-dot bypass (aynı sınıf, Derin Eryılmaz buldu, $5,000 ödül)](https://crbug.com/1472898)
- [Derin Eryılmaz X'te: DevTools trailing-dot bypass orijinal yazısı](https://x.com/deryilz/status/1753394956956295488)
- [CWE-178: Improper Handling of Case Sensitivity](https://cwe.mitre.org/data/definitions/178.html)
- [CWE-20: Improper Input Validation](https://cwe.mitre.org/data/definitions/20.html)
- [Chrome Vulnerability Reward Program](https://bughunters.google.com/about/rules/chrome-friends)

## İlgili içerik

- [DNS rebinding ile SSRF]({{ site.baseurl }}{% post_url 2026-03-14-ssrf-dns-rebinding-vulnerability-tr %}): başka bir host-identity mismatch, ama resolution time'da.
- [IP adresi sınıflandırma tutarsızlıkları]({{ site.baseurl }}{% post_url 2026-03-21-ip-address-classification-inconsistencies-tr %}): aynı şekil, farklı canonicalization surface.
- [lollms'te content-type spoofing]({{ site.baseurl }}{% post_url 2026-05-03-content-type-spoofing-lollms-chat-image-cve-2026-5728-tr %}): attacker-influenced input üzerinde string-equality check'leri.
