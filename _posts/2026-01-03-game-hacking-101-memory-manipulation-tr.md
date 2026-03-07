---
layout: post
title: "Game Hacking 101: Mount and Blade Warband'da Memory Manipulation"
date: 2026-01-03
author: Yunus Aydın
lang: tr
description: "Memory manipulation ile game hacking temellerini öğren. Virtual memory'i anla, Cheat Engine kullan ve Mount and Blade Warband'da oyun değerlerini değiştirmek için C++ kodu yaz."
keywords: "game hacking, memory manipulation, Cheat Engine, C++ game hacking, Mount and Blade Warband, virtual memory, memory addresses, offsets, Windows API, ReadProcessMemory, WriteProcessMemory"
canonical_url: "https://aydinnyunus.github.io/2026/01/03/game-hacking-101-memory-manipulation-tr/"
---

Game hacking, oyunların veriyi memory’de nasıl tuttuğunu anlamak için iyi bir giriş. Memory manipulation ile oyun içi değerleri okuyup yazabilir, mekanikleri deneyebilirsin. Bu yazıda **Mount and Blade Warband** üzerinden adım adım gidiyoruz: virtual memory, Cheat Engine ile tarama ve C++ ile programatik okuma/yazma.

## Mount and Blade Warband nedir?

TaleWorlds’ün orta çağ temalı aksiyon RPG’si. Calradia’da karakter, ordu ve krallık kuruyorsun; sandbox ve mod desteği var. Game hacking pratiği için uygun bir hedef.

![Mount and Blade Warband](https://cdn.taleworlds.com/static/screenshots/1920x1080/mnb-wb-07.jpg)

## Memory’yi anlamak

Oyun process’inin çalışan memory’sinde değişkenler, değerler ve oyun durumu tutuluyor. Bu memory’yi okuyup yazarak değişkenleri değiştirip oynanışı etkileyebilirsin.

### Virtual memory

Her process’e kendi sanal adres alanı veriliyor; sayfalar fiziksel memory veya diske eşleniyor. Program fizikselden fazla memory’ye erişiyormuş gibi çalışabiliyor.

![Virtual Memory vs Physical Memory](https://miro.medium.com/v2/resize:fit:1240/format:webp/1*Lbc8Ng41sKlo7oCGUvuxHg.png)

**Virtual Memory Nasıl Çalışır:**

Bir program çalıştığında, virtual address space'e yüklenir. Bu genellikle segmentlere (code, data, stack) bölünür. İşletim sisteminin Memory Management Unit (MMU) bileşeni, virtual adresleri fiziksel adreslere veya ikincil depolamaya eşler.

**Virtual Memory'nin Faydaları:**

1. **Artırılmış Adreslenebilir Memory**: Her process, fiziksel memory'den daha büyük bir virtual address space'e sahip olabilir
2. **Memory İzolasyonu**: Process'ler birbirinin memory'sine erişemez, bu da güvenliği artırır
3. **Verimli Memory Kullanımı**: Kullanılmayan kısımlar diske saklanabilir, fiziksel memory'yi serbest bırakır
4. **Memory Paylaşımı**: Birden fazla process aynı dosya eşlemelerini paylaşabilir

### Memory adresini bulma ve değiştirme

Manipülasyon için önce hedef değişkenin adresini bulman lazım. Değer veya veri tipine göre tarama yapıyorsun; adresi bulunca okuyup yazıyorsun (kaynak, sağlık, para vb.).

### Pointer’lar

Dinamik yapılarda adresler her açılışta değişebiliyor. Pointer’lar başka adreslere işaret ediyor; pointer zincirini takip ederek doğru hücreye ulaşıyorsun.

## Araçlar

**Memory editor / trainer:** Değer tarayıp değiştiren arayüzler. **Debugger:** Cheat Engine, OllyDbg gibi; breakpoint, memory inceleme, disassembly.

## Cheat Engine kullanımı

Cheat Engine, game hacking’te sık kullanılan memory tarama ve düzenleme aracı. Kısa adımlar:

### 1. Cheat Engine'i İndir ve Kur

[Cheat Engine resmi web sitesini](https://cheatengine.org/) ziyaret et ve işletim sistemin için en son sürümü indir. Kurulum dosyasını çalıştır ve ekrandaki talimatları takip et.

### 2. Cheat Engine'i Başlat ve Oyun Process'ini Seç

Cheat Engine'i başlat ve sol üst köşedeki bilgisayar simgesine tıklayarak process listesini aç. Oyun process'ini seç (örneğin, `mb_warband.exe`). Process'i seçmeden önce oyunun çalıştığından emin ol.

![Cheat Engine - Process Seçimi](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*nsc498XyXZU8FkqcPUee-Q.png)

### 3. İlk Değer Taraması Yap

Değiştirmek istediğin değişkeni belirle (örneğin, oyuncunun parası). Cheat Engine'de:

1. "Value" alanına mevcut değeri gir
2. Dropdown menüden değer tipini seç (örneğin, integer değerler için 4 bytes)
3. Aramayı başlatmak için "First Scan" butonuna tıkla

Tarama, değerinle eşleşen tüm memory adreslerini bulacaktır.

![Mount and Blade Warband - Envanter](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*8fyaHsAQF-cNmtBO_nLWYg.jpeg)

![Cheat Engine - İlk Tarama](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*DjkqgC9A-Ssw8IhbKdJFBQ.png)

### 4. Aramayı Daralt

İlk tarama genellikle birçok adres döndürür. Bunu daraltmak için:

1. Oyunda değeri değiştir (örneğin, para harca veya kazan)
2. Cheat Engine'de yeni değeri gir
3. Aramayı daraltmak için "Next Scan" butonuna tıkla
4. Yönetilebilir sayıda adres kalana kadar tekrarla

![Mount and Blade Warband - Envanter 710 Dinar Gösteriyor](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*dKqwbdnV904vQs8sC-wjxA.jpeg)

![Cheat Engine - Sonraki Tarama](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*9lhMD3ju1butGlBEpaObrw.png)

### 5. Memory Değerlerini Değiştir

Daraltılmış bir liste ile:

1. Bir memory adresine çift tıklayarak adres listesine ekle
2. Değer sütununa çift tıklayarak değiştir
3. İstediğin değeri gir
4. Oyun değişikliği hemen yansıtacaktır

![Cheat Engine - Değeri 1337'ye Değiştir](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*hU9nct96Nx7xihThGPaIgQ.png)

![Mount and Blade Warband - Envanter 1337 Gösteriyor](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*rOQB-YHStAEaXejL_Bc0hA.jpeg)

### 6. Cheat Table'ını Kaydet

Disket simgesine tıklayarak cheat table'ını kaydet. Bu, oyunu tekrar başlattığında cheat'leri yeniden yüklemeni sağlar.

**Not**: Birçok oyun anti-cheat sistemleri kullanır. Cheat table'larını sorumlu bir şekilde kullan ve anti-cheat önlemleri hakkında güncel kal.

## C++ ile memory okuma/yazma

Cheat Engine ile offset’leri bulduktan sonra aynı mantığı C++ ile otomatikleştirebilirsin. Windows API: `ReadProcessMemory`, `WriteProcessMemory`, `OpenProcess` vb.

### Offset’leri bulma

Cheat Engine’de hedef değerin adresini bul, “Find out what accesses this address” ile erişen assembly’yi incele. Base + offset zincirini çıkar.

**Assembly Analizi Örneği:**

```text
mov ecx, [ecx + 140f0]
imul eax, eax, fc8
mov esi, [ecx + eax + 5d0]
```

Bu assembly kodu şunları gösterir:
- `ecx`, `[ecx + 0x140f0]` adresinden yüklenir
- `eax`, `0xfc8` ile çarpılır
- Para değeri, `[ecx + eax + 0x5d0]` adresinden yüklenir

![Cheat Engine - Assembly Kodu](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ChE-pPah2VmH5VteWbxgfg.png)

"cc0701" hexadecimal para değerini decimal'e çevirdiğimizde 13371137 değerini elde ederiz.

![Decimal'den Hexadecimal'e Dönüştürücü](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*vJXIjKljPFcK9VIsKbmVQw.png)

Daha fazla analiz, `ecx`'in `mb_warband.exe + 0x4eb300` adresinden ayarlandığını ortaya çıkarır, bu da bize base address ve offset'leri verir.

![Cheat Engine - Tam Assembly Analizi](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ei2b218nHB4_udLXyQF4Nw.png)

### Memory Manipulation için Windows API

Windows API, process'ler ve memory'leriyle etkileşim kurmak için fonksiyonlar sağlar. Ana fonksiyonlar:

- `CreateToolhelp32Snapshot`: Çalışan process'lerin anlık görüntüsünü oluşturur
- `Process32Next`: Process girişleri arasında yinelenir
- `OpenProcess`: Hedef process'e bir handle açar
- `ReadProcessMemory`: Bir process'ten memory okur
- `WriteProcessMemory`: Bir process'e memory yazar

### Process Yönetimi

İşte bir process'i bulma ve açma:

```cpp
Memory(const std::string_view processName) noexcept
{
    ::PROCESSENTRY32 entry = { };
    entry.dwSize = sizeof(::PROCESSENTRY32);

    const auto snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    while (::Process32Next(snapShot, &entry))
    {
        if (!processName.compare(entry.szExeFile))
        {
            processId = entry.th32ProcessID;
            processHandle = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
            break;
        }
    }

    if (snapShot)
        ::CloseHandle(snapShot);
}
```

Tam `Memory` sınıfı implementasyonu [MemoryBlade repository](https://github.com/aydinnyunus/MemoryBlade/blob/main/memory.h)'sinde mevcuttur.

Bu kod:
1. Tüm çalışan process'lerin anlık görüntüsünü oluşturur
2. Her process arasında yinelenir
3. Process isimlerini hedefle karşılaştırır
4. Bulunduğunda tam erişimle bir handle açar
5. Anlık görüntü handle'ını kapatır

### Memory Okuma

İşte memory okumak için bir template fonksiyon:

```cpp
template <typename T>
constexpr const T Read(const std::uintptr_t& address) const noexcept
{
    T value = { };
    ::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);
    return value;
}
```

Bu fonksiyon:
- Bir memory adresini girdi olarak alır
- Değeri okumak için `ReadProcessMemory` kullanır
- `T` tipinde değeri döndürür
- Herhangi bir veri tipiyle çalışır (int, float, vb.)

### Memory'ye Yazma

İşte memory'ye yazmak için bir template fonksiyon:

```cpp
template <typename T>
constexpr void Write(const std::uintptr_t& address, const T& value) const noexcept
{
    ::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), NULL);
}
```

Bu fonksiyon:
- Bir memory adresi ve değer alır
- Değeri yazmak için `WriteProcessMemory` kullanır
- Herhangi bir veri tipiyle çalışır

### Para Manipülasyonu

İşte oyuncunun parasını manipüle eden eksiksiz bir örnek:

```cpp
#include <iostream>
#include "Memory.h"

int main()
{
    // mb_warband.exe için Memory instance'ı oluştur
    const auto mem = Memory("mb_warband.exe");
    std::cout << "Mount and Blade Warband found!" << std::endl;

    // Oyun modülünün base address'ini al
    const auto base_address = mem.GetModuleAddress("mb_warband.exe");

    // Offset'leri kullanarak değerleri oku
    int ecx = mem.Read<int>(base_address + 0x4eb300);
    int eax = mem.Read<int>(ecx + 0x6e0);
    ecx = mem.Read<int>(ecx + 0x140f0);
    int money = mem.Read<int>(ecx + 0x5d0);

    std::cout << "Current money: " << money << std::endl;

    // Yeni para değerini yaz
    mem.Write<int>(ecx + 0x5d0, 13371137);

    std::cout << "Money updated!" << std::endl;

    return 0;
}
```

Bu kod:
1. Oyun process'ini bulur
2. Executable'ın base address'ini alır
3. Offset'leri kullanarak pointer zincirini takip eder
4. Mevcut para değerini okur
5. Yeni bir para değeri yazar

### Karakter Yeteneklerini Manipüle Etme

Mount and Blade Warband'da karakter yetenekleri spesifik offset'lerde saklanır. İşte bunları değiştirme:

```cpp
#include <iostream>
#include "Memory.h"

int main()
{
    const auto mem = Memory("mb_warband.exe");
    std::cout << "Mount and Blade Warband found!" << std::endl;

    const auto base_address = mem.GetModuleAddress("mb_warband.exe");
    
    // Base pointer'ı al
    int edx = mem.Read<int>(base_address + 0x4eb300);
    edx = mem.Read<int>(edx + 0x140f0);
    
    // Yetenekleri değiştir (hepsi 500'e ayarlandı)
    mem.Write<int>(edx + 0x270, 500); // Strength
    mem.Write<int>(edx + 0x274, 500); // Agility
    mem.Write<int>(edx + 0x278, 500); // Intelligence
    mem.Write<int>(edx + 0x27c, 500); // Charisma

    std::cout << "Skills updated!" << std::endl;

    return 0;
}
```

**Yetenek Offset'leri:**
- `0x270`: Strength (Güç)
- `0x274`: Agility (Çeviklik)
- `0x278`: Intelligence (Zeka)
- `0x27c`: Charisma (Karizma)

![Güç İçin Offset Bulma - Register Analizi](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*f9T5kD_Js7SwqZscsaJWrg.png)

![Güç İçin Offset Bulma - Tam Assembly](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*_5ot9BmTSogfaVEEGZQQ6g.png)

![Çeviklik İçin Offset Bulma - Tam Assembly](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ZEq3P7mvSQnKe0Ic2XsQeA.png)

![Mount and Blade Warband - Yetenekler 500'e Ayarlandı](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*fP24IdP0F4vk5QK4qv3AMQ.jpeg)

## İnteraktif Memory Adresi Görselleştirme

Memory adreslerinin ve offset'lerin nasıl çalıştığını anlamak zor olabilir. Süreci görselleştirelim:

<div id="memory-visualization" class="demo-container">
  <h4 class="demo-title">Memory Adresi Görselleştirme</h4>
  <div class="demo-input-section">
    <label for="base-address" class="demo-label">Base Address (hex):</label>
    <input type="text" id="base-address" value="0x00400000" class="demo-input">
    <label for="offset" class="demo-label">Offset (hex):</label>
    <input type="text" id="offset" value="0x4eb300" class="demo-input">
    <button onclick="visualizeMemory()" class="demo-button">Adresi Hesapla</button>
  </div>
  <div id="memory-result" class="demo-result"></div>
  <div id="memory-chain" class="demo-section"></div>
</div>

<style>
.demo-container {
  margin: 20px 0;
  padding: 20px;
  border: 1px solid var(--border-color);
  border-radius: 8px;
  background: var(--card-bg);
  color: var(--text-color);
  transition: background-color 0.3s ease, border-color 0.3s ease, color 0.3s ease;
}

.demo-title {
  margin-top: 0;
  color: var(--text-color);
}

.demo-input-section {
  margin-bottom: 15px;
}

.demo-label {
  display: block;
  margin-bottom: 5px;
  font-weight: bold;
  color: var(--text-color);
}

.demo-input {
  width: 100%;
  max-width: 300px;
  padding: 8px;
  font-family: monospace;
  border: 1px solid var(--border-color);
  border-radius: 4px;
  background: var(--bg-color);
  color: var(--text-color);
  margin-bottom: 10px;
  transition: background-color 0.3s ease, border-color 0.3s ease, color 0.3s ease;
}

.demo-button {
  margin-top: 10px;
  padding: 8px 16px;
  background: var(--link-color);
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  transition: background-color 0.3s ease;
}

.demo-button:hover {
  background: var(--link-hover);
}

.demo-result {
  margin-top: 15px;
  padding: 10px;
  background: var(--bg-color);
  border-radius: 4px;
  font-family: monospace;
  color: var(--text-color);
}

.demo-section {
  margin-top: 15px;
  color: var(--text-color);
}

.memory-chain-item {
  padding: 8px;
  margin: 5px 0;
  background: var(--bg-color);
  border-left: 3px solid var(--link-color);
  border-radius: 4px;
  font-family: monospace;
}
</style>

<script>
function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

function hexToInt(hex) {
  return parseInt(hex.replace(/^0x/, ''), 16);
}

function intToHex(num) {
  return '0x' + num.toString(16).toUpperCase().padStart(8, '0');
}

function visualizeMemory() {
  const baseAddressInput = document.getElementById('base-address').value;
  const offsetInput = document.getElementById('offset').value;
  const resultDiv = document.getElementById('memory-result');
  const chainDiv = document.getElementById('memory-chain');
  
  try {
    const baseAddress = hexToInt(baseAddressInput);
    const offset = hexToInt(offsetInput);
    const finalAddress = baseAddress + offset;
    
    resultDiv.innerHTML = `
      <strong>Hesaplama:</strong><br>
      Base Address: ${escapeHtml(baseAddressInput)}<br>
      Offset: ${escapeHtml(offsetInput)}<br>
      <strong>Final Adres: ${intToHex(finalAddress)}</strong><br>
      <strong>Decimal: ${finalAddress}</strong>
    `;
    
    // Para manipülasyonu için pointer zinciri örneği göster
    if (offsetInput === '0x4eb300' || offsetInput.toLowerCase() === '4eb300') {
      chainDiv.innerHTML = `
        <strong>Örnek: Para Manipülasyonu Zinciri</strong>
        <div class="memory-chain-item">
          <strong>Adım 1:</strong> base_address + 0x4eb300 oku<br>
          <code>int ecx = mem.Read&lt;int&gt;(base_address + 0x4eb300);</code>
        </div>
        <div class="memory-chain-item">
          <strong>Adım 2:</strong> ecx + 0x140f0 oku<br>
          <code>ecx = mem.Read&lt;int&gt;(ecx + 0x140f0);</code>
        </div>
        <div class="memory-chain-item">
          <strong>Adım 3:</strong> ecx + 0x5d0 (para değeri) oku/yaz<br>
          <code>int money = mem.Read&lt;int&gt;(ecx + 0x5d0);</code><br>
          <code>mem.Write&lt;int&gt;(ecx + 0x5d0, 13371137);</code>
        </div>
      `;
    } else {
      chainDiv.innerHTML = '';
    }
  } catch (e) {
    resultDiv.innerHTML = `<span style="color: #dc3545;">Hata: Geçersiz hex adres</span>`;
    chainDiv.innerHTML = '';
  }
}
</script>

## Etik ve anti-cheat

Tek oyunculu oyunlarda öğrenme ve deney genelde kabul edilir; çok oyunculuda başkalarının deneyimini bozmamak ve ToS’a uymak gerekir. Modern oyunlar memory taraması, kod bütünlüğü ve sunucu tarafı doğrulama kullanıyor; bu yüzden birçok hack sadece tek oyunculuda işe yarıyor.

## Özet

Memory manipulation, yazılımın nasıl çalıştığını anlamak için temel bir beceri. Virtual memory, Cheat Engine ve C++ ile oyun değerlerini okuyup yazabiliyorsun. Çok oyunculu ortamlarda sorumlu ve etik kullanım önemli.

**Öne çıkanlar:** Virtual memory ve process izolasyonu; Cheat Engine ile adres bulma ve değiştirme; Windows API ile C++; offset ve pointer zincirleri; para ve yetenek manipülasyonu örnekleri.

## İlgili içerik

- [CVE-2025-66019: pypdf'te LZW Decompression DoS Zafiyeti](/2025/12/20/cve-2025-66019-pypdf-lzw-dos-tr/)
- [GeoPandas'ta SQL Injection](/2025/12/27/sql-injection-geopandas-tr/)

