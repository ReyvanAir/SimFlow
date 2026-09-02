# Random Branch node

**Class:** `USimFlowNode_RandomBranch`
**Add via:** right-click → **Flow Control → Random Branch**
**Pins:** In → Out 0 … Out N

---

## Overview / Purpose

The Random Branch node **picks one output at random**, optionally weighted.

Use it to vary a scenario between runs: which fault occurs, which room the task
happens in, which distractor appears. Repeat trainees then cannot simply memorise
the sequence.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Num Outputs** | Int (2–16) | `2` | Yes | How many output pins. |
| **Weights** | Array of Float | *empty* | No | Per-output weights. **Missing entries count as 1.** |
| **Avoid Repeats** | Bool | `false` | No | Never pick the same output twice in a row within one run. |

Pins are named `Out_0` … `Out_N`. Values outside 2–16 are clamped.

---

## How the pick is made

1. Each output's weight is taken from `Weights` by index, or **1** if absent.
   Negative weights are clamped to 0.
2. If `Avoid Repeats` is on, the previously picked output's weight is forced to 0.
3. A weighted roll picks one output.
4. **If the total weight is 0**, the node falls back to an unweighted uniform pick
   across all outputs.

That last step is a safety net: setting every weight to 0 does not break the node,
it just makes the choice uniform.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Weights empty** | Every output weighted 1 — a uniform random pick. The normal case. |
| **Weights shorter than Num Outputs** | The listed ones apply; the rest default to 1. |
| **Weights longer than Num Outputs** | Extra entries are ignored. |
| **A weight of 0** | That output is never picked (unless *all* are 0). |
| **All weights 0** | Falls back to a uniform pick. |
| **`Avoid Repeats` on with 2 outputs** | Strictly alternates — it is the only remaining option each time. |
| **The chosen pin is unwired** | Execution ends there. With random selection this shows up intermittently, which makes it confusing to debug. |

**Wire every output.** An unwired pin on a random branch produces a bug that only
appears some of the time.

---

## Avoid Repeats is per-run

The "last picked" memory lives on the node instance, which is created when the flow
starts. Restarting the flow forgets it, so a fresh run can repeat the previous run's
choice.

---

## Dependencies

None.

---

## Example use case: three fault scenarios, one rare

**Goal:** each run presents one of three faults, with the rare one appearing about a
tenth of the time.

1. Right-click → **Flow Control → Random Branch**.
2. Set **Num Outputs** to `3`.
3. Under **Weights**, add three entries: `45`, `45`, `10`.
4. Tick **Avoid Repeats** so the same fault does not come up twice running.
5. Wire each output to its scenario, and converge them with a [Join](join.md) if
   they share an ending.

```
                    ┌──────────────────┐
   ─────────────────┤In  Random Branch │Out 0 ──▶ pressure fault   (45%)
                    │    45 / 45 / 10  │Out 1 ──▶ valve fault      (45%)
                    │    avoid repeats │Out 2 ──▶ sensor fault     (10%)
                    └──────────────────┘
```

Weights are relative, not percentages — `45/45/10` and `9/9/2` behave identically.

---

## Common pitfalls

**One branch never happens.**
Its weight is 0, or `Weights` has fewer entries than you thought and the indexing is
off by one. Weights are indexed from 0.

**The flow sometimes just stops.**
An output pin is unwired and was picked. Wire all of them.

**Avoid Repeats is not preventing a repeat across runs.**
It is per-instance and resets when the flow restarts. Persist the last choice in the
[blackboard](../blackboard.md) and use a [Branch](branch.md) if you need it to
survive a restart.

**Weights are being read as percentages.**
They are relative weights. They do not need to sum to 100.

---

*See also: [Branch node](branch.md) · [Join node](join.md) ·
[Node Reference](README.md)*
