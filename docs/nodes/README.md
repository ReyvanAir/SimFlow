# Node Reference

A **node** is a box in the flow graph. Nodes control *where execution goes*.
A [task](../tasks/README.md) is the *work* that happens inside a
[Task node](task.md).

Add nodes by **right-clicking in the graph**. The menu is grouped by the categories
below.

---

## All nodes

### Flow Control

| Node | Purpose |
|---|---|
| [Start](start.md) | Where execution begins. Every flow needs one. |
| [Delay](delay.md) | Waits, then continues. |
| [Branch](branch.md) | Takes the first output whose condition passes. |
| [Random Branch](random-branch.md) | Picks one output at random, optionally weighted. |
| [Parallel](parallel.md) | Fires every output at once. |
| [Join](join.md) | Converges parallel branches — wait for all, or race. |
| [Loop](loop.md) | Repeats a section of the graph. |
| [Finish](finish.md) | Ends the whole flow with a result. |

### Tasks

| Node | Purpose |
|---|---|
| [Task](task.md) | Runs a single [task](../tasks/README.md). **The node designers use most.** |

### Composition

| Node | Purpose |
|---|---|
| [Sub Flow](sub-flow.md) | Runs another flow asset as a child. This is what makes flows modular. |

### Persistence

| Node | Purpose |
|---|---|
| [Checkpoint](checkpoint.md) | Marks a safe resume point, and can auto-save. |

### Data

| Node | Purpose |
|---|---|
| [Set Blackboard Value](set-blackboard.md) | Writes a [blackboard](../blackboard.md) key inline in the graph. |

---

## Concepts common to every node

### Pins

Execution arrives at an **input pin** and leaves through an **output pin**. Most
nodes have a single `In` and a single `Out`; the interesting ones have more.

An output pin with **nothing wired to it** ends that line of execution. This is not
an error — a flow can legitimately have branches that simply stop — which is why a
misrouted pin fails silently rather than loudly. When a section of your flow never
runs, an unwired pin is the first thing to check.

### The field every node has

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Node Comment** | String (multi-line) | *empty* | A free-form note drawn on the node in the graph. Purely for the author. |

### Node lifecycle

1. Execution arrives at an input pin → `Execute Input`.
2. The node may stay **active**, ticking every frame (Task, Delay, Sub Flow).
3. It pushes execution out through an output pin, which normally deactivates it.

Nodes are **duplicated per running flow**, so they safely hold runtime state and two
actors can run the same asset independently. That is also why editing an asset
mid-play changes nothing until you restart — see
[Glossary · Asset vs. Instance](../glossary.md).

### Re-entrant nodes

Most nodes ignore execution arriving while they are already active. **Join** and
**Loop** are the exceptions — they are built to be re-entered, which is exactly how
a loop body returns to its loop node.

---

## Choosing between a node and a task

Several capabilities exist in both forms — there is a Delay *node* and a Delay
*task*, a Set Blackboard *node* and a Set Blackboard *task*.

| Use the **node** when | Use the **task** when |
|---|---|
| It is a simple step in the graph | You want the [Task node](task.md)'s timeout, retry or abort handling around it |
| You want the graph to read clearly | You want it inside a [Parallel Group](../tasks/parallel-group.md) |
| You do not need a `Failed` pin | You want scoring, an instruction, or a `Task Id` |

The node forms are lighter; the task forms are richer. For a bare pause, the
[Delay node](delay.md) is the better choice.

---

*See also: [Task Reference](../tasks/README.md) · [Conditions](../conditions.md) ·
[Glossary](../glossary.md) · [Documentation index](../README.md)*
