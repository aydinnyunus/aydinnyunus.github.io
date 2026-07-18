# Changelog Analyzer Blog Post — Implementation Plan

> **For Claude:** Use this plan to draft the blog post in `_posts/`. Follow blog-writing.mdc and seo.mdc rules. No code implementation; content and structure only.

**Goal:** Publish a technical blog post that explains the changelog-analyzer project: why security fixes without CVE matter, how the pipeline works (GitHub releases → changelog/commits → LLM security detection → NVD check → alerts), and what readers can do with it.

**Audience:** Security researchers, maintainers, and developers interested in vulnerability discovery, CVE triage, and supply-chain security. Readers should get a clear mental model and, if interested, know how to run the tool.

**Angle:** "I run a list of GitHub repos, pull release changelogs and commits, flag security-related fixes, check NVD for CVEs, and analyze the ones that don’t have a CVE." Personal, practical, no hype.

**Tech stack (for accuracy in post):** Python CLI, GitHub API, GenAI API (e.g. Gemini) for security detection, NVD API for CVE check, SQLite for scans/alerts, optional FastAPI web UI. No need to name every file; focus on flow and concepts.

---

## Task 1: Outline and primary keyword

**Files:**
- Create: `docs/plans/2026-02-27-changelog-analyzer-blog-post.md` (this file; add outline below)
- Reference: `changelog-analyzer/README.md`, `changelog-analyzer/src/changelog_analyzer/scan.py`, `security_detector.py`, `cve_checker.py`

**Step 1: Define primary keyword and H2/H3 skeleton**

- **Primary keyword:** e.g. "security fixes without CVE" or "finding unreported vulnerabilities in changelogs" (pick one for title/description).
- **Proposed H2s:** Problem (why CVE-less fixes matter), Pipeline overview, Changelog and commit analysis, Security detection (LLM + rules), CVE check and alerting, What you get (alerts, CVE reports), Limitations and noise, Takeaway.

**Step 2: Lock the outline**

- Order: Hook → Problem → Pipeline → Components (changelog/commits, detection, CVE) → Output (alerts, reports) → Limitations → Takeaway.
- Ensure each H2 can stand alone for search (descriptive, not generic).

---

## Task 2: Hook and problem section

**Files:**
- Create: `_posts/YYYY-MM-DD-<slug>.md` (date from plan or today; slug e.g. `finding-security-fixes-without-cve-changelog-analyzer`)

**Step 1: Write the hook (first paragraph)**

- Why it matters: Many real security fixes never get a CVE; changelogs and commits are an underused signal.
- What the reader will learn: How a pipeline can surface security-related fixes and separate those without CVE for triage.
- Use primary keyword naturally in the first paragraph.
- 2–4 short sentences; no generic "in today’s world" opener.

**Step 2: Write the problem section**

- Describe the situation: GitHub repos, release notes, commit messages; lots of "fixed security issue" with no CVE.
- Why that’s a gap: Hard to track, no central ID, risk of missing real vulns.
- One concrete example if possible (e.g. "fixes SQL injection in X" with no CVE).
- Keep tone direct; avoid AI-ish emphasis words (crucial, pivotal, landscape).

---

## Task 3: Pipeline overview

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: High-level pipeline**

- Input: GitHub repo URL (or list of repos).
- Steps: Fetch releases → parse changelog (or commits when no body) → detect security-related entries → optionally verify with commits/patches → check NVD for CVE → emit alerts for "security fix, no CVE."
- One short paragraph + optional bullet list or simple diagram description (e.g. "Repo → Releases → Changelog entries → Security detection → CVE check → Alerts").
- No implementation detail here; just flow.

**Step 2: Clarify scope**

- What the tool does: Scans releases/changelogs and commits, uses LLM + heuristics for security, NVD for CVE, outputs JSON alerts and optional CVE-style reports.
- What it doesn’t do: Not a full pentest, not dependency-only scanning (dependency bumps are filtered out).

---

## Task 4: Changelog and commit analysis

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: Where the data comes from**

- Releases from GitHub API; for each release, use release body as changelog.
- If body is empty: use commits between this and the previous release; first line of each commit message becomes a changelog-like entry.
- Short paragraph; no code unless one snippet adds clarity.

**Step 2: Why both matter**

- Changelog = maintainer’s summary; commits = evidence. Tool uses both: changelog for batch detection, commits for verification and patch-level check when confidence is high enough.

---

## Task 5: Security detection (LLM + rules)

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: Detection pipeline**

- Batch analysis of changelog text with an LLM (e.g. Gemini), prompted to identify *exploitable* vulnerability fixes (SQLi, XSS, RCE, SSRF, auth bypass, info disclosure, etc.) and to skip dependency bumps, "security improvements," TLS upgrades, feature additions.
- Confidence score; only entries above threshold (e.g. 0.9 for exploitable) go forward.
- Early filters: skip patterns (e.g. chore(deps), "security improvement") and require security-related keywords before calling the API.

**Step 2: Verification with commits and patches**

- For high-confidence changelog hits: fetch related commits, optionally patch content.
- LLM + heuristics on patches (e.g. parameterized query, escaping, sensitive data removed) to confirm it’s a real fix, not just docs/tests. Filter out test/doc/config-only changes.
- Mention that this reduces false positives; keep description concise.

---

## Task 6: CVE check and alerting

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: NVD and relevance**

- For each detected security fix: extract keywords + vulnerability type, query NVD API.
- Relevance logic: CVE description vs fix description (technical term overlap, repo/product name); explicit CVE ID in changelog = direct match.
- If no relevant CVE: create alert (version, type, description, confidence, reasoning, commit links).

**Step 2: Output and CVE reports**

- Alerts: JSON file per repo/run (e.g. `security_alerts_owner_repo_timestamp.json`) with release version, vuln type, description, confidence, fix commit, affected files.
- Optional: approved alerts can get CVE-style reports (AI or rule-based description) for submission or internal use.
- One short paragraph; link to repo or README for exact commands.

---

## Task 7: What you get and limitations

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: What you get**

- List/scan of repos → alert list for "security fix, no CVE" with version, type, reasoning, and links. Optionally DB (SQLite) and web UI for triage.
- Practical use: Prioritize which fixes might deserve a CVE, or track silent security fixes across dependencies.

**Step 2: Limitations and noise**

- False positives: LLM can mark non-exploitable or vague entries as security fixes; verification and confidence thresholds mitigate but don’t remove.
- NVD coverage: Not every CVE is in NVD quickly; some fixes might have a CVE elsewhere.
- Rate limits: GitHub and NVD APIs; token and throttling matter for large runs.
- One short paragraph; no "challenges and future prospects" section header—fold into limitations.

---

## Task 8: Takeaway and CTA

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: Concise takeaway**

- Mental model: Changelogs and commits are a useful signal for security fixes; many have no CVE; a pipeline can surface and triage them.
- 2–3 sentences; synthesis, not repetition.

**Step 2: Optional CTA**

- If applicable: link to repo (changelog-analyzer) or related post (e.g. CVE triage, vulnerability disclosure). One line; no "hope this helps."

---

## Task 9: SEO and front matter

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: Front matter**

- `layout: post`
- `title`: SEO-friendly, under ~60 chars, primary keyword
- `date`: YYYY-MM-DD
- `author`: Yunus Aydın
- `lang`: en (or tr if post is Turkish)
- `description`: 150–160 chars, primary keyword
- `keywords`: 5–10 comma-separated
- `canonical_url`: full URL
- `image`: optional

**Step 2: Meta and internal links**

- Ensure at least one H2/H3 uses primary or secondary keyword naturally.
- Add 2–3 internal links to related posts if they exist.
- Image alt text: descriptive, not generic.

---

## Task 10: Humanization and final pass

**Files:**
- Modify: `_posts/YYYY-MM-DD-<slug>.md`

**Step 1: Voice and rhythm**

- Vary sentence length; add first person where it fits ("I run this on…", "I wanted to…").
- Remove: filler (in order to, due to the fact that), hedging (could potentially), AI vocabulary (additionally, delve, showcase, pivotal, landscape), generic conclusions.
- Check: no em dashes overuse, no bold on every term, sentence case for headings.

**Step 2: Read aloud and trim**

- Read the post aloud; fix anything that sounds like Wikipedia or marketing.
- Short paragraphs (2–4 lines); scannable bullets where helpful.

---

## Execution options

After this plan is done:

1. **Draft in this session** — I write the post section by section in `_posts/`, following the tasks above; you review and edit.
2. **Draft in a follow-up session** — You open a new chat, paste this plan or the task list, and ask to "implement the blog post plan for changelog-analyzer"; I generate the full draft in one go, then you do the humanization pass.

**If you want a Turkish version:** Add a parallel task set (same structure, `lang: tr`, Turkish slug, TR front matter) after the English post is finalized; re-use outline and facts, adapt tone to blog-writing.mdc Turkish guidelines.
