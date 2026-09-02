# Sub Flow node

**Class:** `USimFlowNode_SubFlow`
**Add via:** right-click → **Composition → Sub Flow**
**Pins:** In → Completed, Failed

---

## Overview / Purpose

The Sub Flow node **runs another flow asset as a child**. This is what makes flows
modular.

Extract "don the PPE" or "perform the safety check" into its own asset once, then
call it from every scenario that needs it. Fix it in one place and every caller
benefits.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Sub Flow** | SimFlow Asset | *null* | **Yes** | The child flow to run. |
| **Entry Name** | Name | `Default` | Yes | Which [Start](start.md) node of the child to begin from. |
| **Inherit Blackboard** | Bool | `true` | No | Copy the parent [blackboard](../blackboard.md) into the child when it starts. |
| **Write Back Blackboard** | Bool | `true` | No | Copy the child blackboard back into the parent when it ends. |

---

## Output pins

| Pin | Taken when |
|---|---|
| **Completed** | The child flow finished successfully |
| **Failed** | The child flow failed or aborted |

---

## Blackboard flow

The two blackboard settings decide how state moves between parent and child:

| Inherit | Write Back | Behaviour |
|---|---|---|
| on | on | **Default.** The child sees everything and its changes come back. Closest to "inline this section". |
| on | off | The child can read parent state but its changes are discarded — a sandbox. |
| off | on | The child starts clean and hands results back. Good for a self-contained scored section. |
| off | off | Fully isolated. |

Both operations are a **merge that overwrites by default**, not a replace. Keys the
child never touched keep their parent values on write-back.

Object references move fine here — this is an in-memory merge, not a save.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Sub Flow is null** | Logs a warning and **triggers `Completed`** — the node passes through rather than failing. |
| **Entry Name does not match a Start node in the child** | The child cannot start. |
| **The child has no [Finish](finish.md) node** | The child never finishes, so the parent waits forever on this node. |
| **A flow calls itself** | Infinite recursion. There is no depth guard — do not do it. |
| **`Failed` unwired** | Execution ends there when the child fails. |

That first row matters: an empty Sub Flow node is a silent pass-through, exactly
like an empty [Task node](task.md). It will not draw attention to itself.

---

## Pause, skip and fail

The Sub Flow node forwards these to the child:

- **Pausing** the parent pauses the child.
- **Skip** is always allowed on this node, and ends the child.
- **Fail** ends the child and leaves through `Failed`.

---

## Dependencies

| Depends on | Why |
|---|---|
| Another flow asset | The thing being run |
| [Blackboard](../blackboard.md) | For inherit / write-back |

---

## Example use case: a reusable PPE check

**Goal:** every scenario starts by making the trainee put on a helmet and gloves.

1. Create a new flow asset `F_PPECheck` with its own Start, two
   [Task nodes](task.md), and a [Finish](finish.md) node.
2. Have it write a result key — a [Set Blackboard Value](set-blackboard.md) node
   setting `PPEComplete = true` before Finish.
3. In your main flow, right-click → **Composition → Sub Flow**.
4. Set **Sub Flow** to `F_PPECheck`, leave **Entry Name** as `Default`.
5. Leave **Inherit Blackboard** and **Write Back Blackboard** on.
6. Wire **Completed** to the rest of the scenario, and **Failed** to an abort path.

```
   Start ──▶ ┌────────────────┐ Completed ──▶ main scenario
             │ Sub Flow       │
             │ F_PPECheck     │ Failed ─────▶ abort
             └────────────────┘
```

Because write-back is on, `PPEComplete` is readable in the parent afterwards — so a
later [Branch](branch.md) can check it.

---

## Common pitfalls

**The node completes instantly and nothing happened.**
`Sub Flow` is empty. Check the log for *"has no asset assigned"*.

**The parent hangs on the sub flow.**
The child has no [Finish](finish.md) node, so it never reports completion.

**Child results are missing in the parent.**
`Write Back Blackboard` is off.

**The child overwrote a parent key unexpectedly.**
Write-back overwrites by default. Turn it off, or use distinct key names in the
child.

**The child cannot find its entry.**
Its Start node was renamed; `Entry Name` must match.

**The editor hangs on Play.**
A flow is calling itself, directly or in a cycle. There is no recursion guard.

---

*See also: [Task node](task.md) · [Blackboard](../blackboard.md) ·
[Start node](start.md) · [Node Reference](README.md)*
