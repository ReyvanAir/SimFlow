# Zones and identity

**Libraries:** `USimFlowIdentityStatics`, and queries on `ASimFlowZone`
**Concept pages:** [Identity](../identity.md) · [Actor Query](../actor-query.md) ·
[Zones](../zones.md)

Tasks already do this judging for you. These nodes exist for the cases where your
own Blueprint needs the same answer — a highlight that only appears on the right
object, a hint system, an interaction that behaves differently depending on what the
player is holding.

## Identity

Static, so no target pin.

| Node | Returns |
|---|---|
| Get Identity Tags (Actor) | Every identity tag on the actor |
| Get Identity Display Name (Actor) | The authored name, falling back to the actor's name |
| Actor Has Identity Tag (Actor, Tag) | bool |

`Get Identity Tags` reads the [SimFlow Identity](../identity.md) component first,
then falls back to `IGameplayTagAssetInterface`. Actors from a GAS project answer
correctly without a second component on them.

`Get Identity Display Name` is what your prompts should use. "Pick up the foam
extinguisher" reads better than the actor label `BP_Ext_Foam_2`, and it stays
correct when someone renames the Blueprint.

## Grading against a query

| Node | Returns |
|---|---|
| Match Actor Against Query (Actor, Query, Instance) | `No Match`, `Related (Near Miss)`, `Exact Match` |
| Actor Satisfies Query (Actor, Query, Instance) | bool |

`Actor Satisfies Query` is the yes-or-no form. `Match Actor Against Query` gives you
the middle value, which is the whole point of the enum — a CO2 extinguisher where a
foam one was asked for is a different kind of wrong from a wrench, and your feedback
can say so.

The `Instance` pin can be null when the query doesn't use a `Blackboard Key`. When
it does, pass the [flow instance](instance.md) or the query can't resolve it and
will never match.

```
On Grab ──▶ Match Actor Against Query ( Grabbed , Required , Get Flow Instance )
              ├── Exact Match   ──▶ green outline
              ├── Related       ──▶ amber outline + "close, but check the label"
              └── No Match      ──▶ nothing
```

Doing the grading in your own Blueprint like this gives the trainee feedback before
they commit, while the task keeps doing the actual assessment.

## Zone contents

Target pin is the zone actor.

| Node | Returns |
|---|---|
| Get Contained Actors | Everything overlapping, settled or not |
| Get Settled Actors | Everything put down and left alone |
| Contains Actor (Actor) | bool |
| Is Actor Settled (Actor) | bool |
| Get Display Name Text | The zone's authored name |

The contained/settled split is the thing to get right. An actor the player is
holding inside the zone is *contained* but not *settled*. Tasks judge on settled,
which is why waving an extinguisher over the bay doesn't count as placing it.

Use contained for live feedback — highlighting the bay while something hovers over
it — and settled for anything that decides an outcome.

## Effective settle values

| Node | Returns |
|---|---|
| Get Effective Settle Time | Still-time the current mode requires |
| Get Effective Settle Speed | Speed limit the current mode applies. `0` means no check. |
| Get Effective Require Detached | Whether an attached actor still counts as held |

A zone's settle behaviour comes from a mode, with **Custom** starting from the
standard values (0.35s and 20 units/s). These three nodes report what's actually
being applied after that resolution, rather than the fields you see in the Details
panel.

Use them for a debug readout, or to drive a progress ring showing how long until an
object counts as placed:

```
Get Effective Settle Time ──▶ [ how long the ring takes to fill ]
```

## Zone events

Three dispatchers, all carrying the actor:

| Dispatcher | Fires |
|---|---|
| On Actor Entered | The moment an actor overlaps, before settling |
| On Actor Exited | It leaves |
| On Actor Settled | It's genuinely put down. This is the one tasks listen to. |

Bind `On Actor Entered` for hover feedback and `On Actor Settled` for anything that
counts.

A zone can also raise `SimFlow.Event.Placed` and `SimFlow.Event.Removed` on every
flow, but only when its **Broadcast Flow Events** is ticked. Off by default.

## Highlighting the right object

Making a grabbable prop glow when it matches what the current task wants:

1. Store the task's `Actor Query` where your prop can read it, or expose one on the
   prop itself.
2. On overlap with the player's hand, call **Match Actor Against Query** with the
   prop and the [flow instance](instance.md).
3. Switch on the result: green for Exact Match, amber for Related, nothing for No
   Match.

Amber on a near miss is worth the extra branch. It tells the trainee they're in the
right family without telling them the answer.

## When it misbehaves

**Every actor scores No Match.** The query uses a `Blackboard Key` and you passed
null for `Instance`.

**A query with no tags matches everything.** An empty query is legitimately
satisfied by anything, and nothing logs. See [actor query](../actor-query.md).

**Get Identity Tags is empty on an actor that clearly has tags.** The tags are on a
component the fallback doesn't reach. Add a SimFlow Identity component.

**Get Settled Actors is empty while the object is visibly in the zone.** It hasn't
settled. It's still held, still moving, or still attached with `Require Detached`
on.

**A widget passed to Match Actor Against Query never matches.** A `UUserWidget` is
neither an Actor nor an Actor Component. Pass the owning actor — see
[payloads that are not actors](../actor-query.md#payloads-that-are-not-actors).

**The zone's settle time doesn't match the Details panel.** The mode is overriding
it. Check `Get Effective Settle Time`.

*Next: [Actor Query](../actor-query.md) · [Zones](../zones.md) ·
[Identity](../identity.md)*
