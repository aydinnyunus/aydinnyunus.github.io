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

## En çok kullanılan varsayılan şifreler

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

## Gerçek hayat testleri

"Audio Şifreli Kapı Kiliti"nde varsayılan admin şifresini denedim, çalıştı. Bu sayede tüm kullanıcıların şifrelerini değiştirebiliyor, alarmı kapatıyor, hatta kullanıcıları silebiliyorsun.

Şifreleri bilmene gerek yok; birkaç adımla admin şifresini sıfırlayabiliyorsun:

1. Kutu arkasındaki reset butonuna 10 saniye bas.
2. DT-8 soketini çıkarıp 5 saniye bekle, sonra tekrar tak.
3. 10 saniye sonra butonu bırakıp aynı anda soketi tekrar çıkar.
4. Bundan sonra varsayılan admin şifresini kullanabiliyorsun.

![Kapı kilidi şifresi atlamayı gösteren gerçek hayat güvenlik testi](https://miro.medium.com/v2/resize:fit:1400/format:webp/0*mGGgCG6CaxKqC6WJ)

## İstatistikler

Şu ana kadar 3 Audio kapı kilidi denedim ve hepsinde varsayılan şifreler vardı (admin şifresi dahil). İstatistiklere katkıda bulunmak isterseniz, siz de test edebilirsiniz.

![Kapı kilitlerinde varsayılan şifre kullanımını gösteren istatistikler](https://miro.medium.com/v2/resize:fit:1122/format:webp/0*v4sneCtbZPjIdYVD)

## Varsayılan şifreler için GitHub deposu

Model bilgisi için ID giriyorsun. gateCracker aracı ile REST API kullanılabiliyor.

![Varsayılan şifre güvenlik açıkları olan kapı kilidi modellerinin listesi](https://miro.medium.com/v2/resize:fit:1140/format:webp/0*aRFq5D78zydmjtjZ)

## Kaynak kod

- [https://github.com/aydinnyunus/gateCracker-REST](https://github.com/aydinnyunus/gateCracker-REST) - REST API
- [https://github.com/aydinnyunus/gateCracker](https://github.com/aydinnyunus/gateCracker) - Ana araç (REST API'ye bağlanır)

## İlgili içerik

Bu araştırma fiziksel güvenlik sistemlerinin ne kadar savunmasız olabildiğini gösteriyor. Dijital tarafta [Instagram dolandırıcılarını hacklemek](/2022/04/11/hacking-instagram-scammers-tr/) yazısında phishing ve OSINT’i anlattım. Daha fazla [güvenlik projesi](/tr/projects/) için projeler sayfasına bakabilirsin.
