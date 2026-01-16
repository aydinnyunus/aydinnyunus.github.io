---
layout: post
title: "Package Takeover: Nimble Package Manager'da Supply Chain Security Zafiyeti"
date: 2026-01-24
author: Yunus Aydın
lang: tr
description: "Nimble package manager'da bulduğum supply chain security zafiyetleri. URL redirection ve nonexistent username exploit teknikleri ile package takeover."
keywords: "supply chain security, Nimble package manager, package takeover, security vulnerability, URL redirection, GitHub username, security research, supply chain attack"
canonical_url: "https://aydinnyunus.github.io/2026/01/24/supply-chain-security-hacking-nimble-packages-tr/"
---

Package manager'lar, yazılım geliştirme sürecinin kritik bir parçası. Ancak bu araçlar, güvenlik açıklarından muaf değil. Bu yazıda, Nimble package manager'da keşfettiğim iki kritik zafiyeti ve bunları nasıl exploit ettiğimi anlatacağım. 2,393 paket analiz ettim ve 139'unun zafiyetli olduğunu buldum. Supply chain security'nin ne kadar önemli olduğunu gösteren gerçek bir case study.

## Nimble Nedir?

Nimble, Nim programlama dili için bir package manager. Geliştiricilerin kütüphaneleri ve bağımlılıkları kolayca yönetmesini sağlıyor. `nimble.directory` adında bir platform üzerinden geliştiriciler paketlerini yayınlayabiliyor ve diğerleri bu kütüphaneleri projelerine entegre edebiliyor.

![nimble.directory](https://nimble.directory)

Platform, GitHub repository'lerini kaynak olarak kullanıyor. Her paket, bir GitHub URL'si ve kullanıcı adı ile ilişkilendiriliyor. İşte bu ilişkilendirme mekanizmasında güvenlik açıkları buldum.

## Zafiyetler: URL Redirection ve Nonexistent Username

İki kritik zafiyet keşfettim:

### 1. URL Redirection Zafiyeti

Bir paketin GitHub URL'si redirect edildiğinde, redirection gerçekleşmeden önce önceki URL'yi ele geçirmek mümkün. Eğer orijinal URL artık geçerli değilse, saldırgan paketin kontrolünü ele geçirebilir.

**Nasıl çalışıyor:**

- Paket A, `github.com/user-old/repo` URL'sine sahip
- Bu URL, `github.com/user-new/repo` adresine redirect ediliyor
- Eğer `user-old` hesabı silinmiş veya repo taşınmışsa, saldırgan `user-old` hesabını oluşturup aynı repo adını kullanarak paketi ele geçirebilir

### 2. Nonexistent Username Zafiyeti

Eğer bir Nimble paketiyle ilişkilendirilmiş GitHub kullanıcı adı mevcut değilse, bu durum paket ele geçirmek için kullanılabilir.

**Nasıl çalışıyor:**

- Paket B, `github.com/nonexistent-user/package` URL'sine sahip
- `nonexistent-user` hesabı GitHub'da yok
- Saldırgan, `nonexistent-user` hesabını oluşturup aynı repo adını kullanarak paketi ele geçirebilir

## Zafiyetleri Nasıl Keşfettim?

Tüm paketleri analiz etmek için bir Python scripti yazdım. Script, iki ana kontrol yapıyordu:

1. **URL redirection kontrolü**: Her paketin GitHub URL'sinin redirect edilip edilmediğini kontrol ediyor
2. **Username existence kontrolü**: GitHub kullanıcı adının mevcut olup olmadığını kontrol ediyor

### Analiz Scripti

```python
import requests

def check_redirected_url(package_url):
    try:
        response = requests.head(package_url, allow_redirects=True)
        if response.history:
            return response.history[-1].url
    except requests.RequestException as e:
        print(f"Error checking URL {package_url}: {e}")
    return None

def check_username_existence(username):
    try:
        response = requests.get(f'https://github.com/{username}')
        return response.status_code != 404
    except requests.RequestException as e:
        print(f"Error checking username {username}: {e}")
    return False

# Tüm paketleri analiz et
for package in all_packages:
    redirected_url = check_redirected_url(package.github_url)
    if redirected_url:
        print(f"Redirected URL found: {redirected_url}")
    
    username = package.username
    if not check_username_existence(username):
        print(f"Username {username} does not exist. Possible takeover.")
```

### Sonuçlar

- **Toplam analiz edilen paket**: 2,393
- **Zafiyetli paket sayısı**: 139
- **Zafiyet oranı**: ~5.8%

Bu sayılar, supply chain security'nin ne kadar kritik olduğunu gösteriyor. Küçük bir package manager'da bile yüzlerce zafiyetli paket bulunabiliyor.

## Case Study: binance Paketini Ele Geçirme

En ilginç case study'lerden biri, "binance" paketiydi. Bu paket, önceden `https://github.com/Imperator26/binance` adresinde barındırılıyordu. URL redirection zafiyetini kullanarak paketi ele geçirdim.

### Exploitation Adımları

1. **URL kontrolü**: Paketin GitHub URL'sinin redirect edilip edilmediğini kontrol ettim
2. **Hesap oluşturma**: Orijinal URL artık geçerli olmadığı için, aynı kullanıcı adını ve repo adını kullanarak yeni bir repository oluşturdum
3. **Malicious payload**: Pakete zararlı kod ekledim
4. **Yayınlama**: Paketi Nimble directory'ye yayınladım

### Malicious Payload

Paketin yeni versiyonuna, import edildiğinde `whoami` komutunu çalıştıran bir script ekledim:

```nim
import osproc

proc runCommand(cmd: string) =
  let result = execProcess(cmd)
  echo result

runCommand("whoami")
```

Bu kod, paket import edildiğinde otomatik olarak çalışıyor. `whoami` komutu, mevcut kullanıcı adını gösteriyor, bu da kodun başarıyla çalıştığını kanıtlıyor.

### Exploit Sonucu

Bir kullanıcı paketi import ettiğinde:

```nim
import binance
echo "binance package imported successfully!"
```

Çıktı şöyle oluyor:

```text
yunus.aydin
binance package imported successfully!
```

`whoami` komutu, mevcut kullanıcı adını gösteriyor. Bu, malicious package'ın başarıyla çalıştığını ve hedef sistemde kod çalıştırabildiğini kanıtlıyor.

## Etki Değerlendirmesi

Bu zafiyetler, supply chain security açısından ciddi riskler oluşturuyor:

### Potansiyel Saldırı Senaryoları

1. **Malicious code injection**: Saldırgan, ele geçirdiği pakete zararlı kod ekleyebilir
2. **Data exfiltration**: Paket, kullanıcı verilerini toplayıp saldırgana gönderebilir
3. **Backdoor installation**: Paket, sistemde kalıcı bir backdoor kurabilir
4. **Credential theft**: Paket, kullanıcı kimlik bilgilerini çalabilir

### Gerçek Dünya Etkisi

- **Geliştiriciler**: Zafiyetli paketleri kullanan geliştiriciler risk altında
- **Kullanıcılar**: Bu paketleri kullanan uygulamaları çalıştıran kullanıcılar etkilenebilir
- **Kuruluşlar**: Kurumsal projelerde kullanılan paketler, tüm organizasyonu etkileyebilir

### İstatistikler

- **2,393 paket** analiz edildi
- **139 zafiyetli paket** bulundu
- **~5.8% zafiyet oranı** tespit edildi

Bu sayılar, package manager'ların güvenlik açısından ne kadar kritik olduğunu gösteriyor. Küçük bir package manager'da bile yüzlerce zafiyetli paket bulunabiliyor.

## Güvenlik Önerileri

Package manager'lar ve geliştiriciler için öneriler:

### Package Manager'lar İçin

1. **URL validation**: Paket URL'lerinin geçerliliğini düzenli olarak kontrol et
2. **Username verification**: GitHub kullanıcı adlarının mevcut olduğunu doğrula
3. **Redirect monitoring**: URL redirect'lerini izle ve uyar
4. **Package ownership verification**: Paket sahipliğini doğrulamak için mekanizmalar ekle

### Geliştiriciler İçin

1. **Package audit**: Kullandığın paketleri düzenli olarak kontrol et
2. **Dependency pinning**: Paket versiyonlarını sabitle
3. **Security scanning**: Paketleri güvenlik taramasından geçir
4. **Trust verification**: Paket sahiplerinin güvenilir olduğundan emin ol

## Sonuç

Bu araştırma, package manager güvenliğinin ne kadar kritik olduğunu gösteriyor. URL redirection ve nonexistent username zafiyetleri, supply chain security açısından ciddi riskler oluşturuyor. Package manager'ların ve geliştiricilerin bu tür zafiyetlere karşı önlem alması gerekiyor.

**Ana çıkarımlar:**

- Package manager'lar, güvenlik açısından kritik altyapı bileşenleri
- URL validation ve username verification mekanizmaları şart
- Düzenli güvenlik denetimleri ve izleme gerekiyor
- Geliştiriciler, kullandıkları paketleri düzenli olarak kontrol etmeli

Supply chain security, modern yazılım geliştirmenin en önemli konularından biri. Bu tür zafiyetleri bulmak ve düzeltmek, tüm ekosistemin güvenliğini artırıyor.

## İlgili İçerik

- [SQL Injection Zafiyeti: GeoPandas to_postgis() Fonksiyonunda Güvenlik Açığı](/2025/12/27/sql-injection-geopandas-tr/)
- [CVE-2025-66019: pypdf Kütüphanesinde LZW Decompression DoS Zafiyeti](/2025/12/20/cve-2025-66019-pypdf-lzw-dos-tr/)
