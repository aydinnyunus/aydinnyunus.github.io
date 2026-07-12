---
layout: post
title: "ModHeader v7.0.17: Browsing Geçmişinizi api.stanfordstudies.com'a Sızdıran Chrome Eklentisi"
date: 2026-07-12
author: Yunus Aydın
lang: tr
description: "ModHeader Chrome eklentisi ziyaret ettiğiniz her domain'i AES-GCM ile şifreleyip api.stanfordstudies.com'a gönderiyor. IndexedDB'de topluyor, upload sonrası kanıtları siliyor."
keywords: "ModHeader, data exfiltration, Chrome eklentisi güvenliği, stanfordstudies.com, AES-GCM, tarayıcı eklentisi malware, veri sızdırma"
canonical_url: "https://aydinnyunus.github.io/2026/07/12/modheader-data-exfiltration-stanfordstudies-tr.html"
---

ModHeader'ın 1.6 milyon kullanıcısı var (900K Chrome + 700K Edge). Geçen hafta Reddit ve HN'de birileri eklentinin `api.stanfordstudies.com/app/log` adresine veri gönderdiğini fark etti. Ben de v7.0.17 sürümünü indirip exfiltration pipeline'ının tamamını reverse engineer ettim. Sonuçlar ilk raporlardan daha vahim: AES-GCM şifreli IndexedDB depolama, rastgele upload zamanlaması, ve upload sonrası kanıt imhası.

## Ne Oluyor

ModHeader HTTP header'larını değiştirmek için kullanılan bir eklenti. İlk tespit Reddit'te u/veqtor tarafından yapıldı. Ben de bunun üzerine eklentinin background script'ini açıp tüm veri toplama zincirini deşifre ettim.

Minified bundle'daki kritik satır:

```javascript
globalThis.maxDomainCount=1e3;
const Qc=1,qw="https://api.stanfordstudies.com/app/log",$w=[],Yw="mod盐header";
```

Obfuscated değişken isimleri (`qw`, `Yw`, `Qc`) ve unicode salt (`盐`) klasik evasion sinyalleri.

## Nasıl Çalışıyor

**Hardcoded AES-GCM anahtarı.** Eklenti başlarken şu key'i import ediyor:

```javascript
qo = await Ho.importKeyFromBase64("aWfU3yG_wksZaQdSnxPJBOId0cAN8KK/UIlZbli7-bE");
```

Toplanan her domain bu key ile şifreleniyor. Network'te ham veri dolaşmıyor, her şey opaque binary gibi görünüyor.

**İki IndexedDB veritabanı.** Biri ayarlar (`settings`), diğeri domain verileri (`temp`). İkisi de arka planda sessizce açılıyor.

**Her kuruluma özel fingerprint.** `SHA-256(Date.now().toString())` ile üretilen unique bir ID her POST request'inde gönderiliyor. Aynı kullanıcıyı farklı oturumlarda tanımak için.

## Veri Toplama Akışı

Adım adım:

1. **Her sayfa yüklemesine hook.** `chrome.tabs.onUpdated` listener'ı her tab'ın URL'ini alıyor. Sessizce, hiçbir UI göstermeden.

2. **Domain'i şifrele ve IndexedDB'ye kaydet.** `Zw()` fonksiyonu domain'i AES-GCM ile şifreliyor, `temp` store'unda `{şifrelidomain: ziyaretSayısı}` olarak tutuyor. Aynı domain peşpeşe gelirse tekrar kaydetmiyor, sayacı artırıyor.

3. **Threshold kontrolü.** İki koşuldan biri tetiklenince upload başlıyor:
   - 1,000 unique domain birikti
   - 24 saat geçti

4. **Hedefe POST.** Tüm domain listesi AES-GCM ile şifrelenip `api.stanfordstudies.com/app/log` adresine JSON olarak gönderiliyor.

5. **Kanıtları sil.** Upload başarılı olunca `await r.clear()` ile IndexedDB siliniyor. Geriye bir şey kalmıyor.

## Sızdırılan Veriler

| Veri | Detay |
|---|---|
| Ziyaret edilen tüm domain'ler | AES-GCM şifreli, ziyaret sayısıyla birlikte |
| Device fingerprint | Her kuruluma özel, SHA-256 hash |
| Browser bilgisi | User agent + platform |
| Upload geçmişi | Hangi upload'lar başarılı, hangileri değil |

## Gizlenme Teknikleri

**Rastgele upload zamanlaması.** `zw()` fonksiyonu `SHA-256(fp + "mod盐header")` çıktısının 8 saatlik mod'u alıp üstüne 7 saat ekliyor. Sonuç: gece yarısından itibaren 7 ila 15 saat arasında rastgele bir upload penceresi. Her kullanıcı farklı saatte upload yapıyor.

**Kanıt imhası.** Upload başarılı → IndexedDB wipe. Geriye kalan tek şey `settings` store'unda bir `uploadRecord: {tarih: true}` satırı.

**Encrypted transport.** AES-GCM şifrelemesi sayesinde network monitor'lar domain listesi yerine random byte dizisi görüyor.

**Sessiz hata yönetimi.** Her try/catch boş. Hata olsa da kullanıcı hiçbir şey fark etmiyor.

## Ekstra Tracking

Eklenti ayrıca kurulum, güncelleme ve kaldırma olaylarını `extensions-hub.com`'a bildiriyor. Her güncellemede `extensions-hub.com/partners/updated/?name=ModHeader` adresine yeni bir tab açılıyor. Kaldırma sırasında da `setUninstallURL` ile aynı adrese POST gidiyor. Yani eklentiyi silseniz bile son bir istek gidiyor.

## Bu Neden Sürekli Karşımıza Çıkıyor

Browser eklentileri çoğu kurumda göz ardı ediliyor. Her sayfada çalışabiliyor, IndexedDB'ye yazabiliyor, network'e çıkabiliyor. Chrome Web Store inceleme süreci obfuscated kodları yakalamakta yetersiz kalıyor, çünkü incelemeci birkaç dakika bakıp geçiyor.

Manifest V3 bazı kısıtlamalar getirdi ama bu eklenti V3 olmasına rağmen aynı exfiltration pipeline'ını çalıştırıyor. Yani V3 tek başına çözüm değil.

Hardcoded AES key, randomized timer, evidence wipe: aynı evasion triad'ını [Foxtopia NPM supply-chain malware](https://aydinnyunus.github.io/2026/06/22/fake-job-offer-npm-supply-chain-malware-foxtopia/) analizinde de görmüştüm. Buradaki unicode salt (`mod盐header`) aynı obfuscation tekniği.

Ekonomik boyutu da var: milyonlarca kullanıcılı ücretsiz eklentiler paraya çevrilmemiş veri dağının üzerinde oturuyor. `api.stanfordstudies.com` gibi akademik görünümlü bir domain kullanmak, veri toplamayı meşru göstermenin klasik bir yolu.

## Etki

- ModHeader yüklüyken ziyaret ettiğiniz her domain kaydediliyor ve dışarı gönderiliyor. Buna internal corporate domain'ler, VPN login sayfaları, cloud console URL'leri dahil.
- Veri wire'da şifreli ama sunucuda decrypt edilebilir durumda. Kullanıcının kontrolünde bir anahtar yok.
- Network kesikse veri IndexedDB'de birikiyor, bağlantı gelince upload devam ediyor.
- extensions-hub.com tracking'i ek bir telemetri katmanı ekliyor.

## Ne Yapmalı

Bu bulgunun CVE'si yok. ModHeader ticari bir eklenti, Chrome Web Store ve Edge Add-ons üzerinden dağıtılıyor. Google ve Microsoft zaten malware olarak işaretlemiş durumda. Tavsiyem: eklentiyi kaldırın ve alternatif (Requestly, Fiddler Everywhere gibi) kullanın.

## Timeline

| Tarih | Olay |
|---|---|
| 10 Temmuz 2026 | Reddit'te ilk tespit (u/veqtor) |
| 11 Temmuz 2026 | Exfiltration pipeline'ının full reverse engineering'i tamamlandı |
| 12 Temmuz 2026 | Chrome Web Store'a abuse raporu gönderildi. Bu yazı yayınlandı. |

## Referanslar

- [Chrome Web Store: ModHeader](https://chromewebstore.google.com/detail/idgpnmonknjnojddfkpgkljpfnnfcklj)
- [Chrome Web Store Abuse Reporting](https://support.google.com/chrome_webstore/answer/7508032)
- [CWE-200: Exposure of Sensitive Information](https://cwe.mitre.org/data/definitions/200.html)
- [OWASP: Data Exposure](https://owasp.org/www-community/vulnerabilities/Information_exposure)
- [Manifest V3 dokümantasyonu](https://developer.chrome.com/docs/extensions/develop/migrate)
- [Reddit'te ilk tespit (u/veqtor): "1.6 Million Combined Installs — Famous Extension"](https://www.reddit.com/r/cybersecurity/comments/1usic57/16_million_combined_installs_famous_extension/)

## İlgili İçerik

- [Hunting Leaked Secrets on the GitHub Archive](/2026/06/30/hunting-leaked-secrets-on-github-archive.html)
- [Foxtopia: Fake Job Offer NPM Supply Chain Malware](/2026/06/22/fake-job-offer-npm-supply-chain-malware-foxtopia.html)
- [Odysseus: Embedding Endpoint Takeover](/2026/06/16/odysseus-embedding-endpoint-takeover.html)