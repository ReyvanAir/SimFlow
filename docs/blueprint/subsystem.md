# Subsystem

**Class:** `USimFlowSubsystem` (a Game Instance Subsystem)
**Get a reference:** **Get Game Instance Subsystem** → class `SimFlowSubsystem`

Every [SimFlow Component](../simflow-component.md) registers itself here on
BeginPlay and unregisters on EndPlay. The subsystem is the list.

It outlives level travel, since it belongs to the game instance rather than the
world. That's what makes it the right place to ask "what flows exist" from UI code
that has no idea what's in the level.

Most of what it does is also on [`USimFlowStatics`](flow-access.md) in static form,
which needs no reference and is shorter to wire. Reach for the subsystem for the
three things the statics don't cover: the full component list, `Stop All Flows`, and
the debug HUD.

## Finding flows

| Node | Returns |
|---|---|
| Get All Flow Components | Every registered flow, in registration order |
| Find Flow By Id (Flow Save Id) | One flow, or null |
| Get Primary Flow | The first running or paused flow, else the first registered, else null |

`Get All Flow Components` is the one with no static equivalent. Use it to build an
instructor panel listing every scenario in the level, or to check how many flows are
actually live before assuming there's one.

Registration order is BeginPlay order, which is not something to depend on. If you
care which flow you get, give them **Flow Save Id** values and look them up by name.

## Bulk control

| Node | Notes |
|---|---|
| Pause All Flows | |
| Resume All Flows | |
| Stop All Flows | Every flow reports **Aborted** |

None of these check authority. On a client with replication on they change the local
mirrors and the next server update reverts them.

`Stop All Flows` has no static wrapper, so this is the only way to reach it from
Blueprint.

## Save and load

| Node | Returns |
|---|---|
| Save All Flows To Slot (Slot Name, User Index) | bool |
| Load All Flows From Slot (Slot Name, User Index, Load Mode) | How many flows were restored |
| Delete Save Slot (Slot Name, User Index) | bool |

One slot holds every registered flow, matched back by **Flow Save Id**. A flow with
no id can be written but never matched on load.

`Delete Save Slot` is static on the C++ class, so its node has no target pin even
though it lives here.

Authority-only, like all SimFlow persistence.

## Debug HUD

| Node | Notes |
|---|---|
| Set Debug HUD Enabled (Enabled) | Toggles the overlay for every flow |
| Is Debug HUD Enabled | bool |
| Build Global Debug Text | Combined status text for every registered flow |

`Set Debug HUD Enabled` is the Blueprint equivalent of `SimFlow.Debug 1` in the
console. Both drive the same flag, so a console toggle changes what
`Is Debug HUD Enabled` returns.

`Build Global Debug Text` returns the whole thing as a string instead of drawing it.
That's the one to use for a world-space debug panel in VR, where the screen-space
overlay isn't visible in the headset.

```
Build Global Debug Text ──▶ Set Text  (on a world-space widget)
```

For a single flow, `Get Debug Text` on the [component](../simflow-component.md) is
narrower and usually more readable.

## A scenario picker

Listing every flow in the level so an instructor can start one:

1. **Get Game Instance Subsystem** → `SimFlowSubsystem`.
2. **Get All Flow Components**.
3. For each, **Get Flow Asset → Get Display Name Text** for the button label.
4. On click, **Get Effective Save Id** from that component and pass it to
   [Request Flow Control](control-and-save.md) with `Start`.

Going through `Request Flow Control` rather than calling `Start Flow` directly keeps
the panel working on a client.

## When it misbehaves

**Get All Flow Components is empty on BeginPlay.** Components register during their
own BeginPlay, and yours may run first. Check on the first tick.

**A flow that was destroyed is still in the list.** It isn't — the subsystem holds
weak pointers and skips dead entries. An entry that looks stale is a component whose
actor still exists.

**Debug HUD won't turn off.** `SimFlow.Debug` and `Set Debug HUD Enabled` share one
flag, but the component's own **Show Debug HUD** checkbox is separate and wins for
that flow.

**Stop All Flows recorded Aborted, not Failed.** That's what it reports. Route
through a [Finish](../nodes/finish.md) node if you want Failed.

**The subsystem is null.** It's a Game Instance Subsystem, so it needs a valid world
context. This happens in construction scripts and in the editor preview.

*Next: [Flow access](flow-access.md) · [Control and save](control-and-save.md) ·
[Troubleshooting](../troubleshooting.md)*
