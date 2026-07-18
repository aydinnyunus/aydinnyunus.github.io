---
layout: post
title: "PyPI, npm, RubyGems'de Sızan Secret Taraması: Microsoft Leak"
lang: tr
author: Yunus Aydın
description: "PyPI, npm, RubyGems ve NuGet paketlerinde sızan AWS token'ları ve private key'leri taradım. Microsoft, Automattic, Palo Alto ve daha fazlasında sızıntı buldum."
keywords: "PyPI secret tarama, npm package sızıntı, AWS token sızıntısı, private key tarama, tedarik zinciri güvenlik taraması, GitLeaks paket tarama, Microsoft secret leak, açık kaynak güvenlik"
canonical_url: "https://aydinnyunus.github.io/2026/03/07/package-repository-secret-scanning-tr/"
---

PyPI, npm, NuGet, RubyGems… Geliştiricilerin projelerine taşıdığı paketlerin çoğu buradan geliyor. Bu paketlerin içinde bazen **secret** da sızıyor: API anahtarları, private key'ler, token'lar. Bir kere yayına çıktı mı veri ihlali ve yetkisiz erişim riski artıyor. Bu yazıda bu depoları nasıl taradığımı, ne bulduğumu ve raporlama sürecini anlatıyorum.

## Paket depolarında secret taraması neden önemli?

Paket depoları açık kaynak kütüphanelerin ana kaynağı. Projeye paket ararken ilk bakılan yer burası. Ama paketler zafiyetsiz değil; sızan secret'lar ciddi risk. Bir AWS anahtarı, bir private key pakete girmişse, yetkisiz erişim ve veri sızıntısı kapıda.

### Secret derken ne kastediyoruz?

**API key**, **auth token**, **şifre**, **encryption key** gibi hassas bilgiler. Bunların paket içinde veya public repo'da görünmesi istenmez; bir kere sızdı mı sonuçları ağır olabiliyor.

**Ne tür secret'lar çıkıyor:**

- AWS access token
- Private key
- Veritabanı şifreleri
- API key
- JWT token
- Webhook URL

### Sızan secret'lar neye yol açar?

Pakete yanlışlıkla secret girmesi, kötü niyetli biri için açık kapı bırakır. Örneğin **AWS** access key ele geçerse yetkisiz erişim, veri ihlali, mali kayıp ve operasyonel sorunlar çıkabilir.

**Olası etkiler:**

- Veri ihlali
- Mali kayıp
- Servis kesintisi
- Phishing
- Dolandırıcılık

## Nasıl taradım? Üç EC2 ile

Her büyük paket deposu için ayrı **EC2** kullandım: **PyPI**, **npm**, **RubyGems**, **NuGet**. Her biri kendi deposundaki güncel paketleri indirip GitLeaks ile tarıyor.

### PyPI (Python Package Index)

PyPI’ye özel EC2, en son indirilen paketleri alıyor, içeriği açıyor ve **GitLeaks** ile secret taraması yapıyor. PyPI Python ekosisteminin merkezi; buradaki sızıntılar doğrudan projelere yansıyor.

![PyPI Secret Scanning](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*cMVRDy-gmpC5Whaqs1aB-A.png)

**PyPI tarama akışı:**

1. Son paket indirmelerini al
2. Paket içeriğini çıkar
3. GitLeaks ile tara
4. Bulguları raporla

### npm (Node Package Manager)

Node.js tarafı için ayrı bir EC2: npm registry’den güncel paketleri indiriyor, çıkarıyor ve yine GitLeaks ile tarıyor. npm JavaScript dünyasının bel kemiği; burada da sızıntı çok.

![npm Secret Scanning](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*Qj1OTPHk-djj2C1Nnkn4VQ.png)

**npm tarama akışı:**

1. npm registry’den son paketleri indir
2. İçeriği analiz et
3. GitLeaks ile secret tespiti
4. Sonuçları kaydet

### RubyGems ve NuGet: tek makinede ikisi

Üçüncü EC2 hem **RubyGems** hem **NuGet** için çalışıyor. İki depodan da güncel paketleri alıp GitLeaks ile tarıyor; Ruby ve .NET tarafındaki sızıntıları raporluyor.

![RubyGems and NuGet Secret Scanning](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*VdrKUPx09FaAltG8A5uN7w.png)

**RubyGems ve NuGet tarama akışı:**

1. Her iki depodan paket indir
2. İçeriği çıkar
3. GitLeaks taraması
4. Secret’ları raporla

## Otomasyon: disk yönetimi

Çok sayıda paket indirip tararken disk dolabiliyor. Yeterli alan kalmazsa süreç yarıda kalıyor. Bunu engellemek için hem taramayı hem disk kontrolünü yapan bir script yazdım.

### Script’e kısa bakış

`free.sh` kabaca şunu yapıyor: diskte belirli bir eşikten (ör. 2 GB) az alan kaldığında önce GitLeaks taramasını çalıştırıp çıktıyı kaydediyor, sonra indirilen paketleri ve geçici dizinleri siliyor. Böylece sürekli tarama yaparken disk taşmıyor.

```bash
#!/bin/bash

# Define the threshold for available disk space in GB
threshold=2

# Check available disk space in GB
available_space=$(df -h / | awk 'NR==2 { print $4 }' | sed 's/G//')

# Convert the available space to a numeric value
available_space_numeric=$(echo $available_space | sed 's/,//')

# Compare available space with the threshold
if [ "$available_space_numeric" -lt "$threshold" ]; then
    # Run gitleaks and write the output to a temporary file
    tmp_file=$(mktemp)
    echo $tmp_file | notify
    gitleaks detect --no-git -v downloaded_packages/ --config ~/config.toml -r=$tmp_file

    # Check if the downloaded_packages directory exists
    if [ -d "downloaded_packages" ]; then
        # Delete the downloaded_packages directory
        rm -rf downloaded_packages
        rm -rf .npm
    fi
else
    echo "Available disk space is greater than or equal to 2GB."
fi
```

**Özet:**

- Eşik: 2 GB (ayarlanabilir)
- Eşik altına inince: GitLeaks çalışıyor, çıktı kaydediliyor, `downloaded_packages` ve `.npm` siliniyor
- Böylece disk dolmadan tarama döngüsü devam ediyor

## PackageSpy: açık kaynak secret tarama aracı

**PackageSpy**, paket depolarında secret, kullanıcı tanımlı anahtar kelime ve pattern aramak için yazdığım açık kaynak bir araç. Kendi projelerinizi veya belirli depoları taramak için kullanabilirsiniz.

**Özellikler:**

- **Birden fazla paket yöneticisi**: npm, PyPI, RubyGems ve daha fazlası
- **Özelleştirilebilir kurallar**: Kendi keyword ve pattern’lerinizi tanımlayabilirsiniz
- **CLI**: Komut satırından kolayca tetiklenir, CI/CD’ye eklenebilir
- **Raporlar**: Bulunan secret’lar, konumları ve kısa açıklamalarla raporlanır

> **GitHub**: [https://github.com/aydinnyunus/PackageSpy](https://github.com/aydinnyunus/PackageSpy)

## Tarama sonuçları: ne çıktı?

### npm’de ne buldum?

![npm Secret Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*wgbGyS0XCVxGR848pkE88w.png)

**Node.js** kullananlar **npm**’e aşina. Taramada npm paketlerinde en çok şunlar çıktı:

**npm’de en sık görülen secret’lar:**

- **AWS access token (%34,3)**: En yaygın tür. Yanlış ellere geçerse AWS hesabına yetkisiz erişim mümkün.
- **HashiCorp Terraform şifreleri (%20,6)**: Altyapıyı kodla yöneten Terraform; bu şifreler altyapıyı değiştirmeye imkân verir.
- **Private key (%12,2)**: Ciddi risk. Kriptografik iletişimde kullanılıyor.
- **Stripe access token (%7,2)**: Doğrudan finansal risk; ödeme işlemleri yapılabiliyor.
- **Slack webhook URL (%25,7)**: Mesaj göndermek için kullanılabiliyor; phishing için de kullanılabilir.
- **Telegram Bot API token (%14,3)**: Bot hesaplarını ele geçirme riski.

### PyPI’de ne buldum?

![PyPI Secret Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*vhk8EDq5WmRg5d0ILWYgew.png)

Python geliştiricileri **PyPI**’ye güveniyor; ama taramada PyPI paketlerinde de ciddi oranda sızıntı çıktı.

**PyPI’de en sık görülen secret’lar:**

- **AWS access token (%54,6)**: Açık ara en fazla görülen. Farklı seviyelerde AWS erişimi veriyor.
- **HashiCorp Terraform şifreleri (%20,8)**: İkinci sırada.
- **Private key (%10,4)**: Güvenli iletişimde kullanılan anahtarlar.
- **JWT (%9,6)**: Kimlik doğrulama ve bilgi taşımak için kullanılıyor; sızınca risk büyük.
- **Diğerleri**: Etsy token, Slack webhook, Telegram Bot API key vb. daha düşük oranlarda.

![PyPI Detailed Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*fCDu80WE8FrFatJzMWf2-g.png)

### RubyGems’te ne buldum?

![RubyGems Secret Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*B8ER6VFc7if0JaaM0NpgAQ.png)

**RubyGems**, Ruby paketlerinin merkezi. Orada da sızan secret oranı yüksek çıktı.

**RubyGems’te en sık görülen secret’lar:**

- **AWS access token (%66,5)**: En büyük pay. Diğer depolara göre oran burada daha yüksek.
- **HashiCorp Terraform şifreleri (%15,8)**: Altyapıyı kod olarak yöneten Terraform şifreleri.
- **Private key (%9,3)**: Şifreleme ve güvenli iletişimde kritik.
- **JWT (%6,8)**: Kimlik doğrulama için yaygın.
- **Stripe access token (%1,7)**: Finansal risk.
- **Slack webhook, OpenAI API key** vb. daha az oranda ama yine de görülüyor.

![RubyGems Detailed Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*nuIG50RYBF8INuFOnvZBig.png)

## Bulguları raporlama

Tarama sonucu ortaya çıkan kritik sızıntıları ilgili şirketlere ve proje sahiplerine bildirdim. Etkilenen servislerin sahibi veya yöneticisi olan firmalara ulaşmak için paket metadata’sı ve public iletişim kanallarını kullandım.

### İletişime nasıl geçtim?

**npm, PyPI, RubyGems, NuGet** gibi depolardaki paket bilgilerinden (package.json, METADATA, gemspec, nuspec) ve proje dokümantasyonundan iletişim bilgilerini topladım. Gerekirse mailing list, forum ve paket yöneticisinin kendi mesaj sistemlerini kullandım.

**Raporlama adımları:**

1. Paket metadata’sında (package.json, METADATA, gemspec, nuspec) iletişim bilgisine bak
2. Proje / repo ve dokümantasyonda maintainer bilgisi ara
3. Mailing list, forum, topluluk kanallarını kontrol et
4. npm owner, PyPI maintainer mesajı gibi kanalları kullan

### Raporladığım yerler

İletişim bilgilerini bulduğum şirket ve organizasyonlara bulguları ilettim. Raporladığım taraflar arasında Microsoft, Automattic, Mapbox, Keeper Security, Pulumi, Weblate, Palo Alto Networks, Telefonica ve indirme sayısı yüksek birkaç özel proje var (toplamda 7,5M+ indirme).

**Kullandığım kanallar:**

- E-posta ile detaylı rapor
- HackerOne / Bugcrowd gibi platformlar (şirket programa dahilse)
- Şirketlerin güvenlik açıklama (disclosure) politikalarına uyum
- Güvenlik ekipleriyle takip ve iletişim

## Özet

Paket depolarında secret taraması, uygulama güvenliği için önemli bir pratik. Ben üç EC2 ile PyPI, npm, RubyGems ve NuGet’i taradım; GitLeaks ve kendi yazdığım PackageSpy ile sonuçları topladım. AWS token’lar her depoda en sık görülen sızıntı türü. Taramayı sürekli ve ölçekli yapmak için otomasyon ve disk yönetimi şart. Bulduğunuz sızıntıları ilgili taraflara sorumlu şekilde raporlamak da ekosistemi güçlendiriyor.

**Kısa çıkarımlar:**

- Paket depolarında secret taraması ciddiye alınması gereken bir güvenlik adımı
- En yaygın sızıntı türü AWS token’ları
- Sürekli tarama için otomasyon ve disk yönetimi gerekli
- PackageSpy gibi araçlar kendi taramalarınızı yapmanızı kolaylaştırır
- Sorumlu açıklama (responsible disclosure) güvenlik ekosistemine katkı sağlar

## İlgili içerik

- [PassDetective: Shell geçmişinizdeki şifre ve secret'ları tespit etme](/2025/12/14/passdetective-kali-linux-tr/)
- [exifLooter: Görsellerden gizli konum bilgisi çıkarma](/2025/12/07/exiflooter-kali-linux-tr/)
