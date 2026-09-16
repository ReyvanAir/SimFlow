# Control and save

**Library:** `USimFlowStatics`
**Needs a target?** No.

The [SimFlow Component](../simflow-component.md) already covers controlling one
flow. These nodes cover the two cases it can't: acting on every flow at once, and
sending a control request from a client without holding a component reference.

## Request Flow Control

```
Request Flow Control (Flow Save Id, Request)
```

One node for every control action, picked from a dropdown: Start, Stop, Restart,
Pause, Resume, Toggle Pause, Retry Current Task, Skip Current Task, Fail Current
Task.

It takes the right route on its own. If the PlayerController has a
[SimFlow Player Component](player-component.md), the request goes through it — a
server RPC from a client, a direct call on the server. Without one it looks the flow
up with `Find Flow By Id` and calls the matching component function directly, which
is correct in single player and on the server.

That makes it the node to build instructor panels and pause menus from. The same
Blueprint works in single player and in multiplayer, and you can drive a whole panel
from one `Request Flow Control` with the enum pin promoted to a variable.

```
Pause button   ──▶ Request Flow Control ( MainTutorial , Toggle Pause )
Retry button   ──▶ Request Flow Control ( MainTutorial , Retry Current Task )
Skip button    ──▶ Request Flow Control ( MainTutorial , Skip Current Task )
```

A request that reaches the server is checked by
[Is Request Authorised](player-component.md#gating-requests) before anything
happens, so a client asking to skip isn't automatically obeyed.

## Every flow at once

| Node | Effect |
|---|---|
| Pause All Flows | Pauses every registered flow |
| Resume All Flows | Resumes every registered flow |

Both forward to the [subsystem](subsystem.md). Neither checks authority, so on a
client they pause the local mirrors and the next server update undoes it. Call them
on the server, or in single player.

There is no `Stop All Flows` static. That one exists only on the
[subsystem](subsystem.md), reachable through **Get Game Instance Subsystem**.

## Save and load, across every flow

| Node | Returns | Notes |
|---|---|---|
| Save All Flows (Slot Name, User Index) | bool | Writes every registered flow into one slot |
| Load All Flows (Slot Name, User Index, Load Mode) | int32 | How many flows were restored |
| Delete Flow Save (Slot Name, User Index) | bool | |

`User Index` is the platform user index and defaults to `0`. `Load Mode` is
**Exact State** or **From Last Checkpoint**, the same two modes the
[component](../simflow-component.md#save-and-load) uses.

A return of `0` from `Load All Flows` means nothing matched, not that the slot was
missing. Flows are matched into the save by **Flow Save Id**, so a flow whose id
changed since the save was written won't be found.

Object references in a blackboard are stripped on save. See
[values and types](../blackboard.md#values-and-types).

Save and load are authority-only, like the rest of SimFlow persistence. On a client
they log and return.

## A pause menu that works in both modes

1. Give the flow a **Flow Save Id**, say `MainTutorial`.
2. Add a **SimFlow Player Component** to your PlayerController Blueprint.
3. Wire each menu button to **Request Flow Control** with that id.
4. For the resume button use **Toggle Pause** rather than tracking state yourself.

Nothing in the menu needs to know whether it's running standalone or as a client.

## When it misbehaves

**Nothing happens when a client presses a button.** The PlayerController has no
SimFlow Player Component, and the fallback path found no flow with that id either.

**The server ignores a client's skip request.** `Is Request Authorised` rejected it.
The default implementation honours `Restrict To Owned Flows`; if you overrode it,
check your override.

**Pause All Flows pauses, then un-pauses a moment later.** You called it on a client
with replication on. The server's next state update overwrites the mirror.

**Load All Flows returns 0 and the slot definitely exists.** The flows in the level
have different `Flow Save Id` values from the ones in the save, or none at all.

*Next: [Flow access](flow-access.md) · [Player Component](player-component.md) ·
[Subsystem](subsystem.md)*
