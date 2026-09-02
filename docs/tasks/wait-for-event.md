# Wait For Event

**Class:** `USimFlowTask_WaitForEvent`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Wait For Event**

---

## Overview / Purpose

Blocks until an **event tag is raised on the flow** — optionally by the *right
object*.

This is the main way the world advances a flow. A button, a grabbable object or an
animation notify raises a tag; this task is waiting for it.

The `Expected Payload` field is what makes it powerful: ten buttons can all
broadcast the same tag, knowing nothing about the procedure, while only the intended
one satisfies the task. See [Events](../events.md).

---

## Field-by-field breakdown

### Event

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Event Tag** | Gameplay Tag | *empty* | **Yes** | The tag this task listens for. |
| **Match Child Tags** | Bool | `true` | No | Listening for `Sim.Grab` also accepts `Sim.Grab.Extinguisher`. |
| **Accept Already Raised** | Bool | `false` | No | *Advanced.* If the tag was raised earlier in this run, finish straight away. |

### Event \| Payload

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Expected Payload** | [Actor Query](../actor-query.md) | *empty* | No | Which object the event has to be about. |
| **Mismatch Policy** | Enum | `Count Mistake (Keep Waiting)` | No | What happens when the tag is right but the object is not. |
| **Payload To Blackboard Key** | Name | `None` | No | *Advanced.* Stores the accepted payload, so later tasks can refer to "the thing they picked". |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Field left empty | What happens |
|---|---|
| **Event Tag** | Logs *"WaitForEvent task has no tag set - finishing immediately"* and **finishes with `Succeeded`**. The task is a no-op pass-through, not a failure. |
| **Expected Payload** | **The payload is not checked at all — any sender satisfies the task.** This is a legitimate "don't care", and often what you want. |
| **Payload To Blackboard Key** | Nothing is stored. Harmless. |

| Situation | What happens |
|---|---|
| **The tag is never raised** | The task waits forever. Set a `Time Limit` on the [Task node](../nodes/task.md) while developing. |
| **Payload is a UMG widget** | Scores `No Match` **and logs a warning**. A `UUserWidget` is neither an Actor nor an Actor Component. Broadcast from the owning actor with `Payload = self`. |
| **Payload is an Actor Component** | Unwrapped to its owning actor and graded normally. A button Blueprint sending the pressed component works fine. |
| **`Accept Already Raised` on with an `Expected Payload` set** | **The already-raised behaviour is ignored**, and a Verbose note is logged. See below. |

> ### Why Accept Already Raised is disabled by a payload check
> A past event kept only its **tag** — the payload was not retained. Honouring
> "already raised" when a payload check exists would let the *wrong* object satisfy
> the task, silently. Failing to take the shortcut is the safe choice, so the task
> waits for a fresh event instead.

---

## How a matching event is handled

1. The tag is compared (`Matches Tag` when `Match Child Tags` is on, otherwise exact
   equality). A non-match is ignored silently.
2. If `Expected Payload` is set, the payload is graded by the
   [Actor Query](../actor-query.md):
   - **Exact** → continue to step 3.
   - **Anything else** → `On Payload Rejected` fires, a mistake description is built
     ("Interacted with Wrench - expected Item.Extinguisher.Foam"), and the
     **Mismatch Policy** is applied. The task does *not* complete.
3. The payload is stored to `Payload To Blackboard Key` if set.
4. The task finishes with `Succeeded`.

---

## Mismatch Policy

| Policy | Behaviour |
|---|---|
| **Ignore (Keep Waiting)** | Silent. The wrong object is simply not the one we want. |
| **Count Mistake (Keep Waiting)** | Records a `SimFlow.Mistake.WrongTarget` mistake and keeps waiting. **Default.** |
| **Count Mistake And Fail Task** | Records the mistake and fails, driving the `Failed` pin. |

---

## Delegates

| Delegate | Signature | Use for |
|---|---|---|
| **On Payload Rejected** | (Payload, Match Quality) | A buzzer, a red outline, a hint — and a *different* hint for a near miss |

---

## Dependencies

| Depends on | Why |
|---|---|
| Something raising the tag | See [Events](../events.md) |
| [Identity](../identity.md) on the sender | Only when `Expected Payload` uses tags |
| [Actor Query](../actor-query.md) | The payload check |

---

## Example use case: the right valve, among five

**Goal:** the trainee must turn **valve 3**. All five valves broadcast the same tag.

1. **Tag the valves.** Add a [SimFlow Identity](../identity.md) to the valve
   Blueprint. On the level instances set Identity Tags to `Control.Valve.One` …
   `Control.Valve.Five`.
2. **Broadcast from each valve.** In the valve Blueprint, when it is turned:
   **Broadcast Flow Event**, Event Tag = `SimFlow.Event.Interact`,
   Payload = `self`.
3. **The task.** Add a [Task node](../nodes/task.md), set **Task** to
   **Wait For Event**.
4. **Event Tag** = `SimFlow.Event.Interact`.
5. **Expected Payload → Required Tags** = `Control.Valve.Three`.
6. **Mismatch Policy** = **Count Mistake (Keep Waiting)** so the trainee can
   self-correct.
7. Set **Payload To Blackboard Key** to `LastValve` if a later task needs to refer
   back to it.
8. Set **Instruction** to `Turn valve 3`.
9. Bind **On Payload Rejected** to flash the correct valve after a wrong attempt.

None of the five valves knows anything about the exercise. Swapping the answer to
valve 4 is a one-field change in the flow asset.

---

## Common pitfalls

**The task completes instantly and nothing happened.**
`Event Tag` is empty — the task finishes immediately by design. Check the log.

**The task never fires even though the event is broadcast.**
The tags do not match. Check `Match Child Tags`, and verify the broadcast actually
runs with a print node.

**A warning says the payload is "neither an Actor nor an ActorComponent".**
You broadcast `self` from a UMG widget graph. Broadcast from the owning actor
instead. This is the single most common first bug.

**Any object satisfies the task.**
`Expected Payload` is empty, so the payload is not checked at all.

**Accept Already Raised does nothing.**
You also set an `Expected Payload`, which disables it by design.

**The flow hangs here forever.**
Nothing raises the tag. Add a `Time Limit` on the Task node and wire `Timed Out` to
a hint.

**Every flow in the level reacts.**
`Broadcast Flow Event` reaches all running flows. Use `Send Flow Event` with a
`Flow Save Id` to target one.

---

*See also: [Events](../events.md) · [Actor Query](../actor-query.md) ·
[Ordered Sequence](ordered-sequence.md) · [Task node](../nodes/task.md) ·
[Task Reference](README.md)*
