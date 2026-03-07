---
layout: post
title: "Kripto Para Dolandırıcılarını Tespit Etme: Wallet-Tracker ile Coin Mixing Analizi"
date: 2026-02-21
author: Yunus Aydın
lang: tr
description: "Wallet-Tracker ile kripto para dolandırıcılarını tespit etme ve coin mixing analizi. Neo4j, Redis ve Neodash kullanarak blockchain transaction analizi."
keywords: "wallet tracker, coin scammers, coin mixing, cryptocurrency security, blockchain analysis, Neo4j, transaction tracking, scam detection, crypto fraud"
canonical_url: "https://aydinnyunus.github.io/2026/02/21/identifying-coin-scammers-wallet-tracker-tr/"
---

Online dolandırıcılık artıyor; phishing’den sahte kripto borsalarına kadar kime güveneceğin belli olmuyor. Kripto dolandırıcılarını ve coin mixing’i takip etmek için **Wallet-Tracker**’ı kullanabilirsin. Araç, bir dolandırıcı wallet adresi verdiğinde o adresten ve o adrese gelen transaction’ları tarayıp Neo4j’de saklıyor; böylece şüpheli aktivite ve coin mixing’i analiz etmek kolaylaşıyor.

> **GitHub**: [https://github.com/aydinnyunus/wallet-tracker](https://github.com/aydinnyunus/wallet-tracker)

![Wallet-Tracker](https://miro.medium.com/v2/resize:fit:1400/format:webp/0*LaMJYY9HgTLGcoPN.png)

## Coin mixing nedir?

Coin mixing, kripto paranın kaynağını gizlemek için başka coin’lerle karıştırılması. İz sürmeyi zorlaştırıyor; para aklama veya yasadışı faaliyetlerde kullanılabiliyor. Wallet-Tracker, Neo4j’deki transaction ve wallet verisiyle coin mixing pattern’lerini tespit etmeye yarıyor.

![Coin Mixing Visualization](https://d3i71xaburhd42.cloudfront.net/8f03f545ff791a70455ad4624208357e41dcfc0c/6-Figure2-1.png)

Coin mixing, blockchain'deki transaction'ların izlenebilirliğini azaltmak için kullanılan bir teknik. Saldırganlar, yasadışı yollarla elde ettikleri kripto paraları, birçok farklı wallet arasında transfer ederek karıştırır. Bu sayede, paranın kaynağını bulmak neredeyse imkansız hale gelir.

## Wallet-Tracker ne yapar?

Bir dolandırıcı wallet adresi verdiğinde, o adresten çıkan ve o adrese gelen tüm transaction’ları tarıyor. Veriler Neo4j graph veritabanında tutuluyor; analiz ve görselleştirme buradan yapılıyor.

**Temel özellikler:**

- Transaction tarama ve analiz
- Coin mixing tespiti
- Şüpheli aktivite işaretleme
- Graph veritabanı ile görselleştirme
- Exchange wallet tespiti

## Mimari

Üç bileşen: **Neo4j**, **Redis**, **Neodash**.

### Neo4j

Tüm transaction ve wallet’lar Neo4j’de tutuluyor. Graph yapısı wallet’lar arası ilişkileri ve para akışını göstermek için uygun.

**Neo4j'nin avantajları:**

- **Graph yapısı**: Wallet'lar ve transaction'lar arasındaki ilişkileri doğal olarak temsil eder
- **Hızlı sorgulama**: Karmaşık graph sorguları hızlı bir şekilde çalıştırılabilir
- **Pattern matching**: Coin mixing gibi pattern'leri tespit etmek için güçlü sorgu yetenekleri
- **Görselleştirme**: Transaction akışını görsel olarak analiz etmeyi kolaylaştırır

Neo4j'de her transaction, bir wallet'tan diğerine bir edge (kenar) olarak temsil edilir. Bu yapı, coin mixing pattern'lerini tespit etmek için mükemmeldir.

### Redis

Cache ve hızlı erişim için. Özellikle bilinen exchange wallet’ları key-value olarak tutuluyor; Neo4j sorgularını hızlandırıyor.

**Redis'in kullanım alanları:**

- **Exchange wallet cache**: Bilinen exchange wallet'larını key-value formatında saklar
- **Hızlı erişim**: Sık kullanılan verilere hızlı erişim sağlar
- **Performance**: Neo4j sorgularını hızlandırmak için cache mekanizması

Exchange wallet'ları Redis'te saklanır çünkü bunlar sık sorgulanır ve hızlı erişim gerektirir. Bu sayede, bir wallet'ın bir exchange'e ait olup olmadığını hızlıca kontrol edebilirsin.

### Neodash

Neo4j verisini web arayüzüyle görselleştiriyor. Hazır sorgular ve özel sorgularla transaction akışını ve coin mixing pattern’lerini inceleyebilirsin.

**Neodash özellikleri:**

- **Önceden tanımlı sorgular**: Yaygın analizler için hazır sorgular
- **Özel sorgular**: Kullanıcılar kendi sorgularını ekleyebilir
- **Görselleştirme**: Graph yapısını görsel olarak inceleme
- **Pattern analizi**: Coin mixing ve şüpheli aktivite pattern'lerini görselleştirme

Neodash, Neo4j veritabanındaki verileri web arayüzü üzerinden görselleştirir. Bu sayede, transaction akışını ve wallet'lar arasındaki ilişkileri kolayca analiz edebilirsin.

## Nasıl çalışıyor?

Verdiğin dolandırıcı wallet adresinden (ve o adrese gelenlerden) transaction’ları tarıyor; komşu wallet’ları da ekliyor. Veriler Neo4j’e yazılıyor; coin mixing ve şüpheli pattern’ler bu grafik üzerinden analiz ediliyor.

### 1. Transaction Tarama

Wallet-Tracker, verilen dolandırıcı wallet adresinden ve ona gelen tüm transaction'ları tarar. Ayrıca, bu wallet'ın komşularını (neighbors) da tarar. Komşular, doğrudan transaction yapılan wallet'lardır.

**Tarama süreci:**

- Dolandırıcı wallet adresinden çıkan transaction'lar
- Dolandırıcı wallet adresine gelen transaction'lar
- Komşu wallet'ların transaction'ları
- Derinlemesine analiz için recursive tarama

### 2. Veri Saklama

Taranan tüm transaction'lar ve wallet'lar Neo4j veritabanında saklanır. Her transaction, bir wallet'tan diğerine bir edge olarak temsil edilir. Transaction'ların fiyat bilgileri ve timestamp'leri de saklanır.

**Saklanan veriler:**

- Wallet adresleri (nodes)
- Transaction'lar (edges)
- Transaction fiyatları
- Transaction timestamp'leri
- Exchange wallet bilgileri (Redis'te)

### 3. Coin Mixing Tespiti

Wallet-Tracker, coin mixing'i tespit etmek için transaction pattern'lerini analiz eder. Coin mixing, genellikle şu pattern'leri gösterir:

- **Çoklu wallet transferleri**: Bir wallet'tan birçok farklı wallet'a küçük miktarlarda transfer
- **Dairesel transferler**: Wallet'lar arasında dairesel transfer pattern'leri
- **Anonim wallet'lar**: Exchange'e bağlı olmayan, yeni oluşturulmuş wallet'lar

**Tespit kriterleri:**

- Şüpheli transaction pattern'leri
- Yüksek transaction hacmi
- Kısa süre içinde birçok wallet'a transfer
- Exchange'e giden transaction'ların eksikliği

### 4. Şüpheli Aktivite İşaretleme

Wallet-Tracker, şüpheli aktiviteleri otomatik olarak işaretler. Örneğin:

- Bilinen bir dolandırıcı adresine büyük miktarda kripto para gönderilmesi
- Yeni bir adrese büyük miktarda kripto para gönderilmesi (coin mixing taktiği)
- Exchange wallet'larına giden transaction'lar (exit node tespiti)

![Wallet-Tracker Interface](https://github.com/aydinnyunus/wallet-tracker/raw/main/img/2022-06-23_20-01.png)

## Kullanım Senaryoları

Wallet-Tracker, birçok farklı senaryoda kullanılabilir:

### Dolandırıcı Tespiti

Bir dolandırıcı wallet adresi verildiğinde, Wallet-Tracker o wallet'ın tüm transaction geçmişini analiz eder. Bu sayede:

- Dolandırıcıya para gönderen kurbanları tespit edebilirsin
- Dolandırıcının para akışını takip edebilirsin
- Dolandırıcının kullandığı coin mixing tekniklerini görebilirsin

### Transaction Geçmişi Analizi

Wallet-Tracker, bir wallet'ın tüm transaction geçmişini görselleştirir. Bu sayede:

- Paranın nereye gittiğini görebilirsin
- Hangi wallet'ların birbiriyle ilişkili olduğunu anlayabilirsin
- Transaction pattern'lerini analiz edebilirsin

### Exit Node Tespiti

Exit node'lar, coin mixing sürecinin sonunda kripto paranın çıktığı noktalardır. Genellikle kripto para borsalarıdır. Wallet-Tracker:

- Exchange wallet'larını tespit eder
- Coin mixing sonrası exchange'e giden transaction'ları bulur
- Paranın nihai varış noktasını gösterir

![Neodash Visualization](https://github.com/aydinnyunus/wallet-tracker/raw/main/img/2022-06-23_20-03.png)

### Pattern Analizi

Wallet-Tracker, coin mixing pattern'lerini analiz eder. Örneğin:

- Paranın belirli adreslere daha fazla gönderilip gönderilmediğini tespit eder
- Dairesel transfer pattern'lerini bulur
- Şüpheli wallet gruplarını belirler

## Sonuç

Online dolandırıcılık ve coin mixing teknikleri artıyor. Wallet-Tracker, Wallet-Tracker CLI, Neo4j veritabanı ve kullanıcı tarafından sağlanan dolandırıcı wallet adresi kullanarak tüm transaction'ları ve wallet'ları saklar, şüpheli aktiviteleri tespit etmeyi kolaylaştırır. İster günlük kripto para kullanıcısı ol, ister ciddi bir yatırımcı, Wallet-Tracker güvende kalmana yardımcı olabilir.

**Ana çıkarımlar:**

- Coin mixing, kripto paranın kökenini gizlemek için kullanılan bir tekniktir
- Wallet-Tracker, transaction'ları analiz ederek coin mixing'i tespit edebilir
- Neo4j graph veritabanı, transaction akışını görselleştirmek için idealdir
- Redis, exchange wallet'larını hızlı erişim için cache'ler
- Neodash, verileri görselleştirmek için web arayüzü sağlar

Blockchain analizi kripto para güvenliği için önemli. Wallet-Tracker gibi araçlar, dolandırıcıları tespit etmek ve coin mixing aktivitelerini analiz etmek için yardımcı oluyor.

> **Wallet-Tracker'ı kullanmak için**: [https://github.com/aydinnyunus/wallet-tracker](https://github.com/aydinnyunus/wallet-tracker)

## İlgili İçerik

- [Hacking Instagram Scammers](/2022/04/11/hacking-instagram-scammers-tr/)
- [PassDetective: Shell Geçmişinizdeki Şifre ve Secret'ları Tespit Etme](/2025/12/14/passdetective-kali-linux-tr/)
