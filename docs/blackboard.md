# Blackboard

**Class:** `USimFlowBlackboard`
**Access via:** `Get Blackboard` on the [SimFlow Component](simflow-component.md), a task, a node or a condition

---

## Overview / Purpose

The blackboard is the **per-flow key/value store**. It is how a flow remembers
things between tasks: score, quiz answers, which object the trainee picked up,
how many times they got something wrong.

One blackboard exists per running [flow instance](glossary.md). Two actors running
the same flow asset have separate blackboards and cannot see each other's values.

Everything [conditions](conditions.md) and [Branch](nodes/branch.md) nodes read
comes from here, which makes the blackboard the join between "what happened" and
"what the flow does next".

---

## Values and types

A blackboard value is an `FSimFlowValue` — a small tagged union with a `Type` and
one payload field. Set the **Type** first in the Details panel and the matching
value field appears.

| Type | Stores | Survives save/load? |
|---|---|---|
| **None** | nothing (an unset value) | — |
| **Bool** | true/false | Yes |
| **Int** | int32 | Yes |
| **Float** | float | Yes |
| **String** | text | Yes |
| **Name** | FName | Yes |
| **Vector** | FVector | Yes |
| **Object** | a UObject reference | **No — cleared on save** |

> **Object values are runtime only.** They are stripped when a flow is serialised
> into a SaveGame, because a hard object pointer cannot be meaningfully restored.
> If you need something about an object to survive a save, store its display name
> as a String alongside it.

### Type coercion

Values compare and convert on a best-effort basis rather than failing:

- **As Number** — bool becomes 0/1; a String is parsed with `Atof`.
- **As Bool** — non-zero numbers are true.
- **Add** — adds where it makes sense: numbers add, strings concatenate, vectors
  add component-wise.

This is why `Add To Value` works on a key that does not exist yet: the missing key
reads as unset, and the delta becomes the new value.

---

## API

### Generic

| Function | Notes |
|---|---|
| **Set Value** (Key, Value) | Writes, replacing whatever was there |
| **Get Value** (Key) | Returns an unset value when the key is missing |
| **Has Value** (Key) | Does the key exist? |
| **Remove Value** (Key) | Deletes one key |
| **Clear All** | Empties the blackboard |
| **Add To Value** (Key, Delta) | Adds to the existing value; **creates the key if missing** |

### Typed convenience

`Set Bool / Int / Float / String / Name / Vector / Object`, and
`Get Bool / Int / Float / String / Name / Vector / Object`.

The getters take a **Default Value** used when the key is absent — so
`Get Int("Attempts", 0)` is safe on a fresh flow. `Get Object` has no default and
returns null.

### Score

| Function | Notes |
|---|---|
| **Add Score** (Delta) | Adds to the well-known `Score` key |
| **Get Score** | Reads it |

### Serialisation

| Function | Notes |
|---|---|
| **To Entries** (bStripObjectReferences = true) | Flattens to an array for saving |
| **From Entries** (Entries, bClearFirst = true) | Restores |
| **Merge From** (Other, bOverwriteExisting = true) | Copies every entry from another blackboard — this is what [Sub Flow](nodes/sub-flow.md) uses |

### Events

**On Value Changed** (Key, New Value) fires on **every** write. Bind it to drive a
live score widget without polling.

---

## Well-known keys

The built-in tasks read and write these. Treat the names as reserved.

| Key | Type | Written by |
|---|---|---|
| `Score` | Float | Task success/failure scoring, `Add Score` |
| `Mistakes` | Int | [Quiz](tasks/quiz.md) when `Count Mistakes` is on |
| `LastResult` | — | The runtime, after each task finishes |
| `LastAnswerIndex` | Int | [Quiz](tasks/quiz.md) |
| `LastAnswerCorrect` | Bool | [Quiz](tasks/quiz.md) |
| `WrongAttempts` | Int | Incremented every time a task rejects the wrong object |
| `CurrentStep` | Int | [Ordered Sequence](tasks/ordered-sequence.md) |

> Keys are `FName` and match **case-insensitively**, like all Unreal names. `Score`
> and `score` are the same key. Pick a convention anyway.

---

## Behaviour when misconfigured

| Situation | What happens |
|---|---|
| **Key is `None`** on a Set node/task | The write still happens, under the literal name `None`. It is not an error, and it is almost never what you meant. |
| **Reading a missing key** | Returns the supplied default (or an unset value for `Get Value`). No warning. |
| **Comparing a missing key** in a condition | [Blackboard Compare](conditions.md#blackboard-compare) returns its `Result When Key Missing` setting, default `false`. |
| **Type mismatch** on compare | Values coerce rather than fail — a String `"5"` compares equal to Int `5`. |
| **Object value after a load** | Null. Object references are stripped on save. |

The silent-default behaviour is the thing to watch: a typo in a key name does not
produce an error anywhere. It produces a condition that is quietly always false.

---

## Dependencies

| Depends on | Why |
|---|---|
| A running [flow instance](glossary.md) | The blackboard belongs to the instance, not the asset |

**Depended on by:** [Conditions](conditions.md), [Branch](nodes/branch.md),
[Loop](nodes/loop.md), [Set Blackboard node](nodes/set-blackboard.md) and
[task](tasks/set-blackboard.md), [Actor Query](actor-query.md) (its `Blackboard Key`
field), [Sub Flow](nodes/sub-flow.md), save/load.

---

## Example use case: a three-strikes rule

**Goal:** after three wrong attempts, route the trainee to a remediation branch.

1. The built-in tasks already increment `WrongAttempts` whenever they reject a
   wrong object — you do not have to write it yourself.
2. After the task, add a **Branch** node ([Branch](nodes/branch.md)).
3. Add one case. Set its **Condition** to **Blackboard Compare**:
   - Key = `WrongAttempts`
   - Operation = `>=`
   - Value → Type = `Int`, Int Value = `3`
4. Label the case `Too many attempts` and wire it to your remediation section.
5. Wire the **Default** pin to the normal continuation.
6. To reset the count for the next exercise, drop a
   [Set Blackboard Value](nodes/set-blackboard.md) node with Key = `WrongAttempts`,
   Type = `Int`, Value = `0`, `Add` off.

Watch it work with `SimFlow.Debug 1` — the debug HUD prints the whole blackboard.

---

## Common pitfalls

**A condition is always false and nothing is logged.**
The key name does not match what wrote it. Missing keys read as defaults silently.
Print the blackboard with `SimFlow.Debug 1` and compare the spelling.

**A value survives into the next run.**
The blackboard belongs to the instance. Restarting the flow makes a fresh one — but
`Merge From` in a [Sub Flow](nodes/sub-flow.md) with write-back on can push child
values back into the parent. That is intended; turn write-back off if you do not
want it.

**An object reference is null after loading a save.**
Expected — object values are stripped on save. Store an identifying String too.

**`Add To Value` on a String did something surprising.**
It concatenates. Adding `"1"` to `"1"` gives `"11"`, not `2`. Use the Int type for
counters.

**Score is not changing.**
`Score On Success` / `Score On Failure` are per-task fields and default to `0`. See
[Task base fields](tasks/README.md#fields-every-task-has).

---

*See also: [Conditions](conditions.md) · [Set Blackboard node](nodes/set-blackboard.md) ·
[SimFlow Component](simflow-component.md) · [Documentation index](README.md)*
