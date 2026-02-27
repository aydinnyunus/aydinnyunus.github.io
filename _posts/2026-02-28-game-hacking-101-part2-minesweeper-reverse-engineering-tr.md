---
layout: post
title: "Game Hacking 101 Part 2: Minesweeper Reverse Engineering ile Memory Analizi"
date: 2026-02-28
author: Yunus Aydın
lang: tr
description: "Minesweeper reverse engineering ile game hacking tekniklerini öğren. Cheat Engine pointer scanning, x64dbg hardware breakpoints ve memory analizi ile oyun içi değerleri tespit et."
keywords: "game hacking, reverse engineering, Minesweeper, Cheat Engine, x64dbg, pointer scanning, hardware breakpoints, memory analysis, game hacking 101, reverse engineering tutorial"
canonical_url: "https://aydinnyunus.github.io/2026/02/28/game-hacking-101-part2-minesweeper-reverse-engineering-tr/"
---

Game Hacking 101 serisinin ilk bölümünde, Mount and Blade Warband'da memory manipulation tekniklerini öğrendik. Bu ikinci bölümde, Minesweeper'ı reverse engineering yaparak daha gelişmiş teknikleri keşfedeceğiz. Cheat Engine ile pointer scanning, x64dbg ile hardware breakpoints ve memory analizi kullanarak oyunun iç mekanizmalarını çözeceğiz. Bu rehber, reverse engineering'in temellerini öğrenmek isteyenler için mükemmel bir başlangıç noktası.

## Minesweeper Nedir?

Minesweeper, Windows'un klasik bir oyunu. Oyuncular, grid üzerindeki gizli mayınları açığa çıkarmaya çalışırken patlayıcı kazalardan kaçınmalı. Görünüşte basit bir oyun gibi görünse de, Minesweeper'ın kodlama teknikleri ve tasarımı reverse engineering için harika bir öğrenme aracı.

![Minesweeper](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*Z9i0L_1LXSJGAAyzPrE7dw.jpeg)

Minesweeper'ın görünürdeki basitliği, aslında içinde barındırdığı karmaşıklığı gizler. Reverse engineering, oyunun mimarisini ve veri yapılarını anlamamıza yardımcı olur. Bu yolculuk, oyunun iç mekanizmalarını çözmek için gerekli temel bilgileri sağlar.

## Grid'i Çözmek

Minesweeper'ın kalbinde, oyuncuların sistematik olarak açtığı bir grid yapısı var. Bu grid'in memory temsilini analiz ederek, oyunun mayınlar, sayılar ve açık/kapalı hücreler hakkındaki bilgileri nasıl sakladığını anlayabiliriz.

**Grid yapısı:**

- Her hücre, bir memory adresinde saklanır
- Mayın bilgisi binary formatında tutulur
- Hücre durumu (açık/kapalı) bir flag ile temsil edilir
- Komşu mayın sayısı hesaplanır ve saklanır

## Algoritmaları Kırmak

Minesweeper, mayınlara komşu hücrelerdeki sayıları hesaplamak ve boş hücrelerin açılmasını sağlamak için algoritmalar kullanır. Bu algoritmaları reverse engineering yaparak, oyunun gerçek zamanlı mantık işleme sürecini anlayabiliriz.

**Temel algoritmalar:**

- **Mayın sayısı hesaplama**: Her hücre için komşu mayın sayısını hesaplar
- **Cascading açılma**: Boş hücrelerin otomatik olarak açılması
- **Oyun durumu kontrolü**: Kazanma/kaybetme durumunu belirleme

## Rastgeleliğin Perdesini Kaldırmak

Minesweeper'da görünüşte rastgele mayın yerleşimi, aslında belirli pattern'ler ve öngörülebilirlik içerir. Random number generation mekanizmasını inceleyerek, mayın yerleşiminin gizemli yöntemini ortaya çıkarabiliriz.

![Random Number Generation](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*S6RdzkvwPEu5jXlZg2XTLw.png)

`srand` kullanılarak üretilen random sayılar, gerçekten rastgele değil, pseudorandom'dur. Bu, aynı seed değeri verildiğinde, her seferinde aynı sayı dizisinin üretileceği anlamına gelir. Minesweeper'ın random number generation'ı `srand`'a dayandığı için, aynı seed (genellikle sistem zamanından türetilir) tekrarlanan oyun oturumlarında tutarlı sonuçlar garanti eder.

**Pseudorandom özellikleri:**

- Seed değeri aynıysa, aynı sequence üretilir
- Sistem zamanı genellikle seed olarak kullanılır
- Bu, mayın yerleşiminin öngörülebilir olmasını sağlar

## Cheat Engine ile Bomb Count Bulma

Minesweeper'da bomb count (mayın sayısı) gösterimi kritik bir özellik. Oyunda, oyuncular flag yerleştirerek bu sayıyı azaltabilir. Reverse engineering, bu değeri manipüle etmenin bir yolunu açığa çıkarır.

### Cheat Engine: Keşif Aracı

Cheat Engine, game hacking ve reverse engineering için güçlü bir araçtır. Bu araç, çalışan bir oyunun memory'sine dalmanıza, Minesweeper'daki bomb count gibi spesifik değerleri bulmanıza ve iç işleyişini incelemenize olanak tanır.

![Cheat Engine](https://upload.wikimedia.org/wikipedia/commons/c/c2/Cheat_Engine_7.1.png)

### Değişken Değer Sorunu

Minesweeper'da, bomb count'u saklayan memory adresi her yeni oyun başlatıldığında değişir. Bu dinamik davranış, bu kritik bilgiyi saklayan tam memory adresini bulmayı zorlaştırır.

**Sorun:**

- Her oyun başlatıldığında memory layout değişir
- Base address'ler dinamik olarak atanır
- Static memory adresleri kullanılamaz

### Bomb Count'u Avlamak

Cheat Engine'i kullanarak, başlangıç bomb count değerini taramaya başlarız. Ancak, bu değerin sürekli değişen doğası nedeniyle, tarama aynı değere sahip birçok memory adresi döndürür. Cheat Engine, memory'yi farklı aralıklarda snapshot'lar, bu da birden fazla eşleşmeye yol açar.

**İlk tarama adımları:**

1. Cheat Engine'i başlat
2. Minesweeper process'ini seç
3. İlk bomb count değerini gir (örneğin, 10)
4. "First Scan" yap
5. Birçok adres bulunur

### İterasyon ile İyileştirme

False positive'leri elemek için, bomb count'u etkileyen stratejik oyun içi aksiyonlar gerçekleştiririz. Örneğin, bir flag yerleştirmek count'u bir azaltır. Bu tür aksiyonlardan sonra, Cheat Engine'de "Next Scan" yaparak potansiyel adreslerin listesini daraltırız, hedefe daha yaklaşırız.

**Daraltma süreci:**

1. Oyun içinde bir flag yerleştir
2. Yeni bomb count değerini gir
3. "Next Scan" yap
4. Adres listesi daralır
5. Tek bir adres kalana kadar tekrarla

### Pointer Scan Sorunu

Minesweeper'ın gizli yönlerini ortaya çıkarma yolculuğunda, Cheat Engine'in güçlü pointer scan özelliği paha biçilmez hale gelir. Bu araç, pointer chain'lerini takip etmeye, bomb count veya click kayıtları gibi kritik değerleri tutan memory adreslerini ortaya çıkarmaya olanak tanır. Ancak, bu güçlü araç birçok potansiyel aday üretir, aramayı karmaşıklaştırır.

![Pointer Scan](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*dVTh_IXpY_IR8qaaUQ-QWA.png)

**Pointer scan zorlukları:**

- İlk pointer scan yaklaşık 60 sonuç üretir
- Her sonuç, oyunun memory'sinde farklı adreslere işaret eder
- Tüm adresler aradığımız bilgiyi içermez
- Birçok adres geçici veya hedef bilgimizle ilgisiz olabilir

### Kalıcı Olanı Geçici Olanlardan Ayırmak

İlk pointer scan bizi yaklaşık 60 sonuçla doldurur, her biri oyunun memory'sinde farklı adreslere işaret eder. Bu veri denizinde, aradığımız cevapları tutan adresler değil. Birçoğu geçici olabilir veya hedef bilgimizle ilgisiz olabilir. Arayışımız, çokluğun içinde tutarlı, güvenilir pointer'ları tanımlamayı gerektirir.

![Pointer Scan with Previous Scans](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*jUitILhctoTzKSPfxmJu5g.png)

### Yeni Bir Başlangıç: Oyunu Yeniden Başlatmak

Minesweeper'ın akışkan memory layout'ını kabul ederek, yeni bir başlangıç yapmayı seçeriz. Oyunu kapatıp yeniden başlatırız, farklı memory konfigürasyonlarıyla yeni bir oturum başlatırız. Bu yenilenmiş bağlamda pointer scan'i yeniden gerçekleştirerek, aradığımız bomb count ve click sayılarını tutarlı olarak barındıran memory adreslerini izole etmeyi hedefleriz.

**Yeniden başlatma stratejisi:**

1. Oyunu kapat
2. Yeni bir oyun başlat
3. Pointer scan'i tekrarla
4. Tutarlı offset'leri bul

### Stabil Offset'lerin Ortaya Çıkması

Yeniden başlatıp yeni bir pointer scan gerçekleştirdikten sonra, umut verici bir pattern ortaya çıkar. İki offset, sonuçlar içinde tutarlı olarak ortaya çıkar, çeşitli oyun oturumlarında varlıklarını korur. Bu stabil offset'ler, sürekli değişen memory adresleri manzarası arasında öne çıkar. Bu keşif, Minesweeper'ın gizli iç yapısını anlama arayışımızda kritik bir adımı işaret eder.

![Candidates for Pointer Scan](https://miro.medium.com/v2/resize:fit:1200/format:webp/1*5OY7mpXiyRt6vmREACdxPQ.png)

**Stabil offset'ler:**

- İki offset tutarlı olarak bulunur
- Farklı oyun oturumlarında aynı kalır
- Memory layout değişse bile çalışır

## x64dbg ile Debugging

Debugging, program execution'ı parçalara ayırma, memory değişikliklerini takip etme ve oyun davranışını etkileyen kritik fonksiyonları tanımlama sürecidir. İlk odağımız, oyuncuların oyun sırasında tıkladığı hücreye odaklanır. Bu memory adresinin etrafındaki aktivitelere bakmak, Minesweeper'ın mekanikleriyle iç içe geçmiş fonksiyonlara dair içgörüler vaat eder.

### Hardware Breakpoints: Kod İzlerini Ortaya Çıkarmak

Bir hücre tıklaması sırasında tetiklenen olayları netleştirmek için, güçlü bir debugging tekniği kullanırız: hardware breakpoints. Tıklanan hücrenin memory adresine bir breakpoint ayarlayarak, debugger memory konumu erişildiğinde veya değiştirildiğinde müdahale eder. Bu taktik, tıklama sırasında ve sonrasında kodun sequence'ine mikroskobik bir görünüm sağlar.

**Hardware breakpoint avantajları:**

- Memory erişimlerini yakalar
- Memory değişikliklerini yakalar
- Kod execution'ı durdurur
- Register'ları ve stack'i inceleme imkanı sağlar

![x64dbg](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ZUyjIu6vw1jO5DhdoqCvsw.png)

Herhangi bir grid karesine tıklandığında, breakpoint tetiklenir. Önemli olarak, `[rcx+1C]` click count'u belirtir, 1C offset'tir.

![x64dbg breakpoint](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*m3HT77bmWk1b29FceNmBKQ.png)

### Kodun Aydınlatılması

Hardware breakpoint'i başlatmak bizi precision debugging dünyasına taşır. Tıklanan hücrenin memory adresiyle her etkileşim bir duraklamayı tetikler, register'ları, stack'i ve kodu gerçek zamanlı olarak inceleme imkanı sağlar. Dikkatli analiz yoluyla, kodun davranışını çözeriz, Minesweeper'ın dinamik oyun mekaniklerini şekillendiren ek fonksiyonları ve etkileşimleri ortaya çıkarırız.

**Analiz süreci:**

1. Breakpoint tetiklendiğinde dur
2. Register'ları incele
3. Stack'i analiz et
4. Assembly kodunu takip et
5. Fonksiyon çağrılarını belirle

### Gizli Mücevherleri Ortaya Çıkarmak

Tıklanan hücrenin memory adresiyle etkileşimlere kodun yanıtını parçalayarak, Minesweeper'ın oyun mantığını etkileyen gizli fonksiyonları ortaya çıkarırız. Bu keşifler, mekanikleri açığa çıkarmaktan cascading algoritmalarına kadar çeşitli yönleri kapsar. Her breakpoint ile, Minesweeper'ın codebase'inin karmaşık dokusunu anlamaya daha yaklaşırız.

![isNotBomb](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*_Q1V_LDZSbA4TmglfdQa4g.png)

Conditional jump (`jne`) aksiyonunu araştırmak, adres hesaplamalarına, offset'lere ve `cmp byte ptr [rsi+rcx], dil` gibi karşılaştırmalara yol açar. `dil`'in 0'da sabit kaldığı, `[rsi+rcx]`'in ise `isBomb` ile ilgili olduğu açıkça görülür. Eğer bir mayın değilse, jump 2EE adresine yönlendirir, `cmp dword ptr [rbx+38], 0` karşılaştırması aracılığıyla offset'i (click count) ilerletir, oyunun ilerlemesini etkiler.

![isNotBomb Registers](https://miro.medium.com/v2/resize:fit:796/format:webp/1*sOh2XycSWt5gdfU0X3nuVQ.png)

`isNotBomb` fonksiyonu sırasında register'ları parçalayarak, `rsi` (rowPtr) ve `rbp` (column) rollerini ortaya çıkarırız, her ikisi de 0'da sürekli olarak sabitlenir. Bu farkındalık, offset hesaplamalarını ve mayın varlığını belirlemek için loop yapıları oluşturmayı kolaylaştırır. Formülümüz şöyle olur: **Minesweeper.exe + 0xAAA38] + 0x18] + 0x58] + 0x10] + 0x8* column] + 0x10] + row**.

**Memory formülü:**

```
Minesweeper.exe + 0xAAA38] + 0x18] + 0x58] + 0x10] + 0x8* column] + 0x10] + row
```

Bu formül, herhangi bir hücrenin memory adresini hesaplamak için kullanılabilir.

### Memory Analizi

Memory analizi değerli verileri ortaya çıkarır. Özellikle, 38f207c 09'u saklar, row ve column sayılarını yansıtır. Vurgulanan değer, 1C offset'i, click count'u belirtir. Diğerleri geçen süreyi gösterir. Bu memory bölgesi, Minesweeper'ın kritik değişkenlerini kapsar.

![Memory Section](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*iilhoc_YQXLcURuq2rGk3A.png)

**Memory bölgesi içeriği:**

- Row ve column sayıları
- Click count (1C offset)
- Geçen süre
- Oyun durumu

## C++ Kodu ile Implementasyon

Tüm bu analizlerden sonra, Minesweeper'ın memory yapısını manipüle eden bir C++ kodu yazabiliriz. Bu kod, pointer chain'leri kullanarak hücre bilgilerine erişir ve oyun durumunu kontrol eder.

> **GitHub**: [https://github.com/aydinnyunus/MineQuest](https://github.com/aydinnyunus/MineQuest)

![Result](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*nbuApRiyt87rFgkaXOX4Gg.png)

**C++ kodunun özellikleri:**

- Pointer chain'leri kullanarak memory'ye erişim
- Hücre durumunu okuma ve yazma
- Mayın bilgisini tespit etme
- Oyun durumunu kontrol etme

## Sonuç

Minesweeper'ı reverse engineering yapmak, kodlama teknikleri, algoritmalar ve veri yapılarının canlı bir karışımını ortaya çıkarır. Bu yolculuk, yazılım geliştirmeye bir takdir sunar, görünüşte mütevazı bir oyunun içinde barındırılan memory yönetimi inceliklerini, UI tasarımını ve daha fazlasını ortaya çıkarır. Bu yolculuk, görünüşte basit oyunların altında yatan zanaatkarlığı aydınlatır, basitlikte bile karmaşıklığın geliştiğini vurgular.

**Ana çıkarımlar:**

- Pointer scanning, dinamik memory adreslerini bulmak için kritiktir
- Hardware breakpoints, kod execution'ını analiz etmek için güçlü bir araçtır
- Memory formülleri, hücre bilgilerine erişmek için kullanılabilir
- Reverse engineering, oyun mekaniklerini anlamak için değerli bir yöntemdir

Game hacking ve reverse engineering, yazılımın düşük seviyede nasıl çalıştığını anlamak için güçlü tekniklerdir. Minesweeper gibi basit görünen oyunlar bile, derinlemesine analiz için zengin bir kaynak sunar.

## İlgili İçerik

- [Kapı Şifrelerini Atlama](/2022/01/07/bypassing-door-passwords-tr/)
- [Instagram Dolandırıcılarını Hacklemek](/2022/04/11/hacking-instagram-scammers-tr/)
