---
layout: post
title: "Game Hacking 101: Need for Speed Most Wanted (2005) Kopya Korumasını Tersine Mühendislik"
date: 2026-09-16
author: Yunus Aydın
categories: reverse-engineering ghidra
---

# Game Hacking 101: Need for Speed Most Wanted (2005) Kopya Korumasını Tersine Mühendislik

16 Eylül 2026 • Yunus Aydın

[Game Hacking 101: Bellek Manipülasyonu](/2026/01/03/game-hacking-101-memory-manipulation/) yazısında Cheat Engine ile canlı değerlerin peşinden gitmiştik. Aynı dil: programı anlamak, trainer’a tapmamak. Soru: **bu 2005 yarışçı CD olmadan hard diskten neden açılıyor, registry’deki “CD key” silinince neden key ekranı gelmiyor?**

Cevaplar `speed.exe` içinde. Analiz [Ghidra](https://ghidra-sre.org/) ve yerel MCP (`search_strings`, `get_xrefs_to`, `decompile_function`) ile `nfs` projesinde. Canlı yazımlar `ReadProcessMemory` / `WriteProcessMemory`; C++ yardımcıları bu yazıda değil, gist’te.

Zaten kurulu bir Black Edition dönemi PC kopyasının **statik tersine mühendisliği**. Crack tarifi, keygen veya modern çevrimiçi oyun tavsiyesi değil.

## Bu kopya nedir

Need for Speed Most Wanted (EA Black Box). 2005 PC perakende: InstallShield (20 karakter key → registry) + SafeDisc (`*_code.exe`) + runtime optik kontrol (CD-ROM kökünde `\bin.dat`, *Please insert Disc 2*).

Bu makinedeki ağaç “Win Setup / Black Edition”: InstallAware, 5 Ocak 2006 tarihli ~6.0 MB `speed.exe` (~26 bin fonksiyon, image base `00400000`, CRT `007c4040`), `Support\` altında artakalan `Need for Speed Most Wanted_code.exe`, çözünürlük için VB6 `nfsmwres.exe`. Süreç **doğrudan `speed.exe`**. Kök `lua5.1.dll` InstallAware; Black Box Lua zaten `speed.exe` içinde. “Crack” **üst üste binmiş birkaç ürün**, tek JMP değil.

Ghidra’da temiz 2005 DVD `speed.exe` yok; “Black Edition orijinali / scene yaması” byte-diff’i iddia etmiyoruz. Aşağıdakiler **bu image’de retail’den sapma**.

## Envanter

| Ne | Retail | Bu image |
| --- | --- | --- |
| Kurulum | InstallShield key UI; red edilince durur | InstallAware: **key UI yok**, yine 20 karakter `ergc` |
| SafeDisc | Attach `*_code.exe` | Wrapper `Support\`’ta **çalıştırılmıyor** |
| `ergc` | Yoksa serial / çıkış | `FUN_006cb680` okur, **her zaman `return 1`** |
| Disk taraması | Yalnızca `DRIVE_CDROM` üzerinde `\bin.dat` | `DAT_008a9260 = 1` → HDD de aday |
| Insert-disc | `FUN_006cbb70` 0 → `_exit(0)` + MessageBox | Bayrak 1 → **MessageBox’a düşülmez** (`006cbbf8`: `mov al,1 / ret`) |
| Oyun içi disk | Marker yoksa false | `FUN_006c2700` **her yolda `true`** |
| FE disk ekranı | `DiscErrorPC.fng` kalır | `FUN_006c2700() \|\| open("foobar")` → sonraki FNG |
| İlk araba | Ucuz starter; GTR Razor’dan sonra | `FUN_00590d40` `M3GTRCAREERSTART` eker, satır `0x12345678` / `0xF0020`, `save+0xa8` o hash, event `16.1.0` |
| Galeri | Rank 1, kilitli | `FUN_0057f4d0` `save+0xb0 = 0x10`. Rank ≥ 16 FNG `0x3a12d2f5` |
| Harita yarışları | `AddAvailableEventToMap` ikon basar | `005dbf20` = `C2 08 00` (`RET 8`) |
| Alış | Fiyat `save+0xb4`’ten düşer | Tip `0x12` `FUN_00590bd0(..., 0xF0002)` — **nakit okunmaz**. Tavan: `0xF0002` sayısı &lt; 25 |
| Safehouse $ | Bounty cop, Cash cüzdan | **Bounty** = `FUN_00590500` (kopya list fiyatı). **Cash** = `+0xb4` (New Career **0**) |

`DAT_00926124` (cheat id 0 “her event complete”) **0**. Unlock-all trainer değil. Extra Options / `.asi` / `givemegtr` yok. Yazılan hileler yalnızca `burgerking` / `castrol`. `http://patches.ea.com/nfs_mostwanted/...` artığı, canlı kanal değil. SecuROM string’i yok.

Aşağıda koruma satırlarının CFG’si, sonra GTR tohumu, sonra canlı deneyler (nakit, alış, KnockoutRacer, heat, Extreme).

## `ergc`’yi bulmak

`Software\Electronic Arts\EA Games\Need for Speed Most Wanted\ergc` `008af628`, boot isimleriyle aynı kümede (`StreamingInstall`, `Language`, `CD Drive`, `Install Dir`, *Please insert Disc 2*, yama URL’leri). Tek xref: `FUN_006cb680`. *Please insert Disc 2* → `FUN_006cbb70`. Aynı mahalle, iki iş. 64-bit Windows’ta 32-bit süreç `HKLM\SOFTWARE\...` görür; Explorer `WOW6432Node` gösterir. Aynı kayıt.

## CD-key yolu: okur, kapı olmaz

```c
LSTATUS st = RegOpenKeyExA(
    HKEY_LOCAL_MACHINE,
    "Software\\Electronic Arts\\EA Games\\Need for Speed Most Wanted\\ergc",
    0, KEY_READ, &hKey);

if (st == 0) {
    st = RegQueryValueExA(hKey, NULL, NULL, &type, &DAT_00982974, &cb);
    if (st != 0)
        DAT_00982974 = 0;
}
/* anahtar yoksa da devam */

RegCloseKey(hKey);
/* sonra Install Dir, CD Drive, FirstTime, CacheSize, SwapSize, Language, StreamingInstall */
return 1;   /* her zaman */
```

`ergc` o alt anahtarın **varsayılan değeri** (`lpValueName == NULL`), 20 karakter, tireler silinmiş. Başarısızlık `DAT_00982974 = 0`; `MessageBox` / `exit` değil. Aynı fonksiyon asıl config yükleyici — installer key sormayı bırakınca string’in durmasının nedeni. `DAT_00982974`’ün diğer xref’i `FUN_00553980` (online login UI), `FUN_006cb400` boot kapısı değil.

`ergc` sil: oyun açılır. CFG bunu söylüyordu.

Çağıran `FUN_006cba70`: `SetErrorMode(1)`, path buffer’ları sıfırla, `FUN_006cb680`, sonra EA “File System” (`FUN_007ee0ca`) — havuz, çalışma dizini. SafeDisc değil.

## Disk yolu: asıl kilit burada

`FUN_006c25a0` `X:\bin.dat` yolunu yerinde kurar (C string yok), `FUN_0065f7a0` ile açar. Runtime kanıtı kriptografik disk check değil: **kökte `\bin.dat` olan bir sürücü**.

`FUN_006c2620` `GetLogicalDrives()` dolaşır. CD-ROM (`GetDriveTypeA == 5`) her zaman denenir. `DAT_008a9260 != 0` ise uzak olmayan sabit diskler de. **Bu image’de dword zaten `1`**, yanında ASCII `911turbo`. Başarılı harf `DAT_0093dc08`’e `"X:\"` yazar.

Boot (`FUN_006cb400`):

```c
FUN_006cba70();                 /* registry + yollar */
uVar = FUN_006cbb70();          /* disk UI / check */
if ((char)uVar == 0)
    _exit(0);
```

Tek sert çıkış `FUN_006cbb70`’in 0 dönmesi. Disassembly’de *Please insert Disc 2* MessageBox ve `"foobar"` duruyor; `DAT_008a9260 == 1` iken CPU `006cbbf8` alır (`B0 01 5E C3` — `mov al,1 / pop esi / ret`) ve düşmez. String’ler duruyor çünkü kimse silmemiş.

Oyun içi `FUN_006c2700` her yolda `true` (marker yok dahil). Frontend `FUN_0058e9c0`: ekran `DiscErrorPC.fng` ise `FUN_006c2700() || open("foobar")` sonraki FNG’ye geçer. Boolean zaten true; `"foobar"` ikinci parmak izi.

## Kariyer: BMW M3 GTR nasıl veriliyor

DRM değil, Extra Options `givemegtr` değil. **New-profile tohumu**.

**1. Garaj.** `FUN_005ab450` isim kopyalar, `FUN_00590d40(this + 0x414)`. Garaj **200 slot × 0x14**. Boş = `-1`. Current-car hash `save+0xa8`, canlı `DAT_0091cf90 + 0x10`.

**2. Ayırıcı.** `FUN_00590a20(garage, name)`: `bStringHash` (`FUN_005af1c0`: seed `-1`, `h = h * 33 + toupper(c)`), katalog `FUN_00748130`, satır (`+0x00` kimlik, `+0x0C` bayrak varsayılan `0xF0010`, `+0x10` boya). `M3GTRCAREERSTART` → `0x38A3C42C` (string VA `008a1044`). **Kariyer-başlangıç koleksiyon anahtarı**; klasör `CARS\BMWM3GTR` (`0x6C401512`) değil. İki isim, iki hash, diskte aynı araba.

**3. Sentinel.** Hemen ardından `FUN_00590d40` slot 0 üzerine yazar (`00590dad`): kimlik `0x12345678`, bayrak `0xF0020`. Hash değil — unique kariyer-başlangıç GTR etiketi. Katalog / vinyl slot 0’ı değiştirmez.

**4. Bağla + `16.1.0`.** `FUN_005a3950` (yeni kariyer), `param_1 != 0` ve `DAT_00926126 == 0`: `save+0xa8`’e `0x38A3C42C`, event `16.1.0` kuyruğa. İlk yarış GTR’de çünkü current-car zaten yazılmış.

**5. Mandal.** `FUN_0057f570`, `save+0xb0 == 0x10` iken: `16.2.1` bitmemişse `+0xa8`’i yeniden yazar, `16.1.0` tekrar kuyruğa girebilir. Prologue mandalı; ikinci hediye değil.

**6. Spawn.** `FUN_007c2780`: id `0x12345678` veya `hash("M3GTRCAREERSTART")` → normal lookup atlanır, görsel `0xF0001`. Frontend `FUN_007a3df0` (tür `0x10`) yalnızca o hash’i kabul eder (`FUN_00460bf0`, aynı ×33, `toupper` yok; string zaten büyük harf).

Cheat Engine “current car”: `DAT_0091cf90` → `+0x10` → `+0xa8`, prologue’da `0x38A3C42C` — araç başına VA değil. No-CD bayrağının yanındaki `911turbo` / `bmwm3gtre46` **debug / NIS spawn fallback** (`DAT_00926064`); normal kariyerde bayrak 0.

### Yazılan hileler: `burgerking` ve `castrol`

CD-key değil, GTR tohumu değil. 2005 promo kelimeleri, oyun içinde yazılır. `FUN_0065d9e0` 32 tuş halkası; eşleşince `FUN_0065c2e0`. Tablo `008f8520`, kayıt **0x34** byte, **iki** satır:

| Offset | Id | Kelime | Etki |
| --- | --- | --- | --- |
| `008f8520` | 3 | `burgerking` | FE `0x98257537` — BK challenge (Mustang), kariyer GTR’si değil |
| `008f8554` | 4 | `castrol` | FE `0x6521e5c2`; `save+0xac \|= 0x40000` — Castrol `CARS\FORDGT` |

Dispatcher id 0 / 1 / 2 / 5 bu build’de **satırsız** (`givemegtr` Extra Options, burada yok).

## CASH_DATA: `save+0xb4`, canlı 149116544 → 123456

Safehouse `CASH_DATA` **bounty değil.** `FUN_00532220` `FUN_00590500`’ü `BOUNTY_DATA`, `[save+0xb4]`’ü `CASH_DATA` basar (aynı `"%$d"`). GOATS’ta `[save+0xb4] = 149116544`. Image’de `MOV 150000000` yok.

New Career `+0xb4`’ü 0 yazar (`FUN_0057f4d0` → `param_1[3] = 0`). Bu profil o sıfırda kalmamış: satış tipi **0x13** `+0xb4`’e **ADD** eder (`FUN_007c4b80` float→int). Alış **SUB yok**. Inner cüzdan `garaj+0x8CA0` ayrı kova (bounty’ye girer). `AwardCash` (`00612130`) racer `+0x120` float; `+0xb4` değil.

Canlı: `WriteProcessMemory` `save+0xb4 ← 123456`. HUD Cash hemen 123456; inner’a dokunulmadı. Oyun içi / blacklist `$` bounty basar (`FUN_00590500`), `CASH_DATA` değil.

## Cash 0 iken her araba nasıl alınır

İki kapı. Nakit **ikincisinde yok**.

1. **Galeri açık.** New Career rank 16 (`save+0xb0 = 0x10`). Byte ≥ 16 ise car-select FNG `0x3a12d2f5`. Razor sonrası gibi görünürsün.
2. **Alış nakit istemiyor.** Tip `0x12` → `FUN_00590bd0(garaj, hash, 0xF0002)`. Kontrol: o bayraklı kopya &lt; 25 + boş slot. `save+0xb4` okunmaz. Ekrandaki fiyat süs. Tip `0x13` tam tersi: `+0xb4`’e **ekler**.

Retail alışta nakit düşer. Bu EXE kopya basar, cüzdanı değiştirmez. Her kopya `0xF0002` → bounty şişer. Nakit şişmesi satış ADD’inden, alıştan değil.

## KnockoutRacer

Script zaten `KnockoutRacer` → `FUN_00611440` (`void(uint handle)` cdecl) bağlı. `GetRacerIndex` (`FUN_0060dae0`) hashmap **value** arar; slot key (`0x1b14`…) ve ISimable değil. Yanlış pointer: `idx - 1 == -2` → `GRace - 0x310`; slot `+0x1c` değişmez. Doğru handle `+0x1c = 1` (DNF). Yerel oyuncu `+4 == 0` olan slot. Yalnız `GRace+0x1960 == 1` iken (yarış). Crack kapısı değil; script DNF yolunu dışarıdan tetiklemek. Event id `16.1.0` `GLOBAL\gameplay.bin`’de; knockout C’de.

## Free roam’da heat 10

Heat `SetRoaming` sonrası (`+0x1960 = 0`). HUD tavanı **10** (`FUN_007a5aa0`, format `HEAT_X%.0f`). Script `SetWorldHeat` (`00612660`) IHeat QI (`LAB_004037e0`), vfunc `+8`. GOATS roam’da o cdecl yol `IHeat+0x1c = 5.0` bıraktı. Doğrudan `FUN_00409060` thiscall (ecx = iface) **10.0** yazdı. `GetWorldHeat` = `*(float *)(IHeat + 0x1c)`. Ambient cop hâlâ `FUN_004293d0` ve yüklü bölge (`+0x196c != 0`) ister; GTR cop kapatmaz.

## Garaj Extreme: koleksiyon hash’i, mesh tuzak

Shop Street/Pro/Extreme **0x198** blokta (`garaj+4000 + idx×0x198`, idx = slot `+0x10`): yedi seviye `+0x118`, paket mask `+0x134`. Tavan **slot+8 koleksiyon hash’ine** bağlı (`FUN_00672e80`). Slot+8 aynı zamanda mesh.

GOATS current slot ident `0x64`, native hash `0x4E085FB3`: tavan `0,0,0,0,0,0,3` — yalnızca weight (cat 6) Extreme. O hash’te cat6=3 + mask dürüst tavan. Başka koleksiyonun hash’ini (`0x46118CE4`) slot+8’e yazmak HUD’u doldurur, **mesh’i siler**. `0x4E085FB3` geri gelince gövde döndü. Unique GTR satırları (`vis 0xFF`, spawn `0xF0001`) shop bloğu yok.

## Sonuç

Paket **tek JMP değil**, iki katman:

1. **Koruma:** InstallAware (key UI yok) + SafeDisc unused + `ergc` kapı değil + `DAT_008a9260` / `FUN_006c2700` / `DiscErrorPC`+`foobar` no-CD.
2. **Oyun akışı (retail’de yok):** New Career GTR tohumu + rank 16 + `AddAvailableEventToMap` `RET 8` + alışın nakit okumaması. Bounty şişmesi ve boş harita bunların **yan etkisi**, ayrı hediye MOV değil.

`ergc` silmek kilit #1’i test eder; bu binary onu boot’ta kullanmıyor. Ghidra MCP sıkıcı kısmı yaptı: bir string’e xref, sonra çağıran, sonra onun çağıranı, `_exit(0)`’ın işi kalmayana kadar.

Güncel vitrin oyunlarından koruma soymak için tarif değil. Asıl beceri crack paketinin tersi: **mit ölene kadar kontrol akışını oku**.
