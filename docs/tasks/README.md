# Task Reference

A **task** is the unit of *work* in a flow. It lives inside a
[Task node](../nodes/task.md), which handles retry, timeout and routing while the
task does the job.

You do not create task assets. Select a [Task node](../nodes/task.md), open the
**Task** dropdown in the Details panel, and pick a class — its settings then appear
inline underneath.

---

## All tasks

| Task | Purpose |
|---|---|
| [Delay](delay.md) | Waits a number of seconds. |
| [Log Message](log-message.md) | Prints a message. Great for blocking out a flow. |
| [Set Blackboard Value](set-blackboard.md) | Writes a [blackboard](../blackboard.md) key. |
| [Wait For Event](wait-for-event.md) | Blocks until an event tag is raised — optionally by the *right object*. |
| [Wait For Condition](wait-for-condition.md) | Blocks until a [condition](../conditions.md) becomes true. |
| [Go To Location](go-to-location.md) | Blocks until the player reaches a place. |
| [Quiz](quiz.md) | A multiple-choice question. |
| [Parallel Group](parallel-group.md) | Runs several child tasks at once inside one node. |
| [Place Object In Zone](place-object-in-zone.md) | "Put the foam extinguisher in the bay." |
| [Ordered Sequence](ordered-sequence.md) | "Press these three buttons, in this order." |

---

## Fields every task has

These come from the task base class and appear on **every** task, above its own
settings.

### Task \| Presentation

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Display Name** | Text | *empty* | Shown in the graph, the debug HUD and your tutorial UI. Falls back to the class name. |
| **Instruction** | Text (multi-line) | *empty* | Longer instruction text, e.g. "Pick up the fire extinguisher". Read it with `Get Current Instruction`. |
| **Task Id** | Name | `None` | Optional tag so UI and analytics can identify this task. Recorded on any [mistake](../glossary.md). |

### Task \| Rules

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Allow Retry** | Bool | `true` | Whether the player may retry this task via the component. |
| **Max Retries** | Int (min 0) | `0` | **0 = unlimited.** When exceeded, a retry request fails the task instead. |
| **Allow Skip** | Bool | `true` | Whether the player or instructor may skip it. |
| **Tick While Paused** | Bool | `false` | *Advanced.* Keeps ticking while the flow is paused. Almost always leave off. |

### Task \| Scoring

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Score On Success** | Float | `0.0` | Added to the `Score` blackboard key when the task succeeds. |
| **Score On Failure** | Float | `0.0` | Added when it fails or times out. Usually negative. |

> **Scoring defaults to 0**, so a flow scores nothing until you set these. If
> `Get Score` always returns 0, this is why.

---

## The task lifecycle

1. **On Task Start** — the task becomes active. Set up here.
2. **On Task Tick** — every frame while running and not paused.
3. **On Task Pause / Resume** — mirrors the flow's pause state.
4. **Finish Task(Result)** — *you* call this when the work is done.
5. **On Task End** — after it ends for any reason. Clean up bindings here.

`Finish Task` is guarded against being called twice in one activation, so a
double-fire from an event binding is harmless.

Read-only state available while running: `Elapsed Time` (excluding paused time),
`Retry Count`, `Is Running`, `Is Paused`, plus `Flow Instance` and `Owning Node`.

---

## Wrong-answer policies

Three built-in tasks judge whether the trainee acted on the *right object*, and
share a common vocabulary for what to do when they did not.

### Mismatch Policy

Used by [Wait For Event](wait-for-event.md) and
[Place Object In Zone](place-object-in-zone.md).

| Policy | Behaviour |
|---|---|
| **Ignore (Keep Waiting)** | Silently keep waiting. The wrong object is simply not the one we want. |
| **Count Mistake (Keep Waiting)** | Record a [mistake](../glossary.md) and keep waiting, so the trainee can correct themselves. **Default.** |
| **Count Mistake And Fail Task** | Record a mistake and fail, driving the node's `Failed` pin. |

### Out Of Order Policy

Used by [Ordered Sequence](ordered-sequence.md).

| Policy | Behaviour |
|---|---|
| **Ignore** | Ignore anything that is not the expected step. |
| **Count Mistake (Stay On Step)** | Record a mistake but stay on the current step. **Default.** |
| **Count Mistake And Restart** | Record a mistake and send the trainee back to step one. |
| **Count Mistake And Fail Task** | Record a mistake and fail the task. |

---

## Writing your own task

Tasks are `Blueprintable`.

1. Content Browser → **Blueprint Class** → pick **SimFlow Task**.
2. Override **On Task Start** and do your setup.
3. Call **Finish Task** with a result when the work is done.
4. Override **On Task End** to unbind anything you bound.

You get retry, skip, timeout, pause and scoring for free from the
[Task node](../nodes/task.md). Useful helpers on the base class:

- `Get Flow Owner` — the actor owning the flow component
- `Get Player Pawn` — in a VR project, the VR pawn
- `Get Blackboard`
- `Record Mistake(Kind, Involved, Description, Severity)`
- `Apply Mismatch Policy(...)` — the shared "right event, wrong object" handling
- `Set Saved Value` / `Get Saved Value` — state that survives a save/load

Your class then appears in the Task dropdown automatically.

---

*See also: [Task node](../nodes/task.md) · [Node Reference](../nodes/README.md) ·
[Glossary](../glossary.md) · [Documentation index](../README.md)*
