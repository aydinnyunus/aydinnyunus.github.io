---
layout: post
title: "If Nobody Reads Code, Why Not Write in Assembly? So Here's Redis in Assembly"
date: 2026-02-14
author: Yunus Aydın
lang: en
description: "Writing a Redis-like key-value store in x86-64 Assembly as a response to vibe-coding culture. When nobody reads code, why not write in the most unreadable language?"
keywords: "assembly programming, x86-64, Redis, NASM, vibe-coding, code reading, low-level programming, socket programming, RESP protocol"
canonical_url: "https://aydinnyunus.github.io/2026/02/14/redis-in-assembly/"
---

Everyone's writing code these days. AI assistants generate functions, classes, entire applications. But here's the thing: nobody's reading it. We're in this weird era where developers can produce thousands of lines without understanding what any of it does. I call it vibe-coding: you vibe with the AI, it writes code, you ship it. Reading? That's for later. Or never.

So I thought: if we're not reading code anyway, why not write in the most unreadable language? Assembly. And what better target than Redis, the in-memory data store that powers half the internet? I built a Redis-like server in x86-64 Assembly for macOS. It supports PING, SET, GET, DEL, and QUIT. It implements the RESP protocol. It works.

This is partly a joke, partly a point. Assembly forces you to understand every instruction. You can't vibe your way through it. Every memory operation, every register, every syscall matters. If vibe-coding is the problem, Assembly is the extreme solution.

## The Problem with Vibe-Coding

AI coding assistants changed everything. Now anyone can generate working code. Ask for a function, get a function. Ask for a full application, get a full application. The code works. It passes tests. It ships.

But here's what's missing: understanding. When you generate code without reading it, you're building on a foundation you don't understand. You're trusting the AI's interpretation of your request. You're hoping it got the edge cases right. You're hoping it's secure.

I've seen this in security reviews. Code that works but has vulnerabilities because the developer didn't understand what it was doing. Code that's inefficient because the developer didn't know there was a better way. Code that's unmaintainable because the developer can't explain how it works.

The irony is that we're generating more code than ever, but understanding less of it. We're writing faster, but reading slower. We're building more, but knowing less.

## Why Assembly?

If nobody reads code, why not write in the language that's hardest to read? That was my logic. Assembly is the extreme case. It's unreadable. It's intimidating. It's perfect for making a point about code reading.

I chose Redis because it's well-known. The RESP protocol is documented. The commands are simple. It's a good target for a vibe-coding project. And if I'm going to generate something unreadable, it might as well be useful.

The irony is that Assembly has this reputation for being the language you can't vibe with. You have to understand every instruction. You have to know how memory works. You have to understand the machine. But here we are, vibe-coding it anyway.

## What I Built

I built a Redis-like key-value store in x86-64 Assembly using NASM syntax for macOS. It's a TCP server that listens on port 6379 (Redis's default port) and implements a subset of Redis commands.

**Supported Commands:**
- `PING` - Returns PONG
- `SET key value` - Stores a key-value pair
- `GET key` - Retrieves a value by key
- `DEL key` - Deletes a key-value pair
- `QUIT` - Closes the connection

The server implements the RESP (REdis Serialization Protocol) format. Commands come in as RESP arrays, and responses follow RESP format. It's compatible with `redis-cli`, so you can actually use the real Redis client to connect to it.

**Storage:**
- Maximum 100 key-value pairs
- Keys limited to 100 bytes
- Values limited to 1000 bytes
- Simple linear search for lookups

This isn't production code. It's a proof of concept. It works, but it's not optimized. It's not secure. It's not feature-complete. But it's a working Redis server in Assembly.

You can find the full source code in this [Gist](https://gist.github.com/aydinnyunus/7beef428ca91fb7eb8aa2965086998d8).

## Technical highlights

Writing a network server in Assembly means dealing with sockets, syscalls, and memory management manually. Here's what that looks like:

**Socket Setup:**
```nasm
; Create socket
mov rdi, AF_INET        ; IPv4
mov rsi, SOCK_STREAM    ; TCP
mov rdx, IPPROTO_TCP    ; Protocol
call _socket
mov [rel sock_fd], rax  ; Save file descriptor
```

**Binding to Port:**
```nasm
; Setup port 6379
mov rdi, PORT
call _htons             ; Convert to network byte order
mov [rel sockaddr + 2], ax
```

**Accepting Connections:**
```nasm
server_loop:
    mov rdi, [rel sock_fd]
    call _accept
    mov [rel client_fd], rax
    call handle_client
    jmp server_loop
```

**RESP Parsing:**
The server parses RESP arrays by looking for the `*` prefix, then searching for command strings in the buffer. It's case-insensitive and handles the RESP format correctly.

**Key-Value Storage:**
Storage is implemented as two arrays: one for keys (100 entries × 100 bytes) and one for values (100 entries × 1000 bytes). Lookups use linear search. Deletions shift entries down. It's simple, but it works.

The full implementation is about 500 lines of Assembly. Every line matters. Every register is used intentionally. There's no magic happening behind the scenes.

## The Real Point

This project started as a joke, but it makes a serious point: code reading matters. If we're going to write code, we should understand it. If we're going to use code, we should read it.

Assembly is the extreme example. It's unreadable by design. But it forces understanding. Maybe we need more of that in our high-level code. Maybe we need to write code that's meant to be read, not just generated.

The vibe-coding culture is convenient. It's fast. It's productive. But it's also dangerous. When you don't understand your code, you can't fix bugs. You can't optimize. You can't secure it.

So read your code. Understand what you write. And if you want to see what Assembly Redis looks like, check out the [Gist](https://gist.github.com/aydinnyunus/7beef428ca91fb7eb8aa2965086998d8).

## Related Content

- [Game Hacking 101: Memory Manipulation in Mount and Blade Warband](/2026/01/03/game-hacking-101-memory-manipulation/)

## References

- [Redis Protocol Specification (RESP)](https://redis.io/docs/reference/protocol-spec/)
- [NASM Documentation](https://www.nasm.us/docs.php)
- [x86-64 System V ABI](https://github.com/hjl-tools/x86-psABI/wiki/X86-psABI)
- [Source Code: Redis in Assembly](https://gist.github.com/aydinnyunus/7beef428ca91fb7eb8aa2965086998d8)
