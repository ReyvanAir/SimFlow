# Blueprint node reference

These are the SimFlow nodes you place in an **Actor, widget or component Blueprint**
— the ones that appear when you right-click in a normal Blueprint graph and type
"SimFlow".

They are not the same thing as the [node reference](../nodes/README.md). That page
covers the boxes you wire together inside a flow asset, in the SimFlow graph editor.
Nothing here goes in a flow graph, and nothing there goes in a Blueprint.

| | [Graph nodes](../nodes/README.md) | Blueprint nodes (this section) |
|---|---|---|
| Where you place them | The SimFlow graph editor | Any Blueprint event graph |
| What they are | `USimFlowNode` subclasses | `UFUNCTION`s on SimFlow classes |
| What they do | Decide where flow execution goes | Let your game read and drive a running flow |
| Example | [Branch](../nodes/branch.md), [Delay](../nodes/delay.md) | `Get Primary Flow`, `Broadcast Flow Event` |

Most projects only need two of these pages. Start with
[flow access](flow-access.md) to get hold of a running flow, then
[events](../events.md) to talk to it.

## Pages

| Page | Covers |
|---|---|
| [Flow access](flow-access.md) | Finding a running flow from anywhere: `Get Primary Flow`, `Find Flow By Id`, `Get Flow From Actor` |
| [Control and save](control-and-save.md) | Driving flows in bulk, and the network-safe `Request Flow Control` |
| [Values](values.md) | Building and reading `SimFlow Value`, plus the text formatters for UI |
| [Blackboard nodes](blackboard-nodes.md) | Every typed getter and setter, as nodes |
| [Flow Instance](instance.md) | The running flow object: state, active tasks, mistakes, events, save |
| [Subsystem](subsystem.md) | The game-instance registry of every flow, and the debug HUD toggle |
| [Player Component](player-component.md) | The client-to-server channel, and how to gate it |
| [Status Widget](status-widget.md) | The UMG base class: bindings, button handlers, design events |
| [Zones and identity](world-queries.md) | Asking the world what's where and what things are |
| [Graph introspection](graph-introspection.md) | Reading and building flow assets from Blueprint |

The [SimFlow Component](../simflow-component.md) has its own page, because it's the
one you add in the editor rather than call from a graph. Its functions are
documented there.

## Where a node lives

Every function belongs to some class, and you need a reference to that class before
the node will compile. The chain is short:

```
Get Primary Flow  ──▶  SimFlow Component  ──▶  Get Flow Instance  ──▶  Flow Instance
                              │                                            │
                              └──▶ Get Blackboard ──▶ Blackboard ◀─────────┘
```

Three of the libraries need no reference at all. `USimFlowStatics`,
`USimFlowIdentityStatics` and `USimFlowPlayerComponent::GetLocal` are static, so
their nodes drop straight into any graph.

## Component or instance?

A lot of functions exist on both the [SimFlow Component](../simflow-component.md)
and the [Flow Instance](instance.md). `Pause Flow` against `Pause Instance`,
`Retry Current Task` against `Retry Active Tasks`.

Use the component. It checks authority, routes client calls to the server, and
fires the replication updates your UI is bound to. The instance functions do none of
that — they act immediately on whatever machine calls them.

The instance is for reading: active nodes, mistakes, elapsed time, the things the
component doesn't surface.

## Authority, in one paragraph

In single player it doesn't matter. In multiplayer with **Replicate Flow** on,
anything that changes a flow has to happen on the server. Component functions and
`Request Flow Control` handle that for you, provided the PlayerController has a
[SimFlow Player Component](player-component.md). Instance and subsystem functions
don't — call them on a client and they change the client's mirror, which the next
server update overwrites.

*Next: [Flow access](flow-access.md) · [Events](../events.md) ·
[SimFlow Component](../simflow-component.md)*
