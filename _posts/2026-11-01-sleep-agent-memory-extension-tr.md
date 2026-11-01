---
layout: post
title: "Sleep: Kodlama Ajanlarına Onaylanabilir Bellek"
lang: tr
author: Yunus Aydın
description: "Sleep, kodlama ajanlarına gözden geçirilebilir bir bellek döngüsü kuruyor. Gözlemleri yakalar, geçici bellekte konsolide eder, kalıcı olanı sadece insan onayıyla sabitler. T0'dan T1'e, T1'den T2'ye."
keywords: "ajan belleği, kodlama ajanı, LLM context window, provisional memory, Claude Code hooks, Pi extension, bellek konsolidasyonu, prompt injection, ajan otomasyonu"
canonical_url: "https://aydinnyunus.github.io/2026/11/01/sleep-agent-memory-extension-tr/"
---

Sleep'i kurma sebebim, ajanlarımın öğrendiği en değerli şeyleri sürekli kaybetmemdi. Her oturumda bir asistana staging gateway'in ne olduğunu, isimlendirme kurallarını, hangi reponun en önce patladığını öğretmek için saatlerimi harcardım. Sonra context window kapanır, hiçbir şey kalmazdı. İstediğim tam tersiydi: neyin işe yaradığını hatırlayan ajanlar, kimliğini kendisi belirleyenler değil. Sleep, Pi, OmO ve Claude Code'a bu bellek döngüsünü ekleyen açık kaynaklı bir araç.

İsmi boşuna değil. Google Research, LLM'lerde sürekli öğrenme için beynin uyanık/uyku döngüsünü kullanan bir framework yayınladı. Fikir şu: uyku fazını, biriken bilgiyi eski sürümleri bozmadan sıkıştıran ayrı bir arka plan işi olarak ayırmak. Bana gayet mantıklı geldi ama ölçek farkı her şeyi değiştiriyor. Onların uyku fazı model ağırlıkları üzerinde çalışıyor. Benimki session buffer'larında dönüyor, hiçbir ağırlığa dokunmuyor. Onlar bilgiyi belirli bir planla sıkıştırıyor, ben insan onayının arkasına alıp replay ile ölçüyorum.

## Boşluk: bellek sadece context window'un içinde

Konuşlandırılmış bir LLM'i öğretmenin iki yolu var, ikisi de kodlama ajanı için yanlış.

Parametreleri sürekli eğitimle güncellersiniz, derken catastrophic forgetting başlar: yeni task'ler gelir, eski performans çöker. Ya da in-context learning'e güvenirsiniz, o zaman öğrettiğiniz her şey ancak o window'un ömrü kadar yaşar, oturum kapandığı an ölür.

İkisi de işinizi görmez. Önem verdiğiniz bilgi, private bir staging gateway ya da bir proje kuralı, tamı tamına genel bir training run'un asla göremeyeceği, context window'un da her çağrının sonunda attığı şeydir.

Yani asıl mesele "modelimi nasıl daha akıllı yapayım" değil. Mesele: "bu ajanın projem hakkında öğrendiklerini, ajana kendi belleğini kurma güvenini vermeden nasıl kalıcı hale getiririm." Bunlar farklı hedefler ve çoğu agent-memory aracı tam burada sapıtıyor.

## Tasarım: üç katman, tek insan kapısı

Sleep belleği üç katmana ayırıyor, ve bunlardan sadece biri gerçekten kalıcı.

| Katman | Ne olduğu | Kim yazıyor |
|---|---|---|
| T0 | Buffer'daki geçici oturum gözlemi | Hook'lar, extension ya da elle capture |
| T1 | Provisional memory, doğrulanmamış arka plan olarak gösterilir | Konsolidasyon ve tekrar eden tercih madenciliği |
| T2 | Kalıcı bellek | Sadece sizin açık `promote` komutunuz |

Asıl garanti T1 ile T2'nin arasındaki o çizgide. Bir bellek, ajan "bunu hatırla" dedi diye kalıcı olmaz. Ancak siz inceleyip bir candidate'ta `promote` çalıştırdığınızda kalıcı hale gelir. Modelin ürettiği her şey provisional kalır, ve kendi çıktısını "kullanıcı onayladı" diye işaretleyemez.

Günlük akış şöyle:

```text
oturum aktivitesi -> T0 buffer -> gated sleep -> T1 provisional memory
                                                     |
                                              incele ya da replay
                                                     |
                                               explicit promote
                                                     |
                                              T2 kalıcı bellek
```

## Nasıl çalışıyor

Pi ve OmO'da Sleep, kullanıcının kurduğu bir extension olarak geliyor. Agent loop'una yüklenip üç mekanik iş görüyor.

1. **Capture.** Sizin kendi sözleriniz ve path taşıyan tool event'leri T0 buffer'ına düşer.
2. **Publish.** Eşleşen provisional memory her request'te modelin önüne enjekte edilir, bir context compaction onu düşürdükten sonra baştan okunur.
3. **Wake.** Agent settle olduğunda bir policy gate konsolidasyonu tetikler. Model çağrısı asla loop'u bloklamaz.

Claude Code'da aynı işler `UserPromptSubmit`, `PostToolUse`, `Stop` ve `PreCompact` hook'ları üzerinden döner. İki entegrasyon da `$SLEEP_HOME` paylaşır, yani bir client'ta incelediğiniz candidate, proje scope'u eşleştiğinde diğerinde de görünür.

Konsolidasyonun kendisi kapılı. Varsayılan kapı en az 3 yakalanmış kayıt ister, bir de bir önceki pass'ten 6 saat geçmesini bekler. Yani her mesajdan sonra model çağrısı yapılmaz; kararı önce siz görürsünüz:

```bash
./sleep.py status -v
./sleep.py sleep --dry-run
./sleep.py sleep --force
```

Status çıktısı bütün tasarımı tek ekranda toplar. Çalışan bir kurulumda `status -v` şunu raporlar:

```text
sleep memory: T0=disposable captures, T1=provisional awaiting review, T2=permanent (promote-only)
T0 buffer      53 records / 4 files / 20584 bytes  (captured while you worked)
T1 candidates  6 provisional, 7 total  (distilled, shown as unverified context)
   quarantine  0 parked (needs human eyes)  (held out until you review)
T2 canonical   ~/.claude-work/.../memory  (96 permanent records; only you can promote)
```

İki satır bütün fikri taşıyor. T0 atılabilir, çalışmanın ara capture'i, kaybolsa bile zarar düşük. T1 açıkça doğrulanmamış context ve asla talimat veremez. T2 tek kalıcı katman ve ancak sizin promote edebileceğinizi söylüyor. Quarantine satırı görünür hale gelmiş güvenlik ağı.

Sonra çıkan candidate'ı gerçek yapmadan önce inceliyorsunuz:

```bash
./sleep.py show <candidate-id>
./sleep.py promote <candidate-id> --dry-run
./sleep.py promote <candidate-id>
```

Bir tanesini reddettiğinizde ise red, silmek yerine bir audit trail bırakır:

```bash
./sleep.py refuse <candidate-id> --note "Incorrect for the current deployment."
```

## Dream döngüsü: mining, konsolidasyon, replay

Sleep'in ayrıca gözetimsiz gece çalışan bir pass'i var, buna dream döngüsü adını verdim. Hiçbir ağırlık güncellemez ve kendi başına kalıcı bellek yazmaz. Üç şey yapar.

**Mine.** Aynı projede farklı oturumlarda tekrar tekrar verdiğiniz talimatları bulur. Bu adım tamamen deterministik, model çağrısı yapmaz.

**Consolidate.** Sleep gate'i geçtiğinde, ayarladığınız model yeni T0 gözlemlerini provisional T1 candidate'larına çevirir. Bu çağrının bütçesi baştan bellidir.

**Replay.** Sırası gelince seçilen task'leri provisional memory kapalıyken ve açıkken çalıştırıp bir karşılaştırma raporu yazar. En sevdiğim kısmı bu: bir bellek işe yarıyor mu diye tahmin etmenizi istemek yerine, gerçek bir task üzerinde etkisini ölçer.

```bash
./sleep.py nightly --dry-run
./sleep.py mine
./sleep.py replay --run --limit 3 --budget 0.10
```

Replay harcamadan önce neyi çalıştıracağını ve her task'ın ne kadara patlayacağını gösteriyor. Çalışan bir kurulumda plan şöyle görünür:

```text
dry run: would execute 3 task(s) x2 conditions (read-only tools, $0.50/task). Pass --run to execute.
  staging-gateway   Records the X-Route header the staging ALB requires.
  mcp-creds         Notes which servers need literal credentials, not env expansion.
  weight-field      Confirms the weight field is silently dropped for this project.
```

Replay sonucu raporlar ama asla promote etmez. `--budget` ise iki kol için task başına maliyete bir üst sınır koyar.

## Herkesin soracağı güvenlik

Ajana bellek vermek bir prompt injection yüzeyi açar ve ben bu gücü körü körüne vermek istemedim. Pipeline'da somut kapılar var.

- **Redaction.** Secret'e benzeyen değerler hiçbir katmana ulaşmaz. Credential'larınız env'de ya da vault'ta kalır.
- **Quarantine.** Talimat şeklindeki ya da injection'a benzeyen metin asla candidate olamaz. Nedeniyle kenara alınır ve raporlanır.
- **Provenance trust.** Sadece `user` kaynaklı metin talimat gibi ifade edilebilir. Agent ve inferred kayıtlar her zaman context olarak render edilir, asla talimat olarak değil.
- **Proje izolasyonu.** Bir projenin provisional belleği başka bir projeye sızmaz; karışık buffer'lar yanlış yere bilgi tıkmak yerine fail-closed olur.
- **Açık bütçe.** Pipeline'daki her model çağrısının bir maliyet tavanı var.

Scope konusunda dürüst olayım: bu bir filter, sandbox değil. Garantisi net olan tek şey şu: kalıcı bellek insan onayının arkasında kalır ve makinenin ürettiği prompt'lar sizin yerinize geçemez.

## Bellek yeniden kullanılınca ölçülür

Bu aracı "transcript'ini kaydet" tarzı saf işlerden ayıran şey bilinçli bir tercih. Çoğu agent-memory projesi context'i kapar, bir sonraki prompt'a doldurur, "bitti" der. Kör nokta da şu: belleğin gerçekten işe yarayıp yaramadığını asla öğrenmezsiniz.

Sleep bu döngüyü replay ile kapatıyor. Aynı task'i provisional memory kapalıyken ve açıkken çalıştırırsınız, ve imanla verilen bir karar yerine bir karşılaştırma alırsınız. Sonuç, bir candidate'ın T2'ye geçip geçmeyeceğine karar verir. Bir ajanın çöp biriktirmesi ile yargı biriktirmesi arasındaki fark budur.

Bu bir safety property, lüks değil. Test edilmemiş bellek, ajanın sizin istemediğiniz şeyleri sessizce yapmaya başlaması demek. Ölçülmüş bellek ise neyin gerçekten değiştiğini söyleyebilmek.

## Neden böyle kurdum

Yeterince uzun ajan oturumları çalıştırdım, modelin kendi kalıcı state'ini özgürce düzenlemesine izin veren her tasarıma şüpheyle bakıyorum. Ajanın yakaladığı ne varsa koruma eğilimi var; o capture yanlış olsa, gürültü olsa ya da bir prompt injection'un ürünü olsa bile. O yüzden insanın gözden geçirme adımı olmazsa olmaz ve ben bunu otomatikleştirmeyi reddediyorum. Uptime hedef değil. Bir candidate'a bakıp "hayır" diyebilmek, ve o hayrın bir audit trail ile kalıcı olması, aracın var olma sebebi.

Belleği gözden geçirilebilir tutmak, ajanları aynı anda hem daha faydalı hem daha az riskli yapan nadir bir değişiklik. Faydalı, çünkü saatlerce aktardığınız bilgi oturumlar arasında buharlaşmıyor. Daha az riskli, çünkü siz olmadan hiçbir şey kalıcı olmuyor.

## Deneyin

```bash
git clone https://github.com/aydinnyunus/sleep.git
cd sleep
./install.sh                 # shared skill ve Claude Code hook'ları
./install.sh --extension     # ayrıca Pi/OmO extension'ı
./sleep.py sleep --force     # bir konsolidasyon pass'i deneyin
```

Installer önce test suite'ini çalıştırır, config'i değiştirmeden önce backup alır ve tekrar çalıştırmak güvenlidir. Dry run tek dosya yazmadan her değişikliği gösterir: `./install.sh --dry-run`.

Ajan belleği benimseyecekseniz sabit noktaları da alın. Kalıcı bellek açık bir insan onay adımı ister, buffer'dan tazeleme bir gate ister, yeniden kullanım bir ölçüm ister. Bunlardan birini atlarsanız güvenemeyeceğiniz bellek elde edersiniz, ki güvenilmez bellek hiç bellek olmamasından daha kötüdür.

## Referanslar

- [Sleep repository](https://github.com/aydinnyunus/sleep)
- [The sleep framework: A new way for LLMs to retain knowledge in continual learning (Google Research)](https://research.google/blog/the-sleep-framework-a-new-way-for-llms-to-retain-knowledge-in-continual-learning/)
- [The sleep framework paper (arXiv)](https://arxiv.org/abs/2606.03979)

## İlgili içerik

- [AI CAPTCHA Bypass: Solving reCAPTCHA with GPT-4o and Gemini](/2025/12/08/ai-captcha-bypass/)
- [PassDetective: Detecting Passwords and Secrets in Your Shell History](/2025/12/14/passdetective-kali-linux/)
