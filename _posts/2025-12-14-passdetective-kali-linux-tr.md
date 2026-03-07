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

Shell geçmişinde yanlışlıkla yazılan parolalar, API key’ler veya secret’lar kalabiliyor; bunlar history dosyanda duruyor ve risk oluşturuyor. **PassDetective**, shell history’ni tarayıp bu tür hassas bilgileri tespit eden bir CLI aracı. Kali Linux ve NixOS’ta kullanılabiliyor; regex ile 40’tan fazla secret tipini tanıyabiliyor.

## PassDetective nedir?

PassDetective Go ile yazılmış bir güvenlik aracı. ZSH ve Bash history dosyalarını tarayıp yanlışlıkla yazılmış parolaları, API key’leri ve diğer hassas bilgileri buluyor. 40’tan fazla secret tipi için regex pattern’leri kullanıyor.

Araç Kali Linux’un resmi deposunda ve NixOS paketinde mevcut; GitHub’da da kullanılıyor.

## Neden PassDetective?

Günlük kullanımda bazen parola veya API key’i doğrudan komut satırına yazmak zorunda kalıyoruz (ör. `curl -u user:pass ...`). Bu komutlar `.zsh_history` veya `.bash_history`’e yazılıyor. Dosya bir backup’ta veya paylaşılan sistemde erişilebilir olursa hassas bilgiler sızabilir.

PassDetective history’ni tarayıp bu tür girdileri buluyor; böylece nerede risk olduğunu görüp önlem alabiliyorsun.

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

Temel kullanım: `extract` komutu ile history taranıyor.

### Yardım

Tüm seçenekler için:

```bash
PassDetective -h
```

![PassDetective Help](https://i.imgur.com/UeMRWTd.png)

### Shell history taraması

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

### Secret tespiti

PassDetective sadece parolaları değil API key’leri ve diğer secret’ları da tespit edebiliyor:

```bash
PassDetective extract --secrets --zsh
```

veya `--bash`. [secret-regex-list](https://github.com/h33tlit/secret-regex-list) projesindeki pattern’leri kullanıyor.

## Tespit edilen secret tipleri

PassDetective şu tür secret’ları tespit edebiliyor (örnekler):

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
- **URL’de parola:** `https://user:password@example.com` formatı  

Ve daha fazlası. Pattern’ler [secret-regex-list](https://github.com/h33tlit/secret-regex-list) projesinden.

## Pratik kullanım

### Düzenli güvenlik kontrolü

History’yi düzenli taramak iyi bir alışkanlık. Örneğin aylık:

```bash
PassDetective extract --all --secrets
```

### Yeni projeye başlamadan önce

Mevcut history’de hassas bilgi var mı diye bakmak için:

```bash
PassDetective extract --zsh --secrets
```

### Sistem temizliği

Sistemden çıkmadan veya backup almadan önce history’de ne olduğunu görmek için PassDetective kullanabilirsin. Araç sadece tespit ediyor; silme işlemini sen yapmalısın.

## Güvenlik önerileri

1. **Temizlik:** PassDetective sadece tespit ediyor; bulduklarını sen manuel temizliyorsun.  
2. **Düzenli tarama:** Özellikle production ortamında history’yi düzenli tara.  
3. **Backup öncesi:** Backup almadan önce history’yi kontrol et.  
4. **Alias:** Araç config’teki alias’ları da tarıyor; alias’ta saklanan secret’lar da bulunabiliyor.

## Özet

PassDetective shell history’deki parola ve secret’ları tespit etmek için kullanışlı. Kali ve NixOS’ta kurulumu kolay; düzenli kullanıldığında yanlışlıkla history’ye girmiş hassas bilgileri bulmana yardımcı oluyor.

Kaynak kod ve güncellemeler: [GitHub - PassDetective](https://github.com/aydinnyunus/PassDetective).

**İlgili içerik:**
- [exifLooter: Fotoğraflardan gizli konum bilgilerini çıkarmak](/2025/12/07/exiflooter-kali-linux-tr/)
- [SQL Injection: GeoPandas to_postgis()](/2025/12/27/sql-injection-geopandas-tr/)







