---
layout: post
title: "PassDetective: Detecting Passwords and Secrets in Your Shell History"
date: 2025-12-14
author: Yunus Aydın
lang: en
description: "Detect accidentally written passwords, API keys, and secrets from shell command history using PassDetective. PassDetective usage on Kali Linux and NixOS."
keywords: "passdetective, shell history, password detection, API key detection, Kali Linux, NixOS, security tool, command history analysis, secret detection, regex scanning"
canonical_url: "https://aydinnyunus.github.io/2025/12/14/passdetective-kali-linux/"
---

Your shell command history might contain accidentally written passwords, API keys, or secrets. This information is stored in your history files and poses a security risk. PassDetective is a command-line tool that scans your shell history to detect such sensitive information. Available on both Kali Linux and NixOS, this tool uses regular expressions to help identify potential security vulnerabilities in your command history.

## What is PassDetective?

PassDetective is a security tool written in Go. Its main purpose is to scan your shell command history files (ZSH and Bash) to detect accidentally written passwords, API keys, and other sensitive information. The tool can recognize over 40 different types of secrets using powerful regex patterns.

The tool has gathered **141 stars** on GitHub and is widely used by the security community. It's also included in Kali Linux's official tools and is available in the NixOS package repository.

## Why PassDetective?

When working in the shell during daily use, we sometimes have to write passwords or API keys on the command line. For example:

```bash
curl -u username:password123 https://api.example.com
```

These types of commands are saved to your shell history and stored in `.zsh_history` or `.bash_history` files. If these files become accessible in some way (for example, in a backup or on a shared system), your sensitive information could be exposed.

PassDetective helps minimize this risk by regularly scanning your history files and detecting potential threats. This way, you can find sensitive information and take necessary precautions.

## Installation

### Kali Linux

PassDetective is available in Kali Linux's official package repository. To install:

```bash
sudo apt install passdetective
```

![PassDetective Kali Linux Installation](https://github.com/aydinnyunus/PassDetective/assets/52822869/dc12e543-3454-423b-b3fe-0511f93e2c3e)

### NixOS

To install PassDetective on NixOS:

```bash
nix-env -iA nixpkgs.passdetective
```

Or in your `configuration.nix` file:

```nix
environment.systemPackages = with pkgs; [
  passdetective
];
```

### Installation via Go

If you want to install from source:

```bash
go install github.com/aydinnyunus/PassDetective@latest
```

## Usage

PassDetective's basic usage is quite simple. The tool can scan your shell history files using the `extract` command.

### Help Menu

First, to see all options of the tool:

```bash
PassDetective -h
```

![PassDetective Help](https://i.imgur.com/UeMRWTd.png)

### Shell History Analysis

To scan your ZSH history:

```bash
PassDetective extract --zsh
```

To scan your Bash history:

```bash
PassDetective extract --bash
```

To scan both shell histories:

```bash
PassDetective extract --all
```

![Extract All](https://i.imgur.com/zKjs3p8.png)

### Secret Detection

PassDetective can detect not only passwords but also API keys and other secrets. For secret scanning:

```bash
PassDetective extract --secrets --zsh
```

or

```bash
PassDetective extract --secrets --bash
```

![Secrets Detection](https://i.imgur.com/yw3v0RI.png)

## Detected Secret Types

PassDetective can detect many different types of secrets, such as:

- **Cloudinary URLs**: URLs starting with `cloudinary://`
- **Firebase URLs**: URLs containing `firebaseio.com`
- **Slack Tokens**: Tokens in `xox[p|b|o|a]-` format
- **RSA Private Keys**: Keys starting with `-----BEGIN RSA PRIVATE KEY-----`
- **SSH Private Keys**: DSA, EC, and PGP private keys
- **AWS Access Key IDs**: Keys in `AKIA[0-9A-Z]{16}` format
- **Google API Keys**: Keys in `AIza[0-9A-Za-z\\-_]{35}` format
- **GitHub Tokens**: GitHub API tokens
- **Stripe API Keys**: Keys starting with `sk_live_`
- **Twilio API Keys**: Keys in `SK[0-9a-fA-F]{32}` format
- **Passwords in URLs**: URLs in `https://username:password@example.com` format

And more. PassDetective uses regex patterns from the [secret-regex-list](https://github.com/h33tlit/secret-regex-list) project.

## Practical Use Cases

### Use Case 1: Regular Security Check

From a security perspective, regularly scanning your shell history files is a good practice. For example, for a monthly check:

```bash
PassDetective extract --all --secrets
```

This command scans both your ZSH and Bash history and detects all secrets.

### Use Case 2: Before Starting a New Project

Before starting a new project, you can check if there's any sensitive information in your current shell history:

```bash
PassDetective extract --zsh --secrets
```

### Use Case 3: System Cleanup

Before leaving a system or creating a backup, if you want to clean your history files, you can use PassDetective first to see what kind of sensitive information exists.

## Security Recommendations

A few points to consider when using PassDetective:

1. **Cleaning History Files**: PassDetective only detects, it doesn't clean. You need to manually clean the detected sensitive information.

2. **Regular Scanning**: Regularly scan your shell history. Especially when working on production systems.

3. **Backup Check**: Check your history files before creating backups.

4. **Alias Usage**: PassDetective also checks aliases in your shell config files. This way, it can also detect sensitive information stored in aliases.

## Conclusion

PassDetective is a useful tool for detecting sensitive information in your shell command history. It can be easily installed and used on both Kali Linux and NixOS. When used regularly, it helps you find passwords and secrets that were accidentally written to history.

The tool is open source and actively developed on GitHub. You can visit the [GitHub repository](https://github.com/aydinnyunus/PassDetective) for more information and source code.

**Related Content:**
- [exifLooter: Extracting Hidden Location Information from Photos](/2025/12/07/exiflooter-kali-linux/)
- [SQL Injection Vulnerability: Security Issue in GeoPandas to_postgis() Function](/sql-injection-geopandas/)



