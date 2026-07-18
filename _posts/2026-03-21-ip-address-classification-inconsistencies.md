---
layout: post
title: "IP Classification Inconsistencies Across Programming Languages"
date: 2026-03-21
author: Yunus Aydın
lang: en
description: "How loopback IPs are classified across Go, Java, Node.js, PHP, Python, Ruby — and why these differences create SSRF and cloud metadata risks."
keywords: "IP address classification, SSRF prevention, loopback IP inconsistencies, private IP handling, 169.254.169.254, cloud metadata SSRF, Go Java Python IP validation, cross-language security"
canonical_url: "https://aydinnyunus.github.io/2026/03/21/ip-address-classification-inconsistencies/"
---

As a security researcher, I've been analyzing **IP address classification** behaviors across various programming languages. Recently, I noticed some intriguing inconsistencies, particularly concerning how loopback and private IP addresses are treated. In this post, I'll share my observations and insights on this matter.

## What is IP Address Classification?

IP address classification is the process of determining whether an IP address is loopback, private, link-local, or public. This classification plays a critical role in security controls and network configurations. However, different programming languages perform this classification differently.

## The Setup

I examined the outputs for several IP addresses, including local and private ranges, across languages such as Go, Java, Node.js, PHP, Python, and Ruby. Here are the key findings from my analysis:

## Loopback IP Address (127.0.0.1)

**127.0.0.1** is the most common loopback address representing localhost. Let's see how it's classified in different languages:

- **Go**: Reports `Is Private: false`, `IsLoopback: true`
- **Java**: Marks it as `Is Private: false`
- **Node.js**: Reports `IsPrivate: true`, `IsLoopback: true`
- **PHP**: Reports `Is Private: true`
- **Python**: Reports `Is Private: True`, `IsLoopback: True`
- **Ruby**: Marks it as `Is Private: false`

**Inconsistency**: The classification of **127.0.0.1** as a private address varies significantly among languages. Some languages (**Go, Node.js, PHP**) identify it as **private**, while others (**Java, Ruby**) **do not**. Generally, **127.0.0.1** should be considered a loopback address and is expected to be classified as **private**.

## IPv6 Loopback Address (::1)

In IPv6, the loopback address is **::1**. Let's see how it's handled in different languages:

- **Go**: Reports `Is Private: false`, `IsLoopback: true`
- **Java**: Marks it as `Is Private: false`
- **Node.js**: Reports `Is Private: true`
- **PHP**: Reports `Is Private: true`
- **Python**: Reports `Is Private: True`, `Is Loopback: True`
- **Ruby**: Marks it as `Is Private: false`

**Inconsistency**: Similar to 127.0.0.1, the IPv6 loopback address **::1** is classified differently across languages. While some (**Node.js, PHP**) mark it as **private**, others (**Java, Ruby**) **do not**. The expectation is that **::1** should be treated as a loopback address and also marked as **private**.

## Private IP Addresses

Private IP addresses are special network ranges defined by RFC 1918 and RFC 4193. Let's see how they're handled in different languages:

- **Go**: Correctly identifies **192.168.1.1** as **private**
- **Java**: Accurately identifies **192.168.1.1**, **10.0.0.1**, and **172.16.0.1** as **private**
- **Node.js**: Shows inconsistencies with **127.0.0.1** and **169.254.169.254**
- **PHP**: Correctly identifies several addresses as **private**, including **169.254.169.254**
- **Python**: Accurately handles **private** IPs but marks **169.254.169.254** as private, which can be confusing
- **Ruby**: Incorrectly identifies **169.254.169.254** as not **private**

## The Case of 169.254.169.254

The IP address **169.254.169.254** belongs to the link-local range (**169.254.0.0/16**), specifically used for Automatic Private IP Addressing (APIPA) and often appears in cloud environments, such as AWS and Google Cloud. This address provides crucial metadata, allowing services running on a virtual machine to access instance information, including security credentials, instance IDs, and other environment data.

![SSRF Vulnerability](https://www.imperva.com/learn/wp-content/uploads/sites/13/2021/12/How-Server-SSRF-works.png)

### Inconsistencies in Output Across Languages

Let's look at how this IP is treated across different programming languages:

- **Go**: Marks **169.254.169.254** as `IsLinkLocalUnicast: true` but not **private** `IsPrivate: false`
- **Java**: Classifies the IP as `Is Private: false`, which aligns with the fact that link-local addresses **aren't strictly private** but have restricted use within the local subnet
- **Node.js**: Marks it as both `IsPrivate: true` and `IsLoopback: false`. This is an inconsistency, as link-local addresses shouldn't be considered private
- **PHP**: Marks it as **private**, which contradicts the general behavior expected from link-local addresses
- **Python**: Correctly identifies it as `IsLinkLocal: True` and also marks it as **private**
- **Ruby**: **Incorrectly** identifies it as **not** **private**

## Why This Matters

Link-local addresses like 169.254.169.254 play an essential role in cloud environments, specifically for instance metadata retrieval. Misclassifying this IP address can lead to significant security issues, particularly in environments prone to **Server-Side Request Forgery (SSRF)** vulnerabilities.

**SSRF risk:**

For example, if an application incorrectly marks 169.254.169.254 as private and allows unrestricted access to it, an attacker exploiting an SSRF vulnerability could extract sensitive instance metadata, including temporary credentials for cloud services. This could allow them to escalate privileges, access cloud resources, and launch additional attacks.

**AWS example:**

In AWS, for instance, accessing `http://169.254.169.254/latest/meta-data/` provides vital metadata about the EC2 instance, including IAM roles. In Google Cloud, similar metadata can be retrieved from this IP. A misclassification that allows requests to this address could expose sensitive information to unauthorized users.

**Google Cloud example:**

In Google Cloud, the same IP address provides access to instance metadata. Misclassification could allow attackers to access instance information and credentials.

## Conclusion

The inconsistencies observed across programming languages highlight a lack of standardized definitions and behaviors for IP address classifications. This discrepancy is particularly critical in security contexts, as differing treatments can lead to unexpected behaviors — especially in scenarios involving server-side requests.

**Key takeaways:**

- Different languages classify the same IP addresses differently
- Loopback addresses (127.0.0.1, ::1) are handled inconsistently
- Link-local addresses (169.254.169.254) can be misclassified
- These inconsistencies can lead to SSRF vulnerabilities
- Particularly risky in cloud environments

## Recommendations

### Standardization

Establish clear guidelines on how to classify IP addresses across different programming environments to minimize these inconsistencies.

### Testing and Validation

Implement comprehensive testing for IP classification functions to ensure consistent and secure behavior across various environments.

### Awareness

Developers must be cognizant of these differences and design applications accordingly, especially when handling requests that may involve local or private network addresses.

## Semgrep Rule for Go

To help ensure proper handling of IP address classifications in Go, consider using the following Semgrep rule:

```yaml
rules:
  - id: go-check-isprivate
    languages: [go]
    patterns:
      - pattern: $IP.IsPrivate()
      - pattern-not: $IP.IsLinkLocalUnicast()
    message: "Ensure IP address handling methods include MustParseAddr or ParseIP, and validate the IP using IsPrivate, IsLoopback, or IsLinkLocalUnicast after parsing."
    severity: WARNING
```

This rule checks for the usage of `IsPrivate()` while ensuring that `IsLinkLocalUnicast()` is not present, prompting developers to validate their IP address handling methods.

## GitHub Repository

You can find the complete code and analysis in my **GitHub** repository: [https://github.com/aydinnyunus/isItPrivate](https://github.com/aydinnyunus/isItPrivate). This repository serves as a resource for understanding IP address classification across various programming languages and highlights the potential security implications of these classifications, particularly regarding SSRF vulnerabilities in cloud environments.

## Reporting the Issue to Google

During my research, I encountered the inconsistency of how 169.254.169.254 is classified across different programming languages. To clarify this, I reported the issue to Google. On March 3rd, they replied:

> _"IP.IsPrivate checks if an IP is part of the IANA defined private blocks, per RFC 1918 and RFC 4193. 169.254.169.254 is not part of either of those ranges. 169.254/16 is a link-local prefix, as correctly reported by IP.IsLinkLocalMulticast and IP.IsLinkLocalUnicast. This appears to be working as intended."_

The inconsistency stems from different interpretations of link-local and private IPs across languages and libraries.

Understanding these nuances is essential for enhancing security and ensuring robust application behavior across different programming languages and environments. By addressing these inconsistencies, we can help mitigate potential vulnerabilities that could compromise cloud-based infrastructures.

## Related Content

- [SQL Injection Vulnerability: Security Issue in GeoPandas to_postgis() Function](/2025/12/27/sql-injection-geopandas/)
- [SSRF Vulnerability: Bypassing Protection with DNS Rebinding Attack](/2026/03/14/ssrf-dns-rebinding-vulnerability/)
