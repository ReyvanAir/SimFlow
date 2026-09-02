# Conditions

**Base class:** `USimFlowCondition`
**Used by:** [Branch](nodes/branch.md) cases, [Loop](nodes/loop.md) break conditions, [Task node](nodes/task.md) abort conditions, [Wait For Condition](tasks/wait-for-condition.md)

---

## Overview / Purpose

A Condition is a reusable **true/false test about the current run**.

Conditions are *instanced sub-objects*: you do not create a condition asset and
reference it. You pick a condition class straight from a dropdown in the Details
panel and configure it inline, wherever a condition is asked for.

---

## The field every condition has

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Invert** | Bool | `false` | Flips the result, so you rarely need a NOT wrapper. |

`Invert` is applied *after* the condition evaluates, including for the AND/OR
composites — so an inverted **All Of** is "not all of them", not "none of them".

---

## Built-in conditions

### Blackboard Compare

Compares a [blackboard](blackboard.md) key against a literal value. The workhorse.

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Key** | Name | `None` | Which key to read |
| **Operation** | Enum | `==` | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| **Value** | SimFlow Value | Type `None` | The literal to compare against |
| **Result When Key Missing** | Bool | `false` | *Advanced.* What to return when the key does not exist |

**When the key is missing** the comparison is not attempted at all — the condition
returns `Result When Key Missing`. This is the setting to change when a check should
pass on the first run, before anything has written the key.

Values coerce rather than fail: a String `"5"` compares equal to an Int `5`. See
[Blackboard · Type coercion](blackboard.md#type-coercion).

### Score Threshold

Shorthand for a compare against the well-known `Score` key.

| Field | Type | Default |
|---|---|---|
| **Operation** | Enum | `>=` |
| **Threshold** | Float | `100.0` |

### Last Task Result Is

True when the most recently finished task ended with the given result.

| Field | Type | Default |
|---|---|---|
| **Expected Result** | Enum | `Succeeded` |

Options are `Succeeded`, `Failed`, `Skipped`, `Timed Out`, `Aborted`. Useful on a
Branch immediately after a Task node when you want one branch to handle several
failure modes together.

### Elapsed Time

True when the flow has been running longer / shorter than a given time.

| Field | Type | Default |
|---|---|---|
| **Operation** | Enum | `>=` |
| **Seconds** | Float (s, min 0) | `60.0` |

Measures **whole-flow** elapsed time, not time on the current task. For a per-task
limit use the Task node's `Time Limit` instead — see [Task node](nodes/task.md).

### Event Was Raised

True when an event tag has been raised on this flow at any point in the run.

| Field | Type | Default |
|---|---|---|
| **Event Tag** | Gameplay Tag | *empty* |

This is a *has it ever happened* test against the run's event history, not a live
wait. To block until an event arrives, use
[Wait For Event](tasks/wait-for-event.md).

### All Of (AND) / Any Of (OR)

Logical composites over a list of child conditions.

| Field | Type | Default |
|---|---|---|
| **Conditions** | Array of instanced conditions | *empty* |

Add children with the **+** button; each gets its own class dropdown, so they nest
arbitrarily deep.

> **An empty list is a trap.** Standard logic makes an empty AND true and an empty
> OR false. If a composite is behaving oddly, check that you actually added children
> — an empty **All Of** will pass every time.

### Constant

Always returns the configured value. Useful as a placeholder while blocking out a
flow, and as an explicit "always take this branch" case.

| Field | Type | Default |
|---|---|---|
| **Value** | Bool | `true` |

### Player Near Location

True when the player pawn is within a radius of a world location. Very common in VR
flows.

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Location** | Vector | `0,0,0` | The world location |
| **Radius** | Float (cm, min 1) | `150.0` | How close counts |
| **Location From Blackboard Key** | Name | `None` | *Advanced.* When set, uses this key's vector instead of `Location` |
| **Ignore Z** | Bool | `true` | *Advanced.* Ignore the vertical axis — useful because HMD height varies |

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **A Branch case has no condition** | Treated as **false** — that case never fires |
| **Blackboard Compare with Key = None** | The key `None` is looked up, almost certainly missing → returns `Result When Key Missing` |
| **Event Was Raised with no tag** | An invalid tag was never raised → false |
| **All Of with no children** | **True** (vacuous AND) |
| **Any Of with no children** | **False** (vacuous OR) |
| **Player Near Location with no player pawn** | False |

The first row is the one that bites: a Branch case you added but never configured
silently never fires, and execution falls through to `Default`.

---

## Writing your own condition

Conditions are `Blueprintable`.

1. Content Browser → **Blueprint Class** → search for and pick **SimFlow Condition**.
2. Open it and override the **Evaluate** event.
3. Return your answer with the return node.
4. Optionally override **Get Condition Description** to return a short string —
   it is shown on graph nodes and in the debug HUD, which makes a graph far easier
   to read at a glance.

Inside the Blueprint, `Cached Instance` gives you the running
[flow instance](glossary.md) for world context and blackboard access, and
`Get Blackboard From` is a convenience accessor.

Your new class then appears in every condition dropdown automatically.

---

## Dependencies

| Depends on | Why |
|---|---|
| A running [flow instance](glossary.md) | Passed in on every evaluation |
| [Blackboard](blackboard.md) | For the compare / score conditions |

---

## Example use case: pass or remediate

**Goal:** at the end of an assessment, send trainees who scored under 70 to a
remediation section.

1. Add a **Branch** node after the last task.
2. Add one case, labelled `Needs remediation`.
3. Set its **Condition** to **Score Threshold**, Operation = `<`, Threshold = `70`.
4. Wire that case's pin to the remediation section.
5. Wire **Default** to the pass section.

To require *both* a score and no more than two mistakes, use **All Of** instead:

- Child 1: **Score Threshold**, `>=`, `70`
- Child 2: **Blackboard Compare**, Key = `Mistakes`, `<=`, Int `2`

and put that on the *pass* case rather than the remediation one.

---

## Common pitfalls

**A branch case never fires.**
Its condition slot is empty. An unset condition is false.

**An All Of composite always passes.**
It has no children. An empty AND is true.

**A comparison never becomes true on the first run.**
The key does not exist yet, so the condition returns `Result When Key Missing`
(default `false`). Either seed the key with a
[Set Blackboard Value](nodes/set-blackboard.md) node at the start of the flow, or
flip that setting.

**Elapsed Time fires far earlier than expected.**
It measures the whole flow's runtime, not the current task's.

**A condition on a Task node's abort slot is firing constantly.**
Abort conditions are evaluated **every frame** while the task runs. Make sure the
test is not true at the moment the task starts. See [Task node](nodes/task.md).

---

*See also: [Branch node](nodes/branch.md) · [Blackboard](blackboard.md) ·
[Wait For Condition](tasks/wait-for-condition.md) · [Documentation index](README.md)*
