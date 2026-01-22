---
layout: post
title: "PassDetective: Shell Geçmişinizdeki Parolaları ve Secret'ları Tespit Etmek"
date: 2025-12-14
author: Yunus Aydın
lang: tr
description: "PassDetective ile shell command history'den yanlışlıkla yazılmış parolalar, API key'ler ve secret'ları tespit etme. Kali Linux ve NixOS'ta PassDetective kullanımı."
keywords: "passdetective, shell history, password detection, API key detection, Kali Linux, NixOS, security tool, command history analysis, secret detection, regex scanning"
canonical_url: "https://aydinnyunus.github.io/2025/12/14/passdetective-kali-linux-tr/"
---

Shell command history'nizde yanlışlıkla yazdığınız parolalar, API key'ler veya secret'lar olabilir. Bu bilgiler history dosyanızda saklanıyor ve güvenlik riski oluşturuyor. PassDetective, shell geçmişinizi tarayarak bu tür hassas bilgileri tespit eden bir komut satırı aracı. Hem Kali Linux'ta hem de NixOS'ta kullanılabilen bu araç, düzenli ifadeler kullanarak komut geçmişinizdeki potansiyel güvenlik açıklarını belirlemenize yardımcı oluyor.

## PassDetective Nedir?

PassDetective, Go programlama dili ile yazılmış bir güvenlik aracı. Temel amacı, shell command history dosyalarınızı (ZSH ve Bash) tarayarak yanlışlıkla yazılmış parolaları, API key'leri ve diğer hassas bilgileri tespit etmek. Araç, güçlü regex pattern'leri kullanarak 40'tan fazla farklı secret tipini tanıyabiliyor.

Araç şu ana kadar GitHub'da **141 star** topladı ve güvenlik topluluğu tarafından yaygın olarak kullanılıyor. Ayrıca Kali Linux'un resmi araçları arasında yer alıyor ve NixOS paket deposunda da mevcut.

## Neden PassDetective?

Günlük kullanımda shell'de çalışırken, bazen parolaları veya API key'leri komut satırına yazmak zorunda kalıyoruz. Örneğin:

```bash
curl -u username:password123 https://api.example.com
```

Bu tür komutlar shell history'nize kaydediliyor ve `.zsh_history` veya `.bash_history` dosyalarında saklanıyor. Eğer bu dosyalar bir şekilde erişilebilir hale gelirse (örneğin bir backup'ta veya paylaşılan bir sistemde), hassas bilgileriniz açığa çıkabilir.

PassDetective, bu riski minimize etmek için history dosyalarınızı düzenli olarak tarayıp potansiyel tehlikeleri tespit ediyor. Böylece hassas bilgileri bulup gerekli önlemleri alabiliyorsunuz.

## Kurulum

### Kali Linux

PassDetective, Kali Linux'un resmi paket deposunda mevcut. Kurulum için:

```bash
sudo apt install passdetective
```

![PassDetective Kali Linux Installation](https://github.com/aydinnyunus/PassDetective/assets/52822869/dc12e543-3454-423b-b3fe-0511f93e2c3e)

### NixOS

NixOS'ta PassDetective'i kurmak için:

```bash
nix-env -iA nixpkgs.passdetective
```

Veya `configuration.nix` dosyanızda:

```nix
environment.systemPackages = with pkgs; [
  passdetective
];
```

### Go ile Kurulum

Kaynak koddan kurulum yapmak isterseniz:

```bash
go install github.com/aydinnyunus/PassDetective@latest
```

## Kullanım

PassDetective'in temel kullanımı oldukça basit. Araç, `extract` komutu ile shell history dosyalarınızı tarayabiliyor.

### Yardım Menüsü

Önce aracın tüm seçeneklerini görmek için:

```bash
PassDetective -h
```

![PassDetective Help](https://i.imgur.com/UeMRWTd.png)

### Shell History Analizi

ZSH history'nizi taramak için:

```bash
PassDetective extract --zsh
```

Bash history'nizi taramak için:

```bash
PassDetective extract --bash
```

Her iki shell history'yi de taramak için:

```bash
PassDetective extract --all
```

![Extract All](https://i.imgur.com/zKjs3p8.png)

### Secret Tespiti

PassDetective, sadece parolaları değil, aynı zamanda API key'leri ve diğer secret'ları da tespit edebiliyor. Secret taraması için:

```bash
PassDetective extract --secrets --zsh
```

veya

```bash
PassDetective extract --secrets --bash
```

![Secrets Detection](https://i.imgur.com/yw3v0RI.png)

## Tespit Edilen Secret Tipleri

PassDetective, aşağıdaki gibi birçok farklı secret tipini tespit edebiliyor:

- **Cloudinary URL'leri**: `cloudinary://` ile başlayan URL'ler
- **Firebase URL'leri**: `firebaseio.com` içeren URL'ler
- **Slack Token'ları**: `xox[p|b|o|a]-` formatındaki token'lar
- **RSA Private Key'ler**: `-----BEGIN RSA PRIVATE KEY-----` ile başlayan key'ler
- **SSH Private Key'ler**: DSA, EC ve PGP private key'ler
- **AWS Access Key ID'leri**: `AKIA[0-9A-Z]{16}` formatındaki key'ler
- **Google API Key'leri**: `AIza[0-9A-Za-z\\-_]{35}` formatındaki key'ler
- **GitHub Token'ları**: GitHub API token'ları
- **Stripe API Key'leri**: `sk_live_` ile başlayan key'ler
- **Twilio API Key'leri**: `SK[0-9a-fA-F]{32}` formatındaki key'ler
- **URL'lerdeki Parolalar**: `https://username:password@example.com` formatındaki URL'ler

Ve daha fazlası. PassDetective, [secret-regex-list](https://github.com/h33tlit/secret-regex-list) projesinden regex pattern'lerini kullanıyor.

## Pratik Kullanım Senaryoları

### Senaryo 1: Düzenli Güvenlik Kontrolü

Güvenlik açısından, shell history dosyalarınızı düzenli olarak taramak iyi bir pratiktir. Örneğin, aylık bir kontrol için:

```bash
PassDetective extract --all --secrets
```

Bu komut, hem ZSH hem de Bash history'nizi tarayıp tüm secret'ları tespit eder.

### Senaryo 2: Yeni Bir Projeye Başlamadan Önce

Yeni bir projeye başlamadan önce, mevcut shell history'nizde hassas bilgi olup olmadığını kontrol edebilirsiniz:

```bash
PassDetective extract --zsh --secrets
```

### Senaryo 3: Sistem Temizliği

Bir sistemden ayrılmadan önce veya bir backup oluşturmadan önce, history dosyalarınızı temizlemek istiyorsanız, önce ne tür hassas bilgiler olduğunu görmek için PassDetective kullanabilirsiniz.

## Güvenlik Önerileri

PassDetective kullanırken dikkat edilmesi gereken birkaç nokta:

1. **History Dosyalarını Temizleme**: PassDetective sadece tespit ediyor, temizlemiyor. Tespit edilen hassas bilgileri manuel olarak temizlemeniz gerekiyor.

2. **Düzenli Tarama**: Shell history'nizi düzenli olarak tarayın. Özellikle production sistemlerinde çalışırken.

3. **Backup Kontrolü**: Backup'larınızı oluşturmadan önce history dosyalarınızı kontrol edin.

4. **Alias Kullanımı**: PassDetective, shell config dosyalarınızdaki alias'ları da kontrol ediyor. Bu sayede alias'larda saklanan hassas bilgileri de tespit edebiliyor.

## Sonuç

PassDetective, shell command history'nizdeki hassas bilgileri tespit etmek için kullanışlı bir araç. Hem Kali Linux'ta hem de NixOS'ta kolayca kurulup kullanılabiliyor. Düzenli olarak kullanıldığında, yanlışlıkla history'ye yazılmış parolaları ve secret'ları bulmanıza yardımcı oluyor.

Araç açık kaynak ve GitHub'da aktif olarak geliştiriliyor. Daha fazla bilgi ve kaynak kod için [GitHub deposunu](https://github.com/aydinnyunus/PassDetective) ziyaret edebilirsiniz.

**İlgili İçerikler:**
- [exifLooter: Fotoğraflardan Gizli Konum Bilgilerini Çıkarmak](/2025/12/07/exiflooter-kali-linux-tr/)
- [SQL Injection Zafiyeti: GeoPandas to_postgis() Fonksiyonunda Güvenlik Açığı](/sql-injection-geopandas-tr/)







