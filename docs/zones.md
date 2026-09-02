# Zones

**Actor:** `SimFlow Zone` (`ASimFlowZone`)
**Place via:** drag a **SimFlow Zone** into the level
**Class group:** SimFlow

---

## Overview / Purpose

A Zone is **a named volume a flow can ask questions about**: *"what is in the parts
bin?"*

It solves two problems at once:

1. **Naming a place.** A zone carries its own [Identity](identity.md) component, so
   a task finds it by tag (`Zone.PartsBin`) exactly the way it finds items — one
   identity mechanism for the whole plugin rather than a second one for zones.
2. **Knowing when something is actually put down.** A trainee holding an object
   *over* the bin has not placed it. A zone tracks not just what overlaps, but
   whether each object has **settled**.

A zone is deliberately **neutral about right and wrong**. It reports what is there;
[Place Object In Zone](tasks/place-object-in-zone.md) holds the opinion. That split
is why the same bay can be the correct answer in one exercise and a distractor in
the next without touching the level.

---

## Field-by-field breakdown

### Zone

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Box** | Box Component | auto-created | — | The volume. Read-only in Details; **resize it on the placed actor**, not in Blueprint defaults. |
| **Identity** | SimFlow Identity | auto-created | — | Names the zone. Set its **Identity Tags** to e.g. `Zone.PartsBin`. |
| **Track Filter** | [Actor Query](actor-query.md) | *empty* | No | Only actors passing this are tracked at all. |
| **Require Identity Component** | Bool | `true` | No | When true, untagged actors such as the player pawn are ignored. |

### Zone \| Settling

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Require Detached** | Bool | `true` | No | An object still attached to something is not considered placed. |
| **Settle Time** | Float (s, min 0) | `0.35` | No | How long an object must sit still inside the zone before it counts as placed. |
| **Settle Speed Threshold** | Float (min 0) | `20.0` | No | Simulating objects must also drop below this speed. **0 skips the check.** |

### Zone \| Events / Debug

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Broadcast Flow Events** | Bool | `false` | No | Also raise `SimFlow.Event.Placed` / `SimFlow.Event.Removed` on every flow, with the actor as payload — so a plain [Wait For Event](tasks/wait-for-event.md) task can use this zone too. |
| **Draw Debug** | Bool | `false` | No | Draws the box in the level — green while tracking something, silver when empty. |

---

## Behaviour when left empty or misconfigured

| Setting | Left empty / zeroed | Effect |
|---|---|---|
| **Identity Tags** on the zone | empty | The zone still works, but no tag query can find it — you must point at it with `Specific Actor`. **Usually a mistake.** |
| **Track Filter** | empty | Falls back to `Require Identity Component`. **This is the normal, recommended case.** |
| **Track Filter** empty *and* **Require Identity Component** off | — | The zone tracks **everything** that overlaps, including the player pawn. |
| **Settle Time** | `0` | An object settles on the first tick it is at rest. Twitchy with physics objects. |
| **Settle Speed Threshold** | `0` | Speed is not checked at all — a sliding object can count as settled. |
| **Box** | not resized | The default box may be far too small to catch anything. Resize it in the level. |

---

## Settling — how an object is judged "put down"

Every tick, for each tracked actor, the zone asks whether it is at rest:

1. **Is it flagged held?** (`SimFlow Identity → Is Held`.) If yes → **not at rest**,
   full stop. This check comes first and overrides everything below.
2. **Is it attached?** If `Require Detached` is on and the actor has an attach
   parent → not at rest.
3. **Is it slow enough?** If `Settle Speed Threshold > 0`:
   - a physics-simulating root component → compare its physics linear velocity
   - otherwise → compare the actor's own velocity

An actor at rest accumulates still-time. When that reaches `Settle Time`, the zone
marks it settled and fires **On Actor Settled**.

**Picking an object back up resets it completely** — the still-time returns to zero
and the settled flag clears, so it must settle again from scratch.

> ### Why the held flag exists
> Attachment is only *one* of the ways a VR framework can hold an object.
> VRExpansion, for one, holds most grip types with a physics constraint and **never
> reparents the actor**, so an object in the trainee's hand can look perfectly
> detached from out here. Call `Set Held(true)` from wherever your grab succeeds
> and `Set Held(false)` on release, and placement checks become exact regardless of
> how grabbing is implemented. See
> [Identity · Held state](identity.md#held-state--set-held--is-held).

---

## Delegates

| Delegate | Fires when | Use it for |
|---|---|---|
| **On Actor Entered** | The moment an actor overlaps, **before** it has settled | Highlighting, hover feedback |
| **On Actor Exited** | The actor stops overlapping | Clearing feedback |
| **On Actor Settled** | The actor is genuinely put down | **This is the one tasks listen to** |

---

## Queries (Blueprint Pure)

| Function | Returns |
|---|---|
| **Get Contained Actors** | Everything overlapping, settled or not |
| **Get Settled Actors** | Everything that has been put down and left alone |
| **Contains Actor** | Is this actor overlapping? |
| **Is Actor Settled** | Has this actor settled? |
| **Get Display Name Text** | The zone's display name, for UI and mistake text |

---

## Dependencies

| Depends on | Why |
|---|---|
| [Identity System](identity.md) | Names the zone; supplies held state for settling |
| [Actor Query](actor-query.md) | The `Track Filter` field |
| Collision overlap | The box must generate overlap events, and items need collision that responds to it |

**Depended on by:** [Place Object In Zone](tasks/place-object-in-zone.md) — which
fails at start if it cannot resolve a zone.

---

## Example use case: a parts bin

**Goal:** a flow can ask "is the foam extinguisher in the bay?"

1. **Create the tags.** Project Settings → Gameplay Tags → add
   `Zone.ExtinguisherBay` and `Item.Extinguisher.Foam`.
2. **Place the zone.** Drag a **SimFlow Zone** into the level beside the bay.
3. **Size the box.** Select the zone, select its **Box** component, and scale it in
   the viewport to cover the bay. Do this on the level instance.
4. **Name it.** On the zone's **Identity** component set **Identity Tags** to
   `Zone.ExtinguisherBay` and **Display Name** to `Extinguisher Bay`.
5. **Leave `Track Filter` empty** and `Require Identity Component` on — the zone
   tracks any item carrying an identity component and ignores the player.
6. **Check the item's collision.** The extinguisher needs a collision primitive
   that generates overlap events with the box.
7. **Turn on `Draw Debug`** while testing — the box turns green when it is tracking
   something, which instantly tells you whether overlaps are arriving.

A [Place Object In Zone](tasks/place-object-in-zone.md) task with
Zone = `Zone.ExtinguisherBay` will now find it.

> **Screenshot needed:** a SimFlow Zone selected in the level with Draw Debug on,
> showing the green box around a parts bin.

---

## Common pitfalls

**Nothing is ever tracked.**
Overlaps are not reaching the box. Check that the item has collision that overlaps
the box's object type, and that the box is big enough. Turn on `Draw Debug` — a
silver box is tracking nothing.

**Objects count as placed while still in the trainee's hand.**
Held state is not wired up and your VR framework does not reparent on grab. Call
`Set Held` from your grab logic. This is the single most common zone problem.

**The player pawn is being tracked.**
`Require Identity Component` was turned off with no `Track Filter` set. Turn it back
on.

**Objects settle, then immediately un-settle, repeatedly.**
The object is jittering above `Settle Speed Threshold` — common for physics objects
resting on uneven collision. Raise `Settle Time`, or raise the threshold.

**A task cannot find the zone.**
The zone's Identity has no tags, or two zones share a tag and the wrong one resolved
first. See [Actor Query · Resolving](actor-query.md#resolving-to-a-single-actor).

**Resizing the box in the Blueprint did nothing.**
Zones are normally sized per level instance. Resize the placed actor's Box component
in the viewport.

---

*See also: [Place Object In Zone](tasks/place-object-in-zone.md) ·
[Identity System](identity.md) · [Actor Query](actor-query.md) ·
[Documentation index](README.md)*
