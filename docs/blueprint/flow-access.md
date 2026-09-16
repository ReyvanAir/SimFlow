# Flow access

**Library:** `USimFlowStatics`
**Add via:** right-click in any Blueprint graph → type the node name
**Needs a target?** No. These are static, so they work anywhere.

Before you can do anything to a flow you need the
[SimFlow Component](../simflow-component.md) running it. These three nodes are how
you get one without wiring an actor reference through your whole level.

| Node | Returns | Use when |
|---|---|---|
| Get Primary Flow | The main running flow, or null | One scenario is running. The common case. |
| Find Flow By Id (Flow Save Id) | The flow with that id, or null | Several flows are running |
| Get Flow From Actor (Actor) | That actor's component, or null | You already have the actor |

All three are pure nodes, so they have no execution pins and re-run wherever you
drag the output.

## Get Primary Flow

Returns the first flow that is running or paused. If none are, it falls back to the
first flow registered with the [subsystem](subsystem.md), which is usually a flow
that hasn't started yet.

That fallback is deliberate — it means you can call `Get Primary Flow` on BeginPlay
and get a valid component back to call `Start Flow` on. It also means a non-null
return doesn't prove anything is running. Check `Is Flow Running` if that matters.

"First" means first registered, which is BeginPlay order. With one flow in the level
that's unambiguous. With several it isn't, so use `Find Flow By Id` instead.

```
Get Primary Flow ──▶ Is Valid ──▶ Get Current Instruction ──▶ Set Text
```

## Find Flow By Id

Matches against the component's **Flow Save Id**. An empty or `None` id never
matches anything, so a flow you want to address this way needs that field set — see
[the component's save fields](../simflow-component.md).

This is also what `Send Flow Event` and the client control routing use internally,
which is why setting `Flow Save Id` is worth doing the moment you have more than one
flow.

## Get Flow From Actor

A thin wrapper around Find Component By Class. Handy in an overlap or interaction
handler where you already have the other actor and want to know whether it hosts a
flow.

## Raising events

Covered in full on the [events](../events.md) page — `Broadcast Flow Event` and
`Send Flow Event` also live in this library.

The one thing worth repeating here: on a client, `Broadcast Flow Event` does not
touch the local mirrors. It sends a single RPC through the local
[SimFlow Player Component](player-component.md) and lets the server raise the tag on
every flow. With no player component on the PlayerController it logs a warning
naming the tag and does nothing else.

## A wrist panel that finds its own flow

For a VR status panel that shouldn't need configuring per level:

1. In the widget's **Event Construct**, call **Get Primary Flow**.
2. Branch on **Is Valid**.
3. Store the result in a variable and bind your text blocks to
   `Get Current Task Name` and `Get Current Instruction` on it.

Or skip all of that and reparent the widget to
[SimFlow Status Widget](status-widget.md), which does the same thing every frame
until it finds a flow.

## When it misbehaves

**Get Primary Flow returns null on BeginPlay.** The flow's actor hasn't begun play
yet, so it hasn't registered. Move the call to the first tick — the same reason
[Start Mode](../simflow-component.md#start-mode) recommends **Auto - On First
Tick**.

**Find Flow By Id always returns null.** The component's `Flow Save Id` is still
`None`. It's blank by default.

**The wrong flow comes back from Get Primary Flow.** Two flows are running and you
got whichever registered first. Give them save ids and use `Find Flow By Id`.

**It works in the editor and returns null in a packaged client.** The flow lives on
a server-only actor. Clients only see a flow whose owning actor replicates and whose
component has `Replicate Flow` on.

*Next: [Events](../events.md) · [Control and save](control-and-save.md) ·
[SimFlow Component](../simflow-component.md)*
