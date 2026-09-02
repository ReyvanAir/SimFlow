# Ordered Sequence

**Class:** `USimFlowTask_OrderedSequence`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Ordered Sequence**

---

## Overview / Purpose

*"Press these three buttons, in this order."*

Every button broadcasts the same tag with itself as the payload and **knows nothing
about the procedure**; this task holds the order.

The point of a dedicated task rather than a chain of
[Wait For Event](wait-for-event.md) tasks: **acting out of turn is a first-class
outcome** rather than something you have to notice by accident. A chain of separate
tasks would simply ignore a premature press; this task can tell the trainee they
did the right thing at the wrong moment.

---

## Field-by-field breakdown

### Sequence

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Event Tag** | Gameplay Tag | *empty* | **Yes** | The event every candidate raises, e.g. `SimFlow.Event.ButtonPressed`. |
| **Match Child Tags** | Bool | `true` | No | Also accept child tags. |
| **Steps** | Array of steps | *empty* | **Yes** | The expected order. |
| **Out Of Order Policy** | Enum | `Count Mistake (Stay On Step)` | No | What happens when the trainee acts out of turn. |
| **Unlisted Input Is Mistake** | Bool | `false` | No | Treat input that is not part of the sequence at all as an out-of-order mistake. |
| **Step Blackboard Key** | Name | `None` | No | *Advanced.* Publishes the current step index, for a progress widget. |

### Each step

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Target** | [Actor Query](../actor-query.md) | *empty* | The object this step expects — the second valve, the green button. |
| **Instruction** | Text (multi-line) | *empty* | Shown while this step is the current one. Read it with `Get Current Step Instruction`. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Field left empty | What happens |
|---|---|
| **Steps** (no entries) | Logs *"has no steps - finishing immediately"* and **finishes with `Succeeded`**. A no-op. |
| **Event Tag** | Logs *"has no event tag set - failing"* and **finishes with `Failed`**. |
| **A step's Target** | That step can never be satisfied — an unset query matches nothing. **The task hangs on that step.** |
| **Step Blackboard Key** | Nothing is published. Harmless. |

Note the asymmetry: **no steps succeeds, no tag fails.** They are different kinds of
mistake — an empty step list is a flow that has not been filled in yet, whereas a
missing tag means the task could never work.

| Situation | What happens |
|---|---|
| **Two steps share the same Target** | The first press satisfies step 1; the second is judged against step 2, matches, and advances. Repeating the same object in sequence works. |
| **`Unlisted Input Is Mistake` off** | Objects not in the sequence are ignored entirely. **The default**, and right when unrelated props share the tag. |
| **`Unlisted Input Is Mistake` on** | Any object raising the tag that is not a step counts as an out-of-order mistake. |
| **Payload is a widget** | Scores `No Match`, logs a warning. See [Actor Query](../actor-query.md#payloads-that-are-not-actors). |

---

## How input is judged

On every event with a matching tag:

1. **Is it the expected step?** If the payload is an `Exact` match for the current
   step's `Target` → `On Step Completed` fires, the step advances, and if that was
   the last step the task finishes with `Succeeded`.
2. **Is it a different step in the sequence?** The task checks every *other* step's
   target. This decides severity, below.
3. **Is it unlisted, and `Unlisted Input Is Mistake` is off?** Ignored silently.
4. Otherwise the **Out Of Order Policy** is applied.

### Severity is graded

When a mistake is recorded, its severity reflects *what kind* of error it was:

| The trainee used | Severity | Reasoning |
|---|---|---|
| A step of the procedure, at the wrong moment | **Related (Near Miss)** | They know the procedure, they got the order wrong |
| A prop that was never part of it | **No Match** | A different kind of error entirely |

The mistake is recorded as `SimFlow.Mistake.WrongOrder` with a description like
*"Step 2: used Valve One - expected Control.Valve.Three"*, and the `WrongAttempts`
[blackboard](../blackboard.md) key is incremented.

---

## Out Of Order Policy

| Policy | Behaviour | Use for |
|---|---|---|
| **Ignore** | Nothing recorded, stay on the step | Free exploration |
| **Count Mistake (Stay On Step)** | Record and stay put. **Default** | Training — let them find it |
| **Count Mistake And Restart** | Record and go back to step one | Procedures where order is the whole point |
| **Count Mistake And Fail Task** | Record and fail, driving the `Failed` pin | Assessment |

**Count Mistake And Restart** is the strict one: a single slip on step 5 sends the
trainee back to step 1. That is correct for a safety procedure where a wrong order
invalidates everything, and frustrating everywhere else.

---

## Delegates and accessors

| Member | Signature | Use for |
|---|---|---|
| **On Step Completed** | (Step Index, Target) | Tick off a checklist item |
| **On Wrong Input** | (Payload, Expected Step Index) | Feedback on a wrong press |
| **Get Current Step Index** | → Int | Progress UI (**0-based**) |
| **Get Current Step Instruction** | → Text | The current step's instruction, for tutorial UI |

`Step Blackboard Key` publishes the same index to the blackboard, which is the
easier route for a widget that already reads blackboard values.

---

## Dependencies

| Depends on | Why |
|---|---|
| Something raising the tag | See [Events](../events.md) |
| [Identity](../identity.md) on each target | Tag queries need identity tags |
| [Actor Query](../actor-query.md) | Every step's `Target` |

---

## Example use case: a three-valve startup procedure

**Goal:** the trainee must open valve A, then B, then C. Doing them out of order is
a recorded mistake but should not end the exercise.

**Setup**

1. **Tags.** Add `Control.Valve.A`, `Control.Valve.B`, `Control.Valve.C`.
2. **Identity.** On the valve Blueprint add a
   [SimFlow Identity](../identity.md); set each level instance's tag and a Display
   Name (`Valve A`, …).
3. **Broadcast.** In the valve Blueprint, when turned: **Broadcast Flow Event**,
   Event Tag = `SimFlow.Event.Interact`, Payload = `self`.

**The task**

4. Add a [Task node](../nodes/task.md), **Task** = **Ordered Sequence**.
5. **Event Tag** = `SimFlow.Event.Interact`.
6. Under **Steps**, click **+** three times:
   - Step 0: Target → Required Tags = `Control.Valve.A`;
     Instruction = `Open valve A`
   - Step 1: `Control.Valve.B`; `Open valve B`
   - Step 2: `Control.Valve.C`; `Open valve C`
7. **Out Of Order Policy** = **Count Mistake (Stay On Step)**.
8. Leave **Unlisted Input Is Mistake** off — other props share the interact tag.
9. Set **Step Blackboard Key** to `CurrentStep`.
10. Set **Score On Success** to `25`.

**UI**

11. Bind **On Step Completed** to tick off a checklist entry.
12. Read `Get Current Step Instruction` to drive the on-screen prompt.

Turning valve C first now records a **Related** mistake — the trainee knows the
procedure but not the order — while turning an unrelated wheel is ignored.

> **Screenshot needed:** the Details panel showing the Steps array expanded with
> three entries, each with its Target query and Instruction.

---

## Common pitfalls

**The task completes instantly.**
`Steps` is empty — that finishes with success by design.

**The task fails instantly.**
`Event Tag` is empty.

**The task hangs on one step.**
That step's `Target` query is empty or does not match the object. An unset query
matches nothing.

**Nothing is ever recorded as out of order.**
The wrong objects are not in the sequence and `Unlisted Input Is Mistake` is off, or
the policy is `Ignore`.

**Every unrelated prop counts as a mistake.**
`Unlisted Input Is Mistake` is on and other actors share the event tag. Turn it off
or use a dedicated tag.

**The progress widget is off by one.**
`Get Current Step Index` is **0-based**. Add 1 for display.

**The sequence restarts unexpectedly.**
The policy is **Count Mistake And Restart**.

---

*See also: [Wait For Event](wait-for-event.md) · [Events](../events.md) ·
[Actor Query](../actor-query.md) · [Task Reference](README.md)*
