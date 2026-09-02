# Parallel Group

**Class:** `USimFlowTask_ParallelGroup`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Parallel Group**

---

## Overview / Purpose

Runs **several child tasks at once inside a single node**.

Use it when you want a small parallel group without cluttering the graph with
[Parallel](../nodes/parallel.md) and [Join](../nodes/join.md) nodes — "play the
narration while the trainee dons the helmet" is one node, not five.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Tasks** | Array of instanced tasks | *empty* | **Yes** | The child tasks. Each gets its own class dropdown and inline settings. |
| **Wait For All** | Bool | `true` | No | When **false**, the group finishes as soon as the **first** child finishes. |
| **Fail If Any Child Fails** | Bool | `true` | No | Any child failing fails the whole group. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Tasks is empty** | There is nothing to wait for. The group has no children to report completion, so it does not advance on its own — treat an empty group as a configuration error. |
| **Wait For All on, one child never finishes** | The group waits forever. Put a `Time Limit` on the [Task node](../nodes/task.md). |
| **Wait For All off** | The first child to finish ends the group. **The others are ended too** — do not rely on their side effects completing. |
| **Fail If Any Child Fails on** | One failure fails the group, even if the others succeeded. |
| **Fail If Any Child Fails off** | Child failures are absorbed; the group succeeds as long as the completion rule is met. |

---

## What the children inherit

Child tasks are full tasks, so each has its own
[presentation, rules and scoring fields](README.md#fields-every-task-has). Note:

- **Each child's scoring applies.** Three children with `Score On Success = 10` add
  30, plus whatever the group itself scores.
- **Pause and resume propagate** to every child.
- **The group's own `Time Limit` lives on the [Task node](../nodes/task.md)**, not on
  the children. Children have no individual timeout.

That last point is the main limitation: you cannot give one child of a group its own
time limit. If you need per-branch timeouts, use real
[Parallel](../nodes/parallel.md) and [Join](../nodes/join.md) nodes so each branch
gets its own Task node.

---

## Group vs. Parallel + Join nodes

| Use **Parallel Group** | Use [Parallel](../nodes/parallel.md) + [Join](../nodes/join.md) |
|---|---|
| Two or three simple concurrent tasks | Branches with several steps each |
| You want the graph to stay compact | You want to see the structure in the graph |
| The branches share one outcome | Each branch needs its own timeout, retry or routing |
| No per-child failure routing needed | You need a `Failed` pin per branch |

---

## Dependencies

| Depends on | Why |
|---|---|
| The child tasks | Whatever you add |

---

## Example use case: narration alongside a physical step

**Goal:** while the trainee puts on the helmet, a narration line plays. Move on when
the helmet is on — the narration should not hold things up.

1. Add a [Task node](../nodes/task.md), **Task** = **Parallel Group**.
2. Under **Tasks**, click **+** twice.
3. **Child 0** = [Wait For Event](wait-for-event.md):
   - Event Tag = `SimFlow.Event.Grab`
   - Expected Payload → Required Tags = `Item.Helmet`
4. **Child 1** = [Delay](delay.md), Duration = `8` (the length of the narration),
   with your audio triggered from the Instruction UI.
5. Set **Wait For All** to **off** — the first to finish ends the group, so donning
   the helmet early moves things along.
6. Turn **Fail If Any Child Fails** off, since the delay cannot fail.
7. Set **Display Name** to `Don the helmet`.

If instead the trainee must *both* don the helmet **and** hear the whole briefing,
leave **Wait For All** on.

---

## Common pitfalls

**The group never finishes.**
`Wait For All` is on and one child is waiting for something that never arrives. Add
a `Time Limit` on the Task node.

**A child's work is cut short.**
`Wait For All` is off, so the first child to finish ended the group and the others
with it.

**The score is higher than expected.**
Every child's `Score On Success` contributes, on top of the group's own.

**One child needs its own timeout.**
Not supported inside a group — the `Time Limit` belongs to the Task node and covers
the whole group. Use [Parallel](../nodes/parallel.md) + [Join](../nodes/join.md)
nodes instead.

**A child failure fails everything.**
`Fail If Any Child Fails` is on by default. Turn it off for optional children.

---

*See also: [Parallel node](../nodes/parallel.md) · [Join node](../nodes/join.md) ·
[Task node](../nodes/task.md) · [Task Reference](README.md)*
