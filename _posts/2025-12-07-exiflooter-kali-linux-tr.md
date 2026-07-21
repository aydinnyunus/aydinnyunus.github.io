---
layout: post
title: "exifLooter: Fotoğraflardan gizli konum bilgilerini çıkarmak"
date: 2025-12-07
author: Yunus Aydın
lang: tr
description: "exifLooter ile fotoğrafların EXIF metadata'sından GPS koordinatları çıkarma, OpenStreetMap entegrasyonu ve Kali Linux'ta kullanım rehberi."
keywords: "exiflooter, EXIF metadata, GPS coordinates, OSINT, Kali Linux, OpenStreetMap, metadata analysis, security research, bug bounty, geolocation"
canonical_url: "https://aydinnyunus.github.io/2025/12/07/exiflooter-kali-linux-tr/"
---

OSINT araştırmalarında fotoğrafların metadata’sından ne kadar bilgi çıktığını fark ettim. exiftool’u daha kullanışlı hale getiren **exifLooter** adında bir araç yazdım. Fotoğrafların EXIF verisinden GPS koordinatlarını çıkarıyor, OpenStreetMap ile entegre çalışıyor. Araç Kali Linux’un resmi araçları arasında.

## exifLooter nedir?

exifLooter, Go programlama dili ile yazılmış bir OSINT aracı. Temel olarak exiftool'un üzerine kurulu ve özellikle GPS coordinates çıkarma ve OpenStreetMap ile görselleştirme konusunda ek özellikler sunuyor. Araç, exiftool'un güçlü metadata analysis yeteneklerini kullanarak GPS coordinates çıkarıyor ve bunları harita üzerinde görselleştiriyor. 

exifLooter hem tek bir görseli hem de bir dizindeki tüm görselleri toplu analiz edebiliyor. Ayrıca metadata’yı temizleme özelliği var; böylece hem OSINT’te bilgi toplama hem de hassas veriyi silme işini tek araçla yapabiliyorsun.

Araç Kali Linux ve Black Arch Linux’un resmi depolarında; GitHub’da da kullanılıyor.

## Neden exifLooter?

Bug bounty yaparken birçok sitenin yüklenen fotoğraflardan EXIF’i temizlemediğini gördüm; bu da hassas bilgi sızıntısına yol açıyor.

Örneğin profil fotoğrafı yükleyen bir kullanıcının GPS koordinatları ev adresi veya sık gidilen yerleri ele verebiliyor. Bu tür veriler OSINT’te işe yarıyor.

## Özellikler

exifLooter'un temel özellikleri şunlar:

- **Single Image Analysis**: Belirli bir image file'ın EXIF data'sını analiz eder
- **Directory Scanning ve Bulk Analysis**: Bir directory'deki tüm image file'larını (jpeg, png, jpg, jiff) otomatik olarak tarar ve toplu olarak analiz eder. Bu özellik sayesinde yüzlerce fotoğrafı tek seferde analiz edebilirsiniz
- **Metadata Removal**: Fotoğraflardan tüm EXIF metadata'sını güvenli bir şekilde temizleme özelliği. Bu özellik, güvenlik açısından sensitive information'ın sızdırılmasını önlemek için kritik
- **Pipe Desteği**: Diğer araçlarla (waybackurls, subfinder gibi) entegre çalışır
- **OpenStreetMap Integration**: GPS coordinates'ı OpenStreetMap üzerinde görselleştirir

## Kali Linux kurulumu

exifLooter artık Kali Linux'un resmi araçları arasında. Kurulum oldukça basit:

```bash
sudo apt update
sudo apt install exiflooter
```

Kurulumdan sonra `exifLooter` command'ını doğrudan kullanabilirsiniz. Araç, exiftool'a bağımlı olduğu için exiftool'un sisteminizde yüklü olması gerekiyor. Kali Linux'ta genellikle zaten yüklü gelir.

![exifLooter Kali Linux kurulum ekran görüntüsü]({{ site.baseurl }}/assets/images/exiflooter-kali-linux/install.png)

## Kullanım örnekleri

### Tek görsel analizi

En basit kullanım şekli, tek bir image file'ı analiz etmek:

```bash
exifLooter --image image.jpeg
```

Bu command, image file'daki tüm EXIF data'sını gösterir ve eğer GPS coordinates varsa bunları çıkarır.

![exifLooter ile tek bir resim analizi örneği](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-24.png)

![exifLooter ile tek bir resim analizi sonuç örneği](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-25.png)

### Dizin taraması ve toplu analiz

exifLooter'un en güçlü özelliklerinden biri, bir directory'deki tüm image'leri otomatik olarak tarayıp toplu analiz yapabilmesi. Bu özellik sayesinde yüzlerce fotoğrafı tek seferde analiz edebilirsiniz:

```bash
exifLooter -d images/
```

Bu command, belirtilen directory'deki tüm image file'larını (jpeg, png, jpg, jiff) otomatik olarak tarar ve her birinin EXIF data'sını analiz eder. Subdirectory'lerdeki image'leri de tarayabilir ve sonuçları düzenli bir şekilde gösterir.

Directory scanning özelliği özellikle şu senaryolarda çok kullanışlı:
- Bir web sitesinden indirilen tüm profil fotoğraflarını analiz etmek
- Sosyal medya araştırmalarında toplu fotoğraf analizi yapmak
- Bug bounty araştırmalarında yüklenen tüm görselleri taramak


### Pipe ile kullanım

exifLooter'ı diğer OSINT araçlarıyla birlikte kullanabilirsiniz. Örneğin, waybackurls ile birlikte:

```bash
cat subdomains | waybackurls | grep "jpeg\|png\|jiff\|jpg" | exifLooter --pipe
```

Bu command, Wayback Machine'den çekilen URL'lerden image file'larını filtreler ve exifLooter ile analiz eder.

### OpenStreetMap entegrasyonu

GPS coordinates'ı OpenStreetMap üzerinde görselleştirmek için:

```bash
exifLooter --open-street-map --image image.jpeg
```

Bu command, image'deki GPS coordinates'ı OpenStreetMap URL'si olarak gösterir. URL'ye tıklayarak harita üzerinde konumu görebilirsiniz.

![exifLooter OpenStreetMap komut çıktısı](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-31_16-17.png)

![exifLooter OpenStreetMap harita görüntüsü](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-31_16-12.png)

### Metadata temizleme

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

## Gerçek hayat senaryoları

### Bug bounty araştırması

Bir bug bounty programında, bir web sitesinin profil fotoğrafı yükleme özelliğini test ediyordum. Yüklediğim test fotoğrafının metadata'sını kontrol ettim ve GPS coordinates'ın hala mevcut olduğunu gördüm. Bu, bir security vulnerability oluşturuyordu çünkü kullanıcıların sensitive location information'ı sızdırılıyordu.

exifLooter ile bu tür açıkları tespit edebilirsiniz. Bu tür güvenlik açıkları, bug bounty programlarında ödül kazandırabilir. Örneğin, HackerOne'da **$200**, BugCrowd'da ise **$500** gibi ödüller kazanılmış. Bu, metadata güvenliğinin ne kadar önemli olduğunu gösteriyor.

### OSINT araştırması

Bir OSINT araştırmasında, sosyal medyada paylaşılan fotoğrafları analiz ediyordum. exifLooter ile bu fotoğrafların metadata'sını çıkardım ve birçok fotoğrafın GPS coordinates içerdiğini gördüm. Bu bilgiler, hedef kişinin sık kullandığı lokasyonları ve hareket desenlerini ortaya çıkarıyordu.

## İstatistikler ve başarılar

- **GitHub Stars**: 475+ star
- **Kali Linux**: Resmi araç olarak eklendi
- **Black Arch Linux**: Resmi araç olarak eklendi
- **Dil**: Go (100%)
- **Bağımlılık**: exiftool

## Güvenlik açığı raporları

exifLooter ile tespit edilebilecek güvenlik açıklarına örnekler:

![BugCrowd P3 güvenlik açığı raporu](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-29.png)

![BugCrowd P4 güvenlik açığı raporu](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-29_1.png)

- **HackerOne Örneği**: [EXIF Geolocation Data Not Stripped](https://hackerone.com/reports/906907) - $200 ödül
- **BugCrowd Örneği**: [EXIF Geolocation Data Not Stripped from Uploaded Images](https://medium.com/@souravnewatia/exif-geolocation-data-not-stripped-from-uploaded-images-794d20d2fa7d) - $500 ödül

Bu raporlar, metadata güvenliğinin ne kadar kritik olduğunu ve exifLooter'ın bu tür açıkları tespit etmede ne kadar etkili olduğunu gösteriyor.

## Kaynak kod ve katkı

exifLooter açık kaynak bir proje ve GitHub'da mevcut:

- **GitHub Repository**: [https://github.com/aydinnyunus/exifLooter](https://github.com/aydinnyunus/exifLooter)
- **Kali Linux Sayfası**: [https://www.kali.org/tools/exiflooter/](https://www.kali.org/tools/exiflooter/)

Katkı için GitHub’da issue veya pull request açabilirsin; katkılar memnuniyetle karşılanıyor.

## Özet

exifLooter, OSINT ve bug bounty’de metadata analizi için kullanışlı bir araç. Özellikle fotoğraflardan GPS koordinatı çıkarma ve OpenStreetMap ile gösterme özellikleri araştırmacılar için işe yarıyor. Kali’ye eklenmesi topluluk tarafından değer gördüğünü gösteriyor. OSINT veya bug bounty yapıyorsan denemeni öneririm.

Daha fazla güvenlik projesi için [projelerim](/tr/projects/) sayfasına bakabilirsin.

## İlgili içerik

OSINT ve güvenlik araştırmaları için [Instagram dolandırıcılarını hacklemek](/2022/04/11/hacking-instagram-scammers-tr/) yazısında phishing ve OSINT’i anlattım. Daha fazla [güvenlik projesi](/tr/projects/) için projeler sayfasına bakabilirsin.

