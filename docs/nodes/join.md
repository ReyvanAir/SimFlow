# Join node

**Class:** `USimFlowNode_Join`
**Add via:** right-click → **Flow Control → Join**
**Pins:** In 0 … In N → Out

---

## Overview / Purpose

The Join node **converges parallel branches**. It is the partner to
[Parallel](parallel.md), and answers "what should happen when several things are
running and I need one continuation?"

Two modes, and they solve quite different problems:

| Mode | Fires | Use for |
|---|---|---|
| **Wait For All** | Once every connected input has been triggered | "Finish both sub-tasks, then continue" |
| **Wait For Any (Race)** | On the **first** input, then ignores the rest | Timeouts and races |

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Num Inputs** | Int (2–16) | `2` | Yes | How many input pins. Changing it rebuilds the pins. |
| **Mode** | Enum | `Wait For All` | Yes | See above. |

Pins are named `In_0` … `In_N`. Values outside 2–16 are clamped.

---

## How each mode behaves

### Wait For All

Each arriving input is recorded. Once the number of **distinct** inputs received
reaches `Num Inputs`, the node fires `Out` and deactivates — which resets its
counters, ready for the next pass. That reset is what lets a Join sit inside a
[Loop](loop.md).

Inputs are recorded **uniquely**: the same pin firing twice counts once. Two arrivals
on `In_0` will not satisfy a two-input join.

### Wait For Any (Race)

The first input to arrive fires `Out` immediately. The node then **stays active on
purpose** so it can absorb the losing branch when it eventually arrives — otherwise
the downstream section would be triggered a second time.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Num Inputs is higher than the number of wired pins** | In **Wait For All**, the join can never be satisfied — **the flow stalls silently.** This is the main Join pitfall. |
| **The same pin fires twice (Wait For All)** | Counts once. The join still waits for the others. |
| **A branch never arrives (Wait For All)** | The join waits forever. |
| **Late arrivals in Wait For Any** | Swallowed deliberately, so the continuation runs exactly once. |
| **`Out` unwired** | The join fires into nothing. |

**Match `Num Inputs` to the number of branches you actually wired.** If you wire two
branches but leave `Num Inputs` at 3, nothing downstream will ever run.

---

## Dependencies

| Depends on | Why |
|---|---|
| [Parallel](parallel.md) or a [Branch](branch.md) with `Fire All Matching Cases` | Something has to produce the branches |

---

## Example use case A: wait for both

**Goal:** the trainee must both don a helmet and sign the permit before proceeding,
in either order.

1. Add a [Parallel](parallel.md) node, `Num Outputs` = 2.
2. Wire each output into its own [Task node](task.md).
3. Add a **Join**, `Num Inputs` = 2, **Mode** = **Wait For All**.
4. Wire each task's `Completed` into `In 0` and `In 1`.
5. Wire `Out` onward.

## Example use case B: a race against the clock

**Goal:** the trainee has 60 seconds; either they finish or the time runs out.

1. Add a [Parallel](parallel.md) node.
2. `Out 0` → the real task. `Out 1` → a [Delay node](delay.md) of `60`.
3. Add a **Join**, **Mode** = **Wait For Any (Race)**.
4. Wire both into it, and `Out` onward.

To know *which* branch won, have each branch write a
[Set Blackboard Value](set-blackboard.md) before the join (`Outcome = "finished"` /
`"timeout"`), then read it with a [Branch](branch.md) after.

```
   Task ──────────┐   ┌──────────────┐
                  ├──▶│ Join         ├──▶ Branch on "Outcome"
   Delay 60s ─────┘   │ Wait For Any │
                      └──────────────┘
```

---

## Common pitfalls

**The flow stalls at the join and nothing is logged.**
`Num Inputs` is higher than the number of branches that actually arrive. This is
silent by design — check the count first.

**The section after the join runs twice.**
You used **Wait For All** with only one branch actually firing twice, or wired two
branches straight into the continuation without a join at all. In race situations
use **Wait For Any**, which absorbs the loser.

**A join inside a loop only works on the first pass.**
That should work — the node resets when it fires. If it does not, check that the
join is genuinely being re-entered rather than bypassed on later iterations.

**Reducing Num Inputs lost my wires.**
Removed pins lose their links.

---

*See also: [Parallel node](parallel.md) · [Loop node](loop.md) ·
[Branch node](branch.md) · [Node Reference](README.md)*
