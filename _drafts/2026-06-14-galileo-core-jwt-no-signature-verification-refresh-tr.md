---
layout: post
title: "Galileo-Core: Refresh Token Akışında JWT İmza Doğrulaması Yok"
date: 2026-06-14
author: Yunus Aydın
lang: tr
description: "9 Nisan 2024'te galileo-core'un refresh token fonksiyonundaki JWT misconfiguration'ı Galileo ekibine raporladım. SDK, JWT'yi verify_signature=False ile decode edip dönen exp claim'ine güveniyordu."
keywords: "galileo-core, Galileo Labs, JWT, refresh token, imza doğrulama, PyJWT, güvenlik araştırması, AI platform güvenliği, sorumlu açıklama"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/galileo-core-jwt-no-signature-verification-refresh-tr/"
---

Ben [galileo-core](https://pypi.org/project/galileo-core/) paketindeki JWT imza doğrulama eksikliğini 9 Nisan 2024'te Galileo Labs ekibine doğrudan raporladım. SDK'nın refresh token akışı, JWT'yi `options={"verify_signature": False}` ile decode ediyor, ardından o doğrulanmamış payload'dan gelen `exp` claim'ine bakıp token'ı yenileyip yenilemeyeceğine karar veriyordu. Doğrulanmamış bir token'a güvenip ona göre güvenlik kararı vermek, static analyzer'ların alarm verdiği klasik bir pattern. Burada da alarm vermeyi hak ediyordu.

Galileo'nun AI evaluation müşterilerinin backend'lerine çektiği SDK client'larını okuyordum. Dependency'sinde `pyjwt` olan her şeyde önce `verify_signature=False` araması yapıyorum, çünkü Python JWT kodundaki en sık ayağa basılan mayın orada. İlk hit `galileo_core/schemas/base_config.py` içindeki, access token'ın yenilenip yenilenmeyeceğine karar veren fonksiyonda çıktı.

## Savunmasız kod

Raporladığım sırada fonksiyon şöyleydi. Aynı şekil güncel build'lerde hâlâ duruyor:

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

Tek satıra iki problem sığmış:

1. `verify_signature=False` PyJWT'nin yaptığı tek kriptografik kontrolü kapatıyor.
2. O decode'un sonucu bir kontrol akışı kararına (yenile/yenileme) giriyor. Yani client, claim payload'ını otorite kabul ediyor.

Refresh akışının kendisi `refresh_token` cookie'sini sunucuya gönderdiği için sunucu da kararı veriyor. Ama bu kontrolden sonra `self.jwt_token` üzerinde çalışan her şey, claim'in doğrulanmış bir token'dan geldiğini varsayıyor. Gelmedi.

## Root cause

Sıfır doğrulama. Sıfır.

1. PyJWT'nin default'u doğrulama yapmak. Bu kod onu açıkça kapatmış.
2. Beklenen `iss`, `aud` veya signing algorithm için bir allowlist yok.
3. Fonksiyon JWKS veya public key resolve etmiyor. Yani client, cache'lediği access token'ın Galileo'nun auth server'ından mı yoksa attacker'ın disk'e yazdığı bir dosyadan mı geldiğini bilmiyor.
4. Güvendiği `exp` claim'i, tampered bir token'ın taşıyacağı claim. Lokal "bu token hâlâ geçerli mi?" kontrolü, config dosyasına veya env'e yazabilen herhangi bir adversary karşısında anlamsız.

## Etki

- Stored bir JWT'yi (config dosyası, env var, leak olmuş cookie) değiştirebilen attacker, `exp` claim'ini jwt.io ile uzatabilir. Client de bu token'ı sonsuza kadar valid kabul edip refresh tetiklemez. Sunucu sonunda reddeder, ama o round-trip olana kadar SDK valid bir session'ı varmış gibi çalışır.
- Lokal trust kararları ("daha refresh etme, exp on dakika sonra") attacker-controlled veriyle veriliyor.
- Codebase audit eden herkes, sunucu telafi etse bile `verify_signature=False`'u red flag olarak okur. Regulated bir ortamda bu, gitmediği sürece her external review'da finding olarak çıkar.

## Proof of concept

```python
# Step 1: mevcut bir access token al (cache'lenmiş config, leak olmuş env)
stolen = "eyJhbGciOi...payload...sig"

# Step 2: exp'i tamper et, jwt.io veya herhangi bir base64 encoder ile, key gerekmez
import base64, json, time
header, payload, _sig = stolen.split(".")
new_payload = base64.urlsafe_b64encode(
    json.dumps({"exp": int(time.time()) + 60 * 60 * 24 * 365}).encode()
).rstrip(b"=").decode()
tampered = f"{header}.{new_payload}.{_sig}"

# Step 3: tampered token'ı SDK config'ine yerleştir
from galileo_core.schemas.base_config import GalileoConfig
GalileoConfig.set_current(jwt_token=tampered, ...)

# Step 4: SDK verify_signature=False ile decode eder, exp > now + 300 görür,
# refresh endpoint'ini hiç çağırmaz. Lokal state "mutlu."
```

Sunucu tampered token taşıyan request'i yine reddeder, yani bedava API erişimi alamazsın. Aldığın şey, ilk request fail olana kadar kendi auth state'i hakkında kendine yalan söyleyen bir client.

## Düzeltme

Doğru şekil, sunucu tarafında doğrulama yapmak ve lokal decode'u tamamen kaldırmak. Ya da her request'te refresh endpoint'ini çağırıp sunucuyu source of truth yapmak. Lokal kontrol kaçınılmazsa, public key ile decode et:

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

Signing key'e erişimi olmayan saf bir client SDK için daha temiz cevap: `exp`'i lokalde hiç okuma. 401'de refresh et, timer üzerinden proactive refresh et, dakikada bir `/me` çağır. Doğrulanmamış bir token'ın payload'ına güvenip client'ın bir sonraki adımını ona göre belirlemek dışında her şey kabul edilebilir.

## Bu pattern neden tekrar tekrar çıkıyor

"Sadece expiry'yi kontrol et" diyen her PyJWT tutorial'ı bu kodu üretiyor. Yazar JWKS dance'inden kaçmak istiyor, test ortamında elinin altında gerçek signing key yok, `verify_signature=False` snippet'in derlenmesini ve test'lerin geçmesini sağlıyor. Code review ya flag'i kaçırıyor ya da "client-side, sadece claim okuyoruz, sunucu doğruluyor" açıklamasını kabul ediyor. Bu cümlelerin ikisi de teknik olarak doğru olabilir ve yine de yanıltıcı bir audit trail ile attacker-controlled byte'lara güvenip routing kararı veren bir runtime üretebilir.

Önemli olan sınır "client vs server" değil. Önemli olan "bu claim, az sonra vereceğim bir karara load-bearing mı?" Cevap evet ise doğrulamak zorundasın. Cevap hayır ise zaten decode etmen gerekmiyor. Galileo-core kodu üçüncü yolu seçmiş: doğrulamadan decode etmek ve sonra karar vermek. Tehlikeli kombinasyon tam olarak bu.

## Raporlama zaman çizelgesi

- **9 Nisan 2024**: Bulguyu Galileo ekibine e-posta ile doğrudan raporladım. Yanıt alınmadı.
- **14 Haziran 2026**: Bu yazı.

## Referanslar

- [galileo-core PyPI sayfası](https://pypi.org/project/galileo-core/)
- [PyJWT signature verification dokümantasyonu](https://pyjwt.readthedocs.io/en/stable/usage.html#requiring-presence-of-claims)
- [CWE-347: Improper Verification of Cryptographic Signature](https://cwe.mitre.org/data/definitions/347.html)
- [OWASP JSON Web Token Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/JSON_Web_Token_for_Java_Cheat_Sheet.html)

## İlgili içerik

- [Langflow Monitor Service'te SQL Injection](/2026/06/14/langflow-sql-injection-monitor-service-tr/)
- [Mage-AI Git Utils'de Command Injection](/2026/06/14/command-injection-mage-ai-git-utils-tr/)
- [CVE'siz Güvenlik Fix'leri Bulmak: Changelog Analyzer](/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer/)
