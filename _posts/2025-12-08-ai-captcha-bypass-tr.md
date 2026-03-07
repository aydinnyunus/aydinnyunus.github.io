---
layout: post
title: "AI ile CAPTCHA bypass: GPT-4o ve Gemini ile otomatik çözüm"
date: 2025-12-08
author: Yunus Aydın
lang: tr
description: "GPT-4o ve Gemini kullanarak reCAPTCHA v2, puzzle, text ve audio CAPTCHA'ları otomatik çözen araç. Kurulum ve kullanım rehberi."
keywords: "ai captcha bypass, GPT-4o, Gemini, CAPTCHA solver, reCAPTCHA v2, Selenium automation, AI security, Black Hat Sector 2025, captcha bypass tool"
canonical_url: "https://aydinnyunus.github.io/2025/12/08/ai-captcha-bypass-tr/"
---

CAPTCHA'ların ne kadar dayanıklı olduğunu test etmek istedim; modern çok modlu modeller (LMM) görsel ve metin CAPTCHA'ları ne kadar iyi çözüyor merak ettim. Bu yüzden GPT-4o ve Gemini kullanarak çeşitli CAPTCHA türlerini otomatik çözen bir araç yazdım. Selenium ile tarayıcıyı açıp CAPTCHA’yı çözüyor, başarılı denemeleri GIF olarak kaydediyor. Araştırma Black Hat Sector 2025’te sunuldu.

## ai-captcha-bypass nedir?

ai-captcha-bypass, Python ile yazılmış bir CLI aracı. GPT-4o ve Gemini kullanarak farklı CAPTCHA türlerini otomatik çözüyor; Selenium ile sayfayı açıp CAPTCHA’yı analiz edip cevabı giriyor.

Hem görsel hem metin CAPTCHA’ları destekliyor, audio CAPTCHA’ları da transcribe edebiliyor. Böylece farklı CAPTCHA’ların dayanıklılığını test edebilirsin. Araç GitHub’da yaygın kullanılıyor ve Black Hat Sector 2025’te sunuldu.

## Neden böyle bir araç?

Bug bounty veya güvenlik testlerinde CAPTCHA’yı elle çözmek zaman alıyor. Modern modellerin ne kadar iyi çözdüğünü görmek ve tekrarlayan testleri otomatikleştirmek için bu aracı yazdım.

## Desteklenen CAPTCHA türleri

1. **Text Captcha:** Basit metin tanıma
2. **Complicated Text Captcha:** Daha fazla bozulma ve gürültü içeren metin CAPTCHA’ları
3. **reCAPTCHA v2:** Google’ın “I'm not a robot” kutusu ve resim seçim challenge’ları
4. **Puzzle Captcha:** Slider puzzle, parçayı doğru yere taşıma
5. **Audio Captcha:** Ses dosyasından harf/sayı transcribe etme

Her tür için özel prompt’lar var; modeller bu prompt’larla daha iyi sonuç veriyor.

## Nasıl çalışıyor?

Akış kısaca:

1. **Browser:** Selenium ile Firefox açılıyor  
2. **Sayfa:** Seçilen CAPTCHA türüne göre demo sayfasına gidiliyor  
3. **Yakalama:** CAPTCHA (görsel, talimat veya puzzle) screenshot alınıyor  
4. **AI:** Görüntü/audio seçilen provider’a (OpenAI veya Gemini) gönderilip CAPTCHA’ya özel prompt ile analiz ediliyor  
5. **Aksiyon:** AI cevabı döndürüyor (metin, koordinat veya seçimler)  
6. **Uygulama:** Selenium ile metin giriliyor, slider hareket ettiriliyor veya resimlere tıklanıyor  
7. **Doğrulama:** Çözülüp çözülmediği kontrol ediliyor  

Başarılı çözümler `successful_solves` klasöründe GIF olarak kaydediliyor.

## Kurulum ve kullanım

### Gereksinimler

- Python 3.7+
- Mozilla Firefox
- OpenAI veya Google Gemini API key'leri

### Kurulum

```bash
git clone https://github.com/aydinnyunus/ai-captcha-bypass
cd ai-captcha-bypass
pip install -r requirements.txt
```

### API Key'lerini Ayarlama

`.env.example` dosyasını `.env` olarak kopyalayın ve API key'lerinizi ekleyin:

```bash
cp .env.example .env
```

`.env` dosyasını açın ve API key'lerinizi ekleyin:

```text
OPENAI_API_KEY="sk-..."
GOOGLE_API_KEY="..."
```

### Kullanım Örnekleri

**Basit text CAPTCHA çözme (OpenAI default):**

```bash
python main.py text
```

**Complicated text CAPTCHA çözme (Gemini ile):**

```bash
python main.py complicated_text --provider gemini
```

**reCAPTCHA v2 çözme (Gemini ile):**

```bash
python main.py recaptcha_v2 --provider gemini
```

**Audio CAPTCHA transcribe etme:**

```bash
python main.py audio --file files/radio.wav --provider openai
```

**Puzzle CAPTCHA çözme (belirli OpenAI model ile):**

```bash
python main.py puzzle --provider openai --model gpt-4o
```

## Başarı örnekleri

Araç farklı CAPTCHA türlerini çözüyor. Başarılı denemelerin GIF’leri repo’daki `successful_solves` klasöründe.

### reCAPTCHA v2

reCAPTCHA v2, Google’ın en çok kullanılan CAPTCHA’larından biri. Hem GPT-4o hem Gemini 2.5 Pro ile başarıyla çözüldü.

![reCAPTCHA v2 başarılı çözüm - OpenAI GPT-4o](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/recaptcha_v2_openai/success_20250906_164422.gif)

*reCAPTCHA v2 başarılı çözüm - OpenAI GPT-4o*

![reCAPTCHA v2 başarılı çözüm - Gemini 2.5 Pro](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/recaptcha_v2_gemini/success_20250906_170027.gif)

*reCAPTCHA v2 başarılı çözüm - Gemini 2.5 Pro*

### Puzzle Captcha

Slider puzzle CAPTCHA’ları parçayı doğru yere taşımayı gerektiriyor. Her iki model de bunları çözebiliyor.

![Puzzle CAPTCHA başarılı çözüm - OpenAI GPT-4o](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/puzzle_openai/success_20250727_173631.gif)

*Puzzle CAPTCHA başarılı çözüm - OpenAI GPT-4o*

![Puzzle CAPTCHA başarılı çözüm - Gemini 2.5 Pro](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/puzzle_gemini/success_20250906_165149.gif)

*Puzzle CAPTCHA başarılı çözüm - Gemini 2.5 Pro*

### Complicated Text Captcha

Yüksek bozulma ve gürültülü metin CAPTCHA’ları insan için bile zor; modeller bunları da okuyabiliyor.

![Complicated Text CAPTCHA başarılı çözüm - OpenAI GPT-4o](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/complicated_text_openai/success_20250906_165751.gif)

*Complicated Text CAPTCHA başarılı çözüm - OpenAI GPT-4o*

![Complicated Text CAPTCHA başarılı çözüm - Gemini 2.5 Pro](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/complicated_text_gemini/success_20250906_165818.gif)

*Complicated Text CAPTCHA başarılı çözüm - Gemini 2.5 Pro*

Bu örnekler modern modellerin CAPTCHA’ları ne kadar iyi çözdüğünü gösteriyor. Her GIF aracın CAPTCHA’yı adım adım nasıl çözdüğünü gösteriyor.

## Güvenlik ve etik kullanım

Araç güvenlik araştırması ve test için yazıldı. CAPTCHA’yı otomatik çözmek bazı sitelerin kullanım koşullarına aykırı olabilir. Bu yüzden:

- Sadece kendi sitende veya izin verilen test ortamlarında kullan  
- Yasal ve etik kurallara uy  
- Başkalarının sitelerinde izinsiz kullanma  

Amaç, CAPTCHA’ların dayanıklılığını test etmek ve gerekirse iyileştirme önermek.

## Proje yapısı

- `main.py`: Ana giriş noktası, CLI argümanları ve test fonksiyonları  
- `ai_utils.py`: OpenAI ve Gemini API çağrıları, prompt’lar  
- `puzzle_solver.py`: Slider puzzle CAPTCHA mantığı  
- `benchmark.py`: Farklı solver’ların başarı oranını ölçen script  
- `successful_solves/`: Başarılı çözümlerin GIF’leri  

## Özet

ai-captcha-bypass, GPT-4o ve Gemini ile CAPTCHA’ları otomatik çözen bir araç. Hem güvenlik testi hem de tekrarlayan denemeler için kullanılabilir. Farklı CAPTCHA türleri destekleniyor, başarılı çözümler GIF olarak kaydediliyor.

CAPTCHA güvenliği hakkında daha fazlası için [exifLooter: Fotoğraflardan gizli konum bilgilerini çıkarmak](/2025/12/07/exiflooter-kali-linux-tr/) yazısına bakabilirsin; diğer [güvenlik projelerime](/tr/projects/) de göz atabilirsin.

## Kaynaklar

- [GitHub Repository](https://github.com/aydinnyunus/ai-captcha-bypass)
- [Black Hat Sector 2025](https://www.blackhat.com/sector/2025/arsenal/schedule/#ai-powered-captcha-bypass-47328) - Sunum detayları
- [OpenAI GPT-4o Documentation](https://platform.openai.com/docs/models/gpt-4o)
- [Google Gemini Documentation](https://ai.google.dev/docs)

