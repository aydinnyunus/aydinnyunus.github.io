---
layout: post
title: "IP adresi sınıflandırmasındaki tutarsızlıklar: diller arası farklar"
date: 2026-03-21
author: Yunus Aydın
lang: tr
description: "IP sınıflandırması diller arasında nasıl değişiyor? Loopback ve özel (private) IP'ler Go, Java, Node.js, PHP, Python ve Ruby'de farklı işlenir; SSRF riski."
keywords: "IP adresi sınıflandırması, loopback, özel IP, private IP, SSRF, bulut güvenliği, Go, Java, Node.js, PHP, Python, Ruby, 169.254.169.254, link-local"
canonical_url: "https://aydinnyunus.github.io/2026/03/21/ip-address-classification-inconsistencies-tr/"
---

Farklı programlama dillerinde **IP adresi sınıflandırması** davranışlarını inceledim. Özellikle loopback ve özel (private) IP'lerin nasıl sayıldığı konusunda diller arasında ciddi tutarsızlıklar var. Bu yazıda gördüklerimi ve neden önemli olduğunu paylaşıyorum.

## IP adresi sınıflandırması ne demek?

Bir adresin loopback, özel (private), bağlantı-yerel (link-local) mi yoksa genel (public) mi olduğuna karar verme işlemi. Güvenlik kontrollerinde ve ağ yapılandırmasında sık kullanılıyor; fakat diller bu sınıflandırmayı aynı yapmıyor.

## Deney düzeni

Go, Java, Node.js, PHP, Python ve Ruby'de çeşitli IP'leri (localhost, özel aralıklar vb.) denedim. Öne çıkanlar şöyle.

## Loopback: 127.0.0.1

**127.0.0.1**, localhost için en bilinen adres. Dillere göre sonuçlar:

- **Go**: `IsPrivate: false`, `IsLoopback: true`
- **Java**: `IsPrivate: false`
- **Node.js**: `IsPrivate: true`, `IsLoopback: true`
- **PHP**: `IsPrivate: true`
- **Python**: `IsPrivate: True`, `IsLoopback: True`
- **Ruby**: `IsPrivate: false`

**Tutarsızlık:** 127.0.0.1'in özel (private) sayılıp sayılmaması dile göre değişiyor. Node.js, PHP ve Python özel diyor; Go, Java ve Ruby demiyor. Birçok geliştirici için beklenti, loopback'in aynı zamanda özel kabul edilmesi yönünde.

## IPv6 loopback: ::1

IPv6 loopback adresi **::1**. Yine farklar var:

- **Go**: `IsPrivate: false`, `IsLoopback: true`
- **Java**: `IsPrivate: false`
- **Node.js**: `IsPrivate: true`
- **PHP**: `IsPrivate: true`
- **Python**: `IsPrivate: True`, `IsLoopback: True`
- **Ruby**: `IsPrivate: false`

**Tutarsızlık:** ::1 için de benzer tablo: Node.js, PHP ve Python özel diyor; Go, Java ve Ruby demiyor.

## Özel (private) IP'ler

RFC 1918 ve 4193'teki özel ağ aralıkları. Dillere göre:

- **Go**: **192.168.1.1**'i doğru şekilde özel olarak tanımlar
- **Java**: **192.168.1.1**, **10.0.0.1** ve **172.16.0.1**'i doğru şekilde özel olarak tanımlar
- **Node.js**: **127.0.0.1** ve **169.254.169.254** konusunda yukarıdaki tablolarla uyumlu, tartışmalı sonuçlar veriyor
- **PHP**: **169.254.169.254** dahil birçok adresi özel olarak işaretliyor (link-local için bu tartışmalı)
- **Python**: Klasik özel aralıkları doğru işler; **169.254.169.254** hem bağlantı-yerel hem özel görünüyor, bu da kafa karıştırıcı olabilir
- **Ruby**: **169.254.169.254**'ü özel olmayan olarak işaretliyor (metadata bağlamında riskli bir yorum)

## 169.254.169.254 neden kritik?

**169.254.169.254**, bağlantı-yerel aralıkta (169.254.0.0/16); APIPA ile ilişkili. AWS ve Google Cloud'da sanal makine **örnek üstverisine** (metadata) erişimde bu adres kullanılıyor: kimlik bilgileri, örnek kimliği vb. Yanlış sınıflandırılırsa SSRF ile üstveri sızabilir.

![SSRF zafiyetinin sunucu tarafında nasıl işlediğine dair şema](https://www.imperva.com/learn/wp-content/uploads/sites/13/2021/12/How-Server-SSRF-works.png)

### Diller 169.254.169.254'ü nasıl görüyor?

- **Go**: `IsLinkLocalUnicast: true`, `IsPrivate: false` (özel değil)
- **Java**: `IsPrivate: false`; bağlantı-yerel adreslerin RFC anlamında özel blokta olmaması ve yalnızca yerel alt ağ bağlamında anlamlı olmasıyla uyumlu
- **Node.js**: `IsPrivate: true`, `IsLoopback: false`. Bağlantı-yerel adreslerin çoğu modele göre özel sayılmamalı; burada anlam tartışması var
- **PHP**: Özel olarak işaretliyor; birçok güvenlik modeli için bağlantı-yerel ile özelin ayrılması beklentisiyle çelişebilir
- **Python**: `IsLinkLocal: True` doğru; ayrıca özel olarak da işaretleniyor
- **Ruby**: Özel olmayan olarak işaretliyor (yanlış güven varsayımına yol açabilecek taraf bu)

## Neden önemli?

**169.254.169.254** gibi bağlantı-yerel adresler bulutta örnek üstverisi için kullanılıyor. Yanlış sınıflandırma, **SSRF** zafiyeti olan ortamlarda ciddi risk: saldırgan üstveriden geçici kimlik bilgileri alabilir, ayrıcalık yükseltebilir veya zincirleme saldırı başlatabilir.

**AWS:** `http://169.254.169.254/latest/meta-data/` EC2 örneği hakkında üstveri (IAM rolleri vb.) sunar. **Google Cloud**'da da benzer. Bu adrese “özel değil” diyerek izin vermek, hassas bilginin sızmalarına yol açabilir.

## Özet

Diller aynı IP'yi farklı sınıflandırıyor; loopback (127.0.0.1, ::1) ve bağlantı-yerel (169.254.169.254) özellikle tutarsız. Bu da SSRF ve bulut üstverisi sızıntısı riskini artırıyor.

**Çıkarımlar:**

- Aynı IP farklı dillerde farklı “özel” / “loopback” sonucu verebiliyor
- Loopback ile bağlantı-yerel ayrımı dil ve kütüphaneye göre değişiyor
- Bu tutarsızlıklar SSRF senaryolarında risk oluşturuyor
- Bulut ortamında özellikle dikkat etmek gerekiyor

## Öneriler

**Standartlaştırma:** IP sınıflandırması için net kurallar ve beklenen davranış tanımları.

**Test:** Farklı ortamlarda IP sınıflandırma yardımcılarını test et; özellikle 169.254.169.254 ve loopback için.

**Farkındalık:** Yerel / özel ağ adresleriyle çalışırken diller arası farkları bil; SSRF koruması tasarlarken bunu hesaba kat.

## Go için Semgrep kuralı

Go'da IP sınıflandırmasını gözden geçirirken aşağıdaki Semgrep kuralını deneyebilirsin:

```yaml
rules:
  - id: go-check-isprivate
    languages: [go]
    patterns:
      - pattern: $IP.IsPrivate()
      - pattern-not: $IP.IsLinkLocalUnicast()
    message: "IP adresi işleme metodlarının MustParseAddr veya ParseIP içerdiğinden ve IP'yi parse ettikten sonra IsPrivate, IsLoopback veya IsLinkLocalUnicast kullanarak doğruladığından emin olun."
    severity: WARNING
```

Bu kural, yalnızca `IsPrivate()` çağrılırken `IsLinkLocalUnicast()` kontrolünün de düşünülmesini hatırlatıyor.

## Kaynak kod

Tüm kod ve çıktılar: [github.com/aydinnyunus/isItPrivate](https://github.com/aydinnyunus/isItPrivate). Diller arası IP sınıflandırması ve SSRF bağlamındaki güvenlik etkileri burada toplu.

## Google'a bildirim

169.254.169.254'ün dillerde nasıl sınıflandırıldığındaki tutarsızlığı netleştirmek için konuyu Google'a ilettim. 3 Mart'taki yanıt özeti:

> _"IP.IsPrivate, bir adresin IANA özel bloklarında olup olmadığına bakar (RFC 1918, 4193). 169.254.169.254 bu aralıklarda değil. 169.254/16 bağlantı-yerel öneki; IP.IsLinkLocalMulticast ve IP.IsLinkLocalUnicast ile doğru raporlanıyor. Tasarlandığı gibi çalışıyor."_

Tutarsızlık, bağlantı-yerel ile özel (private) kavramlarının diller ve kütüphaneler arasında farklı yorumlanmasından geliyor. Bu ayrımı bilmek, SSRF koruması ve bulut güvenliği için önemli.

## İlgili içerik

- [SQL Injection Zafiyeti: GeoPandas to_postgis() Fonksiyonunda Güvenlik Açığı](/2025/12/27/sql-injection-geopandas-tr/)
- [SSRF Zafiyeti: DNS Rebinding Saldırısı ile Bypass](/2026/03/14/ssrf-dns-rebinding-vulnerability-tr/)
