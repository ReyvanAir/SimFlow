# Delay node

**Class:** `USimFlowNode_Delay`
**Add via:** right-click → **Flow Control → Delay**
**Pins:** In → Out

---

## Overview / Purpose

The Delay node **waits, then continues**. It respects pause — a paused flow does not
advance the timer.

Use it for pacing: a beat before narration, a pause between stages, or as the timer
branch of a race (see [Join](join.md)).

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Duration** | Float (s, min 0) | `1.0` | Yes | How long to wait. |

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Duration = 0** | Completes on the next tick — effectively a one-frame pass-through. |
| **The flow is paused** | The timer stops. It resumes where it left off. |
| **`Out` unwired** | The delay runs and then execution ends there. |

---

## Node or task?

There is also a [Delay **task**](../tasks/delay.md). They wait the same way.

| Use the **node** | Use the **task** |
|---|---|
| A plain pause in the graph — **the usual choice** | You want a random extra amount on top (`Random Extra`) |
| You want the graph to stay readable | You want it inside a [Parallel Group](../tasks/parallel-group.md) |
| | You want a timeout or abort condition around it via the [Task node](task.md) |

The node is lighter and reads better in a graph. Reach for the task only when you
need one of the things in the right-hand column.

---

## Dependencies

None.

---

## Example use case: a beat before the next instruction

**Goal:** let a narration line finish before the next task appears.

1. Right-click → **Flow Control → Delay**.
2. Set **Duration** to `2.5`.
3. Wire it between the narration task and the next [Task node](task.md).

```
   Task (narration) ──▶ Delay 2.5s ──▶ Task (next step)
```

Because the delay respects pause, a trainee who pauses mid-narration does not lose
the beat.

---

## Common pitfalls

**The delay seems to take longer than set.**
The flow was paused, or the game itself was paused and the component's
`Follow Game Pause` is on (the default). See
[SimFlow Component](../simflow-component.md).

**The flow stops after the delay.**
`Out` is unwired.

**I need a timeout, not a pause.**
Use the [Task node](task.md)'s `Time Limit`, or race a Delay against the work with a
[Join](join.md) in **Wait For Any** mode.

---

*See also: [Delay task](../tasks/delay.md) · [Join node](join.md) ·
[Node Reference](README.md)*
