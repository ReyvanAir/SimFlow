# Driving any widget

**Library:** `USimFlowComponent`, `USimFlowStatics`
**Needs a target?** The component, yes. The two lookup nodes, no.

[Status Widget](status-widget.md) is convenience, not access. Every function on it
forwards to `USimFlowComponent` behind a null check, and that component is public.
When another plugin owns your widget's parent class, the task name, the countdown,
progress and score are all still yours — you take over the three chores the base
class was handling.

## What the base class does for you

1. Finds a `SimFlow Component` on construct, and keeps retrying while none exists.
2. Binds eight delegates on it, and unbinds them on destruct.
3. Wraps the getters so an unbound flow returns empty text.

Only the third needs replacing, and Blueprint half does it: a getter called on an
invalid object reference returns the type's default rather than crashing. Guard with
**Is Valid** anyway — otherwise an unbound panel shows `0` as though that were a real
score.

## 1. Get the component

Both nodes are static, so there is no target pin to satisfy.

```
Event Construct
   └─▶ Get Primary Flow                    the first running flow
          └─▶ Is Valid ──▶ SET Flow        variable of type SimFlow Component

   or, when the level runs more than one:

   └─▶ Find Flow By Id ( "MainTutorial" )  matches Flow Save Id
```

Create the widget during `BeginPlay` and it can easily beat the flow actor to it —
**Get Primary Flow** comes back null. That race is why the base class re-checks on
every tick. In your own widget, use a repeating 0.25 s timer that retries the lookup
and clears itself once the variable is valid.

## 2. Bind the events

Drag off the component variable and choose **Assign**. That creates the binding and a
matching custom event in one step.

| Event | Carries | Use it for |
|---|---|---|
| On Task Started | Node, Task | Refresh the instruction and task name. The one most panels need |
| On Task Finished | Node, Task, Result | Tick marks, a result flash, advancing a step list |
| On Task Retried | Node, Task | Reset per-task UI without treating it as a new task |
| On Flow Started | — | Show the panel |
| On Flow Paused / Resumed | — | Swap the pause button's icon |
| On Flow Finished | Final State | The debrief. [Scenario records](scenario-records.md) are already written |
| On Checkpoint Reached | Checkpoint | A "progress saved" toast |
| On Quiz Presented | Quiz | Build the answer buttons |
| On Net State Changed | — | The signal clients need |

Open the panel mid-run and it has already missed **On Task Started**, so it sits
blank until the next task begins. Right after binding, call **Get Current Task** and
run your refresh once by hand. The base class does exactly this on the last line of
`BindToFlow`.

Unbind on **Event Destruct**. Otherwise a panel taken off the viewport mid-run leaves
the component holding a reference to it.

## 3. Read the values

All pure, all safe on a Text Block or Progress Bar binding.

| On the base class | On the component |
|---|---|
| Get Task Name | Get Current Task Name |
| Get Instruction | Get Current Instruction |
| Get Progress | Get Progress |
| Get Score | Get Score |
| Get Time Remaining Text | Get Current Task Remaining Time → Format Seconds |
| Is Flow Paused | Is Flow Paused |
| Get Current Quiz | Get Current Quiz |
| Can Retry | Get Current Task → Can Retry |
| Can Skip | Get Current Task → Allow Skip |
| — | Get Run State → Run State To Text |
| — | Get Scenario Record |
| — | Get Debug Text |

**Get Current Task Remaining Time** returns seconds, or `-1` when the task has no
time limit. Branch on that before formatting, or an untimed task displays `-00:01`.

```
Get Current Task Remaining Time
   └─▶ Branch ( Remaining >= 0 )
          True  ──▶ Format Seconds ──▶ Set Text        "01:23"
          False ──▶ Set Visibility ( Collapsed )
```

**Get Scenario Record**, **Get High Score** and **Get Play Count** are the exception:
each opens a save file. Read them once when the panel opens or on **On Flow
Finished**, and keep the result in a variable. See
[scenario records](scenario-records.md).

## 4. Wire the buttons

| Button | Direct | Network-safe |
|---|---|---|
| Pause | Toggle Pause | Request Flow Control ( id, Toggle Pause ) |
| Retry | Retry Current Task | Request Flow Control ( id, Retry Current Task ) |
| Skip | Skip Current Task | Request Flow Control ( id, Skip Current Task ) |
| Restart | Restart Flow | Request Flow Control ( id, Restart ) |
| Quiz answer | Submit Quiz Answer ( index ) | Already forwards to the server itself |

In single player the two columns behave identically. In multiplayer the direct calls
do nothing useful on a client — [Request Flow Control](control-and-save.md) routes
through the [Player Component](player-component.md), and falls back to a direct call
when it is already on the server.

## On a client, refresh on the network signal

With `Replicate Flow` on, a client holds a mirror that updates when the server sends
state, not continuously. **On Net State Changed** is the moment to re-read. Bindings
pick it up on their own; anything cached in a variable does not.

The countdown is the useful exception. **Get Current Task Remaining Time** is computed
from the replicated start time, so on a client it keeps counting down smoothly between
updates instead of stepping.

## Where to put it

Blueprint has no multiple inheritance, so pick one:

**In the widget graph.** Variable, lookup on construct, bindings, unbind on destruct,
all in the foreign widget's own event graph. Right when one widget shows the flow.

**A small Blueprint helper.** One Actor Component holding the lookup, the bindings and
its own re-broadcast events; each widget binds to that. Right when a HUD, a wrist
panel and a debrief screen all want the same flow. You write the retry-and-catch-up
logic once.

Start in the widget graph. Move to a helper when a second widget needs the same thing,
not in anticipation of one.

## Checklist

1. Component variable, set from **Get Primary Flow** or **Find Flow By Id**.
2. Retry the lookup on a short timer until it is valid, then clear the timer.
3. Assign the events you need. **On Task Started** is usually enough.
4. Refresh once from **Get Current Task** the moment you bind.
5. Bind text and bars to the pure getters, guarded with **Is Valid**.
6. Branch the countdown on `>= 0` before formatting.
7. Buttons go through **Request Flow Control** if the project is ever networked.
8. Unbind on **Event Destruct**.

## When it misbehaves

**The panel never populates.** The lookup ran once on construct and found nothing. Add
the retry timer.

**It populates only from the second task onwards.** No catch-up call after binding.

**Everything reads 0 or blank on a client.** `Replicate Flow` is off, so the client has
no mirror to read.

**The countdown shows a negative time.** The task has no time limit and the `-1` was
formatted anyway.

**Buttons work in PIE standalone but not as a client.** Direct calls instead of
**Request Flow Control**, or no Player Component on the PlayerController.

*Next: [Status Widget](status-widget.md) · [Flow access](flow-access.md) ·
[Scenario records](scenario-records.md)*
