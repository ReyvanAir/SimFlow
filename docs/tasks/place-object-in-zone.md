# Place Object In Zone

**Class:** `USimFlowTask_PlaceObject`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Place Object In Zone**

---

## Overview / Purpose

*"Put the foam extinguisher in the bay."*

This task **watches a [Zone](../zones.md) and judges what turns up in it.**

The division of responsibility is the whole point:

- The **zone** reports what is there, without any opinion about right and wrong.
- The **task** holds the opinion.

So the same bay can be the correct answer in one exercise and a distractor in the
next **without touching a single Blueprint**. The level stays neutral; the flow
asset holds the answer.

It also distinguishes *how wrong* a wrong answer was, so your feedback can say
"close — that's the CO2 unit" rather than just "no".

---

## Field-by-field breakdown

### Placement

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Zone** | [Actor Query](../actor-query.md) | *empty* | **Yes** | Which zone to watch. Usually a tag such as `Zone.PartsBin`, or the zone actor itself. |
| **Accepted Items** | [Actor Query](../actor-query.md) | *empty* | **Yes** | What belongs there. **Tags are the useful form** — they cover spawned copies. |
| **Rejected Items** | [Actor Query](../actor-query.md) | *empty* | No | *Advanced.* Explicit wrong answers, always treated as a mistake. |
| **Required Count** | Int (min 1) | `1` | No | How many accepted items must be in the zone **at once**. |
| **Require Settled** | Bool | `true` | No | Wait for the item to be put down and let go, rather than reacting while it is still held. |
| **Wrong Item Policy** | Enum | `Count Mistake (Keep Waiting)` | No | What happens when the wrong thing is placed. |
| **Report Each Wrong Item Once** | Bool | `true` | No | *Advanced.* Only report a given wrong item once, not every time it re-settles. |
| **Placed Item Blackboard Key** | Name | `None` | No | *Advanced.* Stores the last accepted item. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

This is the section most people need, so it is spelled out in full.

| Field left empty | What actually happens |
|---|---|
| **Zone** | **The task fails immediately.** At start it logs *"could not resolve a SimFlow Zone from its Zone query"* and finishes with `Failed`, driving the [Task node](../nodes/task.md)'s `Failed` pin. It does not wait or retry. |
| **Accepted Items** | The task starts fine but **nothing can ever be accepted** — an empty query scores `No Match` against everything. Every item placed is judged wrong, and the task never completes. |
| **Rejected Items** | No explicit rejections. **This is the normal case** — leave it empty unless you have a specific decoy. |
| **Placed Item Blackboard Key** | Nothing is stored. Harmless. |

Other misconfigurations:

| Situation | What happens |
|---|---|
| **Zone query resolves to a non-zone actor** | Treated as unresolved → the task fails at start. The query must find an actual `SimFlow Zone`. |
| **Two zones share the tag** | The **first** world match wins, and which one that is is undefined. See [Actor Query · Resolving](../actor-query.md#resolving-to-a-single-actor). |
| **Required Count higher than the items that exist** | The task can never complete. |
| **Require Settled on, held state not wired** | Items may never settle if your VR framework holds them by constraint. See [Zones · Settling](../zones.md#settling--how-an-object-is-judged-put-down). |
| **Require Settled off** | Reacts the instant an item overlaps — including while still in the trainee's hand. |

> **The two failure modes look completely different.** An empty *Zone* fails loudly
> and instantly. An empty *Accepted Items* hangs forever in silence. If the task
> fails at once, check the Zone; if it never completes, check Accepted Items.

---

## How an item is judged

When the zone reports an item as settled (or contained, if `Require Settled` is
off), the task grades it:

1. **Rejected Items** is checked **first**. An explicit rejection **always wins**,
   even if `Accepted Items` would have let the item through — that is how you call
   out the one item that looks right and is not. A rejected item is graded
   **Related**, i.e. a near miss.
2. Otherwise the item is graded against **Accepted Items**, giving `Exact`,
   `Related` or `No Match`.

**On an Exact match:** the item is stored to `Placed Item Blackboard Key` if set,
`On Correct Item Placed` fires, and the task re-counts the zone.

**On anything else:** `On Wrong Item Placed` fires with the match quality, a mistake
description is built ("Placed CO2 Extinguisher in Extinguisher Bay - expected
Item.Extinguisher.Foam"), and the **Wrong Item Policy** is applied.

### Completion

The task re-counts on every settle, and finishes with `Succeeded` when the number of
**Exact-matching** items currently in the zone reaches `Required Count`.

Because it counts what is *currently* there rather than what has *ever* been there:

- The right item already sitting in the zone when the task starts **counts
  immediately** — the task evaluates the zone's contents at start.
- Taking a correct item **back out** decrements the count again.

### Re-reporting a wrong item

With `Report Each Wrong Item Once` on (the default), a wrong item is reported once.
But **taking it back out of the zone clears that memory**, so if the trainee removes
it and puts it back, they are told again. That is deliberate — a repeated mistake is
worth repeating the feedback for.

---

## Wrong Item Policy

| Policy | Behaviour |
|---|---|
| **Ignore (Keep Waiting)** | Silent. Use when wrong items are simply irrelevant. |
| **Count Mistake (Keep Waiting)** | Records a `SimFlow.Mistake.WrongItem` mistake and keeps waiting. **Default** — lets the trainee correct themselves. |
| **Count Mistake And Fail Task** | Records the mistake and fails, driving the `Failed` pin. Use for high-stakes assessment. |

---

## Delegates

| Delegate | Signature | Use for |
|---|---|---|
| **On Correct Item Placed** | (Item, Placed Count) | A chime, a green highlight, a progress counter |
| **On Wrong Item Placed** | (Item, Match Quality) | Feedback — **a near miss deserves a different hint from a random object** |

`Get Resolved Zone` and `Get Accepted Count` are available while running.

---

## Dependencies

| Depends on | Why |
|---|---|
| A [SimFlow Zone](../zones.md) in the level | **Hard dependency** — the task fails without one |
| [Identity](../identity.md) on the items | Tag queries need identity tags |
| [Actor Query](../actor-query.md) | All three targeting fields |
| Held state wiring | Only if `Require Settled` is on and your framework does not reparent on grab |

---

## Example use case: the right extinguisher in the bay

**Goal:** the trainee must put the **foam** extinguisher in the bay. The CO2 unit is
a plausible wrong answer that should be called out specifically.

**Setup**

1. **Tags.** Project Settings → Gameplay Tags: add `Zone.ExtinguisherBay`,
   `Item.Extinguisher.Foam`, `Item.Extinguisher.CO2`.
2. **The zone.** Place a [SimFlow Zone](../zones.md) at the bay, size its Box, and
   set its Identity tags to `Zone.ExtinguisherBay`, Display Name
   `Extinguisher Bay`.
3. **The items.** On `BP_Extinguisher_Foam` add a
   [SimFlow Identity](../identity.md) with `Item.Extinguisher.Foam` and Display Name
   `Foam Extinguisher`. Same for the CO2 one.
4. **Held state.** In your grab logic call `Set Held(true)` on grab and
   `Set Held(false)` on release.

**The task**

5. Add a [Task node](../nodes/task.md) and set **Task** to **Place Object In Zone**.
6. **Zone → Required Tags** = `Zone.ExtinguisherBay`.
7. **Accepted Items → Required Tags** = `Item.Extinguisher.Foam`.
8. Leave **Required Count** at `1` and **Require Settled** on.
9. Set **Wrong Item Policy** to **Count Mistake (Keep Waiting)**.
10. Set **Instruction** to `Place the foam extinguisher in the bay`.
11. Set **Score On Success** to `10`.

**Feedback**

12. Bind **On Wrong Item Placed** and switch on the match quality:
    - `Related` → *"Close — that's the CO2 unit. You want foam."*
    - `No Match` → *"That doesn't belong in the extinguisher bay."*

The CO2 unit scores `Related` automatically, because it shares
`Item.Extinguisher` with the accepted tag at the default
[Min Related Tag Depth](../actor-query.md#min-related-tag-depth) of 2.

> **Screenshot needed:** the Details panel of a Task node with Place Object In Zone
> selected, showing the Zone and Accepted Items queries filled in.

### Variant: call out the decoy explicitly

To accept *any* extinguisher except the CO2 one:

- **Accepted Items → Required Tags** = `Item.Extinguisher`
- **Rejected Items → Required Tags** = `Item.Extinguisher.CO2`

The rejection is checked first, so the CO2 unit is a mistake even though it matches
`Item.Extinguisher`.

---

## Common pitfalls

**The task fails the instant it starts.**
The `Zone` query is empty or resolves to nothing. Check the log for *"could not
resolve a SimFlow Zone"*. Confirm the zone actor is in the level, has the tag you
asked for, and that the level is loaded.

**The task never completes, no matter what is placed.**
`Accepted Items` is empty — nothing can match an unset query. Fill it in.

**Items never register as placed.**
They are not settling. Usually held state is not wired up and your VR framework
holds by constraint rather than attachment. Turn on the zone's `Draw Debug` and see
[Zones · Settling](../zones.md#settling--how-an-object-is-judged-put-down).

**The correct item is placed but nothing happens.**
Its identity tags do not match `Accepted Items`, or `Require All Tags` is on with
several tags requested. Check with `SimFlow.Debug 1`.

**Wrong items are reported over and over.**
`Report Each Wrong Item Once` is off — or the trainee keeps taking the item out and
putting it back, which intentionally re-arms the report.

**Everything wrong is "No Match", never a near miss.**
Your tag hierarchy is too flat. See
[Identity · Depth matters](../identity.md#depth-matters-more-than-you-would-expect).

**The wrong zone is being watched.**
Two zones share a tag. Tag queries resolve to the first world match. Give them
distinct tags.

**The task completes immediately on start.**
The correct item was already in the zone. The task evaluates existing contents at
start, by design.

---

*See also: [Zones](../zones.md) · [Actor Query](../actor-query.md) ·
[Identity System](../identity.md) · [Task node](../nodes/task.md) ·
[Task Reference](README.md)*
