// NFS Most Wanted (this speed.exe) — set career CASH_DATA (save+0xb4).
//
// HUD: FUN_00532220 pushes FUN_00590500 → BOUNTY_DATA, then [save+0xb4] → CASH_DATA.
// New Career zeros +0xb4; this image never SUBs it. Type 0x13 sell ADDs here.
// Garage inner wallet (garage+0x8CA0) is a different bucket (bounty = slot prices + inner).
//
//   g++ -O2 -o set_cash.exe set_cash.cpp
//   set_cash.exe              → 123456 cash (test value)
//   set_cash.exe 50000        → that cash
//   set_cash.exe 50000 0      → cash and inner wallet

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

constexpr std::uint32_t kCareerMgr = 0x0091CF90;  // DAT_0091cf90
constexpr std::uint32_t kSaveOff   = 0x10;
constexpr std::uint32_t kCashOff   = 0xB4;
constexpr std::uint32_t kGarageOff = 0x414;
constexpr std::uint32_t kInnerOff  = 0x8CA0;  // relative to garage

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

bool WriteU32(HANDLE proc, std::uint32_t addr, std::uint32_t v) {
    return Write(proc, addr, &v, sizeof(v));
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

}  // namespace

int main(int argc, char** argv) {
    const std::uint32_t want_cash = (argc >= 2) ? static_cast<std::uint32_t>(std::strtoul(argv[1], nullptr, 0))
                                                : 123456u;
    const bool set_inner = argc >= 3;
    const std::uint32_t want_inner = set_inner ? static_cast<std::uint32_t>(std::strtoul(argv[2], nullptr, 0)) : 0;

    const DWORD pid = FindSpeedPid();
    if (!pid) {
        std::fprintf(stderr, "speed.exe is not running\n");
        return 1;
    }
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ |
                                  PROCESS_VM_WRITE,
                              FALSE, pid);
    if (!proc) {
        std::fprintf(stderr, "OpenProcess(%lu) failed (%lu)\n", pid, GetLastError());
        return 1;
    }

    std::uint32_t mgr = 0;
    std::uint32_t save = 0;
    if (!ReadU32(proc, kCareerMgr, &mgr) || mgr == 0 || !ReadU32(proc, mgr + kSaveOff, &save) ||
        save == 0) {
        std::fprintf(stderr, "career mgr/save pointer is null (0x%08X -> 0x%08X)\n", mgr, save);
        CloseHandle(proc);
        return 1;
    }

    char name[32]{};
    Read(proc, save, name, sizeof(name) - 1);
    const std::uint32_t cash_addr = save + kCashOff;
    const std::uint32_t inner_addr = save + kGarageOff + kInnerOff;
    std::uint32_t old_cash = 0;
    std::uint32_t old_inner = 0;
    ReadU32(proc, cash_addr, &old_cash);
    ReadU32(proc, inner_addr, &old_inner);

    std::printf("pid %lu  save 0x%08X  profile \"%s\"\n", pid, save, name);
    std::printf("CASH_DATA  [0x%08X]  %u  ->  %u\n", cash_addr, old_cash, want_cash);
    if (!WriteU32(proc, cash_addr, want_cash)) {
        std::fprintf(stderr, "WriteProcessMemory cash failed (%lu)\n", GetLastError());
        CloseHandle(proc);
        return 1;
    }
    if (set_inner) {
        std::printf("inner 8CA0 [0x%08X]  %u  ->  %u\n", inner_addr, old_inner, want_inner);
        if (!WriteU32(proc, inner_addr, want_inner)) {
            std::fprintf(stderr, "WriteProcessMemory inner failed (%lu)\n", GetLastError());
            CloseHandle(proc);
            return 1;
        }
    } else {
        std::printf("inner 8CA0 [0x%08X]  %u  (unchanged; bounty still uses this)\n", inner_addr,
                    old_inner);
    }

    std::uint32_t new_cash = 0;
    std::uint32_t new_inner = 0;
    ReadU32(proc, cash_addr, &new_cash);
    ReadU32(proc, inner_addr, &new_inner);
    std::printf("readback cash %u  inner %u\n", new_cash, new_inner);
    CloseHandle(proc);
    std::puts("open Safehouse / pause career HUD to refresh CASH_DATA widgets");
    return (new_cash == want_cash) ? 0 : 1;
}
