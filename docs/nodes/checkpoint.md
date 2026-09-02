# Checkpoint node

**Class:** `USimFlowNode_Checkpoint`
**Add via:** right-click → **Persistence → Checkpoint**
**Pins:** In → Out

---

## Overview / Purpose

The Checkpoint node **marks a safe resume point**, and can **auto-save** the flow
when execution reaches it.

It does two jobs:

1. It fires the component's **On Checkpoint Reached** event, so your UI can show
   "Progress saved" or update a stage indicator.
2. It is the anchor for the **From Last Checkpoint** load mode — a flow loaded that
   way re-runs from the last checkpoint passed, rather than restoring the exact
   mid-task state.

Execution passes straight through; it never blocks.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Checkpoint Id** | Name | `None` | Recommended | Identifies this checkpoint. |
| **Auto Save** | Bool | `false` | No | Write the flow to a SaveGame slot as soon as this node is reached. |
| **Save Slot Name** | String | *empty* | No | Only shown when Auto Save is on. **Leave empty to use the component's default slot.** |
| **Save User Index** | Int | `0` | No | Only shown when Auto Save is on. |

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Checkpoint Id is `None`** | The node still fires its event and still auto-saves. But your UI cannot tell checkpoints apart, and a debrief cannot report which stage was reached. **Set it.** |
| **Save Slot Name empty, Auto Save on** | Uses the component's `Default Save Slot Name`. This is the normal setup. |
| **Auto Save on, running on a client** | Save is authority-only. On a client it logs and does nothing. |
| **The flow has no checkpoints, loaded with `From Last Checkpoint`** | There is no checkpoint to resume from, so there is nothing to re-run from. Use `Exact State` instead. |

---

## Auto Save cost

Auto-saving writes a SaveGame file synchronously at the moment the node executes.
Placing checkpoints between stages is fine; placing one inside a
[Loop](loop.md) body means a disk write **every iteration**, which will hitch. Put
the checkpoint outside the loop.

---

## Dependencies

| Depends on | Why |
|---|---|
| [SimFlow Component](../simflow-component.md) | Supplies the default save slot and raises the event |

---

## Example use case: stage boundaries in a long exercise

**Goal:** a 40-minute exercise the trainee can leave and resume at the start of the
stage they reached.

1. On the component, set **Default Save Slot Name** (e.g. `TraineeProgress`) and
   tick **Auto Resume From Save On Begin Play**.
2. Set **Default Load Mode** to **From Last Checkpoint**.
3. Between each stage, right-click → **Persistence → Checkpoint**.
4. Give each a distinct **Checkpoint Id**: `Stage1Complete`, `Stage2Complete`, …
5. Tick **Auto Save** on each. Leave **Save Slot Name** empty so they all use the
   component's slot.
6. Bind **On Checkpoint Reached** in your HUD to flash "Progress saved".

```
   stage 1 ──▶ ┌────────────────┐ ──▶ stage 2 ──▶ ┌────────────────┐ ──▶ stage 3
               │ Checkpoint     │                 │ Checkpoint     │
               │ Stage1Complete │                 │ Stage2Complete │
               │ auto save      │                 │ auto save      │
               └────────────────┘                 └────────────────┘
```

Re-launching resumes at the start of the last completed stage. Note that
**From Last Checkpoint restores the blackboard and re-runs from the checkpoint** —
it does not restore a half-finished task.

---

## Common pitfalls

**Loading resumes at the very beginning.**
No checkpoint had been passed yet, or the flow has none. `From Last Checkpoint` has
nothing to anchor to.

**Loading restores a half-finished task instead of the stage start.**
The load mode is `Exact State`, not `From Last Checkpoint`.

**The game hitches repeatedly.**
An auto-saving checkpoint is inside a loop. Move it out.

**Nothing is saved in multiplayer.**
Save is authority-only.

**The UI cannot tell which checkpoint fired.**
`Checkpoint Id` is `None` on all of them.

---

*See also: [SimFlow Component · Save and load](../simflow-component.md#save-and-load) ·
[Loop node](loop.md) · [Node Reference](README.md)*
