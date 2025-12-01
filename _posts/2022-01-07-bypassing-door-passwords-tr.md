---
layout: post
title: "Kapı Şifrelerini Atlama"
date: 2022-01-07
author: Yunus Aydın
lang: tr
description: "Kapı şifreli kilitlerini atlama üzerine yaptığım güvenlik araştırması. Varsayılan şifreler, tuş takımı kapı kilitlerindeki güvenlik açıkları ve Audio akıllı kilitler ile Türkiye'deki diğer popüler giriş kapısı tuş takımlarının gerçek hayat güvenlik testleri."
keywords: "kapı şifresi atlama, tuş takımı kilit güvenliği, varsayılan şifreler, kapı kilidi güvenlik açıkları, akıllı kilit güvenliği, Audio kapı kilidi, güvenlik araştırması, fiziksel güvenlik"
canonical_url: "https://aydinnyunus.github.io/2022/01/07/bypassing-door-passwords-tr/"
image: "https://miro.medium.com/v2/resize:fit:1000/format:webp/0*XEZo3-Sq09f2CK-O"
---

Anahtar yerine, bu tür kilit sistemleri bir yere giriş için sayısal bir kod istiyor. Kodu, hesap makinesine benzer bir tuş takımından giriyorsunuz. Doğru kodu girerseniz kapı açılıyor. Bazıları kilidi açmak için pil veya küçük bir elektrik akımı kullanıyor.

Bazı tuş takımı kilitleri, birkaç yanlış denemeden sonra kapıyı belirli bir süre (genellikle 10-15 dakika) kilitli tutuyor. Bu bir güvenlik özelliği.

Bu araştırmayı yapmamın amacı, aslında ne kadar güvensiz yaşadığımızı göstermek.

![Audio Smart Lock tuş takımı giriş sistemini gösteriyor](https://miro.medium.com/v2/resize:fit:1000/format:webp/0*XEZo3-Sq09f2CK-O)

## Audio Smart Lock

Türkiye'de en popüler giriş kapısı tuş takımları:

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

## En Çok Kullanılan Varsayılan Şifreler:

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

"Audio Şifreli Kapı Kiliti"nde admin şifresini denedim ve işe yaradı. Artık tüm kullanıcıların şifrelerini değiştirebilir, alarmı kapatabilir, hatta tüm kullanıcıları silebiliriz.

Üstelik şifreleri bilmemize bile gerek yok, basit birkaç adımla admin şifresini sıfırlayabiliyoruz:

1. Kutu arkasındaki reset butonuna 10 saniye basıyorsunuz.
2. DT-8 soketini çıkarıp 5 saniye bekliyorsunuz, sonra tekrar takıyorsunuz.
3. 10 saniye sonra butona basmayı bırakıp aynı anda soketi tekrar çıkarıyorsunuz.
4. Bundan sonra varsayılan admin şifresini kullanabilirsiniz :)

![Kapı kilidi şifresi atlamayı gösteren gerçek hayat güvenlik testi](https://miro.medium.com/v2/resize:fit:1400/format:webp/0*mGGgCG6CaxKqC6WJ)

## İstatistikler

Şu ana kadar 3 Audio kapı kilidi denedim ve hepsinde varsayılan şifreler vardı (admin şifresi dahil). İstatistiklere katkıda bulunmak isterseniz, siz de test edebilirsiniz.

![Kapı kilitlerinde varsayılan şifre kullanımını gösteren istatistikler](https://miro.medium.com/v2/resize:fit:1122/format:webp/0*v4sneCtbZPjIdYVD)

## Varsayılan Şifreler için GitHub Deposu

Model hakkında bilgi almak için ID'yi giriyorsunuz. gateCracker aracı ile REST API'yi kullanabilirsiniz.

![Varsayılan şifre güvenlik açıkları olan kapı kilidi modellerinin listesi](https://miro.medium.com/v2/resize:fit:1140/format:webp/0*aRFq5D78zydmjtjZ)

## Kaynak Kod

- [https://github.com/aydinnyunus/gateCracker-REST](https://github.com/aydinnyunus/gateCracker-REST) - REST API
- [https://github.com/aydinnyunus/gateCracker](https://github.com/aydinnyunus/gateCracker) - Ana araç (REST API'ye bağlanır)

## İlgili İçerikler

Bu araştırma, fiziksel güvenlik sistemlerinin ne kadar savunmasız olabileceğini gösteriyor. Dijital tehditler hakkında daha fazla güvenlik araştırması için [Instagram Dolandırıcılarını Hacklemek](/2022/04/11/hacking-instagram-scammers-tr/) yazımı inceleyebilirsiniz. Bu yazıda phishing saldırıları ve OSINT tekniklerini araştırdım. Ayrıca daha fazla [güvenlik projelerim](/tr/projects/) ve araçlarımı keşfedebilirsiniz.

