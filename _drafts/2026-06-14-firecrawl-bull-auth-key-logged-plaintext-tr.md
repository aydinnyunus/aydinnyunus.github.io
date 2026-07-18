---
layout: post
title: "Firecrawl başlangıçta BULL_AUTH_KEY'i log'a düz metin yazıyordu"
author: Yunus Aydın
date: 2026-06-14
lang: tr
description: "Firecrawl her başlangıçta Bull Board admin secret'ını uygulama log'larına yazıyordu. Log'a erişimi olan herkes queue dashboard'unu açabilirdi. Mart 2025'te raporlandı, üç günde fixlendi."
keywords: "Firecrawl, BULL_AUTH_KEY, Bull Board, log injection, CWE-532, log'da hassas bilgi, queue admin, secret sızıntısı, güvenlik araştırması"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/firecrawl-bull-auth-key-logged-plaintext-tr/"
---

Firecrawl'ın API server'ı her başlangıçta Bull Board admin secret'ını log'larına yazıyordu. 10 Mart 2025'te issue #1321 olarak raporladım. Maintainer üç gün sonra üç satır silerek fixledi. CVE atanmadı. Fix küçüktü. Altta yatan bug class, production secret'larını aynı anda yüzlerce yere sessizce sızdıran cinsten.

## Arka plan

[Firecrawl](https://github.com/firecrawl/firecrawl), Mendable'ın web scraping ve crawling API'si. Scrape job'larını planlamak ve çalıştırmak için [Bull](https://github.com/OptimalBits/bull) (Redis tabanlı bir job queue) kullanıyor; o job'ları inceleyip yönetmek için de [Bull Board](https://github.com/felixmosh/bull-board) ile bir web UI expose ediyor.

Bull Board'da built-in authentication yok. Standart kullanım: tahmin edilmesi zor bir URL prefix'inin arkasına mount edip o prefix'i shared secret olarak değerlendirmek. Firecrawl bu iş için `BULL_AUTH_KEY`'i kullanıyor. Dashboard `/admin/<BULL_AUTH_KEY>/queues` adresinde yaşıyor.

Key'i sızdırırsan dashboard'u da sızdırmış oluyorsun.

## Savunmasız kod

`apps/api/src/index.ts` dosyasının startup bloğunda şu vardı:

```typescript
logger.info(
  `For the Queue UI, open: http://${HOST}:${port}/admin/${process.env.BULL_AUTH_KEY}/queues`,
);
```

Lokal dev kolaylığı: server başlarken tıklanabilir URL bas. Sorun şu ki `logger.info()`, sen localhost'ta çalışan bir developer mısın yoksa SIEM'e log gönderen production container mısın umursamıyor. URL, log'un gittiği her yere gidiyor.

Tipik bir production deployment'ta o "her yer" şunları içeriyor:

- Container stdout (orchestrator yakalıyor: Docker, Kubernetes, ECS)
- Cloud log aggregator'lar (CloudWatch, Stackdriver, Datadog, Loki, Grafana Cloud)
- Log'u downstream'e ileten Filebeat / Fluentd / Vector pipeline'ı
- S3 veya muadilindeki uzun vadeli log arşivleri
- Ops ekibinin startup çıktısını yapıştırdığı support ticket'ları
- Birisi "server ayakta mı?" diye sorduğunda Slack'te paylaşılan screenshot'lar

Her biri secret'ın artık yaşadığı ayrı bir yer; ayrı erişim kontrolüyle, ayrı retention politikasıyla, ayrı audit trail'iyle.

## Root cause

Secret doğrudan log message'a interpolate ediliyor, logger redact etmiyor, environment kontrolü yok. Bu üçü birleşince log her restart'ta her ortamda secret yazıyor.

## Etki

Log'a okuma erişimi olan biri için saldırı tek satır: `grep -oE '/admin/[^/]+/queues'` URL'yi buluyor, `curl` dashboard'u açıyor. Oradan Bull Board scrape hedeflerini, request header'larını, response payload'larını ve job metadata üzerinden geçen API key'lerini gösteriyor. Read-only de değil; retry, delete, re-queue action'ları da expose.

"Log erişimi" kulağa kısıtlı geliyor ama nadiren öyledir. Junior ops, contractor, SaaS log vendor'u ve hayatında bir kez bile debugging session'a eklenmiş herkes genellikle log'lara bakabiliyor. Log'a sızan bir secret'ın blast radius'u, aynı secret'ın bir kod dosyasında olmasından çok daha büyük; çünkü log'lar tasarımı gereği yayılıyor, kod yayılmıyor.

## Proof of concept

Exploit script'ine gerek yok. "PoC" log'u okumak:

```bash
# Firecrawl'ı çalıştıran host'ta veya log'un gönderildiği herhangi bir aggregator'da:
grep -oE '/admin/[^/]+/queues' /var/log/firecrawl/*.log | head -1
# /admin/SuperSecretKey123/queues

# Sonra ziyaret et:
curl https://firecrawl.example.com/admin/SuperSecretKey123/queues
```

Saldırının tamamı bu. Secret startup'ta senin için yazıldı; sadece nereye bakacağını bilmen yeterli.

## Düzeltme

[`f87e1171`](https://github.com/firecrawl/firecrawl/commit/f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2) commit'i, Gergő Móricz tarafından, mesajı `fix: don't log bull secret`, 13 Mart 2025'te merge edildi. Üç satırı siliyor:

```diff
-    logger.info(
-      `For the Queue UI, open: http://${HOST}:${port}/admin/${process.env.BULL_AUTH_KEY}/queues`,
-    );
```

Yerine başka bir şey eklenmemiş. Startup log'u artık queue UI'dan hiç bahsetmiyor. URL'i isteyen developer kendi `.env` dosyasından inşa etsin.

Doğru karar. Bunun "log'la ama maskele" versiyonunun, hiç loglamamaktan iyi bir hali yok. Lokal dev için startup helper'ı istiyorsan `NODE_ENV === "development"` arkasına al; o zaman bile structured logger dışında stdout'a yaz ki aggregator'a hiç ulaşmasın.

## Bu pattern neden tekrar tekrar çıkıyor

Kural: `process.env`'de `_KEY`, `_SECRET`, `_TOKEN`, `_PASSWORD` veya `_AUTH` ile biten her şey secret'tır ve log mesajlarına girmez. Ne URL'de, ne error mesajında, ne de "kolaylık olsun" diye basılan startup banner'larında. Logger'ın destination'ları senin kontrolünde değil.

Aynı kuralın daha katı versiyonu: framework seviyesinde bilinen secret pattern'lerini scrub eden bir logger kullan (örn. Pino'nun `redact` opsiyonu, Winston transport'larındaki redaction middleware). Böylece dikkatsiz bir `logger.info({ config })` çağrısı, tek seferde environment'taki her şeyi sızdırmaz.

## Raporlama zaman çizelgesi

- **10 Mart 2025**: Firecrawl'a [#1321](https://github.com/firecrawl/firecrawl/issues/1321) numaralı issue olarak raporlandı
- **13 Mart 2025**: Maintainer Gergő Móricz tarafından [`f87e1171`](https://github.com/firecrawl/firecrawl/commit/f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2) commit'inde düzeltildi
- CVE atanmadı

## Referanslar

- **CWE-532**: [Insertion of Sensitive Information into Log File](https://cwe.mitre.org/data/definitions/532.html)
- **CWE-200**: [Exposure of Sensitive Information to an Unauthorized Actor](https://cwe.mitre.org/data/definitions/200.html)
- **OWASP**: [Logging Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Logging_Cheat_Sheet.html)
- **Firecrawl issue #1321**: [https://github.com/firecrawl/firecrawl/issues/1321](https://github.com/firecrawl/firecrawl/issues/1321)
- **Fix commit**: [`f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2`](https://github.com/firecrawl/firecrawl/commit/f87e11712c5c5ad937c4ca1abd29a2e8594ff1c2)
- **Bull Board**: [https://github.com/felixmosh/bull-board](https://github.com/felixmosh/bull-board)

## İlgili içerik

- [NLTK collocations'da komut enjeksiyonu](https://aydinnyunus.github.io/2026/06/07/command-injection-nltk-collocations-eval-tr/)
- [Paket Repository Secret Tarama](https://aydinnyunus.github.io/2026/03/07/package-repository-secret-scanning-tr/)
- [CVE olmadan güvenlik fix'lerini bulmak: bir changelog analyzer](https://aydinnyunus.github.io/2026/04/11/finding-security-fixes-without-cve-changelog-analyzer/)
