---
layout: post
title: "Sleep: Reviewable Memory for Coding Agents"
lang: en
author: Yunus Aydın
description: "Sleep gives coding agents a reviewable memory cycle. Captures observations, consolidates into provisional memory, and only promotes what a human approves. T0 to T1 to T2."
keywords: "agent memory, coding agent, LLM context window, provisional memory, Claude Code hooks, Pi extension, memory consolidation, agent automation, prompt injection, conscious LLM memory"
canonical_url: "https://aydinnyunus.github.io/2026/11/01/sleep-agent-memory-extension/"
---

I built Sleep because I kept losing the most useful thing my agents learned. Every session I spend a few hours teaching an assistant the staging gateway, the naming conventions, which repos fail first. Then the context window closes and none of it survives. I wanted the opposite: agents that remember what worked, without giving them the keys to rewrite who you are. Sleep is an open-source tool that adds that reviewable memory cycle to Pi, OmO, and Claude Code.

The name is not arbitrary. Google Research published a framework that borrows the brain's wake/sleep pattern for continual learning in LLMs. Their idea is to carve the sleep phase out as a separate offline job that compresses accumulated knowledge without corrupting older versions. I found that pattern deeply practical, but the scale is different and that changes the whole design. Their sleep phase runs over model weights for continual learning. Mine runs over session buffers and never touches a weight. They compress knowledge on a schedule. I gate it behind human approval and measure it with replay.

## The gap: memory only inside the context window

Deployed LLMs have two failure modes when you want them to learn. You can keep updating parameters through continued training, which triggers catastrophic forgetting: new tasks come in, old performance drops. Or you can rely on in-context learning, where everything you teach lives only for the length of that window and dies when the session settles.

Both are bad for a coding agent. The knowledge that matters to you, a private staging gateway or a project convention, is exactly the kind of thing a generic training run never sees. And it is also the kind of thing a context window throws away at the end of every call.

So the practical problem is not "how do I make my model smarter." It is "how do I persist what this agent learned about my project, without trusting the agent to curate its own memory." Those are different goals, and conflating them is where most agent-memory tools go wrong.

## The design: three tiers, one human gate

Sleep separates memory into three tiers, and only one of them is ever permanent.

| Tier | What it is | Who writes it |
|---|---|---|
| T0 | Temporary session observation in a buffer | Hooks, extension, or manual capture |
| T1 | Provisional memory, shown as unverified background | Consolidation and repeated-preference mining |
| T2 | Permanent memory | Only your explicit `promote` |

The core guarantee lives in the boundary between T1 and T2. A memory is never durable because the agent wanted it to be. It becomes durable only when you run `promote` on a candidate you inspected. Everything the model produces stays provisional, and it cannot mark its own output as user-confirmed.

For the day-to-day flow it looks like this:

```text
session activity -> T0 buffer -> gated sleep -> T1 provisional memory
                                                   |
                                             review or replay
                                                   |
                                             explicit promote
                                                   |
                                             T2 permanent memory
```

## How it works

On Pi and OmO, Sleep ships as a user-installed extension. It loads into the agent loop and does three mechanical jobs.

1. **Capture.** The human's own words and tool events that carry a path are moved into the T0 buffer.
2. **Publish.** Matching provisional memory is injected in front of the model on every request, and re-read after a context compaction drops it.
3. **Wake.** After the agent settles, it triggers consolidation through a policy gate. The model call never blocks the loop.

On Claude Code the same jobs run through `UserPromptSubmit`, `PostToolUse`, `Stop`, and `PreCompact` hooks. Both integrations share `$SLEEP_HOME`, so a candidate you review from one client is visible from the other when the project scope matches.

Consolidation itself is gated. By default the gate requires at least 3 captured records and a 6 hour interval since the previous pass, so a model call does not happen after every message. You inspect the decision first:

```bash
./sleep.py status -v
./sleep.py sleep --dry-run
./sleep.py sleep --force
```

The status output is the whole model in one screen. This is what `status -v` reports on a live install:

```text
sleep memory: T0=disposable captures, T1=provisional awaiting review, T2=permanent (promote-only)
T0 buffer      53 records / 4 files / 20584 bytes  (captured while you worked)
T1 candidates  6 provisional, 7 total  (distilled, shown as unverified context)
   quarantine  0 parked (needs human eyes)  (held out until you review)
T2 canonical   ~/.claude-work/.../memory  (96 permanent records; only you can promote)
```

Two lines carry the whole design. T0 is disposable: it is the working capture, low stakes if lost. T1 is explicitly unverified context, it can never issue a directive. T2 is the only durable tier, and it notes that only you can promote. The quarantine line is the safety net made visible.

Then you review the resulting candidate before making it real:

```bash
./sleep.py show <candidate-id>
./sleep.py promote <candidate-id> --dry-run
./sleep.py promote <candidate-id>
```

And when you reject one, the refusal keeps an audit trail instead of deleting it:

```bash
./sleep.py refuse <candidate-id> --note "Incorrect for the current deployment."
```

## The dream cycle: mining, consolidation, replay

Sleep also has an unattended nightly pass it calls the dream cycle, and it never updates weights or writes permanent memory on its own. It does three things.

**Mine.** Find instructions you repeated across separate sessions in the same project. This step is deterministic and makes no model call at all.

**Consolidate.** When the sleep gate passes, ask the configured model to turn recent T0 observations into provisional T1 candidates. Budget for the call is explicit.

**Replay.** When replay is due, run selected tasks with provisional memory off and on, then write a comparison report. This one is the piece I like most: instead of asking you to guess whether a memory helps, it measures the effect on a real task.

```bash
./sleep.py nightly --dry-run
./sleep.py mine
./sleep.py replay --run --limit 3 --budget 0.10
```

Before it spends anything, replay shows you exactly what it would run and what each task would cost. A plan on a live install looks like this:

```text
dry run: would execute 3 task(s) x2 conditions (read-only tools, $0.50/task). Pass --run to execute.
  staging-gateway   Records the X-Route header the staging ALB requires.
  mcp-creds         Notes which servers need literal credentials, not env expansion.
  weight-field      Confirms the weight field is silently dropped for this project.
```

Replay reports results but never promotes. The `--budget` caps the cost per task across both arms.

## The safety that everyone asks about

Giving an agent memory is a prompt injection surface, and I did not want to hand that power over blindly. There are concrete gates in the pipeline.

- **Redaction.** Secret-shaped values never reach any tier. Credentials stay in your env or vault.
- **Quarantine.** Instruction-shaped or injected-looking text never becomes a candidate. It is parked with a reason and reported.
- **Provenance trust.** Only text with source `user` may be phrased as a directive. Agent and inferred records are always rendered as context, never as an instruction.
- **Project isolation.** A project's provisional memory is injected only into that project, and mixed-project buffers fail closed instead of assigning facts to the wrong place.
- **Explicit budget.** Every model call in the pipeline has a cost ceiling.

The threat model is honest about scope. This is a filter, not a sandbox. What it guarantees is specific: durable memory stays behind a human approval, and machine-generated prompts cannot claim to be you.

## When memory gets reused, it gets measured

The part that separates this from a naive "save the transcript" tool is deliberate. Most agent-memory projects grab context, stuff it into the next prompt, and call it done. The blind spot is that you never learn whether the memory actually helped.

Sleep closes that loop with replay. You run the same task with the provisional memory on and off, and you get a comparison instead of a faith-based decision. The result decides whether you promote the candidate into T2 or let it fade. That is the difference between an agent that accumulates clutter and one that accumulates judgment.

The deeper point is that this is a safety property, not a nice-to-have. Untested memory is how an agent quietly starts doing things you never asked it to do. Measured memory is how you can tell what actually changed.

## Why I built it this way instead of trusting the agent

I have run enough long agent sessions to be skeptical of any design that lets the model curate its own durable state. The agent has an incentive to preserve whatever it captured, even when that capture is wrong, noisy, or the product of a prompt injection. So the sacred part of the design is the human review step, and I refuse to automate it away. Uptime is not the goal. Being able to look at a candidate, say no, and have that no stick with an audit trail is the whole reason the tool exists.

Keeping memory reviewable is the rare change that makes agents both more useful and less risky at the same time. Useful, because the knowledge that took you hours to transfer does not evaporate between sessions. Less risky, because nothing durable happens without you.

## Try it

```bash
git clone https://github.com/aydinnyunus/sleep.git
cd sleep
./install.sh                 # shared skill and Claude Code hooks
./install.sh --extension     # also the Pi/OmO extension
./sleep.py sleep --force     # try a consolidation pass
```

The installer runs the test suite first, backs up your config before changing it, and is safe to run again. A dry run shows every change without writing a single file: `./install.sh --dry-run`.

When you adopt agent memory, take the fixed points with it. Permanent memory needs an explicit human approval step, refreshing it from the buffer needs a gate, and reusing it needs a measurement. Skip any of those and you end up with memory you cannot trust, which is worse than no memory at all.

## References

- [Sleep repository](https://github.com/aydinnyunus/sleep)
- [The sleep framework: A new way for LLMs to retain knowledge in continual learning (Google Research)](https://research.google/blog/the-sleep-framework-a-new-way-for-llms-to-retain-knowledge-in-continual-learning/)
- [The sleep framework paper (arXiv)](https://arxiv.org/abs/2606.03979)

## Related Content

- [AI CAPTCHA Bypass: Solving reCAPTCHA with GPT-4o and Gemini](/2025/12/08/ai-captcha-bypass/)
- [PassDetective: Detecting Passwords and Secrets in Your Shell History](/2025/12/14/passdetective-kali-linux/)
