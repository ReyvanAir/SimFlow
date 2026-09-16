# Flow Instance

**Class:** `USimFlowInstance`
**Get a reference:** `Get Flow Instance` on the
[SimFlow Component](../simflow-component.md)
**Glossary:** [asset against instance](../glossary.md)

The instance is the running copy of a flow asset. It owns the duplicated nodes, the
[blackboard](../blackboard.md), the mistake log and the set of raised events.

Most projects never touch it. The [component](../simflow-component.md) forwards the
things you usually want, and it does so safely. Come here when you need what the
component doesn't expose: the list of active nodes, the mistake record, whether a
particular event has fired.

## Read this before calling anything that changes state

Component functions check authority and route client calls to the server. Instance
functions do neither. They act immediately on whichever machine runs them.

In single player that distinction doesn't exist. With **Replicate Flow** on it
matters a lot: calling `Pause Instance` on a client pauses the client's mirror, your
UI updates, and the next server refresh silently undoes it.

| Instead of | Call |
|---|---|
| Pause Instance / Resume Instance | `Pause Flow` / `Resume Flow` on the component |
| Stop Instance | `Stop Flow` |
| Retry Active Tasks | `Retry Current Task` |
| Skip Active Tasks | `Skip Current Task` |
| Fail Active Tasks | `Fail Current Task` |
| Raise Event | `Send Event`, or [Broadcast Flow Event](../events.md) |

The read-only nodes further down are safe anywhere.

## Lifecycle

| Node | Returns | Notes |
|---|---|---|
| Start Instance (Entry Name) | bool | `None` starts from the first entry |
| Pause Instance | | Paused nodes stop ticking |
| Resume Instance | | |
| Stop Instance | | Ends the flow reporting **Aborted** |

`Stop Instance` reports Aborted, not Failed. If you want the run recorded as a
failure, finish through a [Finish](../nodes/finish.md) node set to Failed instead.

## Player controls

| Node | Returns |
|---|---|
| Retry Active Tasks | How many were restarted |
| Skip Active Tasks | How many were skipped |
| Fail Active Tasks | How many were failed |

The count is the useful part. One flow can have several tasks running at once
through [Parallel](../nodes/parallel.md), so "retry the current task" is really
"retry all of them". A return of `0` means nothing was eligible — either no task is
active, or every active task has `Allow Retry` off.

Retry and skip honour the per-task `Allow Retry`, `Max Retries` and `Allow Skip`
fields. Fail doesn't ask permission.

## Events

| Node | Notes |
|---|---|
| Raise Event (Event Tag, Payload) | Raises a tag on this instance only |
| Was Event Raised (Event Tag) | Has this tag fired at any point? |
| Clear Raised Events | Forgets all of them |

`Raise Event` wakes any [Wait For Event](../tasks/wait-for-event.md) task listening
for that tag, then immediately processes anything that unblocked. An invalid tag is
ignored.

The raised set accumulates for the life of the instance and is never cleared for
you. That's deliberate — it's what lets the
[Event Was Raised](../conditions.md#event-was-raised) condition ask about something
that happened five tasks ago. It also means a loop checking the same tag on every
pass sees it as raised forever after the first time. Call `Clear Raised Events` at
the top of the loop body if you need it to re-arm.

Matching follows the tag hierarchy in one direction. Raising
`Sim.Grab.Extinguisher` makes `Was Event Raised(Sim.Grab)` true. Raising `Sim.Grab`
does not make `Was Event Raised(Sim.Grab.Extinguisher)` true.

## Mistakes

| Node | Returns |
|---|---|
| Record Mistake (Kind, Involved, Description, Severity, Task Id) | |
| Get Mistakes | Every mistake, in order |
| Get Mistake Count | int32 |
| Get Mistakes Of Kind (Kind, Match Child Tags) | Filtered |
| Clear Mistakes | |

`Record Mistake` also bumps the `Mistakes` blackboard key, so conditions written
against that counter keep working while the array holds the richer record a debrief
screen needs.

`Severity` is an [ESimFlowMatchQuality](../actor-query.md) — `No Match` for
something unrelated, `Related (Near Miss)` for the right family and wrong item.
Getting this right is what lets a debrief say "you reached for the CO2 extinguisher"
rather than "wrong object".

`Clear Mistakes` empties the array. It does not reset the `Mistakes` blackboard key,
so clear that separately if your conditions read it.

## Save and load

| Node | Returns |
|---|---|
| Save Instance State | A save-state struct |
| Load Instance State (State, Load Mode) | bool |

The raw struct form, below what the
[component](../simflow-component.md#save-and-load) offers. Use these when you're
writing the bytes into your own SaveGame rather than SimFlow's slot.

`Load Mode` is **Exact State** or **From Last Checkpoint**.

## Reading state

| Node | Returns |
|---|---|
| Get Template | The flow asset this is a copy of |
| Get Blackboard | The [blackboard](blackboard-nodes.md) |
| Get Owning Component | The [component](../simflow-component.md) running it |
| Get Run State | `Not Started`, `Running`, `Paused`, `Completed`, `Failed`, `Aborted` |
| Is Running / Is Paused | bool |
| Get Elapsed Time | Seconds since the flow started |
| Get Last Task Result | The result of the task that finished most recently |
| Get Entry Name | Which [Start](../nodes/start.md) node this run began from |
| Get Progress | 0–1, by completed task nodes |
| Get Current Task | The first active task |
| Get Active Nodes | Every active node |
| Get Active Task Nodes | Only the active [Task](../nodes/task.md) nodes |
| Find Runtime Node (Guid) | The running copy of an authored node |
| Get Last Checkpoint Guid | The last [checkpoint](../nodes/checkpoint.md) passed |

`Get Owning Component` walks up through parent instances, so a
[Sub Flow](../nodes/sub-flow.md) child returns the component running the whole tree
rather than null.

`Get Progress` counts task nodes that have completed against every task node in the
asset. Branches the trainee never took still count in the denominator, so a flow
with a large optional section reports lower than it feels. It's a progress bar, not
a measurement.

`Get Active Task Nodes` returning more than one entry means
[Parallel](../nodes/parallel.md) is running. Single-track tutorial UI should use
`Get Current Task`, which gives you the first.

`Find Runtime Node` takes an authored node's Guid and hands back the live duplicate.
This is what the networking layer uses — clients receive only a Guid and resolve the
display text locally.

## Dispatchers

Eleven events fire on the instance. Ten are mirrored onto the
[component](../simflow-component.md#events), which is the better place to bind from
because the component survives a restart and the instance doesn't.

Two are worth coming here for:

| Dispatcher | Carries | Why it's only here |
|---|---|---|
| On Event Raised | Event Tag, Payload | The component doesn't forward it |
| On Mistake Recorded | The mistake struct | Drive live feedback the moment something goes wrong |

The rest — On Flow Started, On Flow Paused, On Flow Resumed, On Flow Finished, On
Task Started, On Task Finished, On Task Retried, On Checkpoint Reached, On Quiz
Presented — bind from the component.

A new instance is built every time a flow starts. Bindings made to the previous one
are gone after a restart, which is the main reason to prefer the component.

## A debrief screen

Reading the run after **On Flow Finished**:

1. From the component, **Get Flow Instance**.
2. **Get Mistakes** for the list, or **Get Mistakes Of Kind** with
   `SimFlow.Mistake.WrongItem` for one category.
3. **Get Elapsed Time** into [Format Seconds](values.md) for a duration.
4. **Get Run State** into [Run State To Text](values.md) for the outcome.
5. **Get Blackboard → Get Score** for the score.

Do this on **On Flow Finished** rather than later. The instance stays valid until
the flow restarts, and then it's replaced.

## When it misbehaves

**Get Flow Instance returns null.** The flow hasn't started. The instance is created
on start, not when the component is added.

**A pause from a client reverts after a second.** You called `Pause Instance`
instead of the component's `Pause Flow`.

**Retry Active Tasks returns 0.** No task is active, or `Allow Retry` is off on the
ones that are.

**Was Event Raised is true when it shouldn't be.** The set is never auto-cleared.
Call `Clear Raised Events` when you want it re-armed.

**Get Progress jumps unevenly.** It counts task nodes in the asset, including
branches this run skipped.

**Bindings stopped firing after a restart.** The instance was replaced. Bind to the
component instead.

**Get Mistake Count and the `Mistakes` blackboard key disagree.** `Clear Mistakes`
empties the array without touching the key.

*Next: [SimFlow Component](../simflow-component.md) ·
[Blackboard nodes](blackboard-nodes.md) · [Events](../events.md)*
