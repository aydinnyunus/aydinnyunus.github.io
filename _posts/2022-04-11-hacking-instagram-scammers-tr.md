---
layout: post
title: "Instagram Dolandırıcılarını Hacklemek"
date: 2022-04-11
author: Yunus Aydın
lang: tr
description: "Instagram phishing dolandırıcılıkları üzerine güvenlik araştırması. Dolandırıcıların phishing siteleri aracılığıyla Instagram hesaplarını nasıl çaldığını ve OSINT teknikleri ile XSS güvenlik açıklarını kullanarak nasıl araştırılacağını öğrenin."
keywords: "instagram dolandırıcıları, phishing, OSINT, XSS, güvenlik araştırması, sosyal mühendislik, siber güvenlik, dolandırıcılık araştırması"
canonical_url: "https://aydinnyunus.github.io/2022/04/11/hacking-instagram-scammers-tr/"
---

Son zamanlarda çok fazla phishing sitesi ve mesaj görüyorum. Bu yüzden dolandırıcıların nasıl çalıştığını ve yüzlerce Instagram hesabını nasıl çaldıklarını araştırmaya karar verdim. Bu siteler muhtemelen anormal aktiviteleri fark ettikleri için kapatıldı.

## Phishing Sitelerini Bulma

Twitter ve Instagram'da gezinirken telif hakkı ihlali mesajları hakkında tweetler ve hikayeler gördüm. İnsanların phishing konusunda bilinçli olması güzel (hepsi değil tabii). Bu siteleri incelemeye başladım ve kimlik bilgilerini nasıl çaldıklarını anladım.

## Nasıl Çalışıyor?

İlk tweet'te gördüğüm bir sitede başladım. Phishing mesajlarından birinde, dolandırıcı **Inhelptechnicanalyse** adlı hesap üzerinden Instagram kullanıcısına mesaj gönderiyordu. Telif hakkı ihlali olduğunu söylüyor ve hesabı doğrulamak için [https://veriyfycontacsupports.com/](https://veriyfycontacsupports.com/) adresine gitmesini ve form doldurmasını istiyordu.

Kullanıcı adı girildikten sonra, kullanıcının profil resmi indirilip arka planda gösteriliyordu. Bu sayede şifre istenen sayfa daha güvenilir görünüyordu.

**İlk Phishing Sitesi:**

![Kurban ve Saldırgan arkadaş oluyor 🥰](https://miro.medium.com/v2/resize:fit:822/1*p6bto-Uwi3XSYcMqwGQgew.png)

Kurban ve saldırgan arkadaş oluyor 🥰

> [https://twitter.com/bengujumping/status/1497926066496753671](https://twitter.com/bengujumping/status/1497926066496753671)

**İkinci Phishing Sitesi:**

![](https://miro.medium.com/v2/resize:fit:838/1*4zJCOkaYXZlLTQKHEdfJ1Q.png)

Birisi siber güvenlik hakkında soruyor [Ömer Çitak](https://twitter.com/om3rcitak)'a, bir yıl sonra aynı hesap ona phishing maili gönderiyor 🙃

> [https://twitter.com/om3rcitak/status/1499341391427776520](https://twitter.com/om3rcitak/status/1499341391427776520)

Kullanıcı adı ve şifre girildikten sonra, site e-posta ve şifrenizi de istiyor. Sonra hesabınızda istediklerini yapabiliyorlar. Tüm bu süreç otomatik çalışıyor.

Dizin brute force için [ffuf](https://github.com/ffuf/ffuf) kullandım, wordlist olarak [SecLists](https://github.com/danielmiessler/SecLists/blob/master/Discovery/Web-Content/directory-list-2.3-medium.txt) kullandım. Ama önemli bir dizin bulamadım. Sonra ilk sayfaya döndüm ve "<h1>a</h1>" gibi bir kullanıcı adı girdim. HTML etiketi çalıştığını görünce, geçerli bir kullanıcı adı ve şifre alanına Blind XSS payload'ı gönderdim. 2-3 saat sonra XSSHunter'da bildirimler geldi. 78.180.5.144 ve 178.246.104.93 IP'leri admin panelini izliyordu. URL rastgele bir string olduğu için [ffuf](https://github.com/ffuf/ffuf) bulamıyordu. Bu linke tıkladım ve kimlik doğrulama yoktu. Phishing sayfasında giriş yapmaya çalışan tüm hesapları görebiliyordum.

2FA açıksa (açık olmalı), script 2FA'yı kapatıyor. Çünkü kurban phishing sayfasına e-posta ve şifre bilgilerini girmiş oluyor.

![](https://miro.medium.com/v2/resize:fit:794/0*bGu8IOfaxouh33b9)

2FA açık mı? Açıksa kapat.

Bu dolandırıcılar 3 günde 3 hesap çalıyor. Yapabileceğim tek şey sitedeki bu bilgileri silmek ve insanları bilinçlendirmek.

## Saldırganları Araştırma

IP adreslerini [whatismyipaddress.com](http://whatismyipaddress.com) üzerinden kontrol edebilirsiniz.

![İlk Phishing Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1400/1*8Y40S0GOWN8lRe-QWkpp3w.png)

> [https://whatismyipaddress.com/ip/178.246.104.93](https://whatismyipaddress.com/ip/178.246.104.93)

![İlk Phishing Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1400/1*hZsMikaiWdPViI9sikFvHg.png)

> [https://whatismyipaddress.com/ip/31.143.156.102](https://whatismyipaddress.com/ip/31.143.156.102)

![İlk Phishing Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1400/1*_1yGGAcvXoxMB_XhvJyEAQ.png)

> [https://whatismyipaddress.com/ip/78.180.5.144](https://whatismyipaddress.com/ip/78.180.5.144)

IP adresleri Türkiye'de.

![İkinci Phishing Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1116/1*eQMDsPw06wnuYHQZz0hegQ.png)

> [https://whatismyipaddress.com/ip/212.47.230.124](https://whatismyipaddress.com/ip/212.47.230.124)

Bu IP adresi Güney Afrika'da.

## Phishing Sitesi Admin Paneli

Admin paneli şu bilgileri içeriyor:

-   Kullanıcı adı
-   Şifre
-   2FA Kurulum Anahtarı
-   6 Haneli Giriş Kodu
-   Değiştirilmiş E-posta Adresi
-   Yedek Kodlar

![Phishing Sitesi Admin Paneli](https://miro.medium.com/v2/resize:fit:1384/1*9wjbicYanp-G6E2i3bkAQg.jpeg)

"GoogleAuthenticator & DuoMobile"a tıkladığımda [https://pilot.albay-arnold.com/Code.php?totp=](https://pilot.albay-arnold.com/Code.php?totp=)<SETUP KEY> adresine yönlendirildim. Bu site her 30 saniyede bir OTP oluşturup admin panelinde gösteriyor. Site hala aktif.

Sonra hangi geçici e-posta adresini kullandıklarını merak ettim. Bunu anlamak için "Mail Hesabına Giriş Yap" butonunu eklemişler ve [https://mbox.reispeke6r.com/](https://mbox.reispeke6r.com/) adresine yönlendiriyor.

Bu sitede geçici e-posta adresleri oluşturabiliyorsunuz. Normalde gelen mailleri görmek için şifre gerekir ama şifreye ihtiyacım yok çünkü hepsini admin panelinde görebiliyorum.

**Örnek URL:**

`https://mbox.reispeke6r.com/mail/?email=<rastgele dize>@mbox.reispeke6r.com&password=<hash>`

E-posta parametresine e-posta adresinizi, şifre parametresine bir hash giriyorsunuz. Bu URL'ye tıkladığınızda görebilirsiniz: Kurbanın e-posta adresi saldırganın e-postasıyla değiştirilmiş mi? 2FA kapatılmış mı? Her ikisi de doğruysa kurban hesabına giriş yapabiliyorsunuz.

## İstatistikler

Saatte kaç hesap hackleniyor?

-   X-Düzlemi — Saat
-   Y-Düzlemi — Hesap Sayısı
-   Her nokta her saati simgeler

![Saatte hacklenen hesapları gösteren istatistikler](https://miro.medium.com/v2/resize:fit:1400/1*1mE_Ei0E79DDW484tkRK2Q.png)

**Ortalama Yaş:** 29.53

**Cinsiyet Yüzdesi:** **%60 E — %40 K** (Profil resmi olmayan veya sahte resim ve kapalı hesaplar hariç)

## Bu bilgileri nasıl aldım?

Osintgram, herhangi bir kullanıcının Instagram hesabını takma adına göre analiz etmek için etkileşimli bir shell sunuyor. Şunları alabilirsiniz:

-   addrs - Hedef fotoğraflarından kayıtlı tüm adresleri al
-   captions - Kullanıcının fotoğraf açıklamalarını al
-   comments - Hedefin gönderilerinin toplam yorumlarını al
-   followers - Hedef takipçilerini al
-   followings - Hedefin takip ettiği kullanıcıları al
-   fwersemail - Hedef takipçilerinin e-postasını al
-   fwingsemail - Hedefin takip ettiği kullanıcıların e-postasını al
-   fwersnumber - Hedef takipçilerinin telefon numarasını al
-   fwingsnumber - Hedefin takip ettiği kullanıcıların telefon numarasını al
-   hashtags - Hedefin kullandığı hashtag'leri al
-   info - Hedef bilgilerini al
-   likes - Hedefin gönderilerinin toplam beğenilerini al
-   mediatype - Kullanıcının gönderi türünü al (fotoğraf veya video)
-   photodes - Hedef fotoğraflarının açıklamasını al
-   photos - Kullanıcının fotoğraflarını çıktı klasörüne indir
-   propic - Kullanıcının profil resmini indir
-   stories - Kullanıcının hikayelerini indir
-   tagged - Hedefin etiketlediği kullanıcıların listesini al
-   wcommented - Hedefin fotoğraflarına yorum yapan kullanıcı listesini al
-   wtagged - Hedefi etiketleyen kullanıcı listesini al

> [https://github.com/Datalux/Osintgram](https://github.com/Datalux/Osintgram)

Takipçi sayısı ve HD profil resmi URL'sini almak için "info" komutunu kullandım. Bu bilgileri aldıktan sonra kaç hesabın etkilendiğini hesaplayan ve profil resimlerini indiren basit bir bash script'i yazdım.

![Hacklenen Kullanıcı Adları ve Tarihleri](https://miro.medium.com/v2/resize:fit:1034/1*Q5qwDscE10ujXk2rGVvUaA.jpeg)

![Etkilenen kullanıcı sayısı](https://miro.medium.com/v2/resize:fit:624/1*8_K0AVmGJUx3BSSXj63OVA.jpeg)

Bu istatistikler sadece **bir gün** için geçerli. Diğer günleri takip edemedim çünkü siteler kapatıldı.

Profil resimlerini aldıktan sonra yaş ve cinsiyet bilgilerini tahmin etmek için "age-gender-estimation" aracını kullandım.

![Yaş ve cinsiyet tahmin sonuçları](https://miro.medium.com/v2/resize:fit:864/1*XBf-oVr_L2Ewb5_Aezfi8A.png)

> [https://github.com/yu4u/age-gender-estimation](https://github.com/yu4u/age-gender-estimation)

Sonuç olarak, tanımadığınız kaynaklardan gelen linklere tıklamayın ve bilgilerinizi girmeyin. Mümkün olan tüm hesaplarda iki faktörlü kimlik doğrulamayı kullanın. Farkındalık için bu yazıyı sosyal medya kullanan arkadaşlarınızla paylaşmanızı öneririm.

