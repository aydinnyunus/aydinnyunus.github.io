---
layout: post
title: "I scanned 34,794 GitHub commits for leaked secrets. 1,702 of them had live credentials."
author: Yunus Aydın
date: 2026-06-30
lang: en
description: "How I used Gemini 2.5 Flash Lite to label ~850k commit messages, distilled the labels into a regex-first filter, and ran TruffleHog on the survivors. 3,708 verified secrets in 1,702 unique commits."
keywords: "github archive, secret scanning, trufflehog, gemini, gh archive, leaked credentials, secret detection, regex from ai, commit message mining, push event scanner, secret leak detection, ai bootstrapped heuristics"
canonical_url: "https://aydinnyunus.github.io/2026/06/30/hunting-leaked-secrets-on-github-archive/"
---

I built a pipeline that watches the **GitHub Archive** firehose for commit messages that smell like a developer just leaked a credential. The pipeline scanned **34,794 commits**, ran TruffleHog on the diffs, and turned up **3,708 verified live secrets** across **1,702 unique commits**. Verified means TruffleHog called the issuing API and the key worked.

The interesting part is not the TruffleHog scan. TruffleHog is solved. The interesting part is the filter that runs before it. GitHub Archive at full volume is millions of events per day. Most of it is noise. You cannot afford to TruffleHog every diff and you cannot afford to AI-classify every commit message either. I tried both.

What worked was using a language model to bootstrap a regex, and then mostly throwing the language model away.

![From 250k hourly PushEvents down to 1,702 unique commits with verified live keys]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/funnel.png)

## The shape of the problem

GH Archive ships a JSON event stream for every public action on GitHub: pushes, PRs, issues, stars. The only events that matter for leaked secrets are `PushEvent` (commit messages + SHAs) and `PullRequestEvent` (PR titles, but only on `opened` / `reopened`). Everything else is exhaust.

A normal hour of `PushEvent` is roughly 250k commits. The signal I want is a tiny subset of those:

```text
remove api key
revoke aws credentials
delete .env
rotate token, pushed by mistake
fix: leaked secret
```

Naive grep on `key|token|secret|password` catches the signal and drowns in matches like "keyboard shortcut", "session token refresh", "password input component". The detection rate is fine. The false positive rate is unworkable when the next step is cloning the repo, fetching the diff, and running a binary on it.

The first naive run on a single GH Archive hour file blew through my Gemini free-tier quota in under a minute and still had hours of commits left to label. AI-first does not scale. Grep-first matches too much. Both lose.

## Phase 1: just ask the LLM

The first version was the lazy one. Every commit message went straight into **Gemini 2.5 Flash Lite** with a one-shot prompt:

```python
prompt = f"""
Analyze the following Git commit message to determine if it is fixing a secret leak.
The message might mention revoking, removing, or rotating keys, tokens, passwords, or other credentials.
Respond with only "true" if it is highly likely to be fixing a secret leak, otherwise respond with "false".

Commit Message:
---
{commit_message}
---
"""
```

This works. Flash Lite is good at this task and the response is one token. But pumping every PushEvent message through an API call is expensive and slow even on a cheap tier, and rate limits hit fast.

So I batched it. 250 commits per request, JSON-formatted reply, indexed by position:

```python
BASE_PROMPT_TEMPLATE = """
Analyze the following Git commit messages to determine if any of them are fixing a secret leak.
A message is considered to be fixing a secret leak if it mentions revoking, removing, or rotating keys, tokens, passwords, or credentials.
Return a JSON object where each key is the numeric index of the commit message (as a string) and the value is a boolean (`true` or `false`).
Ensure your response is only the JSON object.
"""
```

The batch path was faster and cheaper but still bottlenecked on AI calls. After a few weeks of running, two log files had grown enormous:

```text
suspicious_commit_messages.log     59,014 lines
not_suspicious_commits.log        793,199 lines
```

That is roughly **852,000 messages labeled by Gemini**. The cache made repeated AI calls free, but new traffic kept costing. The bigger lesson sitting in those two files was: the labels were not random. They clustered.

## Phase 2: stealing the regex from the model

If you sort `suspicious_commit_messages.log` and skim it, the structure jumps out. Almost every true-positive is a verb plus a noun. The verbs are tiny:

```text
remove, delete, revoke, invalidate, rotate, regenerate, leak, expose, compromise, fix
```

The nouns are larger but bounded. A few generic words (`key`, `token`, `secret`, `credential`, `password`), and then a long tail of brand-specific names: `aws`, `openai`, `slack`, `stripe`, `datadog`, `mongodb_uri`, `firebase`, on and on. Every cloud, every SaaS, every CI provider, every payment processor.

So I asked the model to do something different. Not "classify this message", but **"give me the patterns you are using to classify it"**. The output became a small grammar.

I split it into two tiers. The high-confidence tier is verbs + nouns that are almost always about secrets:

```python
HIGH_CONFIDENCE_ACTION_VERBS = [
    "remove", "delete", "revoke", "invalidate",
    "rotate", "regenerate", "leak", "expose",
    "compromise", "fix",
]

HIGH_CONFIDENCE_OBJECT_NOUNS = [
    "api_key", "apikey", "access_token", "auth_token",
    "private_key", "secret_key", "client_secret",
    "credential", "credentials", "password", "passwd",
    "aws_secret", "aws_access_key", ".env", "dotenv",
    # ...
]
```

The broad tier is forgiving verbs like `update` and `change`, plus the long tail of brand names. Hundreds of detector keywords pulled directly from TruffleHog's own detector catalog (`stripe`, `twilio`, `mailgun`, `sendgrid`, `slackbot`, `cloudflare`, `algolia`, and so on) plus generic infra terms.

```python
BROAD_ACTION_VERBS = [
    "update", "change", "fix", "patch", "clean",
    "remove", "delete", "purge", "wipe", "scrub",
    # ...
]
```

On top of that, three compiled regexes catch the canonical "I just leaked a secret" message shapes directly:

```python
SECRET_REMOVAL_PATTERNS = [
    re.compile(r'\b(remove|delete|revoke|invalidate|rotate|regenerate)\b.*\b(key|token|secret|password|credential)\b', re.IGNORECASE),
    re.compile(r'\b(fix|patch)\b.*\b(leak|expose|compromise)\b', re.IGNORECASE),
    re.compile(r'\b(revert)\b.*for.*security.*reason', re.IGNORECASE),
]
```

The first one alone catches a huge chunk of the truly self-incriminating commits. People literally write `Remove leaked API key` in their commit message, every single day, in public.

## Phase 3: regex first, model only when stuck

With the grammar in hand, the pipeline flipped. The default path is now:

1. Apply `SECRET_REMOVAL_PATTERNS`. If any hit, flag the commit.
2. Apply tier 1 (high-confidence verb AND high-confidence noun in the same message). If hit, flag.
3. Apply tier 2 (broad verb AND broad noun). If hit, flag.
4. Only if a message is ambiguous and not in either cache, fall back to Gemini.

This is regex-first, AI-fallback. The AI is no longer in the hot path. It became a tiebreaker for the long tail.

Two side benefits I did not plan for:

- The pipeline became **deterministic and replayable**. Same input, same output, no API drift between runs.
- The pipeline became **cheap to run offline**. No quota, no rate limits, no network. The only network I/O is fetching the diffs and running TruffleHog.

Anything the regex misses still gets sent to Gemini, and any new true positive Gemini finds becomes a candidate for the next regex update. The model is now training the filter, not running it.

## What TruffleHog found

Once a commit message passed the filter, the pipeline fetched the diff from GitHub and ran TruffleHog with verification enabled. Every "verified" result means TruffleHog called the issuing service's API and the key worked.

Final numbers from one batch run:

```text
PushEvent commits scanned (after filter)   34,794
Unique commit messages matched (keyword)      197
Verified secret records                     3,708
Unique commits containing a verified secret 1,702
```

One matched message frequently maps to hundreds of commits: thousands of different developers literally commit `remove api key` verbatim, and every one of those commits gets pulled. So 197 unique messages expanded into 34,794 commits to scan.

That is roughly a **4.9% verified-leak rate per scanned commit**. Of those, the dominant detectors were the usual suspects: cloud provider keys, SaaS API keys, database connection strings with embedded credentials. The kind of thing that turns into a billing event before anyone notices.

![Top detectors by verified-secret count]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/top-detectors.png)

If you want a sense of how fast "leak on GitHub" becomes "exploit", Mackenzie Jackson and Andrzej Dyjak ran the canary experiment back in 2020: an AWS key seeded via [canarytokens.org](https://canarytokens.org), pushed to a public repo, was first abused **11 minutes** after the push. ([What actually happens when you leak credentials on GitHub](https://dev.to/advocatemack/what-actually-happens-when-you-leak-credentials-on-github-the-experiment-34md)). My pipeline runs on a 5 to 15 minute lag against GH Archive. The attackers are not slower than me.

## What this implies if you ship code

Commit messages are the loudest signal a developer ever produces about their own credential mistakes. If you ever wrote `remove leaked api key`, `revoke aws`, or `fix exposed token` in a public commit, someone has already grepped you. The window between push and exploit is not hours, it is minutes. Rotating the key is mandatory and pretending the original push never happened is not a strategy.

Pre-commit secret scanning is the only intervention that actually wins this race. Anything that runs after `git push` is too late. Here is the same problem from the other side: a TruffleHog pre-commit hook blocking a fake AWS key in real time.

![TruffleHog pre-commit hook blocking an AWS access key before commit]({{ site.baseurl }}/assets/images/hunting-leaked-secrets/trufflehog-pre-commit.gif)

Ten lines of YAML, adds maybe a second to your commit time. It is the cheapest insurance in security tooling.

And about how to use language models in a security pipeline: use them to bootstrap, not to operate. They are extremely good at noticing patterns in unstructured text and extremely bad at being your hot path at scale. Mine your model's outputs for the underlying rule, write the rule down as code, and put the model back on the bench for the next round of pattern discovery.

## Related content

- [What actually happens when you leak credentials on GitHub (Mackenzie Jackson)](https://dev.to/advocatemack/what-actually-happens-when-you-leak-credentials-on-github-the-experiment-34md)
- [GH Archive](https://www.gharchive.org/)
- [TruffleHog](https://github.com/trufflesecurity/trufflehog)
- [TruffleHog pre-commit hooks docs](https://trufflesecurity.com/docs/pre-commit-hooks)

## References

- [GH Archive event reference](https://docs.github.com/en/webhooks-and-events/events/github-event-types)
- [TruffleHog detector catalog](https://github.com/trufflesecurity/trufflehog/tree/main/pkg/detectors)
- [Canary Tokens (Thinkst)](https://canarytokens.org/)
- CWE-798: Use of Hard-coded Credentials
- CWE-540: Inclusion of Sensitive Information in Source Code
