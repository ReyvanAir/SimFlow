# Log Message

**Class:** `USimFlowTask_Log`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Log Message**

---

## Overview / Purpose

Prints a message, then succeeds immediately.

Its real value is **blocking out a flow before the real tasks exist**. Fill a graph
with Log Message tasks, run it end to end, confirm the branching and routing are
right, then replace them one at a time with real work.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Message** | String | `SimFlow` | Yes | The text to print. |
| **Print To Screen** | Bool | `true` | No | Also print on screen, not just to the log. |
| **Screen Duration** | Float (s, min 0) | `3.0` | No | How long the on-screen message stays. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Message empty** | An empty line is printed. Harmless, but useless. |
| **Print To Screen off** | Goes to the output log only — check the Output Log window. |
| **Screen Duration = 0** | The on-screen message appears and disappears the same frame, i.e. invisibly. |

The task always finishes with `Succeeded` on the frame it starts. It never fails and
never blocks.

---

## Dependencies

None.

---

## Example use case: block out a flow before building it

**Goal:** prove the shape of a five-stage exercise before writing any real tasks.

1. Create the flow with [Start](../nodes/start.md), five
   [Task nodes](../nodes/task.md), the [Branch](../nodes/branch.md) you plan to use,
   and a [Finish](../nodes/finish.md).
2. Set every Task node's **Task** to **Log Message**, with messages
   `Stage 1`, `Stage 2`, …
3. Press Play and watch them print in order.
4. Confirm the branching works by seeding the blackboard with a
   [Set Blackboard Value](../nodes/set-blackboard.md) node.
5. Replace each Log Message with the real task, one at a time.

Because an empty [Task node](../nodes/task.md) also passes straight through, you can
even leave the `Task` slot empty — but a Log Message tells you *which* node you are
passing through, which is the point.

---

## Common pitfalls

**Nothing appears on screen.**
`Print To Screen` is off, or `Screen Duration` is `0`. Check the Output Log.

**The messages appear all at once.**
They are meant to. Log Message finishes on the frame it starts, so a chain of them
runs in a single tick. Insert [Delay nodes](../nodes/delay.md) if you want to watch
the flow progress at a readable pace.

**I want the message to appear during a wait, not instead of it.**
Set the real task's **Instruction** field instead — that is what tutorial UI reads.
See [Task fields](README.md#fields-every-task-has).

---

*See also: [Task node](../nodes/task.md) · [Delay node](../nodes/delay.md) ·
[Troubleshooting](../troubleshooting.md) · [Task Reference](README.md)*
