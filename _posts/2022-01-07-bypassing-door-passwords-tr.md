---
layout: post
title: "Kapı Şifrelerini Atlama"
date: 2022-01-07
author: Yunus Aydın
lang: tr
description: "Kapı şifreli kilitlerini atlama konusunda güvenlik araştırması. Varsayılan şifreleri, tuş takımı kapı kilitlerindeki güvenlik açıklarını ve Audio akıllı kilitler ve Türkiye'deki diğer popüler giriş kapısı tuş takımlarının gerçek dünya güvenlik testlerini keşfedin."
keywords: "kapı şifresi atlama, tuş takımı kilit güvenliği, varsayılan şifreler, kapı kilidi güvenlik açıkları, akıllı kilit güvenliği, Audio kapı kilidi, güvenlik araştırması, fiziksel güvenlik"
canonical_url: "https://aydinnyunus.github.io/2022/01/07/bypassing-door-passwords-tr/"
image: "https://miro.medium.com/v2/resize:fit:1000/format:webp/0*XEZo3-Sq09f2CK-O"
---

Anahtar yerine, bu tür kilit sistemi bir tesise veya mülke giriş izni vermek için sayısal bir kod gerektirir. Kod, kullanıcılar tarafından temel bir hesap makinesindekilere benzer sayısal bir tuş takımı üzerinden girilir. Doğru kod girilirse, kapı kilidi veya ölü kilit açılmalıdır. Bazı mekanizmalar kilidi açmak için pil veya küçük bir elektrik akımı gerektirir.

Bazı tuş takımı kilitleri, kodu girmek için birkaç yanlış denemeden sonra kapıyı belirli bir süre (genellikle 10 ila 15 dakika) kilitli tutan entegre bir güvenlik özelliğine sahiptir.

Bu araştırmanın amacı, ne kadar güvensiz yaşadığımızı anlamaktır.

![Audio Smart Lock tuş takımı giriş sistemini gösteriyor](https://miro.medium.com/v2/resize:fit:1000/format:webp/0*XEZo3-Sq09f2CK-O)

## Audio Smart Lock

Türkiye'de En Popüler Giriş Kapısı Tuş Takımları

- Perkotek
- ERD-1120
- Efes Digital Panel
- Mas
- AC 13PX
- Burg Wachter
- DIP40
- Lorex LR-DPH2
- M100
- MB05–03
- MB DYF40
- MLŞ 14–70
- MLŞ 14–107
- MRA 101
- Netalsan Obsidian
- ONDRIVE ED07
- OP705
- OP M400
- OP M500
- Pratik Kart
- Desi Steely
- Audio
- Teknoline
- Teknoline IMR18
- Desi UTOPIC
- WL02
- D45
- A20 Kapı Kilidi

## En Çok Kullanılan Varsayılan Şifreler

- #0000
- 0000
- 0411#
- 0571
- #0789
- 0880#
- 1014
- 1111
- 1200#
- #1234
- **1234
- 1234
- 1234#
- 1357#
- 1453
- 1629#
- 1881
- 1979/
- *1992#
- 2000
- 2013
- 2020
- 2020#
- **2510
- 2707#
- 2828
- 3263
- 4050#
- 4233#
- 4570#
- 5555
- 5656#
- 5689#
- 5757
- 6161
- **6565
- **6771
- 7305
- **7788
- #7889
- **7890
- 8182#
- #8888#
- 88888888
- 8988#
- 9575
- 991453
- 991903
- 992211
- 992525
- 998877
- C12341234
- c2638
- C6161

## Gerçek Hayat Testleri

"Audio Şifreli Kapı Kiliti"nde yönetici şifresini deniyorum ve başarılı. Artık tüm kullanıcıların şifrelerini değiştirebilir, alarmı kapatabilir, tüm kullanıcıları silebiliriz.

Ayrıca, şifreleri bilmemize gerek yok, bazı basit adımlar kullanarak yönetici şifresini sıfırlayabiliriz:

1. Kutu arkasındaki sıfırlama düğmesine 10 saniye basın.
2. DT-8 soketini çıkarın ve 5 saniye bekleyin ve takın.
3. 10 saniye geçtikten sonra, düğmeye basmayı durdurun ve aynı anda soketi tekrar çıkarın.
4. Bundan sonra, varsayılan yönetici şifresini kullanabilirsiniz :)

![Kapı kilidi şifresi atlamayı gösteren gerçek hayat güvenlik testi](https://miro.medium.com/v2/resize:fit:1400/format:webp/0*mGGgCG6CaxKqC6WJ)

## İstatistikler

Şu ana kadar 3 Audio Kapı Kilidi denedim ve hepsinde varsayılan şifreler var (yönetici şifresi dahil). İstatistiklere katkıda bulunabilirsiniz.

![Kapı kilitlerinde varsayılan şifre kullanımını gösteren istatistikler](https://miro.medium.com/v2/resize:fit:1122/format:webp/0*v4sneCtbZPjIdYVD)

## Varsayılan Şifreler için Github Deposu

Model hakkında bilgi almak için ID'yi girin. gateCracker aracı ile REST API kullanın.

![Varsayılan şifre güvenlik açıkları olan kapı kilidi modellerinin listesi](https://miro.medium.com/v2/resize:fit:1140/format:webp/0*aRFq5D78zydmjtjZ)

## Kaynak Kodu

- [https://github.com/aydinnyunus/gateCracker-REST](https://github.com/aydinnyunus/gateCracker-REST) - REST API
- [https://github.com/aydinnyunus/gateCracker](https://github.com/aydinnyunus/gateCracker) - Ana araç (REST API'ye bağlanır)

