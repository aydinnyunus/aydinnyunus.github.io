---
layout: post
title: "Sleep: Reviewable Memory for Coding Agents"
lang: en
author: Yunus Aydın
description: "Sleep gives coding agents a reviewable memory cycle. It captures observations, consolidates them into provisional memory, and only promotes what a human approves. T0 to T1 to T2."
keywords: "agent memory, coding agent, LLM context window, provisional memory, Claude Code hooks, Pi extension, memory consolidation, agent automation, prompt injection, conscious LLM memory"
canonical_url: "https://aydinnyunus.github.io/2026/11/01/sleep-agent-memory-extension/"
---

I built Sleep because I kept losing the most useful thing my agents learned. Every session I spend a couple of hours teaching an assistant the staging gateway, the naming conventions, which repo tends to break first. Then the context window closes and none of it survives. I wanted the opposite: agents that remember what worked, without handing them the keys to rewrite me. Sleep is an open-source tool that adds that reviewable memory cycle to Pi, OmO, and Claude Code.

The name is not clever. Google Research published a framework that borrows the brain's wake/sleep pattern for continual learning in LLMs, and the idea is to carve the sleep phase out as a separate offline job that compresses accumulated knowledge without breaking older versions. I found that deeply practical, but the scale is different and it changes the whole design. Their sleep phase runs over model weights. Mine runs over session buffers and never touches a weight. They compress knowledge on a schedule. I hold it behind human approval and measure it with replay.

## The gap: memory lives only inside the context window

There are two ways to make a deployed LLM learn, and both are wrong for a coding agent.

You can keep updating parameters with continued training. That triggers catastrophic forgetting: new tasks come in, old performance drops. Or you can lean on in-context learning, where everything you teach survives only as long as that window and dies the moment the session settles.

Neither helps. The knowledge you care about, a private staging gateway or a project convention, is exactly what a generic training run never sees and what a context window throws away at the end of every call.

So the real question is not "how do I make my model smarter." It is "how do I make the things this agent learned about my project stick, without trusting the agent to curate its own memory." Those are different goals, and conflating them is where most agent-memory tools go wrong.

## The design: three tiers, one human gate

Sleep splits memory into three tiers, and only one of them ever goes permanent.

| Tier | What it is | Who writes it |
|---|---|---|
| T0 | Temporary session observation in a buffer | Hooks, extension, or manual capture |
| T1 | Provisional memory, shown as unverified background | Consolidation and repeated-preference mining |
| T2 | Permanent memory | Only your explicit `promote` |

The core guarantee sits on the line between T1 and T2. A memory never becomes durable just because the agent wanted it to. It becomes durable only when you run `promote` on a candidate you actually inspected. Everything the model produces stays provisional, and it cannot mark its own output as user-confirmed.

The day-to-day flow looks like this:

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

1. **Capture.** Your own words and the tool events that carry a path land in the T0 buffer.
2. **Publish.** Matching provisional memory is injected in front of the model on every request, and re-read after a context compaction drops it.
3. **Wake.** Once the agent settles, a policy gate triggers consolidation. The model call never blocks the loop.

On Claude Code the same jobs run through `UserPromptSubmit`, `PostToolUse`, `Stop`, and `PreCompact` hooks. Both integrations share `$SLEEP_HOME`, so a candidate you review from one client shows up in the other when the project scope matches.

Consolidation itself is gated. By default the gate wants at least 3 captured records and a 6 hour gap since the last pass, so it does not fire a model call after every message. You look at the decision before it runs:

```bash
./sleep.py status -v
./sleep.py sleep --dry-run
./sleep.py sleep --force
```

The status output is the whole design on one screen. On a working install, `status -v` reports this:

```text
sleep memory: T0=disposable captures, T1=provisional awaiting review, T2=permanent (promote-only)
T0 buffer      53 records / 4 files / 20584 bytes  (captured while you worked)
T1 candidates  6 provisional, 7 total  (distilled, shown as unverified context)
   quarantine  0 parked (needs human eyes)  (held out until you review)
T2 canonical   ~/.claude-work/.../memory  (96 permanent records; only you can promote)
```

Two lines carry the whole idea. T0 is disposable, the working capture, low stakes if lost. T1 is explicitly unverified context and can never issue a directive. T2 is the only durable tier, and it exists so only you can promote. The quarantine line is the safety net made visible.

Then you review the candidate before making it real:

```bash
./sleep.py show <candidate-id>
./sleep.py promote <candidate-id> --dry-run
./sleep.py promote <candidate-id>
```

And when you reject one, the refusal keeps an audit trail instead of wiping it:

```bash
./sleep.py refuse <candidate-id> --note "Incorrect for the current deployment."
```

## The dream cycle: mining, consolidation, replay

Sleep also has an unattended nightly pass it calls the dream cycle. It never updates weights and never writes permanent memory on its own. It does three things.

**Mine.** Find the instructions you repeated across separate sessions in the same project. This step is deterministic and makes no model call at all.

**Consolidate.** When the sleep gate passes, the configured model turns recent T0 observations into provisional T1 candidates. The call has an explicit budget.

**Replay.** When replay is due, run selected tasks with provisional memory off and then on, and write a comparison report. This is the piece I like most: instead of asking you to guess whether a memory helps, it measures the effect on a real task.

```bash
./sleep.py nightly --dry-run
./sleep.py mine
./sleep.py replay --run --limit 3 --budget 0.10
```

Before it spends anything, replay shows you exactly what it will run and what each task will cost. On a working install the plan looks like this:

```text
dry run: would execute 3 task(s) x2 conditions (read-only tools, $0.50/task). Pass --run to execute.
  staging-gateway   Records the X-Route header the staging ALB requires.
  mcp-creds         Notes which servers need literal credentials, not env expansion.
  weight-field      Confirms the weight field is silently dropped for this project.
```

Replay reports results but never promotes. The `--budget` caps the cost per task across both arms.

## The safety that everyone asks about

Giving an agent memory is a prompt injection surface, and I did not want to hand that power over blindly. The pipeline has concrete gates.

- **Redaction.** Secret-shaped values never reach any tier. Credentials stay in your env or vault.
- **Quarantine.** Instruction-shaped or injected-looking text never becomes a candidate. It gets parked with a reason and reported.
- **Provenance trust.** Only text with source `user` may be phrased as a directive. Agent and inferred records are always rendered as context, never as an instruction.
- **Project isolation.** A project's provisional memory is injected only into that project, and mixed-project buffers fail closed instead of assigning facts to the wrong place.
- **Explicit budget.** Every model call in the pipeline has a cost ceiling.

Let me be honest about scope. This is a filter, not a sandbox. What it guarantees is narrow: durable memory stays behind a human approval, and machine-generated prompts cannot pass themselves off as you.

## When memory gets reused, it gets measured

The thing that separates this from a naive "save the transcript" tool is deliberate. Most agent-memory projects grab the context, stuff it into the next prompt, and call it done. The blind spot is that you never find out whether the memory actually helped.

Sleep closes that loop with replay. You run the same task with the provisional memory on and off, and you get a comparison instead of a hope-based decision. The result decides whether a candidate moves into T2 or fades. That is the difference between an agent that hoards noise and one that accumulates judgment.

This is a safety property, not a nice-to-have. Untested memory is how an agent quietly starts doing things you never asked it to do. Measured memory is how you can tell what actually changed.

## Why I built it this way

I have run enough long agent sessions to distrust any design that lets the model curate its own durable state. The agent has every reason to preserve whatever it captured, even when that capture is wrong, noisy, or the product of a prompt injection. So the human review step is load-bearing, and I refuse to automate it away. Uptime is not the goal. Being able to look at a candidate, say no, and have that no stick with an audit trail, is the whole reason the tool exists.

Keeping memory reviewable is the rare change that makes agents more useful and less risky at the same time. Useful, because the knowledge that took you hours to transfer does not evaporate between sessions. Less risky, because nothing durable happens without you.

## Try it

```bash
git clone https://github.com/aydinnyunus/sleep.git
cd sleep
./install.sh                 # shared skill and Claude Code hooks
./install.sh --extension     # also the Pi/OmO extension
./sleep.py sleep --force     # try a consolidation pass
```

The installer runs the test suite first, backs up your config before changing it, and is safe to run again. A dry run shows every change without writing a single file: `./install.sh --dry-run`.

If you adopt agent memory, take the fixed points with it. Permanent memory needs an explicit human approval step, refreshing it from the buffer needs a gate, and reusing it needs a measurement. Skip any of those and you end up with memory you cannot trust, and untrustworthy memory is worse than none at all.

## References

- [Sleep repository](https://github.com/aydinnyunus/sleep)
- [The sleep framework: A new way for LLMs to retain knowledge in continual learning (Google Research)](https://research.google/blog/the-sleep-framework-a-new-way-for-llms-to-retain-knowledge-in-continual-learning/)
- [The sleep framework paper (arXiv)](https://arxiv.org/abs/2606.03979)

## Related Content

- [AI CAPTCHA Bypass: Solving reCAPTCHA with GPT-4o and Gemini](/2025/12/08/ai-captcha-bypass/)
- [PassDetective: Detecting Passwords and Secrets in Your Shell History](/2025/12/14/passdetective-kali-linux/)
