---
layout: post
title: "AI-Powered CAPTCHA Bypass: GPT-4o ve Gemini ile Otomatik CAPTCHA Çözme"
date: 2025-12-08
author: Yunus Aydın
lang: tr
description: "OpenAI GPT-4o ve Google Gemini kullanarak çeşitli CAPTCHA türlerini otomatik olarak çözen AI tabanlı araç. reCAPTCHA v2, puzzle, text ve audio CAPTCHA'ları için detaylı rehber."
keywords: "ai captcha bypass, GPT-4o, Gemini, CAPTCHA solver, reCAPTCHA v2, Selenium automation, AI security, Black Hat Sector 2025, captcha bypass tool"
canonical_url: "https://aydinnyunus.github.io/2025/12/08/ai-captcha-bypass-tr/"
---

Güvenlik araştırmaları yaparken, CAPTCHA'ların ne kadar etkili olduğunu test etmek istedim. Modern AI modellerinin görsel ve metin tabanlı CAPTCHA'ları ne kadar iyi çözebileceğini merak ediyordum. Bu yüzden, OpenAI'nin GPT-4o ve Google'ın Gemini gibi büyük çok modlu modelleri (LMM) kullanarak çeşitli CAPTCHA türlerini otomatik olarak çözen bir araç geliştirdim. Bu araç, Selenium ile web browser automation yaparak CAPTCHA'ları gerçek zamanlı olarak çözüyor ve başarılı çözümleri GIF formatında kaydediyor. Üstelik bu araştırma, Black Hat Sector 2025'te sunuldu.

## ai-captcha-bypass Nedir?

ai-captcha-bypass, Python programlama dili ile yazılmış bir command-line tool. Temel olarak, OpenAI'nin GPT-4o ve Google'ın Gemini gibi gelişmiş AI modellerini kullanarak çeşitli CAPTCHA türlerini otomatik olarak çözüyor. Araç, Selenium ile web browser automation yaparak CAPTCHA'ları gerçek zamanlı olarak analiz ediyor ve çözüyor.

Aracın en önemli özelliklerinden biri, hem görsel hem de metin tabanlı CAPTCHA'ları çözebilmesi. Ayrıca, audio CAPTCHA'ları da transcribe edebiliyor. Bu sayede güvenlik araştırmacıları, farklı CAPTCHA türlerinin güvenlik seviyelerini test edebiliyor.

Araç şu ana kadar GitHub'da **949 star** topladı ve güvenlik topluluğu tarafından yaygın olarak kullanılıyor. Ayrıca Black Hat Sector 2025'te sunuldu.

## Neden ai-captcha-bypass?

Güvenlik araştırmaları yaparken, CAPTCHA'ların ne kadar etkili olduğunu test etmek istedim. Modern AI modellerinin görsel ve metin tabanlı CAPTCHA'ları ne kadar iyi çözebileceğini merak ediyordum. Ayrıca, bug bounty araştırmalarında CAPTCHA'ları bypass etmek gerektiğinde, manuel olarak çözmek zaman alıcı olabiliyor.

## Desteklenen CAPTCHA Türleri

ai-captcha-bypass, şu CAPTCHA türlerini çözebiliyor:

1. **Text Captcha**: Basit metin tanıma CAPTCHA'ları
2. **Complicated Text Captcha**: Daha fazla distortion ve noise içeren metin CAPTCHA'ları
3. **reCAPTCHA v2**: Google'ın "I'm not a robot" checkbox'ı ve image selection challenge'ları
4. **Puzzle Captcha**: Slider puzzle'ları, bir parçanın doğru konuma taşınması gereken CAPTCHA'lar
5. **Audio Captcha**: Ses dosyalarından harf veya sayı transcribe etme

Her CAPTCHA türü için özel prompt'lar hazırlandı ve AI modellerinin en iyi sonuçları vermesi için optimize edildi.

## Nasıl Çalışıyor?

ai-captcha-bypass'un çalışma mantığı oldukça basit:

1. **Browser Launch**: Selenium ile Firefox browser instance'ı başlatılıyor
2. **Navigate**: Belirtilen CAPTCHA türü için demo sayfasına gidiliyor
3. **Capture**: CAPTCHA challenge'ı (image, instruction veya puzzle) screenshot olarak yakalanıyor
4. **AI Analysis**: Yakalanan görüntüler veya audio dosyaları, seçilen AI provider'a (OpenAI veya Gemini) gönderiliyor ve CAPTCHA türüne özel prompt ile analiz ediliyor
5. **Get Action**: AI, çözümü (text, coordinates veya image selections) döndürüyor
6. **Perform Action**: Selenium ile text giriliyor, slider hareket ettiriliyor veya doğru image'ler tıklanıyor
7. **Verify**: CAPTCHA'nın başarıyla çözülüp çözülmediği kontrol ediliyor

Başarılı çözümler, `successful_solves` dizininde GIF formatında kaydediliyor. Bu sayede hangi CAPTCHA'ların başarıyla çözüldüğünü görebiliyorsunuz.

## Kurulum ve Kullanım

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

## Başarı Örnekleri

Araç, çeşitli CAPTCHA türlerini başarıyla çözüyor. GitHub repository'sinde `successful_solves` dizininde başarılı çözümlerin GIF'leri bulunuyor. İşte bazı başarı örnekleri:

### reCAPTCHA v2

reCAPTCHA v2, Google'ın en yaygın kullanılan CAPTCHA türlerinden biri. Hem OpenAI (GPT-4o) hem de Gemini (2.5 Pro) ile başarıyla çözüldü:

![reCAPTCHA v2 başarılı çözüm - OpenAI GPT-4o](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/recaptcha_v2_openai/success_20250906_164422.gif)

*reCAPTCHA v2 başarılı çözüm - OpenAI GPT-4o*

![reCAPTCHA v2 başarılı çözüm - Gemini 2.5 Pro](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/recaptcha_v2_gemini/success_20250906_170027.gif)

*reCAPTCHA v2 başarılı çözüm - Gemini 2.5 Pro*

### Puzzle Captcha

Slider puzzle CAPTCHA'ları, bir parçanın doğru konuma taşınması gereken zorlu bir CAPTCHA türü. Her iki AI modeli de bu tür CAPTCHA'ları başarıyla çözüyor:

![Puzzle CAPTCHA başarılı çözüm - OpenAI GPT-4o](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/puzzle_openai/success_20250727_173631.gif)

*Puzzle CAPTCHA başarılı çözüm - OpenAI GPT-4o*

![Puzzle CAPTCHA başarılı çözüm - Gemini 2.5 Pro](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/puzzle_gemini/success_20250906_165149.gif)

*Puzzle CAPTCHA başarılı çözüm - Gemini 2.5 Pro*

### Complicated Text Captcha

Yüksek distortion ve noise içeren metin CAPTCHA'ları, insanlar için bile zor olabilir. Ancak AI modelleri bu tür CAPTCHA'ları da başarıyla okuyor:

![Complicated Text CAPTCHA başarılı çözüm - OpenAI GPT-4o](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/complicated_text_openai/success_20250906_165751.gif)

*Complicated Text CAPTCHA başarılı çözüm - OpenAI GPT-4o*

![Complicated Text CAPTCHA başarılı çözüm - Gemini 2.5 Pro](https://raw.githubusercontent.com/aydinnyunus/ai-captcha-bypass/refs/heads/main/successful_solves/complicated_text_gemini/success_20250906_165818.gif)

*Complicated Text CAPTCHA başarılı çözüm - Gemini 2.5 Pro*

Bu örnekler, modern AI modellerinin CAPTCHA'ları ne kadar etkili çözebileceğini gösteriyor. Her GIF, aracın gerçek zamanlı olarak CAPTCHA'yı nasıl çözdüğünü gösteriyor.

## Güvenlik ve Etik Kullanım

Bu araç, güvenlik araştırması ve test amaçlı geliştirilmiştir. CAPTCHA'ları otomatik olarak çözmek, bazı web sitelerinin kullanım koşullarını ihlal edebilir. Bu yüzden:

- **Sadece kendi web sitenizde veya izin verilen test ortamlarında kullanın**
- **Yasal ve etik kurallara dikkat edin**
- **Başkalarının web sitelerinde izinsiz kullanmayın**

Araç, güvenlik araştırmacılarının CAPTCHA'ların güvenlik seviyelerini test etmesi ve iyileştirmeler önermesi için geliştirilmiştir.

## Proje Yapısı

- `main.py`: Ana entry point, command-line argument'ları handle ediyor ve uygun test fonksiyonlarını çağırıyor
- `ai_utils.py`: OpenAI ve Gemini API'leri ile etkileşim için fonksiyonlar içeriyor. Prompt'lar burada tanımlanıyor ve API call'ları yapılıyor
- `puzzle_solver.py`: Multi-step slider puzzle CAPTCHA'ları çözmek için özel logic içeriyor
- `benchmark.py`: Farklı solver'ların performansını ve başarı oranını değerlendirmek için multiple test çalıştıran script
- `successful_solves/`: Başarılı çözümlerin GIF'lerinin kaydedildiği dizin

## Sonuç

ai-captcha-bypass, modern AI modellerini kullanarak CAPTCHA'ları otomatik olarak çözen yenilikçi bir araç. Hem güvenlik araştırmacıları hem de geliştiriciler için değerli bir kaynak. Araç, çeşitli CAPTCHA türlerini çözebiliyor ve başarılı çözümleri kaydediyor.

Eğer CAPTCHA güvenliği hakkında daha fazla bilgi edinmek istiyorsanız, [exifLooter: Fotoğraflardan Gizli Konum Bilgilerini Çıkarmak](/2025/12/07/exiflooter-kali-linux-tr/) yazısına göz atabilirsiniz. Ayrıca diğer [güvenlik projelerimi](/tr/projects/) de inceleyebilirsiniz.

## Kaynaklar

- [GitHub Repository](https://github.com/aydinnyunus/ai-captcha-bypass)
- [Black Hat Sector 2025](https://www.blackhat.com/sector/2025/arsenal/schedule/#ai-powered-captcha-bypass-47328) - Sunum detayları
- [OpenAI GPT-4o Documentation](https://platform.openai.com/docs/models/gpt-4o)
- [Google Gemini Documentation](https://ai.google.dev/docs)

