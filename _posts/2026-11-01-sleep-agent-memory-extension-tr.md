---
layout: post
title: "Sleep: Kodlama Ajanlarına Onaylanabilir Bellek"
lang: tr
author: Yunus Aydın
description: "Sleep, kodlama ajanlarına review edilebilir bir bellek döngüsü ekliyor. Gözlemleri yakalar, provisional memory'ye konsolide eder, kalıcı olanı sadece insan onayıyla promote eder. T0'dan T1'e, T1'den T2'ye."
keywords: "ajan belleği, kodlama ajanı, LLM context window, provisional memory, Claude Code hooks, Pi extension, bellek konsolidasyonu, prompt injection, ajan otomasyonu"
canonical_url: "https://aydinnyunus.github.io/2026/11/01/sleep-agent-memory-extension-tr/"
---

Sleep'i kurdum çünkü ajanlarımın öğrendiği en değerli şeyleri sürekli kaybetmekten bıktım. Her oturumda bir asistana staging gateway'ini, isimlendirme kurallarını, hangi reponun önce patladığını öğretmek için birkaç saat harcıyorum. Sonra context window kapanıyor, hiçbir şey kalmıyor. Ben tam tersini istedim: neyin işe yaradığını hatırlayan ajanlar, kimliğini kendisi yazan ajanlar değil. Sleep, Pi, OmO ve Claude Code'a bu bellek döngüsünü ekleyen açık kaynaklı bir araç.

İsim boşuna değil. Google Research, LLM'lerde sürekli öğrenme için beynin uyanık/uyku döngüsünü ödünç alan bir framework yayınladı. Onların fikri, sleep fazını biriktirilmiş bilgiyi eski sürümleri bozmadan sıkıştıran ayrı bir offline job olarak ayırmaktı. Bu pattern bana fazlasıyla pratik geldi, ama ölçek farkı bütün tasarımı değiştiriyor. Onların sleep fazı model weight'leri üzerinde çalışır. Benimki session buffer'larında döner ve hiçbir weight'e dokunmaz. Onlar bilgiyi bir planla sıkıştırır. Ben onu insan onayının arkasına alıp replay ile ölçerim.

## Boşluk: bellek sadece context window'un içinde

Bir LLM tabanlı ajanda öğrenmeyi istediğinizde iki yol var ve ikisi de hatalı. Continuous training ile parametreleri güncellersiniz, derken catastrophe forgetting'e çarparsınız: yeni task'ler gelir, eski performans çöker. Ya da in-context learning'e güvenirsiniz, o zaman öğrettiğiniz her şey sadece o window'un ömrü kadar yaşar, oturum kapanınca da ölür.

İkisi de kodlama ajanı için kötü haberdi. Sizin için değerli olan bilgi, genel bir training run'un göremeyeceği türden şeylerdir: private bir staging gateway, bir proje kuralı. Tam da context window'un her çağrıda attığı türden.

Yani mesele "modelimi nasıl daha akıllı yapayım" değil. Problem şu: "bu ajanın projem hakkında öğrendiklerini nasıl kalıcı yaparım, ama kendi belleğini kurmasına güvenmeden." Bunlar farklı hedefler. Çoğu agent-memory aracı tam bu noktada yanılıyor.

## Tasarım: üç katman, tek insan kapısı

Sleep bellek işini üç katmana ayırıyor. Sadece biri gerçekten kalıcı.

| Katman | Ne olduğu | Kim yazıyor |
|---|---|---|
| T0 | Buffer'daki geçici oturum gözlemi | Hook'lar, extension ya da manuel capture |
| T1 | Provisional memory, unverified background olarak gösterilir | Konsolidasyon ve tekrar eden tercih madenciliği |
| T2 | Kalıcı bellek | Sadece sizin açık `promote` komutunuz |

Ana garanti bu T1/T2 sınırında. Bir bellek ajan 'bunu hatırla' dedi diye kalıcı olmaz. Ancak incelediğiniz bir candidate'ı `promote` ettiğinizde kalıcı olur. Modelin ürettiği her şey provisional kalır; kendi çıktısını kullanıcı onaylıymış gibi işaretleyemez.

Günlük akış şöyle:

```text
oturum aktivitesi -> T0 buffer -> gated sleep -> T1 provisional memory
                                                     |
                                              review ya da replay
                                                     |
                                              explicit promote
                                                     |
                                              T2 kalıcı bellek
```

## Nasıl çalışıyor

Pi ve OmO'da Sleep bir extension olarak kuruluyor, agent loop'una yükleniyor ve üç mekanik iş yapıyor.

1. **Capture.** İnsanın kendi sözleri ve path taşıyan tool event'leri T0 buffer'ına taşınır.
2. **Publish.** Eşleşen provisional memory her request'te modelin önüne enjekte edilir, bir context compaction'ın ardından baştan okunur.
3. **Wake.** Agent settle olunca policy gate üzerinden konsolidasyonu tetikler. Model çağrısı loop'u asla bloklamaz.

Claude Code'da aynı işler `UserPromptSubmit`, `PostToolUse`, `Stop` ve `PreCompact` hook'ları üzerinden döner. Her iki entegrasyon da `$SLEEP_HOME` paylaşır, yani bir client'ta review ettiğiniz candidate, proje scope'u eşleştiğinde diğerinden de görünür.

Konsolidasyonun kendisi kapılı. Varsayılan kapı en az 3 yakalanmış kayıt ister ve bir önceki pass'ten 6 saat geçmesini bekler. Yani her mesajdan sonra model çağrısı yapılmaz. Karar önce sizin önünüze düşer:

```bash
./sleep.py status -v
./sleep.py sleep --dry-run
./sleep.py sleep --force
```

Status çıktısı bütün tasarımı tek ekranda toplar. Kurulu bir sistemde `status -v` şunu raporlar:

```text
sleep memory: T0=disposable captures, T1=provisional awaiting review, T2=permanent (promote-only)
T0 buffer      53 records / 4 files / 20584 bytes  (captured while you worked)
T1 candidates  6 provisional, 7 total  (distilled, shown as unverified context)
   quarantine  0 parked (needs human eyes)  (held out until you review)
T2 canonical   ~/.claude-work/.../memory  (96 permanent records; only you can promote)
```

İki satır bütün tasarımı taşıyor. T0 atılabilir: çalışma capture'i, kaybolsa zarar düşük. T1 açıkça unverified context, asla talimat veremez. T2 tek kalıcı katman ve sadece sizin promote edebileceğinizi söylüyor. Quarantine satırı ise görünür hale gelmiş güvenlik ağı.

Sonra çıkan candidate'ı T2'ye geçirmeden önce incelersiniz:

```bash
./sleep.py show <candidate-id>
./sleep.py promote <candidate-id> --dry-run
./sleep.py promote <candidate-id>
```

Yanlış olanı reddederseniz bu bir silme değildir; audit trail bırakır:

```bash
./sleep.py refuse <candidate-id> --note "Incorrect for the current deployment."
```

## Dream döngüsü: mining, konsolidasyon, replay

Sleep'in gece çalışan, gözetimsiz bir pass'i de var; buna dream döngüsü adını verdim. Hiç weight güncellemez ve kendi başına kalıcı bellek yazmaz. Üç şey yapar.

**Mine.** Farklı oturumlarda aynı projede tekrar tekrar verdiğiniz talimatları bulur. Bu adım deterministik, model çağrısı yapmaz.

**Consolidate.** Sleep gate'i geçince, ayarladığınız model yeni T0 gözlemlerini provisional T1 candidate'larına çevirir. Her çağrının bütçesi baştan bellidir.

**Replay.** Sırası gelince seçilen task'leri provisional memory açıkken ve kapalıyken çalıştırır, sonra bir karşılaştırma raporu yazar. En sevdiğim parça bu: bir bellek işe yarıyor mu diye tahmin etmenizi istemek yerine, gerçek bir task üzerinde etkisini ölçer.

```bash
./sleep.py nightly --dry-run
./sleep.py mine
./sleep.py replay --run --limit 3 --budget 0.10
```

Replay harcamadan önce neyi çalıştıracağını ve her task'ın maliyetini gösterir. Kurulu bir sistemde plan şöyle görünür:

```text
dry run: would execute 3 task(s) x2 conditions (read-only tools, $0.50/task). Pass --run to execute.
  staging-gateway   Records the X-Route header the staging ALB requires.
  mcp-creds         Notes which servers need literal credentials, not env expansion.
  weight-field      Confirms the weight field is silently dropped for this project.
```

Replay sonucu raporlar ama asla promote etmez. `--budget` ise task başına maliyete bir üst sınır koyar.

## Herkesin sorduğu güvenlik

Ajana bellek vermek bir prompt injection yüzeyi açıyor; bu gücü körü körüne vermek istemedim. Pipeline'da somut kapılar var.

- **Redaction.** Secret'e benzeyen değerler hiçbir katmana ulaşmaz. Credential'larınız env'de ya da vault'ta kalır.
- **Quarantine.** Talimat şeklindeki ya da injection'a benzeyen metin asla candidate olamaz. Nedeniyle kenara alınır ve raporlanır.
- **Provenance trust.** Sadece `user` kaynaklı metin talimat gibi ifade edilebilir. Agent ve inferred kayıtlar her zaman context olarak render edilir, asla talimat olarak değil.
- **Proje izolasyonu.** Bir projenin provisional belleği başka bir projeye sızmaz; karışık buffer'lar yanlış yere bilgi tıkmak yerine fail-closed olur.
- **Açık bütçe.** Pipeline'daki her model çağrısının maliyet tavanı var.

Tehdit modeli konusunda dürüst olmak gerek: bu bir filter, sandbox değil. Garantisi net olan tek şey şu: kalıcı bellek insan onayının arkasında kalır ve makine üretimi prompt'lar sizin yerinize geçemez.

## Bellek yeniden kullanıldığında ölçülür

Bu aracı "transcript kaydet" tarzı saf işlerden ayıran şey bilinçli bir tercih. Çoğu agent-memory projesi context'i kapar, bir sonraki prompt'a doldurur, bitti der. Kör nokta da şu: belleğin gerçekten işe yarayıp yaramadığını asla öğrenmezsiniz.

Sleep bu döngüyü replay ile kapatıyor. Aynı task'i provisional memory açıkken ve kapalıyken çalıştırırsınız, imanla verilen karar yerine bir karşılaştırma alırsınız. Sonuç, candidate'ı T2'ye promote edip etmeyeceğinizi belirler. Bir ajanın çöp biriktirmesi ile yargı biriktirmesi arasındaki fark budur.

Daha derinde yatan şey şu: bu bir safety property, lüks değil. Test edilmemiş bellek demek, ajanın istemediğiniz şeyleri sessizce yapmaya başlaması demek. Ölçülmüş bellek ise neyin gerçekten değiştiğini söyleyebilmek.

## Neden ajana güvenmek yerine böyle kurdum

Yeterince uzun ajan oturumları çalıştırdım, modelin kendi kalıcı state'ini kurmasına izin veren her tasarıma şüpheliyim. Agent'ın yakaladığı ne varsa saklama eğilimi var; o capture yanlıştır, gürültüdür ya da bir prompt injection'un ürünüdür, fark etmez. O yüzden tasarımın kutsal kısmı insan review adımı. Ben onu otomatikleştirmeyi reddediyorum. Uptime hedef değil. Bir candidate'a bakıp hayır diyebilmek ve o hayrın audit trail ile kalıcı olması, aracın var olma sebebi.

Belleği review edilebilir tutmak, ajanları aynı anda hem daha faydalı hem daha az riskli yapan nadir bir değişiklik. Faydalı, çünkü saatlerce aktardığınız bilgi oturumlar arasında buharlaşmıyor. Daha az riskli, çünkü siz olmadan hiçbir şey kalıcı olmuyor.

## Deneyin

```bash
git clone https://github.com/aydinnyunus/sleep.git
cd sleep
./install.sh                 # shared skill ve Claude Code hook'ları
./install.sh --extension     # ayrıca Pi/OmO extension'ı
./sleep.py sleep --force     # bir konsolidasyon pass'i deneyin
```

Installer önce test suite'ini çalıştırır, config'i değiştirmeden önce backup alır ve tekrar çalıştırmak güvenlidir. Dry run tek dosya yazmadan her değişikliği gösterir: `./install.sh --dry-run`.

Ajan belleğini benimserken üç sabit noktayı da alın. Kalıcı bellek açık bir insan onay adımı ister, buffer'dan yenileme bir gate ister, yeniden kullanım bir ölçüm ister. Bunlardan birini atlarsanız güvenemeyeceğiniz bellek elde edersiniz, ki o hiç bellek olmamasından daha kötüdür.

## Referanslar

- [Sleep repository](https://github.com/aydinnyunus/sleep)
- [The sleep framework: A new way for LLMs to retain knowledge in continual learning (Google Research)](https://research.google/blog/the-sleep-framework-a-new-way-for-llms-to-retain-knowledge-in-continual-learning/)
- [The sleep framework paper (arXiv)](https://arxiv.org/abs/2606.03979)

## İlgili içerik

- [AI CAPTCHA Bypass: Solving reCAPTCHA with GPT-4o and Gemini](/2025/12/08/ai-captcha-bypass/)
- [PassDetective: Detecting Passwords and Secrets in Your Shell History](/2025/12/14/passdetective-kali-linux/)
