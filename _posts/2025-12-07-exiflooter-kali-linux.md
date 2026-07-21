---
layout: post
title: "exifLooter: Extracting Hidden Location Data from Images"
date: 2025-12-07
author: Yunus Aydın
lang: en
description: "A detailed guide on exifLooter tool for extracting GPS coordinates from EXIF metadata, OpenStreetMap integration, and the OSINT tool added to Kali Linux."
keywords: "exiflooter, EXIF metadata, GPS coordinates, OSINT, Kali Linux, OpenStreetMap, metadata analysis, security research, bug bounty, geolocation"
canonical_url: "https://aydinnyunus.github.io/2025/12/07/exiflooter-kali-linux/"
---

While using exiftool in OSINT research, I developed exifLooter as an enhanced version that makes it easier to extract GPS coordinates and visualize them on OpenStreetMap. exifLooter is built on top of exiftool and provides a more practical solution, especially for extracting GPS coordinates and visualizing them on maps. Moreover, it's now included in Kali Linux's official tools.

## What is exifLooter?

exifLooter is an OSINT tool written in Go. It's essentially built on top of exiftool and provides additional features, especially for extracting GPS coordinates and visualizing them on OpenStreetMap. The tool uses exiftool's powerful metadata analysis capabilities to extract GPS coordinates and visualizes them on maps.

One of exifLooter's most important features is its ability to analyze both individual image files and all images in a specific directory in bulk. Additionally, it offers a critical security feature for metadata removal. This allows you to both collect information in OSINT research and clean metadata for security purposes with a single tool.

The tool has gathered **475+ stars** on GitHub and is widely used by the security community. It's also included in the official tools of Kali Linux and Black Arch Linux.

## Why exifLooter?

While doing bug bounty research, I noticed that sensitive information could be extracted from metadata of images uploaded to websites. Many websites don't strip EXIF data from user-uploaded images. This creates a security vulnerability.

For example, when a user uploads a profile photo, the GPS coordinates in the image can reveal the user's home address or frequently visited locations. This type of information can be very valuable in OSINT research.

## Features

The main features of exifLooter are:

- **Single Image Analysis**: Analyzes EXIF data of a specific image file
- **Directory Scanning and Bulk Analysis**: Automatically scans all image files (jpeg, png, jpg, jiff) in a directory and analyzes them in bulk. This feature allows you to analyze hundreds of photos at once
- **Metadata Removal**: Safely removes all EXIF metadata from images. This feature is critical for preventing the leakage of sensitive information from a security perspective
- **Pipe Support**: Integrates with other tools (waybackurls, subfinder, etc.)
- **OpenStreetMap Integration**: Visualizes GPS coordinates on OpenStreetMap

## Kali Linux Installation

exifLooter is now among Kali Linux's official tools. Installation is quite simple:

```bash
sudo apt update
sudo apt install exiflooter
```

After installation, you can use the `exifLooter` command directly. The tool depends on exiftool, so exiftool needs to be installed on your system. It usually comes pre-installed on Kali Linux.

![exifLooter Kali Linux installation screenshot]({{ site.baseurl }}/assets/images/exiflooter-kali-linux/install.png)

## Usage Examples

### Analyzing a Single Image

The simplest way to use it is to analyze a single image file:

```bash
exifLooter --image image.jpeg
```

This command shows all EXIF data in the image file and extracts GPS coordinates if they exist.

![exifLooter single image analysis example](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-24.png)

![exifLooter single image analysis result example](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-25.png)

### Directory Scanning and Bulk Analysis

One of exifLooter's most powerful features is its ability to automatically scan and bulk analyze all images in a directory. This feature allows you to analyze hundreds of photos at once:

```bash
exifLooter -d images/
```

This command automatically scans all image files (jpeg, png, jpg, jiff) in the specified directory and analyzes each one's EXIF data. It can also scan images in subdirectories and displays results in an organized manner.

The directory scanning feature is particularly useful in these scenarios:
- Analyzing all profile photos downloaded from a website
- Performing bulk photo analysis in social media research
- Scanning all uploaded images in bug bounty research


### Using with Pipe

You can use exifLooter together with other OSINT tools. For example, with waybackurls:

```bash
cat subdomains | waybackurls | grep "jpeg\|png\|jiff\|jpg" | exifLooter --pipe
```

This command filters image files from URLs retrieved from Wayback Machine and analyzes them with exifLooter.

### OpenStreetMap Integration

To visualize GPS coordinates on OpenStreetMap:

```bash
exifLooter --open-street-map --image image.jpeg
```

This command displays GPS coordinates from the image as an OpenStreetMap URL. You can click the URL to view the location on the map.

![exifLooter OpenStreetMap command output](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-31_16-17.png)

![exifLooter OpenStreetMap map view](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-31_16-12.png)

### Metadata Removal

One of exifLooter's most critical security features is its ability to remove metadata from images. This feature allows you to prevent the leakage of sensitive information (GPS coordinates, camera information, capture date, etc.).

To remove metadata from a single image:

```bash
exifLooter --remove --image image.jpeg
```

To remove metadata from all images in a directory:

```bash
exifLooter --remove -d images/
```

These commands safely remove all EXIF data from image files. This feature is especially critical for photos uploaded to websites. Many websites don't strip metadata from user-uploaded images, which creates security vulnerabilities.

The metadata removal feature can be used in these situations:
- Cleaning photos before uploading them to websites
- Preventing the leakage of sensitive information from a security perspective
- Protecting user data with a privacy-by-design approach

## Real-World Scenarios

### Bug Bounty Research

While testing a website's profile photo upload feature in a bug bounty program, I checked the metadata of my test photo and found that GPS coordinates were still present. This created a security vulnerability because users' sensitive location information was being leaked.

You can detect such vulnerabilities using exifLooter. These types of security vulnerabilities can earn rewards in bug bounty programs. For example, **$200** on HackerOne and **$500** on BugCrowd have been earned. This shows how important metadata security is.

### OSINT Research

During an OSINT research, I was analyzing photos shared on social media. I extracted metadata from these photos using exifLooter and found that many photos contained GPS coordinates. This information revealed the target person's frequently visited locations and movement patterns.

## Statistics and Achievements

- **GitHub Stars**: 475+ stars
- **Kali Linux**: Added as official tool
- **Black Arch Linux**: Added as official tool
- **Language**: Go (100%)
- **Dependency**: exiftool

## Security Vulnerability Reports

Examples of security vulnerabilities that can be detected with exifLooter:

![BugCrowd P3 security vulnerability report](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-29.png)

![BugCrowd P4 security vulnerability report](https://raw.githubusercontent.com/aydinnyunus/exifLooter/refs/heads/main/images/2022-07-30_16-29_1.png)

- **HackerOne Example**: [EXIF Geolocation Data Not Stripped](https://hackerone.com/reports/906907) - $200 reward
- **BugCrowd Example**: [EXIF Geolocation Data Not Stripped from Uploaded Images](https://medium.com/@souravnewatia/exif-geolocation-data-not-stripped-from-uploaded-images-794d20d2fa7d) - $500 reward

These reports show how critical metadata security is and how effective exifLooter is in detecting such vulnerabilities.

## Source Code and Contributing

exifLooter is an open source project available on GitHub:

- **GitHub Repository**: [https://github.com/aydinnyunus/exifLooter](https://github.com/aydinnyunus/exifLooter)
- **Kali Linux Page**: [https://www.kali.org/tools/exiflooter/](https://www.kali.org/tools/exiflooter/)

If you want to contribute to the project, you can open an issue or send a pull request on GitHub. All contributions are welcome.

## Conclusion

exifLooter is a powerful tool for metadata analysis in OSINT research and bug bounty programs. Especially its features for extracting GPS coordinates from images and visualizing them with OpenStreetMap are very valuable for researchers.

Its inclusion in Kali Linux shows how valuable the tool is considered by the security community. If you're doing OSINT research or working on bug bounty programs, I highly recommend trying exifLooter.

You can test it and use it in your own research. For more security research and projects, you can visit my [projects](/projects/) page.

You can help raise awareness about metadata security by sharing this post on social media.

## Related Content

Like this tool, for more content on OSINT and security research, you can check out my [Hacking Instagram Scammers](/2022/04/11/hacking-instagram-scammers/) post where I investigate phishing attacks and OSINT techniques. You can also explore more of my [security projects](/projects/) and tools.

