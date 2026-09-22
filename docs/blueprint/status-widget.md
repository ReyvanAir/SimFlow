# Status Widget

> Reparenting is not the only route. When another plugin already owns your widget's
> parent class, [driving any widget](any-widget.md) covers the same ground without
> this base class.

**Class:** `SimFlow Status Widget` (`USimFlowStatusWidget`), abstract
**Use it by:** reparenting your UMG widget Blueprint to it
**Works as:** a screen-space HUD or a world-space VR panel

A base class for tutorial and status panels. Reparent your widget to it and you get
the flow lookup, the event bindings and the button handlers already wired — you
supply the layout.

The alternative is calling [Get Primary Flow](flow-access.md) yourself and binding
every dispatcher by hand. This class is that code, already written.

## Setup

1. Open your widget Blueprint.
2. **File → Reparent Blueprint → SimFlow Status Widget**.
3. Leave **Auto Bind** on.
4. Set **Flow Save Id** if you have more than one flow, or leave it `None` to follow
   the primary one.

Two properties, both editable and Blueprint-writable:

| Property | Default | Meaning |
|---|---|---|
| Flow Save Id | `None` | Which flow to watch. `None` follows the primary flow. |
| Auto Bind | `true` | Keep looking every frame until a flow turns up |

`Auto Bind` is what makes the widget survive being created before the flow starts.
It re-checks on tick until it finds one, then stops.

## Binding

| Node | Notes |
|---|---|
| Bind To Flow (Component) | Point the widget at a specific flow |
| Unbind From Flow | Stop watching |
| Get Bound Flow | The component, or null |

Call `Bind To Flow` when you want to choose the flow yourself — an instructor panel
switching between scenarios, say. It unbinds the old flow first, so calling it
repeatedly is fine.

Turn `Auto Bind` off if you do this, or the widget drifts back to the primary flow.

## Reading state

All pure, all safe when nothing is bound — they return empty text, zero or false.

| Node | Returns |
|---|---|
| Get Task Name | The current task's display name |
| Get Instruction | The current task's instruction text |
| Get Progress | 0–1 |
| Get Score | The `Score` blackboard key |
| Get Time Remaining Text | Already formatted as `MM:SS` |
| Is Flow Paused | bool |
| Can Retry | Whether the current task allows it |
| Can Skip | Whether the current task allows it |

Wire these to property bindings on your text blocks and progress bars. They resolve
against the flow asset rather than replicated text, so they read correctly on
clients.

`Get Time Remaining Text` does the [Format Seconds](values.md) conversion for you,
including the `-1` case for a task with no time limit.

`Can Retry` and `Can Skip` are what your buttons' **Is Enabled** bindings want. A
greyed-out skip button on a task that doesn't allow skipping beats a button that
does nothing.

## Buttons

| Node | Notes |
|---|---|
| Request Pause Toggle | |
| Request Retry | |
| Request Skip | |
| Request Restart | |
| Submit Quiz Answer (Option Index) | Answers the quiz on screen |
| Get Current Quiz | The quiz task, or null |

Wire these straight to **On Clicked**. Each routes through the flow's component, so
they forward to the server on a client without you doing anything.

`Get Current Quiz` returns the quiz task itself, so read `Question` and `Options`
directly off it to build the answer buttons.

## Design events

Override these in your widget's graph. They're the reason to use this class rather
than polling.

| Event | Fires when | Carries |
|---|---|---|
| On Flow Bound | The widget finds its flow | The component |
| On Flow Started | | |
| On Flow Paused | | |
| On Flow Resumed | | |
| On Flow Finished | | The final run state |
| On Task Started | A task begins | The task |
| On Task Finished | A task ends | The task and its result |
| On Quiz Presented | A quiz appears | The quiz task |
| On Flow State Replicated | Server state arrives on a client | |

**On Flow Bound** is where to do first-time setup, not Event Construct — at
construct time there may be no flow yet.

**On Flow State Replicated** only fires on clients, every time the server's state
arrives. Refresh anything you cached rather than recomputing on tick. Bindings that
call the query nodes above update on their own.

## Showing the previous task, and how it went

A panel that reads "previous step failed, current step running" needs something the
query nodes do not give you. `Get Task Name` and `Get Instruction` describe the task
running *now*; nothing holds on to the one before it.

Keep it yourself. **On Task Finished** hands you the task and its result together, so
three widget variables cover it:

```
Event On Task Finished ( Task , Result )
  ├─▶ Set Previous Task Name   = Task ▸ Get Display Name Text
  ├─▶ Set Previous Task Result = Result        ( ESimFlowResult )
  └─▶ Set Has Previous Task    = true
```

Render the result with [`Result To Text`](values.md), which covers all five values —
Succeeded, Failed, Skipped, Timed Out, Aborted:

```
Previous:  {Previous Task Name} — {Result To Text( Previous Task Result )}
Current:   {Get Task Name} — in progress
```

The ordering works in your favour. **On Task Finished** for one task fires before
**On Task Started** for the next, so by the time the new step begins, the cached
"previous" is already the one that just ended. It works in multiplayer too: the event
multicasts, resolving the task from its node id on every machine.

For the whole run rather than just the last step, make the variable an array of a
small struct — name plus result — and Add instead of Set. Same event, one node
different, and you have a debrief list at the end.

> `Get Flow Instance` › `Get Last Task Result` looks like the answer and is not. It
> gives the result with no task name, it starts life reading `Succeeded` before
> anything has run, and the instance is null on a client — which reads as `Succeeded`
> too. See [Flow Instance](instance.md).

For *why* a task failed, `Get Flow Instance` › `Get Mistakes` returns records carrying
a ready-to-show description such as "Placed CO2 Extinguisher — expected Foam
Extinguisher", keyed by task id. Server-side only.

## A VR wrist panel

1. Reparent the widget to **SimFlow Status Widget**.
2. Bind a text block to **Get Instruction**, another to **Get Task Name**.
3. Bind a progress bar to **Get Progress**.
4. Bind a countdown text block to **Get Time Remaining Text**.
5. Add retry and skip buttons, wire them to **Request Retry** and **Request Skip**,
   and bind their **Is Enabled** to **Can Retry** and **Can Skip**.
6. Override **On Quiz Presented** to swap to your quiz panel, building the buttons
   from **Get Current Quiz**.
7. Put the widget on a Widget Component on the player's wrist.

Nothing in there names a flow, so the same panel works in every level.

## When it misbehaves

**Everything is blank.** No flow is bound. Check `Auto Bind` is on, or that the
`Flow Save Id` matches a component in the level.

**It binds to the wrong flow.** `Flow Save Id` is `None` and several flows are
running, so it took the primary one.

**Bindings stop updating after a restart.** The widget rebinds on its own with
`Auto Bind` on. With it off, call `Bind To Flow` again.

**Buttons do nothing on a client.** The PlayerController has no
[SimFlow Player Component](player-component.md).

**The countdown reads `00:00` on every task.** Those tasks have no `Time Limit`.

**On Flow Started never fires.** The widget was created after the flow started, so
it missed the event. Use **On Flow Bound** for setup.

*Next: [Values](values.md) · [Player Component](player-component.md) ·
[Quiz task](../tasks/quiz.md)*
