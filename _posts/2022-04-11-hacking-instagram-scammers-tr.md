---
layout: post
title: "Instagram Dolandırıcılarını Hacklemek"
date: 2022-04-11
author: Yunus Aydın
lang: tr
description: "Instagram phishing dolandırıcılıkları konusunda güvenlik araştırması. Dolandırıcıların phishing web siteleri aracılığıyla Instagram hesaplarını nasıl çaldığını ve OSINT teknikleri ve XSS güvenlik açıklarını kullanarak nasıl araştırılacağını öğrenin."
keywords: "instagram dolandırıcıları, phishing, OSINT, XSS, güvenlik araştırması, sosyal mühendislik, siber güvenlik, dolandırıcılık araştırması"
canonical_url: "https://aydinnyunus.github.io/2022/04/11/hacking-instagram-scammers-tr/"
---

# Instagram Dolandırıcılarını Hacklemek

## Özet

Bu günlerde birçok phishing web sitesi ve mesaj görüyorum. Bu yüzden dolandırıcıların insanları nasıl dolandırdığını ve yüzlerce Instagram hesabını nasıl çaldığını araştırmaya karar verdim. Ayrıca, bu web siteleri muhtemelen anormal faaliyetleri anladıkları için kapatıldı.

## Phishing Web Sitelerini Bulma

Twitter ve Instagram'da gezinirken, Telif Hakkı İhlali mesajları hakkında tweetler ve hikayeler gördüm ve insanların phishing konusunda bilinçli olduğunu görmek güzel (hepsi değil). Bu yüzden bu web sitelerini taramaya başladım ve kimlik bilgilerini nasıl çaldıklarını anladım.

## Mantığı Anlama

İlk tweet'te göreceğiniz bir web sitesinde başladım. Phishing mesajlarından birinde, dolandırıcı **Inhelptechnicanalyse** adlı hesap aracılığıyla Instagram kullanıcısına bir mesaj gönderiyordu, telif hakkı ihlali olduğunu belirtiyor ve kullanıcıdan hesabını doğrulamak için [https://veriyfycontacsupports.com/](https://veriyfycontacsupports.com/) adresini ziyaret etmesini ve bir form doldurmasını istiyordu.

Instagram kullanıcı adı web sayfasına girildikten sonra, kullanıcının profil resmi indirildi ve arka planda gösterildi, şifre istenen sayfanın güvenilirliğini artırmak için.

**İlk Phishing Web Sitesi** :

![Kurban ve Saldırgan arkadaş oluyor 🥰](https://miro.medium.com/v2/resize:fit:822/1*p6bto-Uwi3XSYcMqwGQgew.png)

Kurban ve Saldırgan arkadaş oluyor 🥰

> [https://twitter.com/bengujumping/status/1497926066496753671](https://twitter.com/bengujumping/status/1497926066496753671)

**İkinci Phishing Web Sitesi :**

![](https://miro.medium.com/v2/resize:fit:838/1*4zJCOkaYXZlLTQKHEdfJ1Q.png)

Birisi siber güvenlik hakkında soruyor [Ömer Çitak](https://twitter.com/om3rcitak), Bir yıl sonra, aynı hesap ona phishing e-postası gönderiyor 🙃

> [https://twitter.com/om3rcitak/status/1499341391427776520](https://twitter.com/om3rcitak/status/1499341391427776520)

Kullanıcı adınızı ve şifrenizi girdikten sonra, web sitesi e-postanızı ve şifrenizi istiyor ve sonra hesabınızda her şeyi yapabilirler. Tüm bu süreçlerin otomatik olduğunu düşünüyorum.

Dizin brute force için [ffuf](https://github.com/ffuf/ffuf) kullandım ve wordlist için [SecLists](https://github.com/danielmiessler/SecLists/blob/master/Discovery/Web-Content/directory-list-2.3-medium.txt) kullandım. Ancak önemli bir dizin bulamadım. Bundan sonra ilk sayfaya geri döndüm ve "<h1>a</h1>" gibi bir kullanıcı adı girdim ve HTML etiketinin çalıştığını gördüm. Bu yüzden geçerli bir kullanıcı adı ve şifre alanına Blind XSS Payload gönderdim. Ve 2-3 saat sonra XSSHunter'da bildirimler aldım. 78.180.5.144 ve 178.246.104.93 IP adresleri Yönetici Paneli'ni izliyor. Gördüğünüz gibi URL rastgele bir dizedir, bu yüzden [ffuf](https://github.com/ffuf/ffuf) bu URL'yi bulamıyor. Bu yüzden bu bağlantıya tıkladım ve kimlik doğrulama mekanizması yok ve phishing sayfasında giriş yapmaya çalışan tüm hesapları görebiliyorum.

2FA etkinse (etkinleştirmelisiniz), komut dosyası 2FA'yı devre dışı bırakır. Çünkü kurban phishing sayfasına e-posta ve şifre bilgilerini girdi.

![](https://miro.medium.com/v2/resize:fit:794/0*bGu8IOfaxouh33b9)

2FA açık mı? Eğer öyleyse, kapat

Bu dolandırıcılar 3 günde 3 hesap çalıyor. Yapabileceğim tek şey web sitesindeki bu bilgileri silmek ve insanları bilinçlendirmek.

## Saldırganları Araştırma

IP Adreslerini [whatismyipaddress.com](http://whatismyipaddress.com) adresinde kontrol edebilirsiniz

![İlk Phishing Web Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1400/1*8Y40S0GOWN8lRe-QWkpp3w.png)

> [https://whatismyipaddress.com/ip/178.246.104.93](https://whatismyipaddress.com/ip/178.246.104.93)

![İlk Phishing Web Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1400/1*hZsMikaiWdPViI9sikFvHg.png)

> [https://whatismyipaddress.com/ip/31.143.156.102](https://whatismyipaddress.com/ip/31.143.156.102)

![İlk Phishing Web Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1400/1*_1yGGAcvXoxMB_XhvJyEAQ.png)

> [https://whatismyipaddress.com/ip/78.180.5.144](https://whatismyipaddress.com/ip/78.180.5.144)

IP Adresleri Türkiye'de.

![İkinci Phishing Web Sitesinde XSSHunter Raporu](https://miro.medium.com/v2/resize:fit:1116/1*eQMDsPw06wnuYHQZz0hegQ.png)

> [https://whatismyipaddress.com/ip/212.47.230.124](https://whatismyipaddress.com/ip/212.47.230.124)

Bu IP Adresi Güney Afrika'da

## Phishing Web Sitesi Yönetici Paneli

Yönetici Paneli aşağıdaki bilgileri içerir.

-   Kullanıcı adı
-   Şifre
-   2FA Kurulum Anahtarı
-   6 Haneli Giriş Kodu
-   Değiştirilmiş E-posta Adresi
-   Yedek Kodlar

![Phishing Web Sitesi Yönetici Paneli](https://miro.medium.com/v2/resize:fit:1384/1*9wjbicYanp-G6E2i3bkAQg.jpeg)

"GoogleAuthenticator & DuoMobile"a tıkladığımda, [https://pilot.albay-arnold.com/Code.php?totp=](https://pilot.albay-arnold.com/Code.php?totp=)<SETUP KEY> adresine yönlendirildim. Bu web sitesi her 30 saniyede bir OTP oluşturur ve Yönetici Paneli'nde gösterir. Web sitesi hala aktif.

Bundan sonra, hangi geçici e-posta adresini kullandıklarını merak ettim, bunu anlamak için "Mail Hesabına Giriş Yap (Login to Mail Account)" düğmesini eklediler ve [https://mbox.reispeke6r.com/](https://mbox.reispeke6r.com/) adresine yönlendiriyor.

Bu web sitesinde geçici e-posta adresleri oluşturabilirsiniz. Normalde gelen e-postaları görmek için bir şifre gerekir ama şifreye ihtiyacım yok çünkü hepsini Yönetici Paneli'nde görebiliyorum.

**Örnek URL :**

`https://mbox.reispeke6r.com/mail/?email=<rastgele dize>@mbox.reispeke6r.com&password=<hash>`

E-posta parametresinde, e-posta adresinizi girmeniz gerekir ve şifre parametresinde bir hash vardır. Bu URL'ye tıkladığınızda görebilirsiniz, kurbanın e-posta adresi saldırganın e-posta adresiyle değiştirilmiş mi? 2FA devre dışı mı? Her ikisi de doğruysa, kurban hesabına giriş yapabilirsiniz.

## İstatistikler

Saatte kaç hesap hackleniyor?

-   X-Düzlemi — Saat
-   Y-Düzlemi — Hesap Sayısı
-   Her nokta her saati simgeler

![Saatte hacklenen hesapları gösteren istatistikler](https://miro.medium.com/v2/resize:fit:1400/1*1mE_Ei0E79DDW484tkRK2Q.png)

**Ortalama Yaş:** 29.53

**Cinsiyet Yüzdesi:** **%60 E — %40 K** (Profil resmi olmayan veya sahte resim ve kapalı hesaplar hariç)

## Bu bilgileri nasıl aldım?

Osintgram, herhangi bir kullanıcının Instagram hesapları üzerinde takma adına göre analiz yapmak için etkileşimli bir kabuk sunar. Şunları alabilirsiniz:

-   addrs Hedef fotoğraflar tarafından kayıtlı tüm adresleri al
-   captions Kullanıcının fotoğraf açıklamalarını al
-   comments Hedefin gönderilerinin toplam yorumlarını al
-   followers Hedef takipçilerini al
-   followings Hedef tarafından takip edilen kullanıcıları al
-   fwersemail Hedef takipçilerinin e-postasını al
-   fwingsemail Hedef tarafından takip edilen kullanıcıların e-postasını al
-   fwersnumber Hedef takipçilerinin telefon numarasını al
-   fwingsnumber Hedef tarafından takip edilen kullanıcıların telefon numarasını al
-   hashtags Hedef tarafından kullanılan hashtag'leri al
-   info Hedef bilgilerini al
-   likes Hedefin gönderilerinin toplam beğenilerini al
-   mediatype Kullanıcının gönderi türünü al (fotoğraf veya video)
-   photodes Hedef fotoğraflarının açıklamasını al
-   photos Kullanıcının fotoğraflarını çıktı klasörüne indir
-   propic Kullanıcının profil resmini indir
-   stories Kullanıcının hikayelerini indir
-   tagged Hedef tarafından etiketlenen kullanıcıların listesini al
-   wcommented Hedefin fotoğraflarına yorum yapan kullanıcı listesini al
-   wtagged Hedefi etiketleyen kullanıcı listesini al`

> [https://github.com/Datalux/Osintgram](https://github.com/Datalux/Osintgram)

Takipçi Sayısı ve HD Profil Resmi URL'sini almak için "info" komutunu kullandım. Bu bilgileri aldıktan sonra, kaç hesabın etkilendiğini hesaplayan ve profil resimlerini indiren basit bir bash komut dosyası yazdım.

![Hacklenen Kullanıcı Adları ve Tarihleri](https://miro.medium.com/v2/resize:fit:1034/1*Q5qwDscE10ujXk2rGVvUaA.jpeg)

![Etkilenen kullanıcı sayısı](https://miro.medium.com/v2/resize:fit:624/1*8_K0AVmGJUx3BSSXj63OVA.jpeg)

Bu istatistikler sadece **bir gün** için geçerlidir. Diğer günleri takip edemedim çünkü web siteleri kapalıydı.

Profil resimlerini aldıktan sonra, yaş ve cinsiyet bilgilerini tahmin etmek için "age-gender-estimation" aracını kullandım.

![Yaş ve cinsiyet tahmin sonuçları](https://miro.medium.com/v2/resize:fit:864/1*XBf-oVr_L2Ewb5_Aezfi8A.png)

> [https://github.com/yu4u/age-gender-estimation](https://github.com/yu4u/age-gender-estimation)

Sonuç olarak, tanıdık olmayan kaynaklardan gelen bağlantılara tıklamayın ve bilgilerinizi girmeyin. Mümkün olan tüm hesaplarda iki faktörlü kimlik doğrulamayı kullanın. Son olarak, farkındalık için, bu makaleyi sosyal ağlar/medya kullanan arkadaşlarıyla paylaşmalarını öneririm.

