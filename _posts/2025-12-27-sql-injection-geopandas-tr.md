---
layout: post
title: "SQL Injection Zafiyeti: GeoPandas to_postgis() Fonksiyonunda Güvenlik Açığı"
date: 2025-12-27
author: Yunus Aydın
lang: tr
description: "GeoPandas kütüphanesinin to_postgis() fonksiyonunda bulduğum SQL injection zafiyeti ve nasıl fix ettiğim. Parameterized queries ve güvenli SQL kullanımı."
keywords: "SQL injection, GeoPandas, to_postgis, PostgreSQL, security vulnerability, parameterized queries, security research, bug bounty"
canonical_url: "https://aydinnyunus.github.io/2025/12/27/sql-injection-geopandas-tr/"
---

Bir gün GeoPandas kütüphanesini kullanırken, `to_postgis()` fonksiyonunda bir şeylerin yanlış olduğunu fark ettim. Kullanıcı girdileri doğrudan SQL sorgularına ekleniyordu. Bu, klasik bir SQL injection zafiyetiydi. Zafiyeti bulduktan sonra fix'i de yazdım ve pull request açtım. Bu yazıda ne bulduğumu ve nasıl düzelttiğimi anlatacağım.

## SQL Injection Nedir?

SQL injection, kullanıcı girdilerinin doğrudan SQL sorgularına eklenmesiyle ortaya çıkan bir güvenlik zafiyetidir. Saldırgan, özel karakterler kullanarak veritabanı sorgularını manipüle edebilir.

Bu zafiyet neden tehlikeli? Çünkü:
- Veritabanındaki tüm verilere erişim sağlayabilir
- Veritabanı yapısını değiştirebilir
- Veri silebilir veya değiştirebilir
- PostgreSQL'de `COPY` komutu gibi sistem komutlarını çalıştırabilir

## Zafiyeti Nasıl Buldum?

GeoPandas'ın `to_postgis()` fonksiyonu, GeoDataFrame'leri PostgreSQL veritabanına yazmak için kullanılıyor. Fonksiyon, kullanıcıdan aldığı tablo adı ve schema adı gibi parametreleri doğrudan SQL sorgularına ekliyordu. Gerçek zafiyetli kod şöyleydi:

```python
if connection.dialect.has_table(connection, name, schema):
    target_srid = connection.execute(
        text(f"SELECT Find_SRID('{schema_name}', '{name}', '{geom_name}');")
    ).fetchone()[0]
```

Sorun açık: `schema_name`, `name` ve `geom_name` değişkenleri doğrudan f-string içine ekleniyor. Bu, SQL injection saldırılarına açık.

## Nasıl Exploit Edilebilir?

SQL injection zafiyetini exploit etmek için, `schema_name`, `name` veya `geom_name` parametrelerine özel karakterler ekleyebiliriz. Mesela:

```python
# Saldırgan girdisi
name = "test'; DROP TABLE important_data; --"

# Oluşan SQL sorgusu
# SELECT Find_SRID('public', 'test'; DROP TABLE important_data; --', 'geom');
```

Bu exploit, `Find_SRID` fonksiyonunu çağırdıktan sonra `important_data` tablosunu silebilir.

Başka exploit örnekleri:
- Veri okuma: `name = "test' UNION SELECT password FROM users --"`
- Veri değiştirme: `name = "test'; UPDATE users SET password='hacked' --"`
- Sistem komutları: `name = "test'; COPY (SELECT 1) TO PROGRAM 'rm -rf /' --"`

## Fix: Parameterized Queries

Zafiyeti fix etmek için, kullanıcı girdilerini doğrudan SQL sorgularına eklemek yerine, parameterized queries kullandım. Parameterized queries, kullanıcı girdilerini SQL sorgularından ayırarak, SQL injection saldırılarını önler.

Fix'in nasıl çalıştığı:

1. Tablo adı ve schema adı gibi kullanıcı girdilerini validate ettim
2. Identifier'ları (tablo adı, schema adı) güvenli şekilde escape ettim
3. PostgreSQL'in identifier quoting mekanizmasını kullandım
4. Kullanıcı girdilerini doğrudan SQL sorgularına eklemek yerine, güvenli identifier'lar kullandım

Fix'in avantajları:
- SQL injection saldırılarını önler
- Kullanıcı girdilerini güvenli şekilde işler
- PostgreSQL'in identifier quoting mekanizmasını kullanır
- Mevcut API'yi bozmadan güvenliği artırır

Identifier validation:
- Tablo adları ve schema adları sadece alfanumerik karakterler ve alt çizgi içerebilir
- Özel karakterler ve SQL keyword'leri bloklanır
- Identifier'lar PostgreSQL'in identifier quoting mekanizması ile güvenli şekilde escape edilir

## Sonuç

SQL injection zafiyetleri, özellikle kullanıcı girdilerinin doğrudan SQL sorgularına eklenmesi durumunda çok tehlikeli olabilir. Bu zafiyeti bulmak ve fix etmek, GeoPandas kütüphanesinin güvenliğini önemli ölçüde artırdı.

Bu tür zafiyetleri bulmak ve fix etmek, açık kaynak kütüphanelerinin güvenliğini artırmak için kritiktir. Eğer benzer zafiyetler bulursanız, sorumlu açıklama (responsible disclosure) prensiplerine uyarak geliştirici ekibine bildirmenizi ve mümkünse fix'i de kendiniz yazmanızı öneriyorum.

**İlgili içerikler:**
- [CVE-2025-66019: pypdf Kütüphanesinde LZW Decompression DoS Zafiyeti](/2025/12/20/cve-2025-66019-pypdf-lzw-dos-tr/)
- [SSRF Zafiyeti: DNS Rebinding Saldırısı ile Bypass](/ssrf-dns-rebinding-vulnerability-tr/)

**Kaynaklar:**
- [GitHub PR: BUG: SQL Injection Exploit Report - GeoPandas to_postgis()](https://github.com/geopandas/geopandas/pull/3681)
- [OWASP: SQL Injection](https://owasp.org/www-community/attacks/SQL_Injection)
- [PostgreSQL Identifier Quoting](https://www.postgresql.org/docs/current/sql-syntax-lexical.html#SQL-SYNTAX-IDENTIFIERS)

