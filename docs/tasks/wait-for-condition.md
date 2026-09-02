# Wait For Condition

**Class:** `USimFlowTask_WaitForCondition`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Wait For Condition**

---

## Overview / Purpose

Blocks until a [condition](../conditions.md) becomes true.

Where [Wait For Event](wait-for-event.md) reacts to a discrete moment, this task
polls a **continuous** state: "the player is holding the drill", "valve rotation is
past 90 degrees", "the score is high enough".

It can also require the condition to **hold** for a while, which is how you express
"stand still in the safe zone for three seconds".

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Condition** | Instanced [condition](../conditions.md) | *null* | **Yes** | The test to evaluate. |
| **Check Interval** | Float (s, min 0) | `0.1` | No | Seconds between evaluations. **0 evaluates every frame.** |
| **Required Hold Time** | Float (s, min 0) | `0.0` | No | The condition must stay true this long before the task succeeds. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Condition is empty** | An unset condition evaluates as **false**, so the task **waits forever**. Nothing is logged. |
| **Check Interval = 0** | Evaluates every frame. Fine for cheap conditions; wasteful for expensive ones. |
| **Required Hold Time = 0** | Succeeds the first time the condition is true. The default. |
| **Condition already true at start** | Succeeds almost immediately — after the first check, and after the hold time if set. |
| **Condition flickers true/false** | The hold accumulator **resets** whenever the condition is false. A flickering condition with a hold time may never complete. |

The empty-condition case is silent — unlike [Wait For Event](wait-for-event.md),
which logs and passes through when its tag is missing. A task that hangs with no log
output is usually this.

---

## Check Interval and Required Hold Time

They interact in a way worth understanding:

- The condition is evaluated every `Check Interval` seconds.
- While it reads **true**, hold time accumulates.
- The moment it reads **false**, the accumulated hold time **resets to zero**.

So with `Check Interval = 0.1` and `Required Hold Time = 3.0`, the condition must
read true on roughly 30 consecutive checks. A single false reading starts the count
over.

**Raise `Check Interval` for expensive conditions** — a distance test every frame is
cheap, a condition that scans actors is not. But a long interval with a short hold
time can miss brief windows entirely.

---

## Dependencies

| Depends on | Why |
|---|---|
| [Conditions](../conditions.md) | The thing being evaluated |
| [Blackboard](../blackboard.md) | Indirectly, for the compare conditions |

---

## Example use case: stand in the safe zone for three seconds

**Goal:** the trainee must reach the muster point and stay there briefly.

1. Add a [Task node](../nodes/task.md), **Task** = **Wait For Condition**.
2. Set **Condition** to **Player Near Location**:
   - Location = the muster point's world coordinates
   - Radius = `200`
   - Ignore Z = on (HMD height varies)
3. Set **Required Hold Time** to `3.0`.
4. Leave **Check Interval** at `0.1`.
5. Set **Instruction** to `Go to the muster point and wait`.
6. On the Task node, set **Time Limit** to `60` and wire `Timed Out` to a hint.

Stepping out of the radius resets the three seconds, which is the intended
behaviour — they have to actually stay.

### Variant: wait for a blackboard flag

To wait until some other system sets a flag, use **Blackboard Compare** with
Key = `DrillReady`, Operation = `==`, Value → Bool `true`. Have your Blueprint call
`Set Bool` on the flow's blackboard when ready.

---

## Common pitfalls

**The task hangs and nothing is logged.**
The `Condition` slot is empty. An unset condition is false forever, silently. This
is the most common cause.

**The hold time never completes.**
The condition is flickering. Widen the tolerance — a larger radius, `Ignore Z` on —
or lower the hold time.

**The task completes instantly.**
The condition was already true when the task started. Conditions are evaluated from
the first check, not only after a change.

**Performance drops during this task.**
`Check Interval` is `0` with an expensive condition. Raise it.

**A brief event is missed.**
`Check Interval` is too long to catch a short-lived state. Use
[Wait For Event](wait-for-event.md) for discrete moments instead — polling is the
wrong tool for an instant.

---

*See also: [Conditions](../conditions.md) · [Wait For Event](wait-for-event.md) ·
[Go To Location](go-to-location.md) · [Task Reference](README.md)*
