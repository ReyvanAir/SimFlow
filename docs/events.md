# Events

**Statics:** `USimFlowStatics`
**Consumed by:** [Wait For Event](tasks/wait-for-event.md), [Ordered Sequence](tasks/ordered-sequence.md), [Event Was Raised](conditions.md#event-was-raised)

---

## Overview / Purpose

Events are how **the world talks to a running flow**.

A grabbable object, a button, an animation notify or a zone raises a gameplay tag,
optionally carrying a **payload** — the object the event is about. Tasks listening
for that tag wake up and decide whether the payload was the right object.

The design intent: **the level stays neutral and the flow asset holds the answer.**
Ten buttons can all broadcast the same tag, knowing nothing about the procedure,
while the flow decides which one counted.

---

## Raising an event

| Function | Reaches | Use when |
|---|---|---|
| **Broadcast Flow Event** (Tag, Payload) | **Every running flow** | The usual case — a prop does not know which flow is running |
| **Send Flow Event** (Flow Save Id, Tag, Payload) | One specific flow | Several flows are running and only one should hear it |
| **Send Event** on a component (Tag, Payload) | That component's flow | You already have the component reference |

All three are Blueprint-callable. `Broadcast Flow Event` and `Send Flow Event` are
static and need a world context, which Blueprint supplies automatically.

### Always pass the owning actor as the payload

```
Broadcast Flow Event
    Event Tag = SimFlow.Event.ButtonPressed
    Payload   = self          <- from the ACTOR's graph, not a widget's
```

A payload is graded by an [Actor Query](actor-query.md), which accepts an **Actor**
or an **Actor Component** (unwrapped to its owner). Anything else scores `No Match`
and logs a warning.

> **The most common first bug:** broadcasting `self` from inside a **UMG widget**
> graph. A `UUserWidget` is neither an Actor nor an Actor Component, so it can never
> satisfy a payload check. Broadcast from the actor that owns the widget instead.
> See [Actor Query · Payloads that are not actors](actor-query.md#payloads-that-are-not-actors).

---

## Tags shipped with the plugin

These are native tags, available with no setup.

| Tag | Meaning |
|---|---|
| `SimFlow.Event` | Parent of all built-in event tags |
| `SimFlow.Event.Generic` | General-purpose event |
| `SimFlow.Event.Interact` | The trainee interacted with something |
| `SimFlow.Event.Grab` | Object picked up |
| `SimFlow.Event.Release` | Object let go |
| `SimFlow.Event.ButtonPressed` | Button press |
| `SimFlow.Event.Placed` | Raised by a [zone](zones.md) when an object settles (opt-in) |
| `SimFlow.Event.Removed` | Raised by a zone when an object leaves (opt-in) |

The two zone tags are only raised when that zone's **Broadcast Flow Events** is on.

### Your own event tags

Add them in **Project Settings → Project → Gameplay Tags**, or inline from any tag
picker. Keep them in **your own namespace**, not under `SimFlow.` — that hierarchy
belongs to the plugin and a future version may add to it.

---

## Child tag matching

Listening tasks default to `Match Child Tags = true`, which means listening for
`Sim.Grab` also accepts `Sim.Grab.Extinguisher`.

This lets you build a hierarchy where a task can be as specific or as general as it
needs:

```
Sim.Grab                  <- a task listening here hears all three
Sim.Grab.Extinguisher
Sim.Grab.Wrench
```

Turn it off for an exact-tag-only match.

---

## Event history

The flow instance remembers which tags have been raised during the run. Two things
read that history:

- [Event Was Raised](conditions.md#event-was-raised) — a condition, "has this ever
  happened?"
- [Wait For Event](tasks/wait-for-event.md)'s **Accept Already Raised** — finish
  immediately if the tag fired earlier in this run.

> **Accept Already Raised is ignored when an Expected Payload is set**, and the task
> logs a Verbose note saying so. A past event kept only its tag — there is no
> payload left to check — so honouring it would let the *wrong* object satisfy the
> task. This is deliberate.

---

## Example use case: a button that advances the flow

**Goal:** pressing a physical button in VR completes the current task.

1. Open the button actor's Blueprint (`BP_StartButton`).
2. On whatever fires when it is pressed, add **Broadcast Flow Event**:
   - Event Tag = `SimFlow.Event.ButtonPressed`
   - Payload = `self`
3. Add a **SimFlow Identity** component to the button and tag it, e.g.
   `Control.Button.Start`. See [Identity](identity.md).
4. In the flow, add a Task node with a [Wait For Event](tasks/wait-for-event.md)
   task:
   - Event Tag = `SimFlow.Event.ButtonPressed`
   - Expected Payload → Required Tags = `Control.Button.Start`
5. Every other button can now broadcast the same tag harmlessly — only the one
   tagged `Control.Button.Start` satisfies this task.

---

## Common pitfalls

**The task never fires.**
The tag does not match. Check `Match Child Tags`, and confirm the broadcast is
actually running with a print node — a tag mismatch is silent.

**A warning about a payload that is not an Actor or ActorComponent.**
You broadcast from a widget. Broadcast from the owning actor with `Payload = self`.

**Every flow reacts to one prop.**
`Broadcast Flow Event` reaches all running flows. Use `Send Flow Event` with a
`Flow Save Id` to target one.

**Accept Already Raised is not working.**
An `Expected Payload` is set, which disables it by design. Remove the payload check
or drop the already-raised behaviour.

**The event fires but the wrong object satisfies the task.**
`Expected Payload` is empty, so any sender counts. Fill it in.

**In multiplayer, a client's event does nothing.**
Events must reach the authority. Use the component's `Send Event`, which forwards
from clients when the PlayerController has a **SimFlow Player Component**.

---

*See also: [Wait For Event](tasks/wait-for-event.md) · [Actor Query](actor-query.md) ·
[SimFlow Component](simflow-component.md) · [Documentation index](README.md)*
