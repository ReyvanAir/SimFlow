# Delay task

**Class:** `USimFlowTask_Delay`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Delay**

---

## Overview / Purpose

Waits a fixed number of seconds, then succeeds. Respects pause.

There is also a [Delay **node**](../nodes/delay.md), which is the better choice for
a plain pause in the graph. Use the *task* form when you need something the node
cannot do — see [below](#node-or-task).

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Duration** | Float (s, min 0) | `1.0` | Yes | Base wait time. |
| **Random Extra** | Float (s, min 0) | `0.0` | No | *Advanced.* Randomly adds **up to** this many seconds on top of `Duration`. |

Plus the [fields every task has](README.md#fields-every-task-has).

The actual wait is `Duration + random(0, Random Extra)`, rolled once when the task
starts.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Duration = 0, Random Extra = 0** | Finishes almost immediately — effectively a pass-through. |
| **Duration = 0, Random Extra > 0** | Waits a random time between 0 and `Random Extra`. |
| **The flow is paused** | The timer stops and resumes where it left off. |
| **`Tick While Paused` on** (base field) | The delay keeps counting through a pause. Almost never wanted. |

---

## Node or task?

| Use the [Delay **node**](../nodes/delay.md) | Use the **Delay task** |
|---|---|
| A plain pause — **the usual choice** | You want `Random Extra` |
| You want the graph to read clearly | You want it inside a [Parallel Group](parallel-group.md) |
| | You want a `Task Id`, an instruction, or scoring attached |
| | You want the [Task node](../nodes/task.md)'s abort condition around it |

---

## Dependencies

None.

---

## Example use case: varied ambient pacing

**Goal:** an idle scenario where a radio call arrives every 20–30 seconds, so it
does not feel scripted.

1. Add a [Task node](../nodes/task.md), **Task** = **Delay**.
2. Set **Duration** to `20`.
3. Set **Random Extra** to `10` — the wait becomes 20–30 seconds.
4. Wire `Completed` into the radio-call task.
5. Wrap the pair in a [Loop node](../nodes/loop.md) with `Iterations` = `0` and a
   break condition of your choosing, so the calls repeat until the scenario ends.

An abort condition on the Task node lets a real event cut the wait short — set
**Abort Condition** to **Event Was Raised** with your interrupt tag, and
**Abort Result** to `Succeeded`.

---

## Common pitfalls

**The delay is longer than configured.**
`Random Extra` is set, or the flow was paused. The extra is added on top of
`Duration`, not blended into it.

**The delay does not pause with the game.**
`Tick While Paused` is on, or the component's `Follow Game Pause` is off. See
[SimFlow Component](../simflow-component.md).

**I wanted a timeout, not a pause.**
Use the [Task node](../nodes/task.md)'s `Time Limit`, or race a delay against the
work with a [Join](../nodes/join.md) in **Wait For Any** mode.

**The graph is cluttered with Task nodes that only wait.**
Use the [Delay node](../nodes/delay.md) instead.

---

*See also: [Delay node](../nodes/delay.md) · [Task node](../nodes/task.md) ·
[Task Reference](README.md)*
