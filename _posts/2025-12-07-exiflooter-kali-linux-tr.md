---
layout: post
title: "exifLooter: Fotoğraflardan Gizli Konum Bilgilerini Çıkarmak"
date: 2025-12-07
author: Yunus Aydın
lang: tr
description: "exifLooter aracı ile fotoğrafların EXIF metadata'sından GPS coordinates çıkarma, OpenStreetMap entegrasyonu ve Kali Linux'a eklenen OSINT aracının kullanımı hakkında detaylı rehber."
keywords: "exiflooter, EXIF metadata, GPS coordinates, OSINT, Kali Linux, OpenStreetMap, metadata analysis, security research, bug bounty, geolocation"
canonical_url: "https://aydinnyunus.github.io/2025/12/07/exiflooter-kali-linux-tr/"
---

OSINT araştırmalarında exiftool kullanırken, GPS coordinates'ı daha kolay çıkarabilmek ve OpenStreetMap ile görselleştirebilmek için exifLooter adında bir araç geliştirdim. exifLooter, exiftool'un geliştirilmiş bir versiyonu olarak, özellikle GPS coordinates çıkarma ve harita üzerinde görselleştirme konusunda daha pratik bir çözüm sunuyor. Üstelik artık Kali Linux'un resmi araçları arasında yer alıyor.

## exifLooter Nedir?

exifLooter, Go programlama dili ile yazılmış bir OSINT aracı. Temel olarak exiftool'un üzerine kurulu ve özellikle GPS coordinates çıkarma ve OpenStreetMap ile görselleştirme konusunda ek özellikler sunuyor. Araç, exiftool'un güçlü metadata analysis yeteneklerini kullanarak GPS coordinates çıkarıyor ve bunları harita üzerinde görselleştiriyor. 

exifLooter'un en önemli özelliklerinden biri, hem tekil image file'larını hem de belirli bir directory'deki tüm image'leri toplu olarak analiz edebilmesi. Ayrıca, güvenlik açısından kritik olan metadata removal özelliği de sunuyor. Bu sayede hem OSINT araştırmalarında bilgi toplama hem de güvenlik açısından metadata'yı temizleme işlemlerini tek bir araçla yapabiliyorsunuz.

Araç şu ana kadar GitHub'da **475 star** topladı ve güvenlik topluluğu tarafından yaygın olarak kullanılıyor. Ayrıca Kali Linux ve Black Arch Linux'un resmi araçları arasında yer alıyor.

## Neden exifLooter?

Bug bounty araştırmaları yaparken, web sitelerinde yüklenen fotoğrafların metadata'sından hassas bilgiler çıkarılabileceğini fark ettim. Birçok web sitesi, kullanıcıların yüklediği fotoğraflardan EXIF verilerini temizlemiyor. Bu da güvenlik açığı oluşturuyor.

Örneğin, bir kullanıcı profil fotoğrafı yüklediğinde, fotoğrafın içindeki GPS coordinates kullanıcının ev adresini veya sık kullandığı lokasyonları ortaya çıkarabilir. Bu tür bilgiler OSINT araştırmalarında çok değerli olabiliyor.

## Özellikler

exifLooter'un temel özellikleri şunlar:

- **Single Image Analysis**: Belirli bir image file'ın EXIF data'sını analiz eder
- **Directory Scanning ve Bulk Analysis**: Bir directory'deki tüm image file'larını (jpeg, png, jpg, jiff) otomatik olarak tarar ve toplu olarak analiz eder. Bu özellik sayesinde yüzlerce fotoğrafı tek seferde analiz edebilirsiniz
- **Metadata Removal**: Fotoğraflardan tüm EXIF metadata'sını güvenli bir şekilde temizleme özelliği. Bu özellik, güvenlik açısından sensitive information'ın sızdırılmasını önlemek için kritik
- **Pipe Desteği**: Diğer araçlarla (waybackurls, subfinder gibi) entegre çalışır
- **OpenStreetMap Integration**: GPS coordinates'ı OpenStreetMap üzerinde görselleştirir

## Kali Linux Kurulumu

exifLooter artık Kali Linux'un resmi araçları arasında. Kurulum oldukça basit:

```bash
sudo apt update
sudo apt install exiflooter
```

Kurulumdan sonra `exifLooter` command'ını doğrudan kullanabilirsiniz. Araç, exiftool'a bağımlı olduğu için exiftool'un sisteminizde yüklü olması gerekiyor. Kali Linux'ta genellikle zaten yüklü gelir.

![exifLooter Kali Linux kurulum ekran görüntüsü](https://private-user-images.githubusercontent.com/52822869/289357595-20cfce7d-13be-40d3-a10b-cd7f49a547e6.png?jwt=eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3NjUwNTU4MzYsIm5iZiI6MTc2NTA1NTUzNiwicGF0aCI6Ii81MjgyMjg2OS8yODkzNTc1OTUtMjBjZmNlN2QtMTNiZS00MGQzLWExMGItY2Q3ZjQ5YTU0N2U2LnBuZz9YLUFtei1BbGdvcml0aG09QVdTNC1ITUFDLVNIQTI1NiZYLUFtei1DcmVkZW50aWFsPUFLSUFWQ09EWUxTQTUzUFFLNFpBJTJGMjAyNTEyMDYlMkZ1cy1lYXN0LTElMkZzMyUyRmF3czRfcmVxdWVzdCZYLUFtei1EYXRlPTIwMjUxMjA2VDIxMTIxNlomWC1BbXotRXhwaXJlcz0zMDAmWC1BbXotU2lnbmF0dXJlPWI2ZDE5MDgxNmQ4M2IxYzhjMjU2NGYwMmExYjA0YmIzNWM2MjBmYzU0NTVhZmE2MWEyODFlNGYwMzEzOWMxYjImWC1BbXotU2lnbmVkSGVhZGVycz1ob3N0In0.4FNIQIk1CsNDhMPTbTAED85jHZSOILjbNz2AHo-OIBE)

## Kullanım Örnekleri

### Single Image Analysis

En basit kullanım şekli, tek bir image file'ı analiz etmek:

```bash
exifLooter --image image.jpeg
```

Bu command, image file'daki tüm EXIF data'sını gösterir ve eğer GPS coordinates varsa bunları çıkarır.

![exifLooter ile tek bir resim analizi örneği](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-24.png)

![exifLooter ile tek bir resim analizi sonuç örneği](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-25.png)

### Directory Scanning ve Bulk Analysis

exifLooter'un en güçlü özelliklerinden biri, bir directory'deki tüm image'leri otomatik olarak tarayıp toplu analiz yapabilmesi. Bu özellik sayesinde yüzlerce fotoğrafı tek seferde analiz edebilirsiniz:

```bash
exifLooter -d images/
```

Bu command, belirtilen directory'deki tüm image file'larını (jpeg, png, jpg, jiff) otomatik olarak tarar ve her birinin EXIF data'sını analiz eder. Subdirectory'lerdeki image'leri de tarayabilir ve sonuçları düzenli bir şekilde gösterir.

Directory scanning özelliği özellikle şu senaryolarda çok kullanışlı:
- Bir web sitesinden indirilen tüm profil fotoğraflarını analiz etmek
- Sosyal medya araştırmalarında toplu fotoğraf analizi yapmak
- Bug bounty araştırmalarında yüklenen tüm görselleri taramak


### Pipe ile Kullanım

exifLooter'ı diğer OSINT araçlarıyla birlikte kullanabilirsiniz. Örneğin, waybackurls ile birlikte:

```bash
cat subdomains | waybackurls | grep "jpeg\|png\|jiff\|jpg" | exifLooter --pipe
```

Bu command, Wayback Machine'den çekilen URL'lerden image file'larını filtreler ve exifLooter ile analiz eder.

### OpenStreetMap Integration

GPS coordinates'ı OpenStreetMap üzerinde görselleştirmek için:

```bash
exifLooter --open-street-map --image image.jpeg
```

Bu command, image'deki GPS coordinates'ı OpenStreetMap URL'si olarak gösterir. URL'ye tıklayarak harita üzerinde konumu görebilirsiniz.

![exifLooter OpenStreetMap komut çıktısı](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-31_16-17.png)

![exifLooter OpenStreetMap harita görüntüsü](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-31_16-12.png)

### Metadata Removal

exifLooter'un güvenlik açısından en kritik özelliklerinden biri, fotoğraflardan metadata'yı temizleme yeteneği. Bu özellik sayesinde, sensitive information'ın (GPS coordinates, camera info, capture date vb.) sızdırılmasını önleyebilirsiniz.

Single image'den metadata temizlemek için:

```bash
exifLooter --remove --image image.jpeg
```

Bir directory'deki tüm image'lerden metadata temizlemek için:

```bash
exifLooter --remove -d images/
```

Bu command'lar, image file'lardan tüm EXIF data'sını güvenli bir şekilde temizler. Özellikle web sitelerine yüklenen fotoğraflar için bu özellik kritik öneme sahip. Birçok web sitesi, kullanıcıların yüklediği fotoğraflardan metadata'yı temizlemediği için security vulnerability oluşuyor.

Metadata temizleme özelliği şu durumlarda kullanılabilir:
- Web sitelerine yüklenmeden önce fotoğrafları temizlemek
- Güvenlik açısından hassas bilgilerin sızdırılmasını önlemek
- Privacy-by-design yaklaşımı ile kullanıcı verilerini korumak

## Gerçek Hayat Senaryoları

### Bug Bounty Araştırması

Bir bug bounty programında, bir web sitesinin profil fotoğrafı yükleme özelliğini test ediyordum. Yüklediğim test fotoğrafının metadata'sını kontrol ettim ve GPS coordinates'ın hala mevcut olduğunu gördüm. Bu, bir security vulnerability oluşturuyordu çünkü kullanıcıların sensitive location information'ı sızdırılıyordu.

exifLooter ile bu tür açıkları tespit edebilirsiniz. Bu tür güvenlik açıkları, bug bounty programlarında ödül kazandırabilir. Örneğin, HackerOne'da **$200**, BugCrowd'da ise **$500** gibi ödüller kazanılmış. Bu, metadata güvenliğinin ne kadar önemli olduğunu gösteriyor.

### OSINT Araştırması

Bir OSINT araştırmasında, sosyal medyada paylaşılan fotoğrafları analiz ediyordum. exifLooter ile bu fotoğrafların metadata'sını çıkardım ve birçok fotoğrafın GPS coordinates içerdiğini gördüm. Bu bilgiler, hedef kişinin sık kullandığı lokasyonları ve hareket desenlerini ortaya çıkarıyordu.

## İstatistikler ve Başarılar

- **GitHub Stars**: 475+ star
- **Kali Linux**: Resmi araç olarak eklendi
- **Black Arch Linux**: Resmi araç olarak eklendi
- **Dil**: Go (100%)
- **Bağımlılık**: exiftool

## Güvenlik Açığı Raporları

exifLooter ile tespit edilebilecek güvenlik açıklarına örnekler:

![BugCrowd P3 güvenlik açığı raporu](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-29.png)

![BugCrowd P4 güvenlik açığı raporu](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-29_1.png)

- **HackerOne Örneği**: [EXIF Geolocation Data Not Stripped](https://hackerone.com/reports/906907) - $200 ödül
- **BugCrowd Örneği**: [EXIF Geolocation Data Not Stripped from Uploaded Images](https://medium.com/@souravnewatia/exif-geolocation-data-not-stripped-from-uploaded-images-794d20d2fa7d) - $500 ödül

Bu raporlar, metadata güvenliğinin ne kadar kritik olduğunu ve exifLooter'ın bu tür açıkları tespit etmede ne kadar etkili olduğunu gösteriyor.

## Kaynak Kod ve Katkıda Bulunma

exifLooter açık kaynak bir proje ve GitHub'da mevcut:

- **GitHub Repository**: [https://github.com/aydinnyunus/exifLooter](https://github.com/aydinnyunus/exifLooter)
- **Kali Linux Sayfası**: [https://www.kali.org/tools/exiflooter/](https://www.kali.org/tools/exiflooter/)

Projeye katkıda bulunmak isterseniz, GitHub'da issue açabilir veya pull request gönderebilirsiniz. Tüm katkılar memnuniyetle karşılanıyor.

## Sonuç

exifLooter, OSINT araştırmalarında ve bug bounty programlarında metadata analysis için güçlü bir araç. Özellikle fotoğraflardan GPS coordinates çıkarma ve OpenStreetMap ile görselleştirme özellikleri, araştırmacılar için çok değerli.

Kali Linux'a eklenmesi, aracın güvenlik topluluğu tarafından ne kadar değerli görüldüğünü gösteriyor. Eğer OSINT araştırmaları yapıyorsanız veya bug bounty programlarında çalışıyorsanız, exifLooter'ı mutlaka denemenizi öneriyorum.

Siz de test edebilir ve kendi araştırmalarınızda kullanabilirsiniz. Daha fazla güvenlik araştırması ve projeler için [projelerim](/tr/projects/) sayfasını ziyaret edebilirsiniz.

Bu yazıyı sosyal medyada paylaşarak daha fazla kişinin metadata güvenliği konusunda bilinçlenmesine yardımcı olabilirsiniz.

## İlgili İçerikler

Bu araç gibi, OSINT ve güvenlik araştırmaları hakkında daha fazla içerik için [Instagram Dolandırıcılarını Hacklemek](/2022/04/11/hacking-instagram-scammers-tr/) yazımı inceleyebilirsiniz. Bu yazıda phishing saldırıları ve OSINT tekniklerini araştırdım. Ayrıca daha fazla [güvenlik projelerim](/tr/projects/) ve araçlarımı keşfedebilirsiniz.

