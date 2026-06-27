---
layout: post
title: "AI Az Önce Bir Secret Sızdırdı"
author: Yunus Aydın
date: 2026-06-30
lang: tr
description: "Microsoft, Google, Red Hat, Grafana ve LlamaIndex public GitHub repolarına canlı verified credential pushladı. Pipeline'ı ben yazdım: Gemini 2.5 ile etiketle, regex'e distil et, TruffleHog'u active verification ile koş. 3,830+ verified secret, 1,443 unique repo, %95 rotation."
keywords: "github archive, secret scanning, trufflehog, gemini, gh archive, sızdırılmış credential, secret detection, ai ile regex, microsoft leak, google leak, red hat leak, weights and biases leak, vibe coding güvenlik, commit mesaj analizi, push event scanner"
canonical_url: "https://aydinnyunus.github.io/2026/06/30/hunting-leaked-secrets-on-github-archive-tr/"
---

Public GitHub repolarında **Microsoft, Google, Red Hat, Grafana ve LlamaIndex**'ten canlı, verified credential sızdığını buldum. Eski, atılmış, çürümüş key'ler değil. Bulduğum anda hâlâ ilgili API'ye karşı authenticate eden token'lar. Microsoft Research'ün [`microsoft/maro`](https://github.com/microsoft/maro) repo'su `@microsoft.com` hesabından çalışan bir Weights & Biases token'ı pushlamış. Bir Google çalışanı WandB token'ı sızdırmış; token authenticate ediyor ve ardında Google'ın halka açıklamadığı `gemma_nemo_sft` adlı model dahil 10 internal proje ortaya çıkıyor. Başka bir `@google.com` hesabı production DNS altyapısında zone admin yetkisi olan bir Cloudflare token'ı pushlamış. Red Hat, Grafana, LlamaIndex, BerriAI, Intel: aynı pattern, başka repo.

Bunları bulan pipeline'ı ben yazdım. **GitHub Archive** firehose'unu izliyor, developer'ın yeni leak ettiği credential'lara benzeyen commit mesajlarını yakalıyor, diff'i çekiyor ve TruffleHog'u verification açık olarak koşuyor. Birkaç ayda **1,443 unique repo'da 3,830+ verified canlı secret** çıkardı. Doğrudan iletişime geçtikten sonra **%95'inden fazlası iki hafta içinde rotate edildi**.

![Etkilenen maintainer'lardan gelen disclosure cevapları: "thanks, I revoked and removed it", "fix api key for Cloudinary", "Removed Hard Coded API Key", "SECURITY FIX: Remove hardcoded MongoDB credentials"]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/disclosure-replies.jpg)

Bu sayının her ay daha kötüye gitmesinin sebebi bir isme sahip ve tahmin etmek zor değil. Developer'lar AI editor'larıyla daha hızlı kod yazıyor, security review küçülüyor, Cursor'ın ürettiği `config.py`'nin 42'nci satırındaki secret kimse diff'i okumadan public commit'e düşüyor. AI az önce bir secret sızdırdı. Daha doğrusu: sızdırmana yardım etti. Saldırganlar GitHub'da credential'ı tek haneli dakikalar içinde index'liyor.

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

## Faz 4: active verification, sadece detection değil

Bir key şekline yapılan pattern match gürültüdür. Çalışan bir API call ise bulgudur. TruffleHog'un her detector'u bir verifier fonksiyonu ile geliyor: aday string'i al, ilgili servisin authentication request'ini kur, cevabı gözle. AWS access key'ler `sts:GetCallerIdentity`'ye çarpıyor. GitHub PAT'lar `/user`'a. Postgres URI'leri TCP connection açıp `SELECT 1` çekiyor. WandB token'lar `/graphql`'e gidip kullanıcı kimliğini resolve ediyor.

False positive sorununu öldüren ve "ilginç görünen string"i "şu anda credentialed access"e çeviren şey verification. Benim dataset'imde filtrenin TruffleHog'a yolladığı yaklaşık **42,500 aday string'ten 3,708'i verified döndü** (yani **%8.7 verify oranı**). Geri kalan her şey test fixture, expired token, scramble edilmiş örnek ya da bir key şekline rastgele uyan non-credential string'ti.

![Filtre çıktısı, TruffleHog'a giden aday string sayısı ve verified secret oranı: ~42,500 adayın 3,708'i verified, %8.7 verify oranı]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/verification-rate.jpg)

Verification ayrıca bir sonraki adımı açıyor: enumeration. Verified bir WandB token sadece "bu gerçek" demiyor; GraphQL şemasını yürümene, kullanıcının proje, run ve takım üyelerini listelemene izin veriyor. Aşağıdaki Microsoft ve Google vakalarının "leak"ten "blast radius haritalanmış"a dakikalar içinde geçmesinin yolu buydu.

## Dikkat çeken kurbanlar

Dataset'ten küçük bir seçki. Hepsi raporlandı, kurum tarafından doğrulandı ve rotate edildi. Leak'ler kötü niyetli değildi. Velocity'dendi.

### Microsoft Research, MARO repo'su, Weights & Biases token

[`microsoft/maro`](https://github.com/microsoft/maro) Microsoft Research'ün multi-agent reinforcement learning kütüphanesi, 885 yıldız. `@microsoft.com` hesabından bir çalışan çalışan bir W&B API token'ı commit'lemiş. Token authenticate ediyor. W&B GraphQL endpoint'i kullanıcı object'ini döndürüyor: tam isim, kurumsal email, takım, hesabın eriştiği training run'ların listesi. Microsoft VRP üzerinden raporlandı, 3 gün içinde rotate edildi.

![microsoft/maro repo'sundaki commit diff'i: hardcoded bir WANDB_API_KEY içeren config dosyası eklenmiş]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/microsoft-maro-wandb-diff.jpg)

### Google çalışanı, Weights & Biases token, on internal proje

`@google.com` hesabından bir çalışan WandB token'ı sızdırmış. Token ile authenticate edince Google'ın halka açıklamadığı eğitim altyapısı dahil 10 internal ML projesi ortaya çıkıyor. Proje isimlerinden tek bir tanesi bile başlı başına bir bulgu: halka açıklanmamış `gemma_nemo_sft` modeli. Diğerleri: `multimodal-function-calling`, `pytorch-sweeps`, `data-science-agent`. Raporlandı, 7 gün içinde rotate edildi.

![Google çalışanının `data-science-agent` projesindeki commit diff'i: hardcoded WANDB_API_KEY ortaya 10 internal Google ML projesini çıkarıyor]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/google-data-science-agent-wandb-diff.jpg)

### Google hesabı, Cloudflare token, zone admin

Farklı bir `@google.com` hesabı, farklı bir leak: production DNS altyapısında zone admin yetkisi olan bir Cloudflare API token'ı. Bu token ile attack chain kısa ve çirkin: DNS record'larını değiştir, trafiği yönlendir, edge'de MITM. Google VRP üzerinden raporlandı, 48 saat içinde rotate edildi.

### Listenin geri kalanı

`grafana/grafana` (69,202 yıldız) çalışan bir AWS access key pushlamış. `run-llama/llama_index` (43,358 yıldız) ve `BerriAI/litellm` (26,438 yıldız), ikisi de AI/LLM ekosisteminin core altyapısı, kendi servis token'larını sızdırmış. `intel/BigDL` (2,680 yıldız), Red Hat repoları, birden fazla LlamaIndex alt-projesi. Bu listedeki her isim, yetkin bir security team'in çalıştığı yer. Leak yine de oldu.

## TruffleHog ne buldu, toplamda

Bir commit mesajı filtreden geçtikten sonra pipeline diff'i GitHub'dan çekiyor ve TruffleHog'u verification açık olarak çalıştırıyor. Bir batch run'dan final rakamlar:

```text
Filtreden sonra taranan PushEvent commit'leri   34,794
Match eden unique commit mesajı (keyword)          197
Verified secret kayıt sayısı                      3,708
Verified secret içeren unique commit             1,702
```

Bir mesaj sık sık yüzlerce commit'e karşılık geliyor: binlerce farklı developer aynen `remove api key` yazıyor, hepsi pull ediliyor. Yani 197 unique mesaj 34,794 commit'e açıldı.

Detector dağılımı asıl işe yarayan grafik. Database credential'ları başı çekiyor (Postgres, MongoDB, blockchain RPC URI'leri), arkasından Telegram bot token'ları, HuggingFace token'ları, Vercel deploy token'ları ve AI altyapısının uzun kuyruğu: DeepSeek, Groq, Weights & Biases, ElevenLabs.

![Verified secret sayısına göre en üst detector'lar]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/top-detectors.png)

"GitHub'a leak" ile "exploit" arasındaki süreyi merak ediyorsanız, Mackenzie Jackson ve Andrzej Dyjak 2020'de canary deneyini yapmıştı: [canarytokens.org](https://canarytokens.org) ile üretilmiş bir AWS key, public repo'ya push'landı, push'tan **11 dakika sonra** ilk kez kötüye kullanıldı ([What actually happens when you leak credentials on GitHub](https://dev.to/advocatemack/what-actually-happens-when-you-leak-credentials-on-github-the-experiment-34md)). Benim pipeline GH Archive'a karşı 5 ila 15 dakika lag ile çalışıyor. Saldırgan benden daha yavaş değil ve artık yazan da insan değil.

![Mackenzie Jackson'ın canary experiment'i: AWS key public repo'ya push'landıktan 11 dakika sonra ilk kötüye kullanım]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/canary-timing.jpg)

## AI tarafı: bu neden gittikçe kötüleşiyor

İki gerçek yan yana:

1. AI asistanları kodu, insanın review edebileceğinden hızlı yazıyor. Bileşik etki şu: developer başına günde üretilen satır artıyor, `git commit`'e giden yolda satır başına düşen göz sayısı aynı kalıyor. Generated boilerplate'in içine düşmüş hardcoded bir key'in review'da yakalanma olasılığı, autocomplete kabul oranı arttıkça monoton olarak düşüyor.

2. LLM'ler GitHub üzerine train edildi. Modelin autocomplete ettiği pattern'ler zaten shipped olan pattern'lerden alındı, leak olanlar dahil. Bir asistana "bana S3'e upload yapan hızlı bir script yaz" de, scaffolding'in içine `AWS_ACCESS_KEY_ID = "..."` placeholder'ı ile `# replace with your key` yorumunu koyma olasılığı azımsanmayacak kadar yüksek. Developer bazen değiştiriyor. Bazen oraya gerçek key'i yapıştırıp save'liyor ve commit'liyor.

![AI tarafından üretilmiş kodda hardcoded DeepSeek API key'i: yorumda "replace with your key" placeholder'ı bırakılmış ama gerçek key commit'lenmiş]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/ai-coded-deepseek-leak.jpg)

Defender'lar da AI kullanıyor. TruffleHog detector'ları artık context-aware classifier'lar ile tarihsel regex false positive'lerini azaltıyor. LLM scanner'lar near-miss obfuscation'larda düz regex'i geçiyor. Yani bu cidden bir arms race ve default'u kötü olan taraf daha hızlı kaybediyor.

## Remediation: önce rotate, sonra temizle

Bir key public'e düştükten sonra hangi adımı önce yaptığın, vakayı "gömdüm gitti"den incident'a çeviriyor. Önce issuer'da rotate et, dosyada değil. Eski credential'ı revoke et, yenisini mint'le, başka hiçbir şeye dokunma. Sonra sızan key'in access log'unu baştan sona oku, tanımadığın IP ve user agent'ları ara, saldırganın alert'inden önce geldiğini varsay.

Credential öldükten sonra git history'i temizle. Düz bir `git rm` ve yeni bir commit key'i sadece bir SHA'nın arkasına saklıyor; orijinal hâlâ erişilebilir ve fork ve clone'lar dakikalar içinde alıyor. Doğru fix [`git-filter-repo`](https://github.com/newren/git-filter-repo) ile tam bir history rewrite:

```bash
# Dosyayı tüm history'den kaldır
git-filter-repo --sensitive-data-removal --invert-paths --path config/secrets.yml

# Veya history boyunca belirli value'ları değiştir
echo 'sk-deadbeef0123456789' >> ../passwords.txt
git-filter-repo --sensitive-data-removal --replace-text ../passwords.txt
```

Force-push, ardından GitHub Support'a yaz ve cached commit URL'lerini, diff view'larını ve PR cache'lerini purge etmelerini iste. Aksi halde credential API üzerinden saatlerce hâlâ aratılabiliyor.

"Hangi sağlayıcı için hangi runbook'u takip edeyim" sorusu için [howtorotate.com/docs/tutorials](https://howtorotate.com/docs/tutorials) AWS, GCP, GitHub PAT, Stripe, Postgres ve benim dataset'imdeki issuer'ların çoğu için adım adım rotation rehberi tutuyor. 2026-06 itibarıyla en aktif tuttuğu sağlayıcı setiyle bulduğum en kullanışlı starter pack.

## Rotation: insanın hatırlamasına bel bağlama

Manuel rotation on secret için çalışır, binde devrilir. İlk leak'inden sağ çıkan takımlar aynı üç alışkanlıkta buluşuyor:

- **Default olarak short-lived token**: CI için OIDC federation (GitHub Actions `id-token: write`, AWS `AssumeRoleWithWebIdentity`), cloud workload'lar için workload identity, insan erişimi için 15 dakika - 1 saat TTL. 15 dakikalık token index'lendiğinde zaten non-event.
- **Mümkünse credential-free mimari**: access key yerine IAM role, static token yerine service account, servisler arası mTLS. Var olmayan credential leak olmaz.
- **Kalan her long-lived credential için secrets manager ve 90 günlük rotation**: Vault, AWS Secrets Manager, Doppler, GCP Secret Manager. Seçim, repo'daki `.env` dosyaları çağını bitirmekten daha az önemli. Cadence load-bearing kontrol.

Bunlardan kaçanı yakalayan defense katmanları: pre-commit (TruffleHog, gitleaks, git-secrets) developer laptop'unda blokluyor; aynı scanner'lar CI'da `--no-verify` bypass'ını yakalıyor; nightly full-history scan'ler hook'lardan önce commit'lenmiş legacy key'leri yakalıyor; [GitHub Secret Scanning](https://docs.github.com/en/code-security/secret-scanning) her public repo'da ~130 partner pattern'e karşı çalışıyor ve gerçek bir key'i sen rotation mesajını yazmadan auto-revoke ediyor. AI editor için [Wiz Secure Rules](https://github.com/wiz-sec-public/secure-rules-files) Cursor, Copilot, Cline ve Windsurf'e generation anında hardcoded credential reddetmeyi öğretiyor; CWE-798'i kaynağında düzeltmeye en yakın şey.

## Kod yazıyorsan bu ne anlama geliyor

Commit mesajları, bir developer'ın kendi credential hatası hakkında ürettiği en yüksek sinyal. Public bir commit mesajına `remove leaked api key`, `revoke aws` veya `fix exposed token` yazdıysan, biri seni çoktan grep'ledi. Push ile exploit arasındaki window saat değil, dakika. Key'i rotate etmek zorunlu, ilk push hiç olmamış gibi davranmak strateji değil.

Pre-commit secret scanning bu yarışı gerçekten kazanan tek önlem. `git push` sonrasında çalışan her şey geç kalmış sayılır. Aynı problemi diğer taraftan göstermek gerekirse: TruffleHog pre-commit hook'u, sahte bir AWS key'i gerçek zamanlı blokluyor.

![TruffleHog pre-commit hook, AWS access key'i commit öncesi blokluyor]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/trufflehog-pre-commit.gif)

10 satır YAML, commit süresine belki bir saniye ekliyor. Güvenlik tooling'inde en ucuz sigorta bu.

```yaml
# .pre-commit-config.yaml
repos:
  - repo: https://github.com/trufflesecurity/trufflehog
    rev: v3.82.0
    hooks:
      - id: trufflehog
        args: ['git', 'file://.', '--since-commit', 'HEAD', '--only-verified', '--fail']
```

Bir güvenlik pipeline'ında language model'i nasıl kullanmak gerektiği konusunda ise: bootstrap için kullan, operate etmek için değil. Unstructured text'te pattern fark etmek konusunda çok iyiler, ölçekte hot path olmak konusunda çok kötüler. Modelin çıktısını mine et, altta yatan kuralı kod olarak yaz ve modeli bir sonraki pattern discovery turuna kadar yedek kulübesine al.

Takımın production kodunu AI ile yazıyorsa, bir sonraki commit'in credential içereceğini varsay ve buna göre tasarla. Pre-commit hook'lar. Short-lived token'lar. Gerçekten provada geçirdiğin bir `git history` rewrite planı. Yukarıdaki Microsoft ve Google örnekleri sadece dışarıdan birisi bulup raporladığı için yakalandı ve rotate edildi. Disclosure'dan üç ay sonra etkilenen hesapları tekrar kontrol ettim: rotation oranı %95'in üstünde kaldı ama her beş repodan birinden azı bir sonraki leak'i durdurmak için pre-commit hook kurmuştu. Bir sonrakisi muhtemelen şu an birinin training dataset'inde.

## İlgili içerik

- [GitHub'a leak ettiğinde gerçekte ne oluyor (Mackenzie Jackson)](https://dev.to/advocatemack/what-actually-happens-when-you-leak-credentials-on-github-the-experiment-34md)
- [GH Archive](https://www.gharchive.org/)
- [TruffleHog](https://github.com/trufflesecurity/trufflehog)
- [TruffleHog pre-commit hooks docs](https://trufflesecurity.com/docs/pre-commit-hooks)
- [Wiz Secure Rules: AI ile üretilmiş kod için AI ile üretilmiş güvenlik kuralları](https://github.com/wiz-sec-public/secure-rules-files)

## Referanslar

- [GH Archive event reference](https://docs.github.com/en/webhooks-and-events/events/github-event-types)
- [TruffleHog detector kataloğu](https://github.com/trufflesecurity/trufflehog/tree/main/pkg/detectors)
- [Canary Tokens (Thinkst)](https://canarytokens.org/)
- [GitHub: Repo'dan hassas veri kaldırma](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository)
- [howtorotate.com rotation rehberleri](https://howtorotate.com/docs/tutorials)
- CWE-798: Use of Hard-coded Credentials
- CWE-540: Inclusion of Sensitive Information in Source Code
