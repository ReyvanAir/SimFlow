# Values

**Library:** `USimFlowStatics`
**Needs a target?** No.
**See also:** [values and types](../blackboard.md#values-and-types) for what a
SimFlow Value is

A `SimFlow Value` is the tagged union every [blackboard](../blackboard.md) key
holds. Anywhere a node asks for one — `Set Value`, `Add To Value`, a
[Blackboard Compare](../conditions.md#blackboard-compare) condition — you can either
break out the struct by hand and set `Type` plus the matching field, or use one of
these makers.

The makers are shorter and can't produce a mismatched struct, so prefer them.

## Making a value

| Node | Takes |
|---|---|
| Make SimFlow Value (Bool) | bool |
| Make SimFlow Value (Int) | int32 |
| Make SimFlow Value (Float) | float |
| Make SimFlow Value (String) | string |
| Make SimFlow Value (Name) | name |
| Make SimFlow Value (Object) | object reference |
| Make SimFlow Value (Vector) | vector |

All seven are pure. They set `Type` for you, which is the part that's easy to get
wrong by hand — a struct with `Type = None` and `Int Value = 5` reads as unset, and
nothing warns you.

```
Make SimFlow Value (Int) [3]  ──▶  Set Value ( Attempts )
```

Object values are runtime only. They're cleared when a flow is saved, so store an
identifying String alongside anything that has to survive a save.

## Reading a value back

| Node | Returns | Conversion |
|---|---|---|
| Flow Value To Bool | bool | Any non-zero number is true |
| Flow Value To Int | int32 | Rounds, so `2.6` becomes `3` |
| Flow Value To Float | float | |
| Flow Value To String | string | |

These never fail. They coerce, following the same best-effort rules as
[type coercion](../blackboard.md#type-coercion) — a bool reads as 0 or 1, a String
is parsed with `Atof`, an unset value reads as zero or empty.

`Flow Value To Int` rounding rather than truncating catches people out. A float
value of `2.6` read as an Int is `3`, not `2`.

There is no `Flow Value To Vector` or `Flow Value To Object`. For those, break the
struct and read `Vector Value` or `Object Value` directly, checking `Type` first.

Most of the time you don't need these at all — the
[typed blackboard getters](blackboard-nodes.md) return a plain bool, int or float
already, and they take a default for when the key is missing.

## Formatting for UI

Three nodes exist so your widgets don't have to do this themselves.

| Node | Takes | Returns |
|---|---|---|
| Result To Text | `ESimFlowResult` | "Succeeded", "Failed", "Skipped", "Timed Out", "Aborted" |
| Run State To Text | `ESimFlowRunState` | "Not Started", "Running", "Paused", "Completed", "Failed", "Aborted" |
| Format Seconds | float | `MM:SS`, zero-padded |

The first two read the enum's display name, so they pick up localisation and the
spaced forms — `TimedOut` comes back as "Timed Out".

`Format Seconds` rounds up and clamps at zero, which is what a countdown wants:
`0.2` seconds left still reads `00:01`, and a negative value reads `00:00` rather
than going backwards. Minutes aren't capped, so 90 minutes reads `90:00`.

```
Get Current Task Remaining Time ──▶ Format Seconds ──▶ Set Text
```

`Get Current Task Remaining Time` returns `-1` when the task has no time limit.
Branch on that before formatting, or your countdown shows `00:00` on every untimed
task.

## A debrief line

Showing how the trainee finished:

```
Get Run State ──▶ Run State To Text ──▶ Format Text "Result: {0}"
```

And for the last task:

```
Get Flow Instance ──▶ Get Last Task Result ──▶ Result To Text
```

## When it misbehaves

**A value written with a hand-built struct reads as unset.** `Type` was left at
`None`. Use a maker node.

**An Int came back one higher than expected.** `Flow Value To Int` rounds. Use
`Flow Value To Float` and truncate yourself if you need the other behaviour.

**A countdown sits at `00:00` the whole task.** The task has no `Time Limit`, so
remaining time is `-1` and `Format Seconds` clamps it.

**An object in the blackboard is null after a load.** Expected — object values don't
serialise. See [values and types](../blackboard.md#values-and-types).

*Next: [Blackboard nodes](blackboard-nodes.md) · [Blackboard](../blackboard.md) ·
[Status Widget](status-widget.md)*
