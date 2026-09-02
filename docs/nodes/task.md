# Task node

**Class:** `USimFlowNode_Task`
**Add via:** right-click → **Tasks → Task**
**Pins:** In → Completed, Failed, Skipped, Timed Out

---

## Overview / Purpose

The Task node **runs a single [task](../tasks/README.md)**. It is the node
designers use most, and the bridge between the graph (where execution goes) and the
work (what actually happens).

The division of labour matters: **the node handles retry, timeout, early-out and
routing; the task handles the work.** That is why every task automatically gets
timeout and retry behaviour without implementing any of it.

---

## Field-by-field breakdown

### Task

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Task** | Instanced task | *null* | **Yes** | The task to run. Pick a class from the dropdown; its own settings then appear inline. |
| **Time Limit** | Float (s, min 0) | `0.0` | No | Seconds before the task times out and leaves via `Timed Out`. **0 = no limit.** |
| **Auto Retry On Failure** | Bool | `false` | No | Automatically restart the task when it fails. |
| **Auto Retry Limit** | Int (min 1) | `1` | No | How many automatic retries. Only shown when auto retry is on. |
| **Auto Retry On Timeout** | Bool | `false` | No | Also restart on a timeout. Only shown when auto retry is on. |
| **Fallback To Completed** | Bool | `true` | No | *Advanced.* Unwired `Failed` / `Skipped` / `Timed Out` pins fall through to `Completed`. |

### Task \| Early Out

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Abort Condition** | Instanced [condition](../conditions.md) | *null* | Aborts the task early. **Evaluated every frame.** |
| **Abort Result** | Enum | `Failed` | Which pin the abort leaves through. Only shown when an abort condition is set. |

---

## Output pins

| Pin | Taken when |
|---|---|
| **Completed** | The task succeeded |
| **Failed** | The task failed, or was failed by `Fail Current Task` |
| **Skipped** | The task was skipped |
| **Timed Out** | `Time Limit` elapsed |

**Anything left unwired falls back to `Completed`** while `Fallback To Completed` is
on — which is why simple linear flows stay tidy: you wire `Completed` and ignore the
rest.

> Turn `Fallback To Completed` **off** when a failure must not be allowed to look
> like a success. With it off, an unwired `Failed` pin simply ends that line of
> execution.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Task is empty** | Logs a warning — *"Task node has no task assigned - passing through"* — and **triggers `Completed`**. It does **not** fail. |
| **Time Limit = 0** | No timeout. A task that never finishes blocks the flow forever. |
| **Auto Retry On Failure on, task's `Allow Retry` off** | No retry happens — the task's own rule wins. |
| **Abort Condition true at start** | The task aborts on its first frame. Conditions are evaluated every frame, including the first. |
| **All output pins unwired** | With fallback on, `Completed` is taken and goes nowhere — the flow silently ends there. |

The empty-task case is deliberate: a blocked-out flow full of empty Task nodes still
runs end to end, so you can build the shape of a scenario before writing any tasks.

---

## Retry, skip and fail

Two layers control these, and **both must allow it**:

| Layer | Fields |
|---|---|
| **The node** | `Auto Retry On Failure`, `Auto Retry Limit`, `Auto Retry On Timeout` |
| **The task** | `Allow Retry`, `Max Retries`, `Allow Skip` — see [Task fields](../tasks/README.md#fields-every-task-has) |

**Automatic retry** happens inside the node when the task fails: the node restarts
the task without leaving through any pin. Only after the retries are exhausted does
execution leave via `Failed`.

**Manual retry / skip / fail** come from the
[SimFlow Component](../simflow-component.md) — `Retry Current Task`,
`Skip Current Task`, `Fail Current Task` — typically wired to an instructor panel.

---

## Dependencies

| Depends on | Why |
|---|---|
| A [task](../tasks/README.md) in the `Task` slot | Otherwise the node just passes through |
| [Conditions](../conditions.md) | Only for `Abort Condition` |

---

## Example use case: a timed step that retries once

**Goal:** the trainee has 30 seconds to press the start button. One automatic retry,
then route to a hint.

1. Right-click → **Tasks → Task**.
2. Set **Task** to [Wait For Event](../tasks/wait-for-event.md), Event Tag =
   `SimFlow.Event.ButtonPressed`.
3. Set **Time Limit** to `30`.
4. Tick **Auto Retry On Failure**, set **Auto Retry Limit** to `1`, and tick
   **Auto Retry On Timeout** so a timeout also retries.
5. Wire **Completed** onward to the next step.
6. Wire **Timed Out** to a hint section — this is only reached after the retry is
   used up.
7. Leave **Failed** and **Skipped** unwired; they fall through to `Completed`.

Add an **Abort Condition** of [Player Near Location](../conditions.md#player-near-location)
with `Invert` on, and the task will also abort if the trainee walks away.

```
              ┌────────────────────────┐
   Start ─────┤In   Task               │Completed──▶ next step
              │     (Wait For Event)   │Failed
              │     Time Limit 30s     │Skipped
              │     Auto Retry 1       │Timed Out──▶ hint
              └────────────────────────┘
```

---

## Common pitfalls

**The node completes instantly and nothing happened.**
The `Task` slot is empty. Check the log for *"has no task assigned"*.

**A failure looks like a success.**
`Fallback To Completed` is on and the `Failed` pin is unwired. Wire it, or turn the
fallback off.

**The flow hangs on one task forever.**
`Time Limit` is `0` and the task is waiting for something that never arrives. Set a
time limit while developing.

**Auto retry does nothing.**
The task's own `Allow Retry` is off, or the result was a timeout and
`Auto Retry On Timeout` is off.

**The task aborts immediately.**
The `Abort Condition` is already true on the first frame. Abort conditions are
evaluated every frame from the start.

**`Timed Out` never fires even though the task is stuck.**
Auto retry with `Auto Retry On Timeout` keeps restarting it. Check
`Auto Retry Limit`.

---

*See also: [Task Reference](../tasks/README.md) · [Conditions](../conditions.md) ·
[SimFlow Component](../simflow-component.md) · [Node Reference](README.md)*
