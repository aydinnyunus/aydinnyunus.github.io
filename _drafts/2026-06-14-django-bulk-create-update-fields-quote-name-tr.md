---
layout: post
title: "Django bulk_create() update_fields: quote_name() escape etmeyi unutuyor"
date: 2026-06-14
author: Yunus Aydın
lang: tr
description: "Django'nun quote_name() fonksiyonu SQL identifier'larını çift tırnakla sarıyor ama içerideki tırnağı escape etmiyor. bulk_create() update_fields'a bozuk bir alan adı verince ON CONFLICT cümlesi kırılıyor."
keywords: "Django, SQL injection, bulk_create, update_fields, quote_name, ORM, identifier escape, on_conflict_suffix_sql"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/django-bulk-create-update-fields-quote-name-tr/"
---

Ben Django `bulk_create()` içinde olası bir SQL injection bulup Django güvenlik ekibine raporladım. Bug `quote_name()` fonksiyonunda: SQL identifier'ları dış çift tırnakla sarıyor ama identifier içine gömülü çift tırnağı escape etmiyor. `update_fields`'a içinde `"` olan bir alan adı gönder, üretilen `ON CONFLICT ... DO UPDATE SET` cümlesi kırık (ya da column ismine ne sokabildiğine göre, injectable) çıkıyor.

Bunu `bulk_create()` kod yoluna bakarken buldum. PostgreSQL üzerinde `ON CONFLICT` SQL'ini Django nasıl kuruyor merak ediyordum. PoC blog post'a sığacak kadar küçük.

Django 4.2.x ve 5.x üzerinde, PostgreSQL 15 ve SQLite 3.45 ile reproduce ettim.

## Model

Django, model tanım anında `db_column`'a neredeyse her stringi koymanıza izin veriyor. İçinde tırnak olan biri de dahil:

```python
class Experiment(models.Model):
    start_datetime = models.DateTimeField()
    end_datetime = models.DateTimeField(null=True, blank=True)
    test = models.TextField(null=True, blank=True, db_column='test"')
```

Buradaki kaldıraç `db_column='test"'`. Çoğu developer bunu bilerek yapmaz, ama değer hiç sanitize edilmeden SQL identifier üretimine akıyor.

## Savunmasız view

Sorunu sergileyen yapay bir view. `field` query parametresi direkt `update_fields`'a gidiyor:

```python
def vuln_bulk_create(request):
    payload = request.GET.get('field')

    start = datetime(2015, 6, 15)
    end = datetime(2015, 7, 2)
    objects = [
        Experiment(start_datetime=start, end_datetime=start),
        Experiment(start_datetime=end, end_datetime=end),
    ]

    Experiment.objects.bulk_create(
        objects,
        update_conflicts=True,
        update_fields=[payload, 'end_datetime'],
        unique_fields=['start_datetime', 'end_datetime'],
    )
```

Kullanıcı input'unu direkt `update_fields`'a geçirmek developer hatası. Bu fikri akılda tutun.

## Root cause: quote_name() escape etmiyor

PostgreSQL için `quote_name()` (`django/db/backends/postgresql/operations.py`):

```python
def quote_name(self, name):
    if name.startswith('"') and name.endswith('"'):
        return name  # Quoting once is enough.
    return '"%s"' % name
```

Bu dört satırda iki sorun var.

Birincisi, **içerideki `"` karakteri escape edilmiyor.** `name` eğer `te"st` ise, fonksiyon `"te"st"` döndürüyor. Üç tırnak, iki değil. SQL identifier ortadaki tırnakta erken kapanıyor, sonrası yeni SQL token'ı olarak parse ediliyor.

İkincisi, **"zaten quote'lanmış" kontrolü çok gevşek.** `startswith('"') AND endswith('"')` ile çalışıyor. `"foo"` versen, olduğu gibi geri dönüyor, normal. Ama `"foo` (sadece baştaki tırnak) verirsen, kontrol fail oluyor ve fonksiyon sarıyor: `""foo"`. Aynı erken-kapanma problemi farklı bir açıdan.

`on_conflict_suffix_sql()` builder'ı bu "quote'lanmış" stringleri `%`-format template'ine besliyor:

```python
def on_conflict_suffix_sql(self, fields, on_conflict, update_fields, unique_fields):
    if on_conflict == OnConflict.UPDATE:
        return "ON CONFLICT(%s) DO UPDATE SET %s" % (
            ", ".join(map(self.quote_name, unique_fields)),
            ", ".join(
                [
                    f"{field} = EXCLUDED.{field}"
                    for field in map(self.quote_name, update_fields)
                ]
            ),
        )
```

`db_column='test"'` ile, Django'nun `INSERT ... ON CONFLICT ... DO UPDATE` için ürettiği SQL şu:

```sql
INSERT INTO "vuln_experiment" (..., "test"") VALUES (...)
ON CONFLICT ("start_datetime", "end_datetime")
DO UPDATE SET "test"" = EXCLUDED."test"", "end_datetime" = EXCLUDED."end_datetime"
```

`"test""` token'ı kırık. Açılan tırnak identifier başlatıyor, ikinci tırnak kapatıyor, üçüncü tırnak temiz kapanmayan yeni bir identifier açıyor. PostgreSQL burada syntax hatası fırlatıyor. Ama aynı kod yolu, daha dikkatli seçilmiş bir column ismiyle statement'ı parse edilebilir tutuyor. Mesela `db_column='id" = 1, "x'` düşün. `quote_name()` bunu sardıktan sonra `DO UPDATE SET` fragment'ı şuna dönüşüyor: `"id" = 1, "x" = EXCLUDED."id" = 1, "x"`. Ortadaki kapanan tırnak Django'nun amaçladığı identifier'ı sonlandırıyor, `= 1, "x"` SET listesine saldırgan-kontrollü bir atama olarak sızıyor, ve sonraki `EXCLUDED.` referansı Django'nun planlamadığı bir yere bakıyor. Injection'ın şekli bu: klasik `; DROP TABLE` değil, `DO UPDATE SET` cümlesini yeniden yazma kabiliyeti.

## Neden "olası" diyorum, "kesin" değil

Bug ile gerçek exploit arasında iki kapı var.

İlk kapı: zararlı identifier'ı field listesine sokmak. İki yol var:

1. Developer `request.GET` / `request.POST` verisini doğrudan `update_fields`, `only`, `defer`, `order_by` veya identifier string'i alan başka bir ORM kwarg'ına geçirir. Django [güvenlik politikası](https://docs.djangoproject.com/en/dev/topics/security/#sql-injection-protection) bunu açıkça documented misuse olarak işaretliyor: identifier'lar trusted kaynaklardan gelmek zorunda.
2. Developer içinde `"` olan bir `db_column` tanımlar. Garip ama model tanım anında legal ve hiçbir validation reddetmiyor.

İkinci kapı: `_check_bulk_create_options` ve `model._meta.get_field()`. `update_fields` için Django isimleri `get_field()` üzerinden çözüyor. Yani değer, model üzerinde gerçek bir alan adına denk gelmek zorunda. `te"st` adlı bir model field'ı kayıt edilmiş olmalı. `db_column` bu yüzden daha gerçekçi açı: model tanımlarını etkileyebilen biri (güvensiz kaynaklardan gelen migration'lar, üretilmiş modeller, plugin metadata'sı, schema introspection'dan beslenen model tanımları) bozuk bir identifier'ı içeri sokabilir.

## Etki

Bozuk identifier SQL execution'a ulaşırsa, masada üç sonuç var:

- `ON CONFLICT ... DO UPDATE SET` cümlesi bozuk, PostgreSQL statement'ı reddediyor. O kod yolu için denial of service.
- Column ismini tamamen kontrol eden saldırgan, kapanan tırnaktan sonra SQL ekler. Klasik identifier tabanlı SQL injection.
- SQLite'ın `on_conflict_suffix_sql()`'i de aynı `quote_name()`'i ve aynı `%`-format assembly'sini kullanıyor, yani `db_column='test"'` orada da aynı bozuk token'ı üretiyor.

## Düzeltme

`quote_name()` içerideki `"` karakterini ikileyerek escape etmeli. SQL standart identifier escape kuralı bu. Şöyle bir şey:

```python
def quote_name(self, name):
    if name.startswith('"') and name.endswith('"'):
        return name
    return '"%s"' % name.replace('"', '""')
```

"Zaten quote'lanmış" shortcut'ı emekli edilmeli, ya da sadece edge'lerde balanced bir unescaped tırnak çiftinde fire eden bir şekilde sıkılaştırılmalı. Şu anki `startswith and endswith` kontrolü, güvenli görünen ama olmayan validation tipinden.

## Bu pattern neden tekrar tekrar çıkıyor

Identifier quoting çözülmüş bir problem gibi hissettiriyor, çünkü string quoting çözüldü. Django tüm değerleri DB driver üzerinden parametrize ediyor, yani `Model.objects.filter(...)`'a geçen her değer prepared statement'tan akıyor. Ama identifier'lar tamamen farklı bir yoldan gidiyor: ORM'ler onları `%` formatlamayla birleştiriyor ve input'a güveniyor. Örtük contract şu: "identifier'lar developer'dan gelir, request'ten değil". Type sistem ya da API yüzeyinde bu contract'ı net iletecek bir şey yok.

Yani birkaç yılda bir, developer `update_fields=[request.GET.get('field')]` yazıyor. API kabul ediyor, test geçiyor, failure mode'u kafasında `"` olan biri o input'u denemeden görünmez. [CWE-89](https://cwe.mitre.org/data/definitions/89.html) injection class'ını kapsıyor, ama derin ders şu: identifier alan API'lar ya güvensiz input'u yüksek sesle reddetmeli ya da defansif olarak escape etmeli. Sessizce `"` kabul edip SQL'e yapıştırmak, iki dünyanın da en kötüsü.

## Raporlama zaman çizelgesi

- **14 Haziran 2026**: Bug ve PoC'yi `security@djangoproject.com`'a raporladım.
- **Durum**: Yazı yazılırken yanıt bekleniyor. Django güvenlik ekibinden cevap geldiğinde post'u güncelleyeceğim. Eğer documented misuse olarak sınıflandırırlarsa (güvenlik politikaları kuvvetli ima ediyor), patch senin uygulama kodunda olmalı: `update_fields`'a asla kullanıcı input'u geçirme. Hardening fix olarak kabul ederlerse, `quote_name()` proper double-escape alır.

## Referanslar

- Django kaynak: [`postgresql/operations.py` quote_name](https://github.com/django/django/blob/main/django/db/backends/postgresql/operations.py)
- Django kaynak: [`sqlite3/operations.py` quote_name](https://github.com/django/django/blob/main/django/db/backends/sqlite3/operations.py)
- Django güvenlik politikası: [SQL injection koruması](https://docs.djangoproject.com/en/dev/topics/security/#sql-injection-protection)
- [CWE-89: SQL Injection](https://cwe.mitre.org/data/definitions/89.html)
- PostgreSQL dokümantasyonu: [Identifier quoting kuralları](https://www.postgresql.org/docs/current/sql-syntax-lexical.html#SQL-SYNTAX-IDENTIFIERS)

## İlgili içerik

- [CPython http.server ve wsgiref'te CRLF Injection](/2026/04/24/crlf-injection-cpython-http-server-wsgiref-tr/)
- [CVE olmadan güvenlik fix'i bulmak: bir changelog analyzer](/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer/)
- [DNS rebinding ile SSRF](/2026/03/14/ssrf-dns-rebinding-vulnerability-tr/)
