# Finish node

**Class:** `USimFlowNode_Finish`
**Add via:** right-click → **Flow Control → Finish**
**Pins:** In → *(no output)*

---

## Overview / Purpose

The Finish node **ends the whole flow** and reports a result. It is the counterpart
to [Start](start.md), and the only node that terminates a run.

A flow may have as many Finish nodes as you like — a success ending, a failure
ending, an "aborted because the trainee left" ending — each reporting a different
result to your debrief UI.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Finish Mode** | Enum | `Complete (Success)` | Yes | The result reported for the whole run. |
| **Stop Other Branches** | Bool | `true` | No | Stop any still-running parallel branches. **Almost always what you want.** |

### Finish Mode

| Mode | Final run state |
|---|---|
| **Complete (Success)** | `Completed` |
| **Fail** | `Failed` |
| **Abort** | `Aborted` |

The final state arrives at your UI through the component's **On Flow Finished**
delegate, and is readable any time via `Get Run State`.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **The flow has no Finish node** | The flow runs to the end of its wiring and then simply sits there in the `Running` state. Nothing reports completion. |
| **`Stop Other Branches` off with branches still running** | The flow reports finished while other branches keep executing. Occasionally useful; usually a bug. |
| **Several Finish nodes reached at once** | The first one to execute ends the run; with `Stop Other Branches` on, the rest never arrive. |

The missing-Finish case is a common "the flow never ends" report — there is no
implicit completion when execution runs out of wiring.

---

## Dependencies

| Depends on | Why |
|---|---|
| Nothing | It only needs execution to reach it |

**Depended on by:** the [SimFlow Component](../simflow-component.md)'s
`On Flow Finished` delegate and `Get Run State`.

---

## Example use case: pass and fail endings

**Goal:** report success or failure so a debrief screen can show the right result.

1. After the final task, add a **Branch** ([Branch](branch.md)).
2. Give it one case: **Score Threshold**, `>=`, `70`, labelled `Passed`.
3. Right-click → **Flow Control → Finish**. Set **Finish Mode** to
   **Complete (Success)**. Wire the `Passed` case to it.
4. Add a second Finish node with **Finish Mode** = **Fail**. Wire the **Default**
   pin to it.
5. In your HUD Blueprint, bind **On Flow Finished** on the component and switch on
   the final state to show the right debrief.

```
                    ┌──────────┐   Passed   ┌────────────────┐
   last task ───────┤ Branch   ├───────────▶│ Finish         │
                    │ Score>=70│            │ Complete       │
                    │          │  Default   └────────────────┘
                    │          ├───────────▶┌────────────────┐
                    └──────────┘            │ Finish  (Fail) │
                                            └────────────────┘
```

The list of [mistakes](../glossary.md) collected on the instance is still available
at this point, which is what a debrief reads.

---

## Common pitfalls

**The flow never ends.**
There is no Finish node on the path that executed, or the pin leading to it is
unwired. Execution running out of wiring does not complete a flow.

**The flow ends while a parallel branch is mid-task.**
That is `Stop Other Branches` doing its job. If you wanted both to complete, join
them with a [Join](join.md) node set to **Wait For All** before finishing.

**`On Flow Finished` reports Completed when the trainee failed.**
The Finish node reached had `Finish Mode` left at the default. Add a separate Fail
node.

---

*See also: [Start node](start.md) · [Join node](join.md) ·
[SimFlow Component](../simflow-component.md) · [Node Reference](README.md)*
