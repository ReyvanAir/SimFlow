# Graph introspection

**Classes:** `USimFlowAsset`, `USimFlowNode` and its subclasses
**Get a reference:** `Get Flow Asset` on the
[SimFlow Component](../simflow-component.md), or `Get Template` /
`Get Active Nodes` on a [flow instance](instance.md)

Reading a flow asset and its nodes from Blueprint. This is the narrowest corner of
the API — most projects never need it. It earns its place for editor utilities,
custom debug overlays, and flows built at runtime.

Nothing here places a node in a graph. These read and write the
[graph nodes](../nodes/README.md) an asset already contains, or build new ones
programmatically.

## Reading an asset

| Node | Returns |
|---|---|
| Get Display Name Text | `Flow Display Name`, falling back to the asset name |
| Get Node Count | How many nodes the asset holds |
| Get Entry Names | Every [Start](../nodes/start.md) node's entry name |
| Find Node By Guid (Guid) | One authored node, or null |

`Get Entry Names` is the useful one. Populate a dropdown with it and you have a
scenario picker that can't offer an entry that doesn't exist — better than a text
field where a typo silently starts nothing.

```
Get Flow Asset ──▶ Get Entry Names ──▶ [ build combo box ]
                                              │
                   Start Flow From Entry ◀────┘
```

`Get Node Count` counts every node, not just tasks. For a progress denominator use
`Get Progress` on the [instance](instance.md) instead.

## Validating

```
Validate Flow (out Errors, out Warnings)
```

Reports what a designer should know about: no entry node, links pointing at nodes
that no longer exist, task nodes with no task assigned, nodes nothing can reach.

Both outputs are string arrays. Empty errors means the flow will run; warnings can
be real problems that happen not to be fatal.

Worth calling from an editor utility widget over every flow asset in the project
before a build. The same checks run in the graph editor, but only for the asset
that's open.

## Node introspection

On any node:

| Node | Returns |
|---|---|
| Get Flow Instance | The [instance](instance.md) running this node |
| Get Blackboard | Shortcut to that instance's [blackboard](blackboard-nodes.md) |
| Is Node Active | Whether it's currently executing |
| Get Node Active Time | Seconds since it activated |

On specific node types:

| Node | On | Returns |
|---|---|---|
| Get Task | [Task node](../nodes/task.md) | The task object |
| Get Remaining Time | [Task node](../nodes/task.md) | Seconds left on the time limit |
| Get Child Instance | [Sub Flow node](../nodes/sub-flow.md) | The child flow's instance |

`Get Node Active Time` resets each time the node activates, so a re-entered
[Loop](../nodes/loop.md) starts counting again.

`Get Child Instance` is how you reach into a running sub-flow — its blackboard, its
active tasks, its mistakes. A [Sub Flow](../nodes/sub-flow.md) child has its own
instance, and this is the only handle on it.

Pair these with `Get Active Nodes` on the instance to build a live graph overlay:

```
Get Active Nodes ──▶ For Each ──▶ Get Node Active Time ──▶ draw beside the node
```

## Trigger Output

```
Trigger Output (Pin Name, Stay Active)
```

Pushes execution out of a named output pin. This is the primitive every node uses to
continue the flow, exposed so a Blueprint node subclass can drive itself.

`Stay Active` left off deactivates the node afterwards, which is what almost
everything wants. Leave it on for a node that fires an output and keeps running, the
way [Parallel](../nodes/parallel.md) does.

Only call this on a node you wrote. Triggering an output on a built-in node from
outside puts the flow in a state that node doesn't expect.

## Building a flow at runtime

| Node | Returns |
|---|---|
| Add Node (Node Class) | The new node, typed to the class you passed |
| Remove Node (Node) | |
| Connect Nodes (From, From Pin, To, To Pin) | bool |
| Disconnect Nodes (From, From Pin, To, To Pin) | bool |

For generating scenarios from data — a procedure defined in a spreadsheet, a
randomised assessment, a flow assembled from a difficulty setting.

Build against a fresh asset, not one on disk. These change the asset, so editing the
one your project ships modifies it for every flow using it, and in the editor that
change can be saved.

Pin names have to match what the node actually declares. `Connect Nodes` returns
false for a pin that doesn't exist, and that false is the only thing you'll get —
nothing logs.

Call `Validate Flow` when you've finished assembling. It's much easier to read than
a flow that starts and quietly does nothing.

## A scenario picker from entry names

1. **Get Flow Asset** from the component.
2. **Get Entry Names**.
3. Build one button per name, labelled with it.
4. On click, **Start Flow From Entry** with that name.

Entries renamed in the asset show up in the menu without touching the widget.

## When it misbehaves

**Find Node By Guid returns null for a node that exists.** You're searching the
asset with a runtime node's Guid, or the reverse. `Find Node By Guid` is the
authored side; `Find Runtime Node` on the [instance](instance.md) is the live side.

**Get Task returns null on a Task node.** No task is assigned. That's one of the
things `Validate Flow` reports.

**Connect Nodes returns false and nothing is logged.** A pin name doesn't match.
Check the node's declared pins.

**Changes to an asset at runtime don't affect the running flow.** Nodes are
duplicated into the instance when the flow starts. Restart to pick them up.

**A runtime-built flow starts and immediately stops.** No entry node, or the entry's
`Out` is unwired. `Validate Flow` catches both.

**Get Node Active Time keeps resetting.** The node is being re-entered — normal
inside a [Loop](../nodes/loop.md).

*Next: [Node reference](../nodes/README.md) · [Flow Instance](instance.md) ·
[Troubleshooting](../troubleshooting.md)*
