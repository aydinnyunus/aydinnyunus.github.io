---
layout: post
title: "Jekyll ile Blog Kurulumu"
date: 2024-01-20
author: Yunus Aydın
---

GitHub Pages ve Jekyll kullanarak statik bir blog sayfası kurmak oldukça kolay.

## Jekyll Nedir?

Jekyll, statik site oluşturucu bir araçtır. Markdown dosyalarınızı alır ve güzel bir web sitesine dönüştürür.

## Avantajları

1. **Hızlı**: Statik dosyalar olduğu için çok hızlı yüklenir
2. **Güvenli**: Sunucu tarafında çalışan kod yok
3. **Ücretsiz**: GitHub Pages ile ücretsiz hosting
4. **Kolay**: Markdown ile yazı yazmak çok kolay

## Kurulum

```bash
gem install jekyll bundler
jekyll new my-blog
cd my-blog
bundle exec jekyll serve
```

Bu kadar basit! Artık blogunuz hazır.

