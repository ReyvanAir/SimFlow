# Identity System

**Component:** `SimFlow Identity` (`USimFlowIdentityComponent`)
**Add via:** Add Component → SimFlow Identity
**Class group:** SimFlow

---

## Overview / Purpose

The Identity system answers one question: **what is this object?**

Without it, a flow can only refer to objects by pointing at a specific actor placed
in a level. That breaks the moment an object is spawned at runtime, duplicated, or
swapped for a different variant. Identity replaces "that actor over there" with
"any foam extinguisher", which survives all three.

An identity component carries **gameplay tags** describing what its actor *is*.
A task then asks for `Item.Extinguisher` and any foam or CO2 extinguisher answers,
or asks for `Item.Extinguisher.Foam` and only the foam one does.

**Why gameplay tags rather than the actor's own `Tags` array:** gameplay tags are
validated at author time, they autocomplete in the Details panel, and — critically —
they *nest*. That nesting is what lets a task tell a near miss (`Item.Extinguisher.CO2`
when it wanted `Item.Extinguisher.Foam`) apart from something completely wrong
(`Item.Wrench`). See [Match quality](actor-query.md#match-quality).

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Identity Tags** | Gameplay Tag Container | *empty* | Effectively yes | What this object is, e.g. `Item.Extinguisher.Foam`. Several tags are fine. |
| **Display Name** | Text | *empty* | No | Shown in mistake text and tutorial UI. Falls back to the actor's name when empty. |
| **Identity Id** | Name | `None` | No | A stable id for **your own** analytics and script. **Nothing in SimFlow reads it** — see below. |

### Identity Tags

The load-bearing field. Everything else on the component is optional.

- Leave it **empty** and the actor is still *tracked* by zones (it has an identity
  component, which is what `Require Identity Component` checks), but it can never
  satisfy a tag-based [Actor Query](actor-query.md) — it has no tags to match. In
  practice an identity component with no tags is almost always a mistake.
- Multiple tags are allowed and are frequently useful: an object can be both
  `Item.Extinguisher.Foam` and `Item.Heavy`.
- Matching against multiple tags is controlled by the *query's*
  `Require All Tags` setting, not by anything on this component.

### Display Name

Purely presentational. It appears in:

- generated mistake descriptions — *"Placed **CO2 Extinguisher** in the bay —
  expected Foam Extinguisher"*
- anything you build that calls `Get Identity Display Name`

When empty, SimFlow falls back to the actor's name (`AActor::GetName()`), which is
typically something like `BP_Extinguisher_C_2`. That is fine for debugging and poor
in front of a trainee, so set it on anything a trainee might get wrong.

### Identity Id — read this before using it

`Identity Id` is a free-form Name field that **no part of SimFlow reads**.

It is not a query field ([Actor Query](actor-query.md) has no `Identity Id`
option), it is not written into save data, and no built-in task or condition
consults it. Setting it has no effect on flow behaviour whatsoever.

What it is *for*: a stable, human-chosen handle you can read from your own Blueprint
or C++ — for analytics events, telemetry, external LMS reporting, or addressing a
specific actor from your own script. Read it directly off the component:

```
Get Component By Class (SimFlow Identity) → Identity Id
```

**Do not** expect it to make a task find an object. To make an object findable by a
flow, give it **Identity Tags**.

> If you want tasks to be able to target one specific instance, the supported ways
> are the query's `Specific Actor` field (for a placed level actor) or
> `Blackboard Key` (for one chosen at runtime). See
> [Actor Query](actor-query.md#which-field-to-use).

---

## Held state — Set Held / Is Held

Two functions on the component, and the part most VR projects need:

| Function | Type | Purpose |
|---|---|---|
| **Set Held** (`bIn Is Held`) | Callable | Flags this object as currently held by the trainee. |
| **Is Held** | Pure | True while something is holding this object. |

**Why this exists.** A [Zone](zones.md) cannot reliably work out for itself whether
an object is in someone's hand. Attachment is only *one* of the ways a VR framework
can hold an object — VRExpansion, for example, holds most grip types with a physics
constraint and never reparents the actor. So an object sitting in the trainee's hand
can look perfectly detached from the outside, and a zone would wrongly judge it as
"placed".

**What to do:** call `Set Held (true)` from wherever your grab succeeds, and
`Set Held (false)` on release. Placement checks then become exact regardless of how
grabbing is implemented in your project.

The held flag is **transient** — it is not saved, and it resets to `false` when the
game starts. That is correct: nothing is being held at load time.

> Held state is checked *before* the attachment fallback. An object flagged held is
> never at rest, whatever its attachment or velocity says. See
> [Zones · Settling](zones.md#settling--how-an-object-is-judged-put-down).

---

## How identities are created

### On the Blueprint, not the level instance

Add the component to the **item Blueprint**, not to each copy you place in the
level. Every instance you place or spawn then carries the same tags automatically.

1. Open your item Blueprint (e.g. `BP_Extinguisher_Foam`).
2. **Add Component → SimFlow Identity**.
3. Select it, and in the Details panel set **Identity Tags**.
4. Set **Display Name** to something a trainee would recognise.
5. Compile and save.

That is the whole setup. There is no registration step, no manifest, and no
subsystem to notify — tasks and zones discover identity components by looking at
the actor directly.

### Already using Gameplay Ability System?

If your actors already implement `IGameplayTagAssetInterface`, SimFlow reads those
tags too. `Get Identity Tags` returns the identity component's tags **and** the
interface's owned tags, appended together. A GAS project therefore does not need a
second component on every actor — though adding one is still the way to get
`Display Name` and held state.

---

## Adding custom identity tags

Identity tags are ordinary Unreal **gameplay tags**. SimFlow does not maintain a
separate tag registry, so you create them exactly the way you create any gameplay
tag in your project.

### The editor route (recommended for designers)

1. **Edit → Project Settings → Project → Gameplay Tags**.
2. Expand **Gameplay Tags** and click **Add New Gameplay Tag**.
3. Enter the tag name, e.g. `Item.Extinguisher.Foam`.
4. Optionally add a comment describing it.
5. The tag is written to `Config/DefaultGameplayTags.ini` in **your project** and is
   immediately available in every tag picker.

You can also add a tag inline: click any **Identity Tags** picker, then
**Add New Gameplay Tag** at the bottom of the dropdown.

### Tags shipped with the plugin

SimFlow ships these natively. They are available without any setup:

| Tag | Used for |
|---|---|
| `SimFlow.Event` | Parent of all built-in event tags |
| `SimFlow.Event.Generic` | General-purpose event |
| `SimFlow.Event.Interact` | "The trainee interacted with something" |
| `SimFlow.Event.Grab` | Object picked up |
| `SimFlow.Event.Release` | Object let go |
| `SimFlow.Event.ButtonPressed` | Button press |
| `SimFlow.Event.Placed` | Raised by a zone when an object settles (opt-in) |
| `SimFlow.Event.Removed` | Raised by a zone when an object leaves (opt-in) |
| `SimFlow.Mistake` | Parent of all mistake kinds |
| `SimFlow.Mistake.WrongItem` | Wrong object placed |
| `SimFlow.Mistake.WrongTarget` | Wrong object in an event payload |
| `SimFlow.Mistake.WrongOrder` | Acted out of turn |
| `SimFlow.Mistake.WrongAnswer` | Wrong quiz answer |
| `SimFlow.Sample.GrabExtinguisher` | Used by the sample flow |
| `SimFlow.Sample.PullPin` | Used by the sample flow |

Note what is **not** in that list: there are **no shipped `Item.*` or `Zone.*` tags**.
Those appear throughout the documentation as examples, but the hierarchy for your
own objects is yours to create. `Item.Extinguisher.Foam` will not exist in your
project until you add it.

> **Do not add your project's item tags under `SimFlow.`** Keep them in your own
> namespace (`Item.`, `Zone.`, `Tool.`, or your project's prefix). The `SimFlow.`
> hierarchy is the plugin's, and a future version may add tags under it.

---

## Naming and format restrictions

Identity tags follow Unreal's gameplay tag rules — SimFlow adds none of its own.

| Rule | Detail |
|---|---|
| **Hierarchy separator** | `.` — `Item.Extinguisher.Foam` is three levels deep |
| **No spaces** | The tag editor rejects them |
| **Must be registered** | A tag has to exist in the project's tag list before it can be selected; you cannot type an arbitrary string into a tag field |
| **Case** | Tags match case-insensitively but are stored with the case you enter — pick a convention and hold to it |
| **Depth** | No hard limit, but see the depth note below |
| **Leading/trailing dots** | Not permitted |

The editor validates as you type and will not let you create a malformed tag, so
these rules are mostly enforced for you rather than something to memorise.

### Depth matters more than you would expect

Tag depth is not cosmetic. The `Min Related Tag Depth` setting on every
[Actor Query](actor-query.md#min-related-tag-depth) decides how many *leading* tag
nodes two tags must share before a wrong answer counts as a **near miss** rather
than as completely wrong — and that difference drives the feedback a trainee gets.

At the default of `2`:

| Query asks for | Trainee provides | Shared depth | Result |
|---|---|---|---|
| `Item.Extinguisher.Foam` | `Item.Extinguisher.Foam` | — | **Exact Match** |
| `Item.Extinguisher.Foam` | `Item.Extinguisher.CO2` | 2 (`Item.Extinguisher`) | **Related (Near Miss)** |
| `Item.Extinguisher.Foam` | `Item.Wrench` | 1 (`Item`) | **No Match** |

So a flat hierarchy (`Extinguisher_Foam`, `Extinguisher_CO2`, `Wrench`) makes every
wrong answer identical and throws away the plugin's ability to say
*"right idea, wrong extinguisher"*. **Design your tag tree so that things which are
plausibly confusable share a parent.**

A workable convention:

```
Item.Extinguisher.Foam
Item.Extinguisher.CO2
Item.Extinguisher.Water
Item.Tool.Wrench
Item.Tool.Screwdriver
Zone.PartsBin
Zone.ExtinguisherBay
```

---

## Dependencies

| Depends on | Why |
|---|---|
| Nothing at runtime | The component is self-contained; no subsystem or registration |
| **Gameplay Tags** (engine) | The tags themselves live in your project's tag list |

**Depended on by:**

- [Actor Query](actor-query.md) — reads identity tags to grade a match
- [Zones](zones.md) — `Require Identity Component` and settling both consult it
- [Place Object In Zone](tasks/place-object-in-zone.md) — judges placed items by identity
- [Wait For Event](tasks/wait-for-event.md) — checks event payloads against identity
- [Ordered Sequence](tasks/ordered-sequence.md) — matches each step's target

---

## Example use case: making an extinguisher recognisable

**Goal:** a flow can ask for "the foam extinguisher" and get the right answer even
for copies spawned at runtime.

1. **Create the tags.** Project Settings → Gameplay Tags → add
   `Item.Extinguisher.Foam` and `Item.Extinguisher.CO2`.
2. **Open `BP_Extinguisher_Foam`.** Add Component → SimFlow Identity.
3. Set **Identity Tags** to `Item.Extinguisher.Foam`.
4. Set **Display Name** to `Foam Extinguisher`.
5. **Repeat for `BP_Extinguisher_CO2`** with `Item.Extinguisher.CO2` and
   `CO2 Extinguisher`.
6. **Wire up held state.** In whatever handles grabbing, on a successful grab call
   `Set Held (true)` on the grabbed actor's identity component; on release call
   `Set Held (false)`.
7. Place both extinguishers in the level.

A [Place Object In Zone](tasks/place-object-in-zone.md) task asking for
`Item.Extinguisher.Foam` now accepts the foam one, and reports the CO2 one as a
**near miss** rather than as a random wrong object — so your feedback can say
*"close — that is the CO2 unit, you want foam"*.

> **Screenshot needed:** the Details panel of `BP_Extinguisher_Foam` showing the
> SimFlow Identity component with Identity Tags and Display Name filled in.

---

## Common pitfalls

**The task never completes and nothing is logged.**
The actor has no identity component, or has one with no tags. A tag query against an
actor with no identity tags scores `No Match` silently. Confirm with
`Actor Has Identity Tag` in a debug print.

**Everything is a "No Match", never a near miss.**
Your tag hierarchy is too flat, or `Min Related Tag Depth` is higher than your tags
are deep. See [depth matters](#depth-matters-more-than-you-would-expect).

**Objects register as placed while still in the trainee's hand.**
Held state is not wired up. Call `Set Held` from your grab logic — attachment alone
is not reliable in VR. See [Held state](#held-state--set-held--is-held).

**I set an Identity Id and the task still cannot find the object.**
`Identity Id` is not used by anything in SimFlow. Use **Identity Tags**. See
[above](#identity-id--read-this-before-using-it).

**Tags added to the item in the level do not apply to spawned copies.**
You added the component to a placed instance rather than to the Blueprint. Move it
to the Blueprint so every copy inherits it.

**A GAS actor matches tags it should not.**
`Get Identity Tags` merges identity-component tags with `IGameplayTagAssetInterface`
owned tags. If your GAS actor owns a tag that collides with your item hierarchy, it
will match. Keep the two namespaces apart.

---

*See also: [Actor Query](actor-query.md) · [Zones](zones.md) ·
[Place Object In Zone](tasks/place-object-in-zone.md) ·
[Documentation index](README.md) · [Glossary](glossary.md)*
