# Example Workflows

Three complete, real-world flows built from the nodes and tasks in the reference.
Each shows the whole picture — level setup, tags, graph, and the UI bindings —
rather than one feature in isolation.

Work through them in order if you are new: each introduces a little more.

| Workflow | Shows |
|---|---|
| [Fire Extinguisher Drill](fire-extinguisher-drill.md) | Identity, zones, placement judging, near-miss feedback, scoring |
| [Valve Startup Procedure](valve-startup-procedure.md) | Ordered sequences, out-of-order handling, progress UI, checkpoints |
| [Assessment with Debrief](assessment-with-debrief.md) | Quizzes, branching on score, mistakes, sub-flows, a debrief screen |

---

## What they assume

All three assume you have completed [Getting Started](../getting-started.md) — the
plugin is installed, you can create a flow asset, and you have a
[SimFlow Component](../simflow-component.md) somewhere that runs it.

They also assume the **general shape** of a SimFlow project:

1. **Items and controls carry an [Identity](../identity.md)** with gameplay tags
   saying what they are.
2. **Props broadcast events** and know nothing about the exercise.
3. **The flow asset holds the answers** — which object is right, in what order, in
   which zone.

That separation is what lets you build one level and many exercises.

---

## A note on tags

Every example uses tags like `Item.Extinguisher.Foam` and `Zone.PartsBin`. **None of
these ship with the plugin** — the `SimFlow.*` event and mistake tags do, but your
own object hierarchy is yours to create in Project Settings → Gameplay Tags.

Group things that are plausibly confusable under a shared parent, or the near-miss
feedback these examples rely on will not work. See
[Identity · Depth matters](../identity.md#depth-matters-more-than-you-would-expect).

---

*See also: [Node Reference](../nodes/README.md) · [Task Reference](../tasks/README.md) ·
[Documentation index](../README.md)*
