# Player Component

**Component:** `SimFlow Player Component` (`USimFlowPlayerComponent`)
**Add via:** Add Component → SimFlow Player Component, on your **PlayerController**
**Needed when:** clients have to drive the flow

A flow usually lives on the Game State or a level actor, and neither has an owning
connection. A client can't send an RPC to something it doesn't own, so it can't talk
to the flow directly.

This component sits on the PlayerController, which always has a connection, and
forwards requests to whichever flow the server says owns a given save id.

Single player doesn't need it. A server-authoritative flow that clients only watch
doesn't need it. Add it the moment a client has to answer a quiz, press retry, or
raise an interaction event.

## Getting a reference

```
Get Local (World Context) ──▶ SimFlow Player Component
```

`Get Local` is static and finds the local player's component, returning null when
the PlayerController doesn't have one. It's the only node here you call without a
target.

You rarely need it. [Request Flow Control](control-and-save.md) and
[Broadcast Flow Event](../events.md) both call `Get Local` internally and fall back
sensibly when there's nothing there.

## Requests

| Node | Asks the server to |
|---|---|
| Request Control (Flow Save Id, Request) | Perform a control action |
| Request Quiz Answer (Flow Save Id, Quiz Node Guid, Option Index) | Submit an answer |
| Request Event (Flow Save Id, Event Tag, Payload) | Raise an event tag |

`Request` on the first is the same dropdown
[Request Flow Control](control-and-save.md) uses: Start, Stop, Restart, Pause,
Resume, Toggle Pause, Retry Current Task, Skip Current Task, Fail Current Task.

`Request Event` accepts `None` for the flow id, which raises the tag on every flow —
the multiplayer path behind `Broadcast Flow Event`.

`Request Quiz Answer` wants the quiz node's Guid rather than a task reference,
because that's what replicates. Read it off the current task, or let
[Submit Quiz Answer](status-widget.md) on the status widget handle it.

All three are safe to call on the server too. They take the direct path instead of
an RPC.

## Restrict To Owned Flows

One editable property, `false` by default.

Turn it on and the server rejects requests from a client for flows it doesn't own.
Leave it off and any client can control any flow, which is what you want for
instructor controls and not what you want for trainees.

It only gates the default authorisation check. Override that check and this flag
does nothing unless your override honours it.

## Gating requests

**Is Request Authorised (Target Flow, Request)** is a BlueprintNativeEvent that runs
on the server for every incoming request, before anything happens. Override it in a
Blueprint subclass to write your own rules.

```
Is Request Authorised
   ├── Request == Skip Current Task  ──▶  return Is Instructor
   └── otherwise                     ──▶  return true
```

Clients are not trusted. Everything they send arrives here first, which makes this
the only place worth enforcing anything. A check in the UI stops an honest player
and nobody else.

Worth gating: Skip, Fail, Stop and Restart. A trainee who can skip a task can skip
an assessment.

The default implementation honours `Restrict To Owned Flows` and allows everything
else.

## A trainee controller that can answer but not skip

1. Create a Blueprint subclass of **SimFlow Player Component**.
2. Override **Is Request Authorised**.
3. Return false when `Request` is `Skip Current Task`, `Fail Current Task` or
   `Stop`, true otherwise.
4. Add that subclass to your trainee PlayerController.
5. Give instructors a separate PlayerController with the unmodified component.

Quiz answers and interaction events still go through, so the trainee can do the
scenario. They just can't skip out of it.

## When it misbehaves

**Client buttons do nothing and nothing is logged.** The component isn't on the
PlayerController. This is the most common multiplayer setup mistake.

**Broadcast Flow Event logs a warning about a missing player component.** Same
cause. On a client the event has nowhere to go.

**Requests work in PIE as listen server and fail as a client.** You're testing the
server path. Run two clients against a dedicated server to exercise the RPC.

**Get Local returns null in a widget's Event Construct.** The PlayerController may
not be ready. Call it later, or bind on possession.

**Every client can pause everyone's flow.** `Restrict To Owned Flows` is off and
`Is Request Authorised` isn't overridden. That's the default.

**Quiz answers land on the wrong flow.** `Flow Save Id` was left as `None`, so the
server resolved a different flow. Set it.

*Next: [Control and save](control-and-save.md) · [Status Widget](status-widget.md) ·
[SimFlow Component](../simflow-component.md)*
