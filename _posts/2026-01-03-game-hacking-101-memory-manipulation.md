---
layout: post
title: "Game Hacking 101: Memory Manipulation in Mount and Blade Warband"
date: 2026-01-03
author: Yunus Aydın
lang: en
description: "Learn game hacking fundamentals through memory manipulation. Understand virtual memory, use Cheat Engine, and write C++ code to modify game values in Mount and Blade Warband."
keywords: "game hacking, memory manipulation, Cheat Engine, C++ game hacking, Mount and Blade Warband, virtual memory, memory addresses, offsets, Windows API, ReadProcessMemory, WriteProcessMemory"
canonical_url: "https://aydinnyunus.github.io/2026/01/03/game-hacking-101-memory-manipulation/"
---

Game hacking opens a window into how games store and manage data in memory. By understanding memory manipulation, you can modify in-game values, experiment with game mechanics, and gain deeper insights into how software works at a low level. In this guide, we'll explore memory manipulation techniques using Mount and Blade Warband as our target, covering everything from basic concepts to writing C++ code that reads and writes game memory.

## What is Mount and Blade Warband?

Mount and Blade Warband is a medieval-themed action role-playing game developed by TaleWorlds Entertainment. Set in the fictional continent of Calradia, the game combines strategic gameplay, combat, and open-world exploration. Players create characters, build armies, engage in battles, and establish kingdoms. Its sandbox-style gameplay and modding support make it an excellent choice for learning game hacking techniques.

![Mount and Blade Warband](https://cdn.taleworlds.com/static/screenshots/1920x1080/mnb-wb-07.jpg)

## Understanding Memory

Memory is where a computer stores data and instructions. In game hacking, we focus on the game process's volatile memory, where variables, values, and game states reside. By manipulating this memory, we can alter game variables and control various aspects of gameplay.

### What is Virtual Memory?

Virtual memory is a memory management technique that gives each process its own virtual address space, independent of physical memory. Programs can operate as if they have access to more memory than physically installed. This virtual address space is divided into pages, which can be mapped to physical memory or stored on secondary storage.

![Virtual Memory vs Physical Memory](https://miro.medium.com/v2/resize:fit:1240/format:webp/1*Lbc8Ng41sKlo7oCGUvuxHg.png)

**How Virtual Memory Works:**

When a program runs, it's loaded into virtual address space, typically divided into segments (code, data, stack). The operating system's Memory Management Unit (MMU) maps virtual addresses to physical addresses in memory or secondary storage.

**Benefits of Virtual Memory:**

1. **Increased Addressable Memory**: Each process can have a larger virtual address space than physical memory
2. **Memory Isolation**: Processes can't access each other's memory, enhancing security
3. **Efficient Memory Utilization**: Unused portions can be stored on disk, freeing physical memory
4. **Memory Sharing**: Multiple processes can share the same file mappings

### Locating Memory Addresses

To manipulate memory, we need to find the specific addresses where game variables are stored. Memory addresses act as unique identifiers for each piece of data. We use memory scanning techniques to search for addresses based on criteria like the variable's current value or data type.

### Modifying Memory

Once we've identified memory addresses, we can modify values stored at those addresses. This might involve increasing resources, adjusting health or stamina, or unlocking features. Memory modification typically involves:

1. Reading the current value from a memory address
2. Performing calculations or changes
3. Writing the modified value back to the address

### Pointers and Dynamic Memory Analysis

Complex game hacking scenarios involve dynamic memory structures and pointers. Pointers are memory addresses that point to other locations. They're used to access dynamically allocated memory or navigate complex data structures. Understanding pointers allows us to traverse the game's memory space more effectively.

## Tools for Memory Manipulation

Several tools aid in memory manipulation:

- **Memory Editors/Trainers**: User-friendly interfaces to scan and modify memory values in real-time
- **Debuggers**: Tools like Cheat Engine or OllyDbg provide advanced features like breakpoints, memory inspection, and disassembly

These tools simplify locating and manipulating memory addresses, enabling precise and efficient changes.

## Cheat Engine: Memory Editing Made Easy

Cheat Engine is a popular memory editing tool used in the game hacking community. It allows you to scan, modify, and manipulate memory values in real-time. Let's walk through using Cheat Engine step by step.

### 1. Download and Install Cheat Engine

Visit the [official Cheat Engine website](https://cheatengine.org/) and download the latest version for your operating system. Run the installer and follow the on-screen instructions.

### 2. Launch Cheat Engine and Select a Game Process

Launch Cheat Engine and click the computer icon in the top-left corner to open the process list. Select the game process (e.g., `mb_warband.exe`). Make sure the game is running before selecting the process.

![Cheat Engine - Select Process](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*nsc498XyXZU8FkqcPUee-Q.png)

### 3. Perform Initial Value Scan

Identify the variable you want to modify (e.g., player's money). In Cheat Engine:

1. Enter the current value in the "Value" field
2. Select the value type from the dropdown (e.g., 4 bytes for integers)
3. Click "First Scan" to initiate the search

The scan will find all memory addresses matching your value.

![Mount and Blade Warband - Inventory](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*8fyaHsAQF-cNmtBO_nLWYg.jpeg)

![Cheat Engine - First Scan](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*DjkqgC9A-Ssw8IhbKdJFBQ.png)

### 4. Refine the Search

The initial scan usually returns many addresses. To narrow it down:

1. Change the value in-game (e.g., spend or earn money)
2. Enter the new value in Cheat Engine
3. Click "Next Scan" to refine the search
4. Repeat until you have a manageable number of addresses

![Mount and Blade Warband - Inventory Shows 710 Dinar](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*dKqwbdnV904vQs8sC-wjxA.jpeg)

![Cheat Engine - Next Scan](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*9lhMD3ju1butGlBEpaObrw.png)

### 5. Modify Memory Values

With a refined list:

1. Double-click a memory address to add it to the address list
2. Double-click the value column to modify it
3. Enter your desired value
4. The game will reflect the change immediately

![Cheat Engine - Modify Value to 1337](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*hU9nct96Nx7xihThGPaIgQ.png)

![Mount and Blade Warband - Inventory Shows 1337](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*rOQB-YHStAEaXejL_Bc0hA.jpeg)

### 6. Save Your Cheat Table

Click the floppy disk icon to save your cheat table. This allows you to reload cheats when you launch the game again.

**Note**: Many games use anti-cheat systems. Use cheat tables responsibly and stay updated on anti-cheat measures.

## Using C++ for Memory Manipulation

While Cheat Engine is great for learning, writing your own C++ code gives you more control and understanding. Let's explore how to manipulate memory programmatically.

### Finding Offsets

Offsets are the key to reliable memory manipulation. Here's how to find them using Cheat Engine:

1. Find the memory address of your target value
2. Right-click the address and select "Find out what accesses this address"
3. Cheat Engine will show assembly instructions that access this address
4. Analyze the assembly to understand the memory structure and offsets

**Example Assembly Analysis:**

```text
mov ecx, [ecx + 140f0]
imul eax, eax, fc8
mov esi, [ecx + eax + 5d0]
```

This assembly code shows:
- `ecx` is loaded from `[ecx + 0x140f0]`
- `eax` is multiplied by `0xfc8`
- The money value is loaded from `[ecx + eax + 0x5d0]`

![Cheat Engine - Assembly Code](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ChE-pPah2VmH5VteWbxgfg.png)

Converting the hexadecimal money value "cc0701" to decimal gives us 13371137.

![Decimal to Hexadecimal Converter](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*vJXIjKljPFcK9VIsKbmVQw.png)

Further analysis reveals that `ecx` is set from `mb_warband.exe + 0x4eb300`, giving us our base address and offsets.

![Cheat Engine - Full Assembly Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ei2b218nHB4_udLXyQF4Nw.png)

### Windows API for Memory Manipulation

The Windows API provides functions to interact with processes and their memory. Key functions include:

- `CreateToolhelp32Snapshot`: Creates a snapshot of running processes
- `Process32Next`: Iterates through process entries
- `OpenProcess`: Opens a handle to a target process
- `ReadProcessMemory`: Reads memory from a process
- `WriteProcessMemory`: Writes memory to a process

### Process Handling

Here's how to find and open a process:

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

The complete `Memory` class implementation is available in the [MemoryBlade repository](https://github.com/aydinnyunus/MemoryBlade/blob/main/memory.h).

This code:
1. Creates a snapshot of all running processes
2. Iterates through each process
3. Compares process names to find the target
4. Opens a handle with full access when found
5. Closes the snapshot handle

### Reading Memory

Here's a template function to read memory:

```cpp
template <typename T>
constexpr const T Read(const std::uintptr_t& address) const noexcept
{
    T value = { };
    ::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);
    return value;
}
```

This function:
- Takes a memory address as input
- Uses `ReadProcessMemory` to read the value
- Returns the value of type `T`
- Works with any data type (int, float, etc.)

### Writing to Memory

Here's a template function to write memory:

```cpp
template <typename T>
constexpr void Write(const std::uintptr_t& address, const T& value) const noexcept
{
    ::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), NULL);
}
```

This function:
- Takes a memory address and value
- Uses `WriteProcessMemory` to write the value
- Works with any data type

### Manipulating Money

Here's a complete example that manipulates the player's money:

```cpp
#include <iostream>
#include "Memory.h"

int main()
{
    // Create Memory instance for mb_warband.exe
    const auto mem = Memory("mb_warband.exe");
    std::cout << "Mount and Blade Warband found!" << std::endl;

    // Get base address of the game module
    const auto base_address = mem.GetModuleAddress("mb_warband.exe");

    // Read values using offsets
    int ecx = mem.Read<int>(base_address + 0x4eb300);
    int eax = mem.Read<int>(ecx + 0x6e0);
    ecx = mem.Read<int>(ecx + 0x140f0);
    int money = mem.Read<int>(ecx + 0x5d0);

    std::cout << "Current money: " << money << std::endl;

    // Write new money value
    mem.Write<int>(ecx + 0x5d0, 13371137);

    std::cout << "Money updated!" << std::endl;

    return 0;
}
```

This code:
1. Finds the game process
2. Gets the base address of the executable
3. Follows the pointer chain using offsets
4. Reads the current money value
5. Writes a new money value

### Manipulating Character Skills

Character skills in Mount and Blade Warband are stored at specific offsets. Here's how to modify them:

```cpp
#include <iostream>
#include "Memory.h"

int main()
{
    const auto mem = Memory("mb_warband.exe");
    std::cout << "Mount and Blade Warband found!" << std::endl;

    const auto base_address = mem.GetModuleAddress("mb_warband.exe");
    
    // Get base pointer
    int edx = mem.Read<int>(base_address + 0x4eb300);
    edx = mem.Read<int>(edx + 0x140f0);
    
    // Modify skills (all set to 500)
    mem.Write<int>(edx + 0x270, 500); // Strength
    mem.Write<int>(edx + 0x274, 500); // Agility
    mem.Write<int>(edx + 0x278, 500); // Intelligence
    mem.Write<int>(edx + 0x27c, 500); // Charisma

    std::cout << "Skills updated!" << std::endl;

    return 0;
}
```

**Skill Offsets:**
- `0x270`: Strength
- `0x274`: Agility
- `0x278`: Intelligence
- `0x27c`: Charisma

![Finding Offsets for Power - Analyzing Registers](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*f9T5kD_Js7SwqZscsaJWrg.png)

![Finding Offsets for Power - Full Assembly](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*_5ot9BmTSogfaVEEGZQQ6g.png)

![Finding Offsets for Agility - Full Assembly](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ZEq3P7mvSQnKe0Ic2XsQeA.png)

![Mount and Blade Warband - Skills Set to 500](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*fP24IdP0F4vk5QK4qv3AMQ.jpeg)

## Interactive Memory Address Visualization

Understanding how memory addresses and offsets work can be challenging. Let's visualize the process:

<div id="memory-visualization" class="demo-container">
  <h4 class="demo-title">Memory Address Visualization</h4>
  <div class="demo-input-section">
    <label for="base-address" class="demo-label">Base Address (hex):</label>
    <input type="text" id="base-address" value="0x00400000" class="demo-input">
    <label for="offset" class="demo-label">Offset (hex):</label>
    <input type="text" id="offset" value="0x4eb300" class="demo-input">
    <button onclick="visualizeMemory()" class="demo-button">Calculate Address</button>
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
      <strong>Calculation:</strong><br>
      Base Address: ${escapeHtml(baseAddressInput)}<br>
      Offset: ${escapeHtml(offsetInput)}<br>
      <strong>Final Address: ${intToHex(finalAddress)}</strong><br>
      <strong>Decimal: ${finalAddress}</strong>
    `;
    
    // Show pointer chain example for money manipulation
    if (offsetInput === '0x4eb300' || offsetInput.toLowerCase() === '4eb300') {
      chainDiv.innerHTML = `
        <strong>Example: Money Manipulation Chain</strong>
        <div class="memory-chain-item">
          <strong>Step 1:</strong> Read base_address + 0x4eb300<br>
          <code>int ecx = mem.Read&lt;int&gt;(base_address + 0x4eb300);</code>
        </div>
        <div class="memory-chain-item">
          <strong>Step 2:</strong> Read ecx + 0x140f0<br>
          <code>ecx = mem.Read&lt;int&gt;(ecx + 0x140f0);</code>
        </div>
        <div class="memory-chain-item">
          <strong>Step 3:</strong> Read/Write ecx + 0x5d0 (money value)<br>
          <code>int money = mem.Read&lt;int&gt;(ecx + 0x5d0);</code><br>
          <code>mem.Write&lt;int&gt;(ecx + 0x5d0, 13371137);</code>
        </div>
      `;
    } else {
      chainDiv.innerHTML = '';
    }
  } catch (e) {
    resultDiv.innerHTML = `<span style="color: #dc3545;">Error: Invalid hex address</span>`;
    chainDiv.innerHTML = '';
  }
}
</script>

## Ethical Considerations

Game hacking is a powerful skill, but it comes with responsibilities:

- **Single-player games**: Generally acceptable for learning and experimentation
- **Multiplayer games**: Can ruin others' experiences and violate terms of service
- **Respect developers**: Understand that anti-cheat systems exist for good reasons
- **Use responsibly**: Don't use hacks to gain unfair advantages in competitive environments

## Anti-Cheat Systems

Modern games implement various anti-cheat measures:

1. **Memory Scanning**: Periodic checks for unauthorized modifications
2. **Code Integrity Checks**: Verification that game code hasn't been modified
3. **Behavior Analysis**: Detection of impossible or suspicious actions
4. **Server-Side Validation**: Critical values verified on game servers

Understanding these systems helps you appreciate why some hacks work in single-player but not multiplayer games.

## Conclusion

Memory manipulation is a fundamental game hacking technique that provides deep insights into how software works. By understanding virtual memory, using tools like Cheat Engine, and writing C++ code, you can modify game values and experiment with game mechanics. Remember to use these skills responsibly and ethically, especially in multiplayer environments.

The key concepts we covered:
- Virtual memory and how processes isolate memory
- Using Cheat Engine to find and modify memory addresses
- Writing C++ code using Windows API functions
- Understanding offsets and pointer chains
- Practical examples for manipulating money and skills

Whether you're interested in game development, reverse engineering, or cybersecurity, understanding memory manipulation is a valuable skill that opens many doors.

## Related Content

- [CVE-2025-66019: LZW Decompression DoS Vulnerability in pypdf](/2025/12/20/cve-2025-66019-pypdf-lzw-dos/)
- [SQL Injection in GeoPandas](/2025/12/27/sql-injection-geopandas/)

