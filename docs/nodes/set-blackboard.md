# Set Blackboard Value node

**Class:** `USimFlowNode_SetBlackboard`
**Add via:** right-click → **Data → Set Blackboard Value**
**Pins:** In → Out

---

## Overview / Purpose

Writes a [blackboard](../blackboard.md) key **inline in the graph**, with no task
object needed. Execution passes straight through.

Use it to seed a key before a [Branch](branch.md) reads it, to reset a counter
between attempts, or to record which path was taken.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Key** | Name | `None` | **Yes** | Which blackboard key to write. |
| **Value** | SimFlow Value | Type `None` | **Yes** | Set the **Type** first; the matching value field then appears. |
| **Add** | Bool | `false` | No | Add to the existing value instead of replacing it. |

### Value types

`Bool`, `Int`, `Float`, `String`, `Name`, `Vector`, `Object`. See
[Blackboard · Values and types](../blackboard.md#values-and-types).

---

## Set vs. Add

| Add | Behaviour |
|---|---|
| **off** | Replaces whatever was stored. |
| **on** | Adds to it — numbers add, strings **concatenate**, vectors add component-wise. **Creates the key if it does not exist.** |

`Add` on a String concatenates rather than summing: adding `"1"` to `"1"` gives
`"11"`. Use `Int` for counters.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Key is `None`** | The write still happens, under the literal key name `None`. No warning. Almost never what you meant. |
| **Value Type is `None`** | An unset value is written. Conditions comparing against it will not behave usefully. |
| **`Add` on a missing key** | The key is created with the delta as its value. |
| **`Add` with mismatched types** | Coerced on a best-effort basis rather than failing. |
| **`Out` unwired** | The write happens, then execution ends there. |

Both of the first two cases are silent. A key-name typo produces a condition that is
quietly always false — see [Blackboard · pitfalls](../blackboard.md#common-pitfalls).

---

## Node or task?

There is also a [Set Blackboard Value **task**](../tasks/set-blackboard.md) with
identical fields.

| Use the **node** | Use the **task** |
|---|---|
| A simple write in the graph — **the usual choice** | You want it inside a [Parallel Group](../tasks/parallel-group.md) |
| You want the graph readable | You want scoring or a `Task Id` attached to the write |

---

## Dependencies

| Depends on | Why |
|---|---|
| [Blackboard](../blackboard.md) | The thing being written |

---

## Example use case: seed a key, then branch on it

**Goal:** a [Branch](branch.md) checks `Attempts` at the top of the flow, before
anything has written it.

1. Right after [Start](start.md), right-click → **Data → Set Blackboard Value**.
2. Set **Key** to `Attempts`, **Value → Type** to `Int`, **Int Value** to `0`,
   **Add** off.
3. Later, after each failed attempt, add another Set Blackboard Value node with the
   same key, `Int Value` = `1`, and **Add on**.
4. The Branch can now compare `Attempts` reliably — the key always exists.

```
   Start ──▶ Set "Attempts" = 0 ──▶ … ──▶ Set "Attempts" += 1 ──▶ Branch
```

Seeding like this avoids relying on **Blackboard Compare**'s
`Result When Key Missing` fallback, which is easy to forget about.

---

## Common pitfalls

**A later condition never sees the value.**
The key names differ. Names are case-insensitive but spelling is not forgiven.
Print the blackboard with `SimFlow.Debug 1`.

**A counter jumps to a string like "111".**
The value type is `String` and `Add` is on, so it concatenated. Switch to `Int`.

**The value is gone after loading a save.**
The type is `Object`. Object references are stripped on save.

**Add did nothing on the first run.**
It should create the key. If the value looks wrong, check the `Type` — adding an
`Int` to a key holding a `Float` coerces.

---

*See also: [Blackboard](../blackboard.md) ·
[Set Blackboard Value task](../tasks/set-blackboard.md) ·
[Branch node](branch.md) · [Node Reference](README.md)*
