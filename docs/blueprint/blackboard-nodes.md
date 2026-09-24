# Blackboard nodes

**Class:** `USimFlowBlackboard`
**Get a reference:** `Get Blackboard` on the
[SimFlow Component](../simflow-component.md), on a [flow instance](instance.md), or
on a task, node or condition
**Concept page:** [Blackboard](../blackboard.md)

What the blackboard is, which keys are reserved and how coercion works are all on
the [blackboard](../blackboard.md) page. This is the node list.

Every node here needs a blackboard reference on its target pin. The usual chain:

```
Get Primary Flow ──▶ Get Blackboard ──▶ Set Int ( Attempts , 0 )
```

## Typed setters

| Node | Value pin |
|---|---|
| Set Bool (Key, Value) | bool |
| Set Int (Key, Value) | int32 |
| Set Float (Key, Value) | float |
| Set String (Key, Value) | string |
| Set Name (Key, Value) | name |
| Set Vector (Key, Value) | vector |
| Set Object (Key, Value) | object reference |

Each one writes and sets the value's type in a single node, which is why they beat
`Set Value` plus a [maker](values.md) for a literal. Writing over an existing key of
a different type just replaces it — no warning, and the type changes.

`Set Object` stores a runtime-only reference. It's cleared on save.

## Typed getters

| Node | Returns | Default pin |
|---|---|---|
| Get Bool (Key, Default Value) | bool | `false` |
| Get Int (Key, Default Value) | int32 | `0` |
| Get Float (Key, Default Value) | float | `0.0` |
| Get String (Key, Default Value) | string | empty |
| Get Name (Key, Default Value) | name | `None` |
| Get Vector (Key, Default Value) | vector | zero |
| Get Object (Key) | object | — no default pin |

The `Default Value` pin is what comes back when the key doesn't exist, which makes
these safe on a flow that just started. `Get Int("Attempts", 0)` on a fresh
blackboard returns `0` rather than failing.

`Get Object` is the exception with no default. It returns null.

A missing key never logs anything. That's the most common source of a condition
that's quietly always false — see
[when it misbehaves](../blackboard.md#when-it-misbehaves) on the concept page.

## Generic accessors

| Node | Notes |
|---|---|
| Set Value (Key, Value) | Takes a [SimFlow Value](values.md) |
| Get Value (Key) | Returns an unset value when the key is missing |
| Has Value (Key) | The only way to tell "missing" from "set to the default" |
| Remove Value (Key) | Deletes one key |
| Clear All | Empties the blackboard |
| Add To Value (Key, Delta) | Adds, creating the key if absent |

Reach for these when the type isn't known until runtime, or when you're copying
values around without caring what's in them.

`Has Value` is worth knowing about. Since every typed getter folds a missing key
into its default, `Get Int("Score", 0)` returning `0` can't tell you whether the
trainee scored nothing or the key was never written. `Has Value` can.

## Score

| Node | Notes |
|---|---|
| Add Score (Delta) | Adds to the well-known `Score` key |
| Get Score | Reads it |

Shorthand for `Add To Value` and `Get Float` against `Score`, which is the key the
built-in task scoring writes to.

## Serialisation

| Node | Notes |
|---|---|
| To Entries (Strip Object References) | Flattens to an array. Strip defaults to `true`. |
| From Entries (Entries, Clear First) | Restores. Clear First defaults to `true`. |
| Merge From (Other, Overwrite Existing) | Copies every entry from another blackboard |

`Merge From` is what [Sub Flow](../nodes/sub-flow.md) uses to hand state between
parent and child. You'd call it directly to seed a fresh flow from a previous run's
results.

## On Value Changed

An event dispatcher, not a function. It fires on every write, carrying the key and
the new value.

```
Get Blackboard ──▶ Bind Event to On Value Changed ──▶ [ if Key == Score, refresh widget ]
```

Binding this is how you drive a live score display without checking it every frame.
It fires for every key, so filter on the key pin.

## A score widget with no polling

1. On **Event Construct**, get the flow and then **Get Blackboard**.
2. **Bind Event to On Value Changed**.
3. In the bound event, branch on `Key == Score`.
4. On true, call **Get Score** and set your text.

## When it misbehaves

**A getter returns the default and the key is definitely set.** Key names are
`FName` and case-insensitive, so that isn't it — check for a trailing space, or
print the whole blackboard with `SimFlow.Debug 1`.

**Set Object survived into the next session.** It didn't. Object values are stripped
on save and read back null.

**Add To Value concatenated instead of adding.** The key holds a String. `"1"` plus
`"1"` is `"11"`. Use Int for counters.

**On Value Changed fires constantly.** It fires for every key. Filter on the key
pin.

**Changes made on a client vanish.** The blackboard replicates server to client when
`Replicate Blackboard` is on. Writes on a client are overwritten by the next update.

*Next: [Blackboard](../blackboard.md) · [Values](values.md) ·
[Conditions](../conditions.md)*
