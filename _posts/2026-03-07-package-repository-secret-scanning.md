---
layout: post
title: "I Scanned PyPI, npm, and RubyGems for Leaked Secrets. Microsoft, Automattic, Palo Alto and Many More"
date: 2026-03-07
author: Yunus Aydın
lang: en
description: "Uncover hidden dangers by performing secret scanning in package repositories. Detect AWS tokens, private keys, and other secrets in PyPI, npm, RubyGems, and NuGet packages."
keywords: "secret scanning, package repository, PyPI, npm, RubyGems, NuGet, AWS tokens, private keys, security scanning, GitLeaks, PackageSpy, supply chain security"
canonical_url: "https://aydinnyunus.github.io/2026/03/07/package-repository-secret-scanning/"
---

In the ever-shifting realm of cybersecurity, staying one step ahead of potential threats is a non-negotiable mission. Package repositories like **PyPI**, **npm**, **NuGet**, and **RubyGems** are goldmines of software packages, cherished by developers worldwide. While these packages are indispensable for crafting powerful applications, they may also harbor concealed secrets, making developers and organizations susceptible to data breaches and malicious exploits. In this blog post, we embark on a journey to unearth the significance of secret scanning within the latest packages from various repositories, revealing some startling revelations.

## The Crucial Role of Package Repository Secret Scanning

Package repositories stand as the go-to source for software enthusiasts, housing a myriad of open-source libraries. They are often the starting point for developers when questing for packages to infuse into their projects. However, these packages are not immune to vulnerabilities, and secret leaks represent a perilous abyss.

### Demystifying Secrets

Secrets come in various guises, encompassing **API keys**, **authentication tokens, passwords, and encryption keys**. These are classified as sensitive nuggets of information that should never see the light of day, for their compromise can herald catastrophic consequences.

**Types of secrets:**

- AWS access tokens
- Private keys
- Database passwords
- API keys
- JWT tokens
- Webhook URLs

### The Far-reaching Ramifications of Secret Leaks

Inadvertent inclusion of secrets in packages exposes a chink in the armor, inviting nefarious actors to wreak havoc. For instance, a mishandled **AWS (Amazon Web Services)** access key might pave the way for unauthorized entry, unleashing a torrent of data breaches, financial setbacks, and operational chaos.

**Potential impacts:**

- Data breaches
- Financial losses
- Service disruptions
- Phishing attacks
- Financial fraud

## Unveiling Secrets: A Three-Pronged Approach

To underscore the gravity of secret scanning, we've adopted an innovative approach using dedicated **EC2** machines for each major package manager, namely **PyPI, npm, RubyGems, and NuGet**. Let's delve into the specifics of our approach for each:

### PyPI: Python Package Index

Our PyPI-specific EC2 machine tirelessly parses the latest PyPI package downloads, extracts their contents, and performs a thorough GitLeaks scan to identify any secrets hidden within Python packages. PyPI packages are a cornerstone of the Python ecosystem, and securing them is paramount.

![PyPI Secret Scanning](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*cMVRDy-gmpC5Whaqs1aB-A.png)

**PyPI scanning process:**

1. Parse latest package downloads
2. Extract package contents
3. Run GitLeaks secret scan
4. Report findings

### npm: Node Package Manager

Dedicated to the Node.js ecosystem, our npm EC2 machine is on a mission to parse the latest npm package downloads, extract them, and run GitLeaks scans to uncover any concealed secrets within Node.js packages. npm is the backbone of JavaScript development, and safeguarding it is essential.

![npm Secret Scanning](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*Qj1OTPHk-djj2C1Nnkn4VQ.png)

**npm scanning process:**

1. Download latest packages from npm registry
2. Analyze package contents
3. Run GitLeaks secret detection
4. Record results

### RubyGems and NuGet: Multitasking Marvel

Our third EC2 machine is a multitasker, handling both RubyGems and NuGet repositories. It extracts the latest RubyGems and NuGet packages, meticulously scans them using GitLeaks, and reports any secrets that may compromise the security of Ruby and .NET applications.

![RubyGems and NuGet Secret Scanning](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*VdrKUPx09FaAltG8A5uN7w.png)

**RubyGems and NuGet scanning process:**

1. Download packages from both repositories
2. Extract package contents
3. Run GitLeaks scan
4. Report secrets

## Automating the Full Process

When running tasks that involve downloading and analyzing large amounts of data, it's crucial to monitor and manage disk space. Without proper disk space management, the system can run out of space, causing disruptions and potentially failing the task. To address this issue, we've created a script that automates both the secret scan and disk space management.

### Script Overview

Let's break down the `free.sh` script step by step to understand its functionality:

```bash
#!/bin/bash

# Define the threshold for available disk space in GB
threshold=2

# Check available disk space in GB
available_space=$(df -h / | awk 'NR==2 { print $4 }' | sed 's/G//')

# Convert the available space to a numeric value
available_space_numeric=$(echo $available_space | sed 's/,//')

# Compare available space with the threshold
if [ "$available_space_numeric" -lt "$threshold" ]; then
    # Run gitleaks and write the output to a temporary file
    tmp_file=$(mktemp)
    echo $tmp_file | notify
    gitleaks detect --no-git -v downloaded_packages/ --config ~/config.toml -r=$tmp_file

    # Check if the downloaded_packages directory exists
    if [ -d "downloaded_packages" ]; then
        # Delete the downloaded_packages directory
        rm -rf downloaded_packages
        rm -rf .npm
    fi
else
    echo "Available disk space is greater than or equal to 2GB."
fi
```

**Script functions:**

1. **Threshold definition**: Sets minimum disk space threshold (2 GB)
2. **Disk space check**: Monitors current disk space
3. **Comparison**: Performs cleanup if space is below threshold
4. **GitLeaks scan**: Runs secret scanning
5. **Cleanup**: Removes downloaded packages

This automation prevents disk space exhaustion while performing continuous scanning.

## PackageSpy: Open-Source Secret Scanning Tool

**PackageSpy** is an innovative, open-source tool designed to scan package managers for secrets, user-defined keywords, and patterns. It helps developers safeguard their projects and ensure that sensitive information remains hidden from prying eyes.

### PackageSpy Features

**Support for Multiple Package Managers**: PackageSpy supports popular package managers like **npm, PyPI, RubyGems**, and **more**, making it versatile and adaptable to different development environments.

**Customizable Scanning Rules**: Developers can define their own scanning rules, keywords, and patterns to identify secrets specific to their projects. This flexibility ensures that **PackageSpy** can cater to diverse security requirements.

**Command-Line Interface (CLI)**: **PackageSpy**'s user-friendly CLI interface allows developers to initiate scans easily and integrate it into their development workflows.

**Interactive Reports**: After scanning, **PackageSpy** generates detailed reports highlighting any secrets or keywords found, their locations, and suggested actions for mitigation.

**Continuous Integration (CI) Integration**: **PackageSpy** seamlessly integrates with **CI/CD** pipelines, allowing developers to automate scans during the development process, preventing secrets from being committed to repositories.

> **GitHub**: [https://github.com/aydinnyunus/PackageSpy](https://github.com/aydinnyunus/PackageSpy)

## Analyzing Secret Scan Output

### Understanding the Risks of Exposed Secrets in NPM Packages

![npm Secret Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*wgbGyS0XCVxGR848pkE88w.png)

If you're a developer using **Node.js**, you're likely familiar with the **Node Package Manager (NPM)**. However, there's a hidden risk lurking in the shadows: the inadvertent exposure of secrets. Scan results reveal the types of secrets most commonly found in **NPM** packages and discuss the potential risks associated with such exposures.

**Secrets found in npm:**

- **AWS Access Tokens (34.3%)**: The most common secret type. These tokens provide access to Amazon Web Services and can lead to unauthorized access if they fall into the wrong hands.
- **HashiCorp Terraform Passwords (20.6%)**: Second most common. Terraform manages infrastructure and these passwords can allow infrastructure modification.
- **Private Keys (12.2%)**: A severe security threat. Private keys are used in cryptographic protocols for secure communications.
- **Stripe Access Tokens (7.2%)**: Direct financial risk. These tokens allow credit card transactions.
- **Slack Webhook URLs (25.7%)**: Particularly concerning. These URLs allow message sending and can lead to phishing attacks.
- **Telegram Bot API Tokens (14.3%)**: Can compromise bot interactions.

### The Silent Alarm: Exposed Secrets in PyPI Packages

![PyPI Secret Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*vhk8EDq5WmRg5d0ILWYgew.png)

Python developers, take heed. The Python Package Index (PyPI) is an indispensable resource, but recent findings show that it's also a minefield of security risks due to exposed secrets.

**Secrets found in PyPI:**

- **AWS Access Tokens (54.6%)**: The dominant risk. These tokens serve as a passport to Amazon Web Services, granting various levels of access.
- **HashiCorp Terraform Passwords (20.8%)**: The runner-up. Terraform automates infrastructure deployment.
- **Private Keys (10.4%)**: Hidden dangers. Vital for secure communications in various protocols.
- **JWTs (9.6%)**: Small percentage, big problems. These tokens are widely used for authentication and information exchange.
- **Other Secrets**: Etsy access tokens, Slack webhook URLs, and Telegram Bot API tokens are found in smaller percentages.

![PyPI Detailed Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*fCDu80WE8FrFatJzMWf2-g.png)

### The Red Flags in Ruby: Secrets Exposure in RubyGems

![RubyGems Secret Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*B8ER6VFc7if0JaaM0NpgAQ.png)

Ruby developers, it's time for a security check-up. RubyGems, the package manager that serves as a hub for distributing Ruby programs and libraries, has become a hotbed for exposed secrets.

**Secrets found in RubyGems:**

- **AWS Access Tokens (66.5%)**: Take the lion's share. This is not just a majority; it's a dominance that should raise eyebrows.
- **HashiCorp Terraform Passwords (15.8%)**: A distinct concern. Terraform manages infrastructure as code.
- **Private Keys (9.3%)**: Small pieces, big puzzle. Crucial for the security of communications in various encryption protocols.
- **JWTs (6.8%)**: Heavily used for authentication processes.
- **Stripe Access Tokens (1.7%)**: Financial implications. Misuse of these tokens can lead to financial fraud and loss.
- **Slack Webhook URLs and OpenAI API Keys**: Less prevalent but noteworthy.

![RubyGems Detailed Analysis](https://miro.medium.com/v2/resize:fit:1400/format:webp/1*nuIG50RYBF8INuFOnvZBig.png)

## Reporting Findings

Our secret scanning efforts have uncovered critical vulnerabilities within packages hosted on popular repositories, exposing sensitive information that could lead to severe security breaches. We take the responsibility of reporting these findings to the respective companies and organizations that own or manage the affected services.

### Finding Contacts

To identify and contact the owners or maintainers of the affected projects associated with the following companies, we utilize information available through package managers such as **npm, PyPI, RubyGems, NuGet, and others**.

**Reporting process:**

1. **Package Manager Investigation**: Check package.json, METADATA, gemspec, or nuspec files for contact information
2. **Project Documentation**: Search official documentation and repository for maintainer information
3. **Publicly Available Communication Channels**: Look for mailing lists, forums, or community channels
4. **Package Manager Messaging System**: Use npm owner add or PyPI maintainer messaging system

### Reporting Method

Once the contact information is obtained, we initiate the reporting process to the respective companies:

**Reported companies:**

- Microsoft
- Automattic
- Mapbox
- Keeper Security
- Pulumi
- Weblate
- Palo Alto Networks
- Telefonica Global
- Private (+7.5M Downloads)

**Reporting channels:**

- **Email Communication**: Send detailed emails to identified contacts within companies
- **HackerOne/Bugcrowd Platforms**: Submit vulnerabilities through bug bounty platforms if companies participate
- **Security Disclosure Policy**: Adhere to companies' security disclosure policies
- **Follow-Up and Collaboration**: Maintain open lines of communication with security teams

## Conclusion

Secret scanning within the latest packages from various repositories is an indispensable practice for upholding the security of software applications. Our three-pronged approach with dedicated EC2 machines, along with the introduction of the user-centric scanning tool, highlights our commitment to thorough security. By proactively identifying and mitigating secrets, developers can significantly diminish the odds of security breaches, safeguarding their organizations and users from the perils that lurk in the shadows.

**Key takeaways:**

- Secret scanning in package repositories is a critical security practice
- AWS tokens are the most common exposed secret type
- Automation is essential for continuous scanning
- Tools like PackageSpy provide developers with a robust security layer
- Responsible disclosure strengthens the security ecosystem

Always remember, the potency of open source blossoms through collaboration and responsible coding practices. Let us join hands in fortifying the software ecosystem, rendering it a safer haven for all.

## Related Content

- [PassDetective: Detecting Passwords and Secrets in Your Shell History](/2025/12/14/passdetective-kali-linux/)
- [exifLooter: Extracting Hidden Location Data from Images](/2025/12/07/exiflooter-kali-linux/)