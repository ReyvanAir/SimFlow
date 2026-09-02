# Loop node

**Class:** `USimFlowNode_Loop`
**Add via:** right-click → **Flow Control → Loop**
**Pins:** In, Continue → Loop Body, Completed

---

## Overview / Purpose

The Loop node **repeats a section of the graph** — a fixed number of times, or until
a condition passes.

It is the one node whose wiring is not obvious, because it needs a **return wire**:

- **Loop Body** goes *out* to the section you want to repeat.
- The end of that section comes *back* into **Continue**.

Without the return wire the body runs exactly once and the flow stops.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Iterations** | Int (min 0) | `3` | Yes | How many times to run the body. **0 means "until the break condition passes"**. |
| **Break Condition** | Instanced [condition](../conditions.md) | *null* | No | Evaluated **before each iteration**. When it passes, the loop exits via `Completed`. |
| **Max Iterations** | Int (min 1) | `1000` | No | *Advanced.* Safety valve so an infinite loop cannot hang the game. |
| **Iteration Blackboard Key** | Name | `None` | No | *Advanced.* Writes the current **0-based** iteration into this [blackboard](../blackboard.md) key. |

---

## Pins

| Pin | Direction | Meaning |
|---|---|---|
| **In** | Input | Entry. **Resets the iteration counter to 0.** |
| **Continue** | Input | The body returns here to request the next iteration. |
| **Loop Body** | Output | Fires once per iteration. |
| **Completed** | Output | Fires when the loop is done. |

The node **stays active** while looping, and is re-entrant — arriving at `Continue`
while it is active is the normal path, not an error.

---

## How an iteration is decided

Each time `In` or `Continue` fires, in this order:

1. `In` sets the counter to 0; `Continue` increments it.
2. **Max Iterations** reached? → log a warning and exit via `Completed`.
3. **Iterations > 0** and counter has reached it? → exit via `Completed`.
4. **Break Condition** set and passing? → exit via `Completed`.
5. Otherwise write the iteration key if set, and fire **Loop Body**.

Note the order: the break condition is checked **before** the body runs, so a
condition that is already true means the body never runs at all.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **`Continue` is not wired back** | The body runs **once**, then execution ends there. The most common Loop mistake. |
| **Iterations = 0, no Break Condition** | Loops until **Max Iterations** (1000), then logs *"hit MaxIterations"* and exits. |
| **Iterations = 0 with a Break Condition** | The intended "loop until" form. |
| **Break Condition already true on entry** | The body never runs; the loop exits immediately via `Completed`. |
| **`Loop Body` unwired** | Fires into nothing; the loop cannot progress and stalls. |
| **The body never returns to `Continue`** (e.g. a task fails down an unwired pin) | The loop stalls silently. |

---

## Dependencies

| Depends on | Why |
|---|---|
| [Conditions](../conditions.md) | Only for `Break Condition` |
| [Blackboard](../blackboard.md) | Only for `Iteration Blackboard Key` |

---

## Example use case: three attempts, or until they get it right

**Goal:** let the trainee retry a placement up to three times, stopping early on
success.

1. Right-click → **Flow Control → Loop**. Set **Iterations** to `3`.
2. Set **Break Condition** to **Blackboard Compare**: Key = `Placed`,
   Operation = `==`, Value → Type `Bool`, `true`.
3. Set **Iteration Blackboard Key** to `Attempt` so a widget can show "Attempt 2 of 3".
4. Wire **Loop Body** into a [Task node](task.md) running
   [Place Object In Zone](../tasks/place-object-in-zone.md).
5. After that task, add a [Set Blackboard Value](set-blackboard.md) node writing
   `Placed = true`.
6. Wire that node's `Out` **back into the Loop node's `Continue` pin**.
7. Wire the Loop's **Completed** onward.

```
        ┌──────────────┐
  ──────┤In    Loop    │Loop Body──▶ Task (Place Object) ──▶ Set "Placed"
   ┌───▶┤Continue      │Completed──▶ next section              │
   │    └──────────────┘                                       │
   └───────────────────────────────────────────────────────────┘
```

Because the break condition is checked *before* each iteration, a successful first
attempt exits after one pass.

---

## Common pitfalls

**The body runs once and the flow stops.**
`Continue` is not wired back. This is the classic Loop mistake.

**The log says "hit MaxIterations".**
`Iterations` is 0 and the break condition never passes — so it ran 1000 times. Fix
the condition, or set a real iteration count.

**The body never runs.**
The break condition is already true when the loop is entered.

**The loop stalls partway.**
Something in the body ended without returning to `Continue` — often a task leaving
through an unwired `Failed` pin. Wire the failure paths back to `Continue` too, or
turn on the Task node's `Fallback To Completed`.

**The iteration key is off by one in the UI.**
It is **0-based**. Add 1 for display.

---

*See also: [Task node](task.md) · [Conditions](../conditions.md) ·
[Join node](join.md) · [Node Reference](README.md)*
