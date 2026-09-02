# Actor Query

**Struct:** `FSimFlowActorQuery`
**Appears in:** [Place Object In Zone](tasks/place-object-in-zone.md), [Wait For Event](tasks/wait-for-event.md), [Ordered Sequence](tasks/ordered-sequence.md), [Zones](zones.md)

---

## Overview / Purpose

An Actor Query answers **"which object does this task mean?"**

It is the single targeting mechanism used everywhere in SimFlow. Rather than each
task inventing its own way to point at an object, they all embed this struct — so
once you understand it you understand targeting across the whole plugin.

The important design point: a query does not return a yes/no. It **grades** the
answer as `No Match`, `Related (Near Miss)` or `Exact Match`, which is what lets a
flow tell *"you grabbed the CO2 unit instead of the foam one"* apart from
*"you grabbed a wrench"*.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Specific Actor** | Soft Object Ptr (Actor) | *null* | No | One particular actor placed in the level. |
| **Blackboard Key** | Name | `None` | No | Reads the target from a [blackboard](blackboard.md) object key at runtime. |
| **Required Tags** | Gameplay Tag Container | *empty* | No | Matches any actor whose [identity](identity.md) carries these tags. |
| **Require All Tags** | Bool | `true` | No | `true` = actor must have every tag; `false` = any one is enough. |
| **Required Class** | Class (Actor) | *null* | No | *Advanced.* Extra narrowing by actor class. |
| **Required Actor Tag** | Name | `None` | No | *Advanced.* Plain `AActor` tag, for actors you cannot add a component to. |
| **Min Related Tag Depth** | Int (min 1) | `2` | No | *Advanced.* How many leading tag nodes count as a near miss. |

> **Fill in exactly one of the first three fields** in the common case. `Required
> Class` and `Required Actor Tag` are narrowing modifiers, not primary selectors.

---

## Which field to use

| You want to say | Use | Works for spawned objects? |
|---|---|---|
| "That button, the one on the left wall" | **Specific Actor** | No — it points at one placed actor |
| "Any foam extinguisher" | **Required Tags** | **Yes** |
| "Whatever they picked up a moment ago" | **Blackboard Key** | Yes |
| "Any actor of class BP_Valve" | **Required Class** | Yes, but coarse |
| "That third-party actor I cannot modify" | **Required Actor Tag** | Yes |

**Required Tags is the form to reach for by default.** It is the only primary
selector that survives objects being spawned, duplicated or streamed in.

---

## Behaviour when left empty

**An empty query matches nothing.** `MatchActor` returns `No Match` immediately when
no field is set — it does *not* fall through to "match anything".

The consequence differs by where the query is used, and this catches people out:

| Where | Empty query means |
|---|---|
| **Wait For Event → Expected Payload** | Payload is not checked at all — **any** sender satisfies the task |
| **Place Object In Zone → Zone** | Task **fails immediately** with a warning — it cannot find a zone |
| **Place Object In Zone → Accepted Items** | Nothing can ever be accepted; the task never completes |
| **Place Object In Zone → Rejected Items** | No explicit rejections (this is the normal case) |
| **Zone → Track Filter** | Falls back to `Require Identity Component` — the normal case |
| **Ordered Sequence → step Target** | That step can never be satisfied |

Note the asymmetry: an empty *Expected Payload* is a deliberate "don't care", while
an empty *Accepted Items* is a dead end. The difference is that the payload check is
skipped entirely when the query is unset, whereas the accepted-items check always
runs and always scores `No Match`.

---

## How matching works

The resolver checks fields **in this order**, and the first one that is set decides:

1. **Specific Actor** — pointer equality → Exact Match, or fall through.
2. **Blackboard Key** — reads the object key and compares → Exact Match, or fall through.
3. **Required Tags** — the graded path (see below).
4. **Required Class / Required Actor Tag** alone — Exact Match when both constraints pass.

### Match quality

Only the tag path can produce `Related`. With tags set:

- **Exact Match** — the tag test passes (`HasAll` or `HasAny`, per `Require All
  Tags`) *and* the class *and* actor-tag constraints pass.
- **Related (Near Miss)** — the tag test failed, but the actor's tags share at least
  `Min Related Tag Depth` leading nodes with a requested tag.
- **No Match** — neither.

### Min Related Tag Depth

At the default of `2`:

| Query asks for | Actor has | Shared leading nodes | Result |
|---|---|---|---|
| `Item.Extinguisher.Foam` | `Item.Extinguisher.Foam` | — | **Exact** |
| `Item.Extinguisher.Foam` | `Item.Extinguisher.CO2` | 2 (`Item.Extinguisher`) | **Related** |
| `Item.Extinguisher.Foam` | `Item.Wrench` | 1 (`Item`) | **No Match** |

Raise it to `3` to make near misses stricter; lower it to `1` to make everything
sharing a root tag a near miss. Values below 1 are clamped to 1.

This only pays off if your tag tree groups confusable things under a shared parent —
see [Identity · Depth matters](identity.md#depth-matters-more-than-you-would-expect).

---

## Resolving to a single actor

`Resolve()` returns **one** actor, and behaves differently per field:

- **Specific Actor** — returns it **without forcing a synchronous load**. An actor
  in an unloaded level comes back `null`.
- **Blackboard Key** — returns the stored object cast to `AActor`.
- **Tags / Class** — **scans the world and returns the first Exact Match.**

That last case matters: tag queries describe a *set*, so "first match" is arbitrary
when several actors qualify. Resolve is meant for finding **one** thing — a
[zone](zones.md) — not for picking among items. If two zones share a tag, which one
you get is undefined.

---

## Payloads that are not actors

When a query grades an *event payload* it accepts:

- an **Actor** — graded directly
- an **Actor Component** — unwrapped to its owning actor first

Anything else scores `No Match` **and logs a warning**. The usual culprit is a UMG
widget broadcasting `self` from a button's `OnClicked`: a `UUserWidget` is neither
an Actor nor an Actor Component.

SimFlow deliberately does **not** walk `GetTypedOuter<AActor>()` to find an actor
behind a widget — a widget created with `CreateWidget(PlayerController, ...)` outers
to the controller, so that would confidently match the *wrong* actor. Failing loudly
beats lying quietly.

**Fix:** broadcast from the owning actor with `Payload = self`, not from the widget.

---

## Dependencies

| Depends on | Why |
|---|---|
| [Identity System](identity.md) | Supplies the tags the tag path reads |
| [Blackboard](blackboard.md) | Only when `Blackboard Key` is used |
| Gameplay Tags (engine) | The tags themselves |

---

## Example use case: one query, three exercises

**Goal:** the same extinguisher bay is the right answer in one exercise and a
distractor in the next, without editing a single Blueprint.

1. Tag the bay's [zone](zones.md) identity `Zone.ExtinguisherBay`.
2. Tag the items `Item.Extinguisher.Foam` and `Item.Extinguisher.CO2`.
3. **Exercise A** — Place Object In Zone: Zone = `Zone.ExtinguisherBay`,
   Accepted Items = `Item.Extinguisher.Foam`.
4. **Exercise B** — same zone, Accepted Items = `Item.Extinguisher.CO2`. The foam
   unit now scores **Related**, so the flow can say *"right family, wrong agent"*.
5. **Exercise C** — Accepted Items = `Item.Extinguisher`, Rejected Items =
   `Item.Extinguisher.CO2`. Both are "extinguishers", but the CO2 one is explicitly
   called out as the plausible-looking wrong answer.

The level never changed. The flow asset holds the answer.

---

## Common pitfalls

**The task never completes and nothing is logged.**
The query is empty, or the actor has no matching identity tags. An unset query
scores `No Match` silently on the accepted-items path.

**A warning about a payload that is "neither an Actor nor an ActorComponent".**
You broadcast from a widget. Broadcast from the owning actor with `Payload = self`.

**Everything is No Match, never Related.**
Your tag tree is flat, or `Min Related Tag Depth` exceeds your tag depth.

**Specific Actor is set but resolves to null.**
The actor lives in a level that is not loaded — Resolve does not force a
synchronous load. Use tags instead for streamed content.

**Two zones share a tag and the wrong one is used.**
Tag queries resolve to the *first* world match. Give each zone a distinct tag, or
point at it with `Specific Actor`.

**Require All Tags is on and nothing matches.**
With several `Required Tags`, the actor must carry **all** of them. Set it to
`false` for "any of these".

---

*See also: [Identity System](identity.md) · [Zones](zones.md) ·
[Glossary](glossary.md) · [Documentation index](README.md)*
