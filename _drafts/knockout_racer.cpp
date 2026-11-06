// NFS Most Wanted (Black Edition / this speed.exe) — knock out every racer except local player.
//
// Live-verified against:
//   speed.exe  SHA256 80774C2E5D619B4F120B48D4462896FD504C263399D203A238769CFFDE1D253C
//   image base 0x00400000
//
// KnockoutRacer is cdecl void(uint handle) at 0x00611440. The argument is NOT the
// racer key in the GRace slot (0x1b14…) and NOT slot+4. GetRacerIndex walks
// DAT_0092FEB4 (sorted {key,handle} pairs) and matches puVar[1] == handle, i.e. the
// hashmap VALUE. Passing the key or the ISimable at handle+4 returns -1 and then
// FUN_005dbed0(GRace, -2) writes off-object. Do not do that.
//
// Build (x64 injector is fine; shellcode is 32-bit because the game is WoW64):
//   cl /EHsc /O2 knockout_racer.cpp
//   g++ -O2 -o knockout_racer.exe knockout_racer.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

// Ghidra names from this binary.
constexpr std::uint32_t kKnockoutRacer = 0x00611440;  // FUN_00611440 cdecl
constexpr std::uint32_t kGRacePtr      = 0x0091E000;  // DAT_0091e000
constexpr std::uint32_t kSimMapPtr     = 0x0092FEB4;  // DAT_0092feb4
constexpr std::uint32_t kSimMapCount   = 0x0092FEBC;  // DAT_0092febc

constexpr std::uint32_t kRacerStride   = 0x194;
constexpr std::uint32_t kRacersOff     = 0x18;
constexpr std::uint32_t kRacerCountOff = 0x1958;
constexpr std::uint32_t kRaceModeOff   = 0x1960;  // 1 = in race
constexpr std::uint32_t kKnockedOff    = 0x1C;    // set by FUN_00600210
constexpr std::uint32_t kLocalHintOff  = 0x4;     // 0 on the local player's slot

bool Read(HANDLE proc, std::uint32_t addr, void* buf, std::size_t n) {
    SIZE_T got = 0;
    return ReadProcessMemory(proc, reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(addr)),
                             buf, n, &got) && got == n;
}

bool Write(HANDLE proc, std::uint32_t addr, const void* buf, std::size_t n) {
    SIZE_T put = 0;
    return WriteProcessMemory(proc, reinterpret_cast<LPVOID>(static_cast<uintptr_t>(addr)),
                              buf, n, &put) && put == n;
}

bool ReadU32(HANDLE proc, std::uint32_t addr, std::uint32_t* out) {
    return Read(proc, addr, out, sizeof(*out));
}

DWORD FindSpeedPid() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    for (BOOL ok = Process32First(snap, &pe); ok; ok = Process32Next(snap, &pe)) {
        if (_stricmp(pe.szExeFile, "speed.exe") == 0) {
            pid = pe.th32ProcessID;
            break;
        }
    }
    CloseHandle(snap);
    return pid;
}

// Sorted {key, handle} table. Binary search matches FUN_005d3530 / FUN_005d51f0.
bool LookupHandle(HANDLE proc, std::uint32_t key, std::uint32_t* handle) {
    std::uint32_t table = 0;
    std::uint32_t count = 0;
    if (!ReadU32(proc, kSimMapPtr, &table) || !ReadU32(proc, kSimMapCount, &count) ||
        table == 0 || count == 0 || count > 4096) {
        return false;
    }
    std::vector<std::uint32_t> pairs(count * 2);
    if (!Read(proc, table, pairs.data(), pairs.size() * sizeof(std::uint32_t))) {
        return false;
    }
    int lo = 0;
    int hi = static_cast<int>(count);
    while (lo < hi) {
        const int mid = lo + (hi - lo) / 2;
        const std::uint32_t k = pairs[static_cast<std::size_t>(mid) * 2];
        if (k < key) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo >= static_cast<int>(count) || pairs[static_cast<std::size_t>(lo) * 2] != key) {
        return false;
    }
    *handle = pairs[static_cast<std::size_t>(lo) * 2 + 1];
    return *handle != 0;
}

// 32-bit cdecl: for each handle, push handle; call KnockoutRacer; add esp,4. Then xor eax,eax; ret.
std::vector<std::uint8_t> BuildShellcode(std::uint32_t code_va,
                                         const std::vector<std::uint32_t>& handles) {
    std::vector<std::uint8_t> code;
    code.reserve(handles.size() * 12 + 3);
    auto emit = [&](std::uint8_t b) { code.push_back(b); };
    auto emit32 = [&](std::uint32_t v) {
        code.push_back(static_cast<std::uint8_t>(v));
        code.push_back(static_cast<std::uint8_t>(v >> 8));
        code.push_back(static_cast<std::uint8_t>(v >> 16));
        code.push_back(static_cast<std::uint8_t>(v >> 24));
    };
    for (std::uint32_t h : handles) {
        emit(0x68);  // push imm32
        emit32(h);
        const std::uint32_t next_ip = code_va + static_cast<std::uint32_t>(code.size()) + 5;
        emit(0xE8);  // call rel32
        emit32(kKnockoutRacer - next_ip);
        emit(0x83);
        emit(0xC4);
        emit(0x04);  // add esp, 4
    }
    emit(0x31);
    emit(0xC0);  // xor eax, eax
    emit(0xC3);  // ret
    return code;
}

bool CallKnockoutRacer(HANDLE proc, const std::vector<std::uint32_t>& handles) {
    void* mem = VirtualAllocEx(proc, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE,
                               PAGE_EXECUTE_READWRITE);
    if (!mem) {
        std::fprintf(stderr, "VirtualAllocEx failed (%lu)\n", GetLastError());
        return false;
    }
    const auto va = static_cast<std::uint32_t>(reinterpret_cast<uintptr_t>(mem));
    const std::vector<std::uint8_t> code = BuildShellcode(va, handles);
    bool ok = Write(proc, va, code.data(), code.size());
    if (!ok) {
        std::fprintf(stderr, "WriteProcessMemory failed (%lu)\n", GetLastError());
    } else {
        DWORD tid = 0;
        HANDLE th = CreateRemoteThread(proc, nullptr, 0,
                                       reinterpret_cast<LPTHREAD_START_ROUTINE>(mem), nullptr, 0,
                                       &tid);
        if (!th) {
            std::fprintf(stderr, "CreateRemoteThread failed (%lu)\n", GetLastError());
            ok = false;
        } else {
            const DWORD wr = WaitForSingleObject(th, 4000);
            DWORD exit_code = 0;
            GetExitCodeThread(th, &exit_code);
            CloseHandle(th);
            if (wr != WAIT_OBJECT_0) {
                std::fprintf(stderr, "remote thread wait 0x%lX exit 0x%lX\n", wr, exit_code);
                ok = false;
            }
        }
    }
    VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
    return ok;
}

}  // namespace

int main() {
    const DWORD pid = FindSpeedPid();
    if (!pid) {
        std::fprintf(stderr, "speed.exe is not running\n");
        return 1;
    }
    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
                              FALSE, pid);
    if (!proc) {
        std::fprintf(stderr, "OpenProcess(%lu) failed (%lu) — run as admin if the game is elevated\n",
                     pid, GetLastError());
        return 1;
    }

    std::uint32_t grace = 0;
    if (!ReadU32(proc, kGRacePtr, &grace) || grace == 0) {
        std::fprintf(stderr, "GRace pointer at 0x%08X is null\n", kGRacePtr);
        CloseHandle(proc);
        return 1;
    }
    std::uint32_t n = 0;
    std::uint32_t mode = 0;
    ReadU32(proc, grace + kRacerCountOff, &n);
    ReadU32(proc, grace + kRaceModeOff, &mode);
    std::printf("pid %lu  GRace 0x%08X  racers %u  mode %u\n", pid, grace, n, mode);
    if (mode != 1) {
        std::fprintf(stderr, "not in a race (GRace+0x1960 != 1); refusing to call KnockoutRacer\n");
        CloseHandle(proc);
        return 1;
    }
    if (n == 0 || n > 16) {
        std::fprintf(stderr, "implausible racer count %u\n", n);
        CloseHandle(proc);
        return 1;
    }

    int local_slot = -1;
    std::vector<std::uint32_t> targets;
    for (std::uint32_t i = 0; i < n; ++i) {
        const std::uint32_t slot = grace + kRacersOff + i * kRacerStride;
        std::uint32_t key = 0;
        std::uint32_t hint = 0;
        std::uint8_t knocked = 0;
        ReadU32(proc, slot, &key);
        ReadU32(proc, slot + kLocalHintOff, &hint);
        Read(proc, slot + kKnockedOff, &knocked, 1);
        std::uint32_t handle = 0;
        const bool ok = LookupHandle(proc, key, &handle);
        std::printf("  slot %u  key 0x%08X  +4 0x%08X  +0x1c %u  handle 0x%08X%s\n", i, key,
                    hint, knocked, handle, ok ? "" : " (lookup failed)");
        if (hint == 0 && local_slot < 0) {
            local_slot = static_cast<int>(i);
            continue;
        }
        if (ok) {
            targets.push_back(handle);
        }
    }
    if (local_slot < 0) {
        std::fprintf(stderr, "no local player slot (+4==0); aborting so we do not knockout you\n");
        CloseHandle(proc);
        return 1;
    }
    if (targets.empty()) {
        std::fprintf(stderr, "no opponent handles\n");
        CloseHandle(proc);
        return 1;
    }

    std::printf("local slot %d; knockout %zu opponent handle(s) via 0x%08X\n", local_slot,
                targets.size(), kKnockoutRacer);

    if (!CallKnockoutRacer(proc, targets)) {
        CloseHandle(proc);
        return 1;
    }

    std::puts("after:");
    for (std::uint32_t i = 0; i < n; ++i) {
        const std::uint32_t slot = grace + kRacersOff + i * kRacerStride;
        std::uint32_t key = 0;
        std::uint8_t knocked = 0;
        ReadU32(proc, slot, &key);
        Read(proc, slot + kKnockedOff, &knocked, 1);
        std::printf("  slot %u  key 0x%08X  knocked=%u%s\n", i, key, knocked,
                    static_cast<int>(i) == local_slot ? "  (you)" : "");
    }
    CloseHandle(proc);
    return 0;
}
