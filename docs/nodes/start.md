# Start node

**Class:** `USimFlowNode_Entry`
**Add via:** right-click → **Flow Control → Start**
**Pins:** *(no input)* → Out

---

## Overview / Purpose

The Start node is **where execution begins**. Every flow needs at least one, and a
new flow asset is created with one already in place.

A flow may have **several** Start nodes, each with its own name. That is how one
asset holds several related scenarios — a full run, a short demo, and a "resume at
part two" entry — chosen by the [SimFlow Component](../simflow-component.md) at
runtime.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Entry Name** | Name | `Default` | Yes | Pass this name to `Start Flow From Entry` to begin here. |

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Entry Name is `None` or blank** | No component setting can address it. The flow will not start from this node. |
| **Two Start nodes share a name** | Ambiguous — which one runs is not defined. Give every entry a distinct name. |
| **The component's `Entry Name` does not match any Start node** | The flow does not start. **Both default to `Default`**, so this only bites after a rename. |
| **The flow has no Start node** | Nothing can run. |
| **`Out` is unwired** | The flow starts and immediately ends. |

---

## Dependencies

| Depends on | Why |
|---|---|
| [SimFlow Component](../simflow-component.md) | Its `Entry Name` selects which Start node runs |

---

## Example use case: a demo entry alongside the full run

**Goal:** one asset that can run the whole 20-minute exercise, or jump to the
5-minute demo for a trade show.

1. Leave the existing Start node's **Entry Name** as `Default` and wire it to the
   full sequence.
2. Right-click → **Flow Control → Start** to add a second one.
3. Set its **Entry Name** to `Demo`.
4. Wire it to the shortened section.
5. At runtime, call **Start Flow From Entry** with `Demo`, or set the component's
   **Entry Name** to `Demo` before it auto-starts.

```
  ┌──────────────┐
  │ Start        │Out──▶ full exercise
  │ "Default"    │
  └──────────────┘

  ┌──────────────┐
  │ Start        │Out──▶ short demo
  │ "Demo"       │
  └──────────────┘
```

Both entries share the same [blackboard](../blackboard.md) keys and the same
[Finish](finish.md) nodes — only the path in differs.

---

## Common pitfalls

**The flow never starts after renaming an entry.**
The component's `Entry Name` still says `Default`. They must match exactly.

**Adding a second Start node made the first stop working.**
It did not — but if both are named `Default`, which one runs is undefined. Rename
one.

**The flow ends instantly.**
The `Out` pin is not wired to anything.

---

*See also: [Finish node](finish.md) · [SimFlow Component](../simflow-component.md) ·
[Node Reference](README.md)*
