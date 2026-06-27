---
layout: post
title: "GitHub Archive üzerinde 34,794 commit taradım, 1,702'sinde canlı credential vardı"
author: Yunus Aydın
date: 2026-06-30
lang: tr
description: "Gemini 2.5 Flash Lite ile ~850 bin commit mesajını etiketledim, etiketleri regex'e dönüştürdüm, hayatta kalanları TruffleHog ile taradım. 1,702 unique commit'te 3,708 verified secret."
keywords: "github archive, secret scanning, trufflehog, gemini, gh archive, sızdırılmış credential, secret detection, ai ile regex üretimi, commit mesaj analizi, push event scanner, leaked api key"
canonical_url: "https://aydinnyunus.github.io/2026/06/30/hunting-leaked-secrets-on-github-archive-tr/"
---

**GitHub Archive** firehose'unu izleyip developer'ların yeni leak ettiği credential'lara benzeyen commit mesajlarını yakalayan bir pipeline yazdım. Pipeline **34,794 commit** taradı, diff'leri TruffleHog'a verdi, sonuçta **1,702 unique commit**'te **3,708 verified secret** çıktı. Verified demek: TruffleHog ilgili servisin API'sini çağırdı, key çalıştı.

İlginç kısım TruffleHog tarafı değil. TruffleHog çözülmüş bir problem. İlginç kısım TruffleHog'dan önce gelen filtre. Tam volume'da GH Archive günde milyonlarca event akıtıyor. Çoğu gürültü. Her diff'i TruffleHog ile taramaya bütçen yetmez, her commit mesajını AI'a sormaya da yetmez. İkisini de denedim.

Çalışan şey şu oldu: bir dil modelini regex bootstrap'lamak için kullanmak, sonra modeli büyük ölçüde bir kenara bırakmak.

![250k saatlik PushEvent'ten 1,702 unique commit ve verified canlı key'e]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/funnel.png)

## Problemin şekli

GH Archive, GitHub'daki her public aksiyon için JSON event stream yayınlıyor: push, PR, issue, star. Sızdırılmış secret açısından sadece iki event önemli: `PushEvent` (commit mesajları + SHA'lar) ve `PullRequestEvent` (PR title, ama sadece `opened` / `reopened` action'larında). Gerisi gürültü.

Normal bir saatte yaklaşık 250k `PushEvent` commit geliyor. Aradığım sinyal bunun çok küçük bir alt kümesi:

```text
remove api key
revoke aws credentials
delete .env
rotate token, pushed by mistake
fix: leaked secret
```

Naive `key|token|secret|password` grep'i sinyali yakalar ama "keyboard shortcut", "session token refresh", "password input component" gibi match'lerde boğulur. Detection oranı fena değil. Bir sonraki adımın repo'yu clone'lamak, diff'i çekmek ve üzerinde binary çalıştırmak olduğunu düşününce false positive oranı kullanılamaz hale geliyor.

Bir GH Archive saat dosyasındaki ilk naive run, Gemini free-tier quota'sını bir dakikadan kısa sürede yedi ve etiketlenmesi gereken saatlerce commit hâlâ duruyordu. AI-first ölçeklenmiyor. Grep-first çok şey yakalıyor. İkisi de kaybediyor.

## Faz 1: direkt LLM'e sor

İlk versiyon tembel versiyondu. Her commit mesajı tek seferlik bir prompt ile **Gemini 2.5 Flash Lite**'a gidiyordu:

```python
prompt = f"""
Analyze the following Git commit message to determine if it is fixing a secret leak.
The message might mention revoking, removing, or rotating keys, tokens, passwords, or other credentials.
Respond with only "true" if it is highly likely to be fixing a secret leak, otherwise respond with "false".

Commit Message:
---
{commit_message}
---
"""
```

Çalışıyor, evet. Flash Lite bu işte iyi ve cevap tek token. Ama her PushEvent mesajını API call'a sokmak ucuz tier'da bile pahalı ve yavaş, rate limit hızlıca yiyor.

O yüzden batch'ledim. Request başına 250 commit, JSON cevap, index ile sıralı:

```python
BASE_PROMPT_TEMPLATE = """
Analyze the following Git commit messages to determine if any of them are fixing a secret leak.
A message is considered to be fixing a secret leak if it mentions revoking, removing, or rotating keys, tokens, passwords, or credentials.
Return a JSON object where each key is the numeric index of the commit message (as a string) and the value is a boolean (`true` or `false`).
Ensure your response is only the JSON object.
"""
```

Batch yolu daha hızlı ve daha ucuzdu ama hâlâ AI çağrılarında bottleneck'liydi. Birkaç hafta çalıştırdıktan sonra iki log dosyası kocaman olmuştu:

```text
suspicious_commit_messages.log     59,014 satır
not_suspicious_commits.log        793,199 satır
```

Yaklaşık **852,000 mesaj Gemini tarafından etiketlenmiş** demek. Cache, tekrar eden API call'ları bedavaya getiriyordu ama yeni trafik para yakmaya devam ediyordu. O iki dosyada oturan asıl ders şuydu: etiketler rastgele değildi. Cluster yapıyorlardı.

## Faz 2: regex'i modelden çalmak

`suspicious_commit_messages.log` dosyasını sıralayıp gözden geçirince yapı kendini gösteriyor. True positive'lerin neredeyse tamamı bir verb artı bir noun. Verb'ler küçük bir küme:

```text
remove, delete, revoke, invalidate, rotate, regenerate, leak, expose, compromise, fix
```

Noun'lar daha büyük ama sınırlı. Birkaç generic kelime (`key`, `token`, `secret`, `credential`, `password`), sonra uzun bir brand-specific kuyruk: `aws`, `openai`, `slack`, `stripe`, `datadog`, `mongodb_uri`, `firebase`, gider gider. Her cloud, her SaaS, her CI sağlayıcı, her ödeme servisi.

Modele farklı bir şey sordum. "Bu mesajı sınıflandır" değil, **"sınıflandırırken kullandığın pattern'leri ver"**. Çıktı küçük bir grammar haline geldi.

İki tier'a böldüm. High-confidence tier'ı neredeyse her zaman secret hakkında olan verb + noun'lardan oluşuyor:

```python
HIGH_CONFIDENCE_ACTION_VERBS = [
    "remove", "delete", "revoke", "invalidate",
    "rotate", "regenerate", "leak", "expose",
    "compromise", "fix",
]

HIGH_CONFIDENCE_OBJECT_NOUNS = [
    "api_key", "apikey", "access_token", "auth_token",
    "private_key", "secret_key", "client_secret",
    "credential", "credentials", "password", "passwd",
    "aws_secret", "aws_access_key", ".env", "dotenv",
    # ...
]
```

Broad tier ise `update`, `change` gibi daha toleranslı verb'ler ve brand isimlerinin uzun kuyruğu. TruffleHog'un kendi detector kataloğundan çekilmiş yüzlerce keyword (`stripe`, `twilio`, `mailgun`, `sendgrid`, `slackbot`, `cloudflare`, `algolia` ve böyle devam ediyor) artı generic infra terimleri.

```python
BROAD_ACTION_VERBS = [
    "update", "change", "fix", "patch", "clean",
    "remove", "delete", "purge", "wipe", "scrub",
    # ...
]
```

Bunların üstüne üç tane compile edilmiş regex, klasik "secret leak ettim" mesaj şekillerini doğrudan yakalıyor:

```python
SECRET_REMOVAL_PATTERNS = [
    re.compile(r'\b(remove|delete|revoke|invalidate|rotate|regenerate)\b.*\b(key|token|secret|password|credential)\b', re.IGNORECASE),
    re.compile(r'\b(fix|patch)\b.*\b(leak|expose|compromise)\b', re.IGNORECASE),
    re.compile(r'\b(revert)\b.*for.*security.*reason', re.IGNORECASE),
]
```

İlk regex tek başına self-incriminating commit'lerin büyük bir kısmını yakalıyor. İnsanlar gerçekten commit mesajına `Remove leaked API key` yazıyor, her gün, public repo'da.

## Faz 3: önce regex, sıkışınca model

Grammar elimde olunca pipeline tersine döndü. Default path artık şu:

1. `SECRET_REMOVAL_PATTERNS` uygula. Match varsa commit'i flag'le.
2. Tier 1'i uygula (high-confidence verb VE high-confidence noun aynı mesajda). Match varsa flag.
3. Tier 2'yi uygula (broad verb VE broad noun). Match varsa flag.
4. Sadece mesaj ambiguous ise ve iki cache'te de yoksa Gemini'ye fallback.

Yani regex-first, AI-fallback. AI artık hot path'te değil. Long tail için tiebreaker oldu.

Planlamadığım iki yan fayda çıktı:

- Pipeline **deterministic ve replayable** oldu. Aynı input, aynı output, runlar arasında API drift'i yok.
- Pipeline **offline çalışacak kadar ucuz** oldu. Quota yok, rate limit yok, network yok. Tek network I/O'su diff'i çekmek ve TruffleHog çalıştırmak.

Regex'in kaçırdığı her şey hâlâ Gemini'ye gidiyor, Gemini'nin bulduğu her yeni true positive bir sonraki regex update'i için aday oluyor. Model artık filtreyi train ediyor, filtreyi çalıştırmıyor.

## TruffleHog ne buldu

Bir commit mesajı filtreden geçtikten sonra pipeline diff'i GitHub'dan çekiyor ve TruffleHog'u verification açık olarak çalıştırıyor. Her "verified" sonuç, TruffleHog'un ilgili servisin API'sini çağırdığı ve key'in çalıştığı anlamına geliyor.

Bir batch run'dan final rakamlar:

```text
Filtreden sonra taranan PushEvent commit'leri   34,794
Match eden unique commit mesajı (keyword)          197
Verified secret kayıt sayısı                      3,708
Verified secret içeren unique commit             1,702
```

Bir mesaj sık sık yüzlerce commit'e karşılık geliyor: binlerce farklı developer aynen `remove api key` yazıyor, hepsi pull ediliyor. Yani 197 unique mesaj 34,794 commit'e açıldı.

Yani **commit başına yaklaşık %4.9 verified-leak oranı**. İçindeki dominant detector'lar her zamanki şüpheliler: cloud provider key'leri, SaaS API key'leri, embedded credential içeren database connection string'leri. Kimse fark etmeden faturaya dönüşen şeyler.

![Verified secret sayısına göre en üst detector'lar]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/top-detectors.png)

"GitHub'a leak" ile "exploit" arasındaki süreyi merak ediyorsanız, Mackenzie Jackson ve Andrzej Dyjak 2020'de canary deneyini yapmıştı: [canarytokens.org](https://canarytokens.org) ile üretilmiş bir AWS key, public repo'ya push'landı, push'tan **11 dakika sonra** ilk kez kötüye kullanıldı ([What actually happens when you leak credentials on GitHub](https://dev.to/advocatemack/what-actually-happens-when-you-leak-credentials-on-github-the-experiment-34md)). Benim pipeline GH Archive'a karşı 5 ila 15 dakika lag ile çalışıyor. Saldırgan benden daha yavaş değil.

## Kod yazıyorsan bu ne anlama geliyor

Commit mesajları, bir developer'ın kendi credential hatası hakkında ürettiği en yüksek sinyal. Public bir commit mesajına `remove leaked api key`, `revoke aws` veya `fix exposed token` yazdıysan, biri seni çoktan grep'ledi. Push ile exploit arasındaki window saat değil, dakika. Key'i rotate etmek zorunlu, ilk push hiç olmamış gibi davranmak strateji değil.

Pre-commit secret scanning bu yarışı gerçekten kazanan tek önlem. `git push` sonrasında çalışan her şey geç kalmış sayılır. Aynı problemi diğer taraftan göstermek gerekirse: TruffleHog pre-commit hook'u, sahte bir AWS key'i gerçek zamanlı blokluyor.

![TruffleHog pre-commit hook, AWS access key'i commit öncesi blokluyor]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/trufflehog-pre-commit.gif)

10 satır YAML, commit süresine belki bir saniye ekliyor. Güvenlik tooling'inde en ucuz sigorta bu.

Bir güvenlik pipeline'ında language model'i nasıl kullanmak gerektiği konusunda ise: bootstrap için kullan, operate etmek için değil. Unstructured text'te pattern fark etmek konusunda çok iyiler, ölçekte hot path olmak konusunda çok kötüler. Modelin çıktısını mine et, altta yatan kuralı kod olarak yaz ve modeli bir sonraki pattern discovery turuna kadar yedek kulübesine al.

## İlgili içerik

- [GitHub'a leak ettiğinde gerçekte ne oluyor (Mackenzie Jackson)](https://dev.to/advocatemack/what-actually-happens-when-you-leak-credentials-on-github-the-experiment-34md)
- [GH Archive](https://www.gharchive.org/)
- [TruffleHog](https://github.com/trufflesecurity/trufflehog)
- [TruffleHog pre-commit hooks docs](https://trufflesecurity.com/docs/pre-commit-hooks)

## Referanslar

- [GH Archive event reference](https://docs.github.com/en/webhooks-and-events/events/github-event-types)
- [TruffleHog detector kataloğu](https://github.com/trufflesecurity/trufflehog/tree/main/pkg/detectors)
- [Canary Tokens (Thinkst)](https://canarytokens.org/)
- CWE-798: Use of Hard-coded Credentials
- CWE-540: Inclusion of Sensitive Information in Source Code
