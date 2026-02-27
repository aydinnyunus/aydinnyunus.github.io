---
layout: post
title: "Game Hacking 101 Part 2: Memory Analysis with Minesweeper Reverse Engineering"
date: 2026-02-28
author: Yunus Aydın
lang: en
description: "Learn game hacking techniques through Minesweeper reverse engineering. Use Cheat Engine pointer scanning, x64dbg hardware breakpoints, and memory analysis to identify in-game values."
keywords: "game hacking, reverse engineering, Minesweeper, Cheat Engine, x64dbg, pointer scanning, hardware breakpoints, memory analysis, game hacking 101, reverse engineering tutorial"
canonical_url: "https://aydinnyunus.github.io/2026/02/28/game-hacking-101-part2-minesweeper-reverse-engineering/"
---

In the first part of the Game Hacking 101 series, we learned memory manipulation techniques in Mount and Blade Warband. In this second part, we'll explore more advanced techniques by reverse engineering Minesweeper. We'll use Cheat Engine for pointer scanning, x64dbg for hardware breakpoints, and memory analysis to decode the game's inner mechanics. This guide is an excellent starting point for those who want to learn the fundamentals of reverse engineering.

## What is Minesweeper?

Minesweeper is a classic Windows game. Players try to reveal hidden mines on a grid while avoiding explosive accidents. While it may seem like a simple game, Minesweeper's coding techniques and design make it an excellent learning tool for reverse engineering.

![Minesweeper](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*Z9i0L_1LXSJGAAyzPrE7dw.jpeg)

Minesweeper's apparent simplicity hides the complexity it actually contains. Reverse engineering helps us understand the game's architecture and data structures. This journey provides the foundational knowledge needed to decode the game's inner mechanics.

## Deciphering the Grid

At the core of Minesweeper lies a grid structure that players systematically uncover. By analyzing the memory representation of this grid, we can understand how the game stores information about mines, numbers, and open/closed cells.

**Grid structure:**

- Each cell is stored at a memory address
- Mine information is stored in binary format
- Cell state (open/closed) is represented by a flag
- Adjacent mine count is calculated and stored

## Cracking the Algorithms

Minesweeper uses algorithms to calculate numbers in cells adjacent to mines and to orchestrate the cascading effect of opening empty cells. Reverse engineering these algorithms gives us insights into the game's real-time logic processing.

**Core algorithms:**

- **Mine count calculation**: Calculates the number of adjacent mines for each cell
- **Cascading opening**: Automatic opening of empty cells
- **Game state check**: Determines win/lose conditions

## Revealing the Veil of Randomness

The seemingly random mine placement in Minesweeper contains subtle patterns and predictability. By examining the random number generation mechanism, we can uncover the enigmatic method behind mine placement.

![Random Number Generation](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*S6RdzkvwPEu5jXlZg2XTLw.png)

Random numbers generated using `srand` are not truly random but rather pseudorandom. This means that given the same seed value, the same sequence of numbers will be produced every time. Since Minesweeper's random number generation relies on `srand`, the same seed (often derived from system time) guarantees consistent results in repeated gameplay sessions.

**Pseudorandom characteristics:**

- Same seed produces the same sequence
- System time is often used as seed
- This makes mine placement predictable

## Finding the Bomb Count with Cheat Engine

The bomb count display in Minesweeper is a critical feature. In the game, players can reduce this count by placing flags. Reverse engineering reveals a way to manipulate this value, potentially reshaping the game's dynamics.

### Cheat Engine: A Toolkit for Exploration

Cheat Engine is a powerful tool for game hacking and reverse engineering. This tool allows us to delve into the memory of a running game, pinpoint specific values like the bomb count in Minesweeper, and peer into its inner workings.

![Cheat Engine](https://upload.wikimedia.org/wikipedia/commons/c/c2/Cheat_Engine_7.1.png)

### Navigating the Mutable Value Predicament

In Minesweeper, the memory address housing the bomb count shifts with each new game initiation. This dynamic behavior poses a challenge when pinpointing the precise memory address responsible for storing this vital information.

**The problem:**

- Memory layout changes with each game start
- Base addresses are dynamically assigned
- Static memory addresses cannot be used

### Hunting for the Bomb Count

Leveraging Cheat Engine, we embark by scanning for the initial bomb count value. However, owing to the ever-changing nature of this value, the scan yields numerous memory addresses bearing the same value. Cheat Engine captures snapshots of memory at varying intervals, leading to multiple matches.

**Initial scan steps:**

1. Start Cheat Engine
2. Select Minesweeper process
3. Enter initial bomb count value (e.g., 10)
4. Perform "First Scan"
5. Many addresses are found

### Refinement via Iteration

To sieve out false positives, strategic in-game actions that influence the bomb count are executed. Placing a flag, for instance, decreases the count by one. Following such actions, a "Next Scan" in Cheat Engine narrows down the list of potential addresses, guiding us closer to the target.

**Narrowing process:**

1. Place a flag in the game
2. Enter new bomb count value
3. Perform "Next Scan"
4. Address list narrows
5. Repeat until a single address remains

### The Quandary of Pointer Scans

As we embark on the odyssey to uncover Minesweeper's concealed facets, Cheat Engine's potent pointer scan feature becomes invaluable. This tool enables the tracing of pointer chains, revealing memory addresses holding vital values such as bomb counts or click records. However, this powerful tool yields a plethora of potential candidates, complicating the search.

![Pointer Scan](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*dVTh_IXpY_IR8qaaUQ-QWA.png)

**Pointer scan challenges:**

- Initial pointer scan produces approximately 60 results
- Each result points to different addresses in the game's memory
- Not all addresses contain the information we seek
- Many addresses may be transient or unrelated to our target information

### Sifting the Transient from the Enduring

The initial pointer scan inundates us with around 60 outcomes, each leading to diverse addresses within the game's memory. Amidst this sea of data, not all addresses hold the answers we seek. Many might be transient or unrelated to our target information. Our pursuit requires the identification of consistent, reliable pointers amidst the multitude.

![Pointer Scan with Previous Scans](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*jUitILhctoTzKSPfxmJu5g.png)

### A Fresh Start: Restarting the Game

Acknowledging the fluid memory layout of Minesweeper, we opt for a fresh commencement. We close the game and restart, initializing a new session with distinct memory configurations. By re-performing a pointer scan within this renewed context, we aim to isolate memory addresses consistently harboring the bomb count and click tallies we seek.

**Restart strategy:**

1. Close the game
2. Start a new game
3. Repeat pointer scan
4. Find consistent offsets

### Emergence of Stable Offsets

Upon restarting and conducting a new pointer scan, a promising pattern emerges. Two offsets consistently surface within the results, retaining their presence across various game sessions. These stable offsets stand out amidst the ever-shifting landscape of memory addresses. This revelation marks a crucial stride in our quest to fathom Minesweeper's cryptic internals.

![Candidates for Pointer Scan](https://miro.medium.com/v2/resize:fit:1200/format:webp/1*5OY7mpXiyRt6vmREACdxPQ.png)

**Stable offsets:**

- Two offsets are consistently found
- Remain the same across different game sessions
- Work even when memory layout changes

## Debugging with x64dbg

Debugging unfolds as a process of dissecting program execution, tracking memory changes, and identifying pivotal functions influencing gameplay behavior. Our initial focus lands on a pivotal memory address — the cell players click during gameplay. Peering into the activities surrounding this memory location promises insights into the functions interwoven with Minesweeper's mechanics.

### Hardware Breakpoints: Unearthing Code Footprints

To gain clarity on events triggered by a cell click, we wield a formidable debugging technique — hardware breakpoints. By setting a breakpoint at the clicked cell's memory address, the debugger intervenes whenever the memory location undergoes access or modification. This tactic bestows a microscopic view of the code's sequence during and post-click.

**Hardware breakpoint advantages:**

- Captures memory accesses
- Captures memory modifications
- Stops code execution
- Allows examination of registers and stack

![x64dbg](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*ZUyjIu6vw1jO5DhdoqCvsw.png)

Upon clicking any grid square, the breakpoint triggers. Notably, `[rcx+1C]` designates the click count, with 1C as the offset.

![x64dbg breakpoint](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*m3HT77bmWk1b29FceNmBKQ.png)

### Illuminating Code in Action

Initiating the hardware breakpoint transports us to a realm of precision debugging. Every interaction with the clicked cell's memory address prompts a pause, enabling scrutiny of registers, stacks, and code in real time. Through meticulous analysis, we unravel the code's behavior, unveiling additional functions and interactions that shape Minesweeper's dynamic gameplay.

**Analysis process:**

1. Stop when breakpoint triggers
2. Examine registers
3. Analyze stack
4. Follow assembly code
5. Identify function calls

### Unearthing Hidden Jewels

By dissecting the code's response to interactions with the clicked cell's memory address, we uncover latent functions influencing Minesweeper's gameplay logic. These revelations span diverse facets — unveiling mechanics, cascading algorithms, and more. With each breakpoint, we inch closer to comprehending the intricate tapestry of Minesweeper's codebase.

![isNotBomb](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*_Q1V_LDZSbA4TmglfdQa4g.png)

Investigating the conditional jump (`jne`) action leads to address calculations, offsets, and comparisons like `cmp byte ptr [rsi+rcx], dil`. It becomes evident that `dil` remains constant at 0, while `[rsi+rcx]` pertains to `isBomb`. If not a bomb, the jump directs to address 2EE, advancing the offset (click count) through comparison `cmp dword ptr [rbx+38], 0`, influencing the game's progression.

![isNotBomb Registers](https://miro.medium.com/v2/resize:fit:796/format:webp/1*sOh2XycSWt5gdfU0X3nuVQ.png)

Dissecting the registers during the `isNotBomb` function exposes the roles of `rsi` (rowPtr) and `rbp` (column), both persistently anchored at 0. This realization facilitates offset calculations and loop construction to ascertain bomb presence. Our formula becomes **Minesweeper.exe + 0xAAA38] + 0x18] + 0x58] + 0x10] + 0x8* column] + 0x10] + row**.

**Memory formula:**

```
Minesweeper.exe + 0xAAA38] + 0x18] + 0x58] + 0x10] + 0x8* column] + 0x10] + row
```

This formula can be used to calculate the memory address of any cell.

### Memory Analysis

Memory analysis uncovers valuable data. Notably, 38f207c stores 09, reflecting row and column counts. The highlighted value, 1C offset, signifies click count. Others indicate elapsed time. This memory region encapsulates Minesweeper's critical variables.

![Memory Section](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*iilhoc_YQXLcURuq2rGk3A.png)

**Memory region contents:**

- Row and column counts
- Click count (1C offset)
- Elapsed time
- Game state

## C++ Code Implementation

After all these analyses, we can write C++ code that manipulates Minesweeper's memory structure. This code uses pointer chains to access cell information and control the game state.

> **GitHub**: [https://github.com/aydinnyunus/MineQuest](https://github.com/aydinnyunus/MineQuest)

![Result](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*nbuApRiyt87rFgkaXOX4Gg.png)

**C++ code features:**

- Memory access using pointer chains
- Reading and writing cell state
- Detecting mine information
- Controlling game state

## Conclusion

Reverse engineering Minesweeper unveils a vibrant blend of coding techniques, algorithms, and data structures. This expedition extends an appreciation for software development, unearthing memory management intricacies, UI design, and more, encapsulated within a seemingly modest game. The journey illuminates the craftsmanship underlying seemingly straightforward games, underscoring that even in simplicity, complexity thrives.

**Key takeaways:**

- Pointer scanning is critical for finding dynamic memory addresses
- Hardware breakpoints are a powerful tool for analyzing code execution
- Memory formulas can be used to access cell information
- Reverse engineering is a valuable method for understanding game mechanics

Game hacking and reverse engineering are powerful techniques for understanding how software works at a low level. Even seemingly simple games like Minesweeper offer a rich source for deep analysis.

## Related Content

- [Bypassing Door Passwords](/2022/01/07/bypassing-door-passwords/)
- [Hacking Instagram Scammers](/2022/04/11/hacking-instagram-scammers/)
