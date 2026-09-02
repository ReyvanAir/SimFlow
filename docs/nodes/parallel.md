# Parallel node

**Class:** `USimFlowNode_Parallel`
**Add via:** right-click → **Flow Control → Parallel**
**Pins:** In → Out 0 … Out N

---

## Overview / Purpose

The Parallel node **fires every output at once**, so several sections of the graph
run simultaneously.

Use it for things that genuinely happen together: a countdown running while the
trainee works, ambient narration alongside a procedure, or two independent
sub-tasks.

Pair it with a [Join](join.md) node to converge the branches again.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Num Outputs** | Int (2–16) | `2` | Yes | How many output pins. Changing it rebuilds the pins. |

Pins are named `Out_0` … `Out_N`. Values outside 2–16 are clamped.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **No output pins are wired** | The node finishes without triggering anything — that line of execution ends. |
| **Only some pins are wired** | **Only the wired pins fire.** Unwired pins are skipped entirely, not fired into nothing. |
| **Num Outputs reduced** | The removed pins lose their links. Links on surviving pins are preserved. |
| **Branches never converge** | Each runs to its own end. If one reaches a [Finish](finish.md) with `Stop Other Branches` on, the others are killed mid-task. |

That second row is worth knowing: the node snapshots which pins actually have links
and fires only those, so an unused output costs nothing.

---

## Parallel does not mean threaded

Branches are interleaved on the game thread, tick by tick. "Parallel" here means
*several parts of the graph are active at once*, not concurrent execution. Ordering
within a single tick is not something to rely on.

---

## Dependencies

| Depends on | Why |
|---|---|
| [Join](join.md) | Only if you need the branches to converge |

---

## Example use case: a timer running alongside the work

**Goal:** the trainee performs a procedure while a 60-second countdown runs. Whoever
finishes first ends the section.

1. Right-click → **Flow Control → Parallel**. Leave **Num Outputs** at `2`.
2. Wire **Out 0** into the procedure — a [Task node](task.md) with
   [Ordered Sequence](../tasks/ordered-sequence.md).
3. Wire **Out 1** into a [Delay node](delay.md) set to `60`.
4. Add a [Join](join.md) node with **Mode** = **Wait For Any (Race)**.
5. Wire the end of the procedure into the Join's **In 0**, and the Delay's `Out`
   into **In 1**.
6. Wire the Join's `Out` onward.

```
              ┌────────────┐  Out 0 ──▶ Ordered Sequence ──┐   ┌──────────┐
   ───────────┤  Parallel  │                               ├──▶│   Join   ├──▶
              │            │  Out 1 ──▶ Delay 60s ─────────┘   │ Wait Any │
              └────────────┘                                   └──────────┘
```

Whichever branch arrives first wins; the Join swallows the loser so the downstream
section cannot run twice.

---

## Common pitfalls

**Only one branch seems to run.**
The other output pin is not wired. Unwired pins are skipped.

**A branch gets cut off mid-task.**
Another branch reached a [Finish](finish.md) node with `Stop Other Branches` on.
Converge with a [Join](join.md) first.

**The section after the branches runs twice.**
Both branches reach it independently. Put a [Join](join.md) in **Wait For All** mode
between them.

**Changing Num Outputs lost my wires.**
Reducing the count removes those pins and their links.

---

*See also: [Join node](join.md) · [Parallel Group task](../tasks/parallel-group.md) ·
[Node Reference](README.md)*
