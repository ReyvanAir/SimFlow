# Branch node

**Class:** `USimFlowNode_Branch`
**Add via:** right-click → **Flow Control → Branch**
**Pins:** In → Case 0 … Case N, Default

---

## Overview / Purpose

The Branch node is **condition-based routing**. It evaluates each case in order and
leaves through the **first one that passes**, or through `Default` when none do.

It is the main decision point in a flow: pass/fail routing, difficulty selection,
remediation, "have they already done this?".

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Cases** | Array of cases | *empty* | Yes | The ordered list of conditions. Each adds an output pin. |
| **Fire All Matching Cases** | Bool | `false` | No | Fire **every** case that passes instead of only the first — fans out in parallel. |
| **Has Default Pin** | Bool | `true` | No | Show a `Default` pin taken when nothing matched. |

### Each case

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Label** | String | *empty* | Label shown on the output pin. Empty gives `Case N`. |
| **Condition** | Instanced [condition](../conditions.md) | *null* | The test. An empty slot is **false**. |

Pins are named `Case_0`, `Case_1` … internally and rebuilt whenever you add or
remove a case. **Links are preserved** as long as the pin name still exists — so
inserting a case in the middle shifts the ones after it and can move your wires.
Add new cases at the end where you can.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **A case has no condition** | Treated as **false**. That case never fires. |
| **No cases at all** | Nothing matches → `Default` fires, or the node just ends if `Has Default Pin` is off. |
| **Nothing matches, `Has Default Pin` off** | The node finishes without triggering anything — **that line of execution silently stops.** |
| **A case pin is unwired** | It fires and goes nowhere. |
| **`Fire All Matching Cases` on** | Every passing case fires. Downstream sections run in parallel — converge them with a [Join](join.md). |

The combination of "no match" and "no default pin" is the quiet one: it is a
legitimate way to end a branch, and indistinguishable from a mistake.

---

## Order matters

Cases are evaluated **top to bottom** and the first pass wins (unless
`Fire All Matching Cases` is on). Put the **most specific** case first:

```
Case 0:  Score >= 90     "Distinction"
Case 1:  Score >= 70     "Pass"
Default:                 "Fail"
```

Reverse those two and everything scoring 90+ leaves through `Pass`, because it is
tested first and passes.

---

## Dependencies

| Depends on | Why |
|---|---|
| [Conditions](../conditions.md) | Every case needs one |
| [Blackboard](../blackboard.md) | Indirectly — most conditions read it |

---

## Example use case: three-way routing on score

**Goal:** route to distinction, pass or remediation at the end of an assessment.

1. Right-click → **Flow Control → Branch**.
2. Under **Cases**, click **+** twice.
3. **Case 0:** Label `Distinction`. Condition = **Score Threshold**, `>=`, `90`.
4. **Case 1:** Label `Pass`. Condition = **Score Threshold**, `>=`, `70`.
5. Leave **Has Default Pin** on — `Default` is the remediation route.
6. Wire each pin to its section.

```
                     ┌──────────────────┐
   last task ────────┤In   Branch       │Distinction──▶ certificate
                     │                  │Pass────────▶ debrief
                     │                  │Default─────▶ remediation
                     └──────────────────┘
```

To require **both** a score and a mistake limit for the pass, set that case's
condition to **All Of** with two children — see
[Conditions](../conditions.md#all-of-and--any-of-or).

---

## Common pitfalls

**A case never fires.**
Its condition slot is empty (empty = false), or an earlier, broader case is
catching everything first.

**Everything goes to Default.**
The conditions are reading a blackboard key that does not exist. Missing keys make
**Blackboard Compare** return `Result When Key Missing`, default `false`. Check with
`SimFlow.Debug 1`.

**Execution just stops at the branch.**
Nothing matched and `Has Default Pin` is off, or the matching pin is unwired.

**Wires moved after I added a case.**
Pins are named by index. Inserting in the middle renumbers the later ones. Add at
the end.

**Two branches run at once unexpectedly.**
`Fire All Matching Cases` is on.

---

*See also: [Conditions](../conditions.md) · [Random Branch](random-branch.md) ·
[Join node](join.md) · [Node Reference](README.md)*
