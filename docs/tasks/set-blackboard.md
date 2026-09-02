# Set Blackboard Value (task)

**Class:** `USimFlowTask_SetBlackboard`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Set Blackboard Value**

---

## Overview / Purpose

Writes (or adds to) a [blackboard](../blackboard.md) key, then finishes immediately.

There is also a [Set Blackboard Value **node**](../nodes/set-blackboard.md) with
identical fields, and for a simple write in the graph the node is the better choice.
Use the task form when you need it **inside a
[Parallel Group](parallel-group.md)**, or when you want a `Task Id` or scoring
attached to the write.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Key** | Name | `None` | **Yes** | Which blackboard key to write. |
| **Value** | SimFlow Value | Type `None` | **Yes** | Set the **Type** first; the matching value field then appears. |
| **Add** | Bool | `false` | No | Add to the existing value instead of replacing it. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Key is `None`** | The write happens under the literal key name `None`. No warning. Almost never intended. |
| **Value Type is `None`** | An unset value is stored. Conditions comparing against it will not behave usefully. |
| **`Add` on a missing key** | The key is created with the delta as its value. |
| **`Add` on a String** | **Concatenates.** Adding `"1"` to `"1"` gives `"11"`. Use `Int` for counters. |
| **`Add` with mismatched types** | Coerced best-effort rather than failing. |

Both empty cases are silent. A key-name typo produces a condition that is quietly
always false — see [Blackboard · pitfalls](../blackboard.md#common-pitfalls).

---

## Dependencies

| Depends on | Why |
|---|---|
| [Blackboard](../blackboard.md) | The thing being written |

---

## Example use case: score a step inside a parallel group

**Goal:** while the trainee performs a check, record that the briefing was
delivered — as part of the same group, so it cannot be skipped separately.

1. Add a [Task node](../nodes/task.md), **Task** =
   [Parallel Group](parallel-group.md).
2. **Child 0** = the real check task.
3. **Child 1** = **Set Blackboard Value**:
   - Key = `BriefingDelivered`
   - Value → Type = `Bool`, Bool Value = `true`
   - Add = off
4. Leave the group's **Wait For All** on.

A later [Branch](../nodes/branch.md) can now test `BriefingDelivered` with
**Blackboard Compare**.

For a plain write between two steps in the graph, use the
[node form](../nodes/set-blackboard.md) instead — it reads better and costs less.

---

## Common pitfalls

**A later condition never sees the value.**
The key names differ. Names are case-insensitive but spelling is not forgiven. Print
the blackboard with `SimFlow.Debug 1`.

**A counter became a string like "111".**
The value type is `String` with `Add` on, so it concatenated. Switch to `Int`.

**The value is gone after loading a save.**
The type is `Object`. Object references are stripped on save. See
[Blackboard](../blackboard.md#values-and-types).

**I used a whole Task node just to set a key.**
Use the [node form](../nodes/set-blackboard.md).

---

*See also: [Set Blackboard Value node](../nodes/set-blackboard.md) ·
[Blackboard](../blackboard.md) · [Parallel Group](parallel-group.md) ·
[Task Reference](README.md)*
