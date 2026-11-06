---
layout: post
title: "Game Hacking 101: Reverse Engineering Copy Protection in Need for Speed Most Wanted (2005)"
date: 2026-09-16
author: Yunus Aydın
categories: reverse-engineering ghidra
---

# Game Hacking 101: Reverse Engineering Copy Protection in Need for Speed Most Wanted (2005)

16 September 2026 • Yunus Aydın

In [Game Hacking 101: Memory Manipulation](/2026/01/03/game-hacking-101-memory-manipulation/) we followed live values with Cheat Engine. Same idea here — understand the program, don’t worship the trainer — but the question is: **why does this 2005 racer boot from a hard disk with no CD, and why does deleting the registry “CD key” not produce a key prompt?**

The answers live in `speed.exe`. Analysis used [Ghidra](https://ghidra-sre.org/) plus a local MCP bridge (`search_strings`, `get_xrefs_to`, `decompile_function`) on project `nfs`. Live writes used `ReadProcessMemory` / `WriteProcessMemory`; the C++ helpers will live in a gist, not in this post.

This is **static reverse-engineering** of a Black Edition–era PC copy we already had installed. Not a crack tutorial, not a keygen, not advice for modern online games.

## What this copy is

Need for Speed Most Wanted (EA Black Box). Retail PC in 2005 was InstallShield (20-character key → registry) + SafeDisc (`*_code.exe` wrapper) + a runtime optical check (`\bin.dat` on a CD-ROM, *Please insert Disc 2*).

This machine’s tree is a later “Win Setup / Black Edition” pack: InstallAware, `speed.exe` dated 5 January 2006 (~6.0 MB, ~26k functions, image base `00400000`, CRT `007c4040`), leftover `Need for Speed Most Wanted_code.exe` in `Support\`, VB6 `nfsmwres.exe` for resolution. Launch is **direct `speed.exe`**. Root `lua5.1.dll` is InstallAware; Black Box Lua is already inside `speed.exe`. “The crack” is **several products stacked**, not one JMP.

There is no clean 2005 DVD `speed.exe` in the Ghidra project, so this is not a byte-diff of “Black Edition original vs scene patch.” It is what **this image does that retail is not supposed to do**.

## Inventory

| What | Retail | This image |
| --- | --- | --- |
| Installer | InstallShield key UI; refuse and stop | InstallAware: **no key UI**, still writes 20-char `ergc` |
| SafeDisc | You attach to `*_code.exe` | Wrapper in `Support\` is **not launched** |
| `ergc` | Missing key → serial / exit | `FUN_006cb680` reads it, **always `return 1`** |
| Disc scan | `\bin.dat` only on `DRIVE_CDROM` | `DAT_008a9260 = 1` → HDD is a candidate |
| Insert-disc | `FUN_006cbb70` returns 0 → `_exit(0)` + MessageBox | Flag 1 → **never the MessageBox** (`006cbbf8`: `mov al,1 / ret`) |
| In-game disc | Missing marker → false | `FUN_006c2700` **true on every path** |
| FE disc screen | `DiscErrorPC.fng` stays up | `FUN_006c2700() \|\| open("foobar")` → next FNG |
| First car | Cheap starter; GTR after Razor | `FUN_00590d40` plants `M3GTRCAREERSTART`, row `0x12345678` / `0xF0020`, `save+0xa8` that hash, event `16.1.0` |
| Lot | Rank 1, locked | `FUN_0057f4d0` `save+0xb0 = 0x10`. Rank ≥ 16 uses FNG `0x3a12d2f5` |
| Map races | `AddAvailableEventToMap` drops icons | `005dbf20` = `C2 08 00` (`RET 8`) |
| Buy | Price subtracted from `save+0xb4` | Type `0x12` `FUN_00590bd0(..., 0xF0002)` — **never reads cash**. Cap: `0xF0002` count &lt; 25 |
| Safehouse $ | Bounty = cops, Cash = wallet | **Bounty** = `FUN_00590500` (clone list prices). **Cash** = `+0xb4` (New Career **0**) |

`DAT_00926124` (cheat id 0 “every event complete”) is **0**. Not an unlock-all trainer. Extra Options / `.asi` / `givemegtr` are not in this EXE. Typed cheats are only `burgerking` / `castrol`. EA patch URLs (`http://patches.ea.com/nfs_mostwanted/...`) are leftovers, not a live channel. No SecuROM strings.

The sections below are CFG for the protection rows, then the GTR seed, then live experiments (cash, buy, KnockoutRacer, heat, Extreme).

## Finding `ergc`

String `Software\Electronic Arts\EA Games\Need for Speed Most Wanted\ergc` at `008af628`, packed with boot names (`StreamingInstall`, `Language`, `CD Drive`, `Install Dir`, *Please insert Disc 2*, the patch URLs). One xref: `FUN_006cb680`. *Please insert Disc 2* xrefs `FUN_006cbb70`. Same neighborhood, two jobs. On 64-bit Windows the 32-bit process sees `HKLM\SOFTWARE\...`; Explorer shows `WOW6432Node`. Same key.

## The CD key path: read, never gate

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
/* missing key: also continue */

RegCloseKey(hKey);
/* then Install Dir, CD Drive, FirstTime, CacheSize, SwapSize, Language, StreamingInstall */
return 1;   /* always */
```

`ergc` is the **default value** of that subkey (`lpValueName == NULL`), 20 characters, dashes stripped. Failure is `DAT_00982974 = 0`, not `MessageBox` / `exit`. The function is also the real config loader — that is why the string survived after the installer stopped asking for a key. The other xref of `DAT_00982974` is `FUN_00553980` (online login UI), not the boot gate in `FUN_006cb400`.

Delete `ergc`: the game still starts. Predicted by the CFG.

Caller `FUN_006cba70`: `SetErrorMode(1)`, clear path buffers, `FUN_006cb680`, then EA “File System” (`FUN_007ee0ca`) — allocator pools, current directory, not SafeDisc.

## The disc path: this is the real lock

`FUN_006c25a0` builds `X:\bin.dat` in place (no C string) and opens it via `FUN_0065f7a0`. Runtime proof of purchase was **a drive with `\bin.dat` at the root**, not a cryptographic disc check.

`FUN_006c2620` walks `GetLogicalDrives()`. CD-ROM (`GetDriveTypeA == 5`) is always tried. If `DAT_008a9260 != 0`, non-remote fixed disks are tried too. **In this image that dword is already `1`**, sitting in `.data` next to ASCII `911turbo`. A successful letter is stored as `"X:\"` in `DAT_0093dc08`.

Boot (`FUN_006cb400`):

```c
FUN_006cba70();                 /* registry + paths */
uVar = FUN_006cbb70();          /* disc UI / check */
if ((char)uVar == 0)
    _exit(0);
```

The only hard exit is `FUN_006cbb70` returning 0. Disassembly still has the MessageBox for *Please insert Disc 2* and a `"foobar"` probe, but with `DAT_008a9260 == 1` the CPU takes `006cbbf8` (`B0 01 5E C3` — `mov al,1 / pop esi / ret`) and never falls in. Strings remain because nobody stripped them.

Mid-game `FUN_006c2700` returns `true` on every path (missing marker included). Frontend boot `FUN_0058e9c0`: if the screen is `DiscErrorPC.fng`, `FUN_006c2700() || open("foobar")` advances to the next FNG. The boolean already stuck at true makes `"foobar"` redundant; it is a second fingerprint.

## Career: how the BMW M3 GTR is handed to you

Not DRM, not Extra Options `givemegtr`. A **new-profile seed**.

**1. Garage.** `FUN_005ab450` copies the profile name, then `FUN_00590d40(this + 0x414)`. Garage is **200 slots × 0x14**. Empty = `-1`. Current-car hash is `save+0xa8`, live under `DAT_0091cf90 + 0x10`.

**2. Allocator.** `FUN_00590a20(garage, name)`: `bStringHash` (`FUN_005af1c0`: seed `-1`, `h = h * 33 + toupper(c)`), catalog lookup `FUN_00748130`, fill a row (`+0x00` identity, `+0x0C` flags default `0xF0010`, `+0x10` paint). `M3GTRCAREERSTART` → `0x38A3C42C` (string VA `008a1044`). That is a **career-start collection key**, not folder `CARS\BMWM3GTR` (`0x6C401512`). Two names, two hashes, one car on disk.

**3. Sentinel.** Immediately after, `FUN_00590d40` overwrites row 0 (`00590dad`): identity `0x12345678`, flags `0xF0020`. Not a hash — a programmer tag for the unique career-start GTR. Catalog walk / vinyl rows do not replace slot 0.

**4. Bind + `16.1.0`.** `FUN_005a3950` (new career), `param_1 != 0` and `DAT_00926126 == 0`: writes `0x38A3C42C` to `save+0xa8` and queues event `16.1.0`. First race is in the GTR because current-car is already set.

**5. Latch.** `FUN_0057f570` while `save+0xb0 == 0x10`: if `16.2.1` is not done, rewrite `+0xa8` and maybe re-queue `16.1.0`. Prologue latch, not a second gift.

**6. Spawn.** `FUN_007c2780`: id `0x12345678` or `hash("M3GTRCAREERSTART")` → skip normal garage lookup, visual `0xF0001`. Frontend `FUN_007a3df0` (kind `0x10`) only accepts that hash (`FUN_00460bf0`, same ×33 without `toupper`; string is already uppercase).

Cheat Engine “current car”: `DAT_0091cf90` → `+0x10` → `+0xa8`, expecting `0x38A3C42C` during the prologue — not a VA per vehicle. Next to the no-CD flag, ASCII `911turbo` / `bmwm3gtre46` are **debug / NIS spawn fallbacks** (`DAT_00926064`); in a normal career boot that flag is 0.

### Typed cheats: `burgerking` and `castrol`

Not a CD key and not the GTR seed. 2005 promo strings, typed in-game. `FUN_0065d9e0` keeps a 32-key ring; a match calls `FUN_0065c2e0`. Table at `008f8520`, **0x34**-byte rows, **two** of them:

| Offset | Id | Word | Effect |
| --- | --- | --- | --- |
| `008f8520` | 3 | `burgerking` | FE `0x98257537` — BK challenge (Mustang), not the career GTR |
| `008f8554` | 4 | `castrol` | FE `0x6521e5c2`; `save+0xac \|= 0x40000` — Castrol `CARS\FORDGT` |

Dispatcher ids 0 / 1 / 2 / 5 have **no rows** in this build (`givemegtr` is Extra Options, not here).

## CASH_DATA: `save+0xb4`, live 149116544 → 123456

Safehouse `CASH_DATA` is **not** bounty. `FUN_00532220` formats `FUN_00590500` as `BOUNTY_DATA` and `[save+0xb4]` as `CASH_DATA` (same `"%$d"`). On GOATS, `[save+0xb4] = 149116544`. No `MOV 150000000` in the image.

New Career zeros `+0xb4` (`FUN_0057f4d0` → `param_1[3] = 0`). This profile did not stay at zero: sell type **0x13** **ADDs** into `+0xb4` (`FUN_007c4b80` float→int). Buy never **SUBs**. Inner wallet `garage+0x8CA0` is a different bucket (feeds bounty). `AwardCash` (`00612130`) writes a racer `+0x120` float, not `+0xb4`.

Live: `WriteProcessMemory` `save+0xb4 ← 123456`. HUD Cash became 123456; inner wallet untouched. In-game / blacklist `$` print bounty (`FUN_00590500`), not `CASH_DATA`.

## How every car buys when cash is 0

Two gates. Cash is **not** on the second.

1. **Lot open.** New Career sets rank 16 (`save+0xb0 = 0x10`). Car-select FNG `0x3a12d2f5` when that byte ≥ 16. You look post-Razor.
2. **Buy does not charge.** Type `0x12` → `FUN_00590bd0(garage, hash, 0xF0002)`. Checks: fewer than 25 copies with that flag + empty slot. `save+0xb4` is never read. On-screen price is display only. Type `0x13` is the inverse: **adds** into `+0xb4`.

Retail subtracts cash on buy. This EXE clones a slot and leaves the wallet alone. Each clone is `0xF0002`, so bounty inflates. Cash inflation is the sell ADD, not buy.

## KnockoutRacer

Scripts already bind `KnockoutRacer` → `FUN_00611440` (`void(uint handle)` cdecl). `GetRacerIndex` (`FUN_0060dae0`) looks up the hashmap **value**, not the slot key (`0x1b14`…) and not ISimable. Wrong pointer: `idx - 1 == -2` writes `GRace - 0x310`; slot `+0x1c` unchanged. Correct handle sets `+0x1c = 1` (DNF). Local player is the slot with `+4 == 0`. Only call while `GRace+0x1960 == 1` (in a race). This is the script DNF path fired from outside, not a crack gate. Event id `16.1.0` lives in `GLOBAL\gameplay.bin`; knockout is C.

## Heat 10 in free roam

Heat is after `SetRoaming` (`+0x1960 = 0`). HUD ceiling is **10** (`FUN_007a5aa0`, format `HEAT_X%.0f`). Script `SetWorldHeat` (`00612660`) QI’s IHeat (`LAB_004037e0`) and calls vfunc `+8`. Calling that cdecl path on GOATS roam left `IHeat+0x1c = 5.0`. Direct `FUN_00409060` thiscall (ecx = iface) wrote **10.0**. `GetWorldHeat` is `*(float *)(IHeat + 0x1c)`. Ambient cops still need `FUN_004293d0` and a loaded region (`+0x196c != 0`); they are not disabled by the GTR.

## Garage Extreme: collection hash, mesh trap

Shop Street/Pro/Extreme lives in a **0x198** blob (`garage+4000 + idx×0x198`, idx = slot `+0x10`): seven level dwords at `+0x118`, package mask at `+0x134`. Ceiling is per **collection hash at slot+8** (`FUN_00672e80`). Slot+8 is also the mesh.

GOATS current slot ident `0x64`, native hash `0x4E085FB3`: ceiling `0,0,0,0,0,0,3` — only weight (cat 6) has Extreme. Writing cat6=3 + mask on that hash is the honest max. Copying another collection’s hash (`0x46118CE4`) onto slot+8 filled HUD levels and **deleted the mesh**. Restoring `0x4E085FB3` brought the body back. Unique GTR rows (`vis 0xFF`, spawn `0xF0001`) have no shop blob.

## Conclusion

The pack is **not one JMP**. Two layers:

1. **Protection:** InstallAware (no key UI) + unused SafeDisc + `ergc` not a gate + `DAT_008a9260` / `FUN_006c2700` / `DiscErrorPC`+`foobar` no-CD.
2. **Gameplay retail does not run:** New Career GTR seed + rank 16 + `AddAvailableEventToMap` `RET 8` + buy that never reads cash. Inflated bounty and an empty map are **side effects**, not a gift `MOV`.

Deleting `ergc` tests lock #1. This binary does not use it at boot. Ghidra MCP did the unglamorous part: xrefs on one string, then the caller, then the caller’s caller, until `_exit(0)` had nothing left to do.

Do not take this as a recipe to strip protection from current storefront games. The useful skill is the opposite of a crack pack: **read the control flow until the myth dies**.
