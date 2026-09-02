# Troubleshooting

Symptoms first. Find the one that matches, then follow the link for detail.

---

## Start here: the two diagnostic tools

### The debug overlay

```
SimFlow.Debug 1
```

Draws live flow state on screen: the active node, the current task, elapsed time and
the **whole blackboard**. `SimFlow.Debug 0` turns it off.

**Reach for this before anything else.** Most "it does not work" reports are
answered by seeing which node is actually active and what the blackboard holds.

You can also tick **Show Debug HUD** on the
[SimFlow Component](simflow-component.md) for a single flow, or call
`Get Debug Text` to drop the same information into a world-space VR widget.

### The log

SimFlow logs under the **`LogSimFlow`** category. Open the Output Log and filter for
it. Several misconfigurations log a warning that names the exact problem — a task
with no zone, an event task with no tag, a loop hitting its iteration cap.

**If a task is misbehaving, check the log before guessing.** The silent failures
listed below are the ones that log *nothing*, which is precisely why they are hard.

---

## Nothing happens at all

| Check | Detail |
|---|---|
| Is `Flow Asset` set on the component? | [SimFlow Component](simflow-component.md) |
| Is `Start Mode` still `Manual`? | Nothing runs until `Start Flow` is called |
| Does the component's `Entry Name` match a [Start](nodes/start.md) node's `Entry Name`? | Both default to `Default`; a rename breaks it |
| Is there a [Start](nodes/start.md) node at all? | |
| Is the `Out` pin of Start actually wired? | An unwired pin ends execution silently |

## The flow starts, then cannot find things

`Start Mode` is `Auto - On Begin Play`. Other actors may not have begun play yet, so
a task that immediately looks for a [zone](zones.md) or item finds nothing.

**Fix:** switch to **Auto - On First Tick**.

## The flow never ends

There is no [Finish](nodes/finish.md) node on the path that ran, or the pin leading
to it is unwired. **Execution running out of wiring does not complete a flow** — it
just stops, with the run state still `Running`.

## The flow ends too early

- A [Finish](nodes/finish.md) node was reached with `Stop Other Branches` on (the
  default) while another branch was still working. Converge branches with a
  [Join](nodes/join.md) first.
- A [Sub Flow](nodes/sub-flow.md) or [Task node](nodes/task.md)'s `Failed` pin is
  unwired, ending that line of execution.

---

## A task never completes

The most common category. Work down this list:

| Symptom | Likely cause |
|---|---|
| **Nothing in the log** | An empty [Actor Query](actor-query.md) or an empty [condition](conditions.md) — both are silently false |
| [Wait For Condition](tasks/wait-for-condition.md) hangs | Its `Condition` slot is empty. An unset condition is false forever |
| [Place Object In Zone](tasks/place-object-in-zone.md) hangs | `Accepted Items` is empty — nothing can ever match |
| [Ordered Sequence](tasks/ordered-sequence.md) hangs on a step | That step's `Target` is empty or does not match |
| [Quiz](tasks/quiz.md) hangs | Nothing calls `Submit Answer` |
| [Wait For Event](tasks/wait-for-event.md) hangs | The tag never arrives — see below |

**While developing, set a `Time Limit` on the [Task node](nodes/task.md)** and wire
`Timed Out` to a [Log Message](tasks/log-message.md). A hang becomes a message
naming the task.

## A task completes instantly when it should wait

Several tasks pass through by design when a required field is empty — this is so a
half-built flow still runs end to end:

| Task | Empty field | Result |
|---|---|---|
| [Task node](nodes/task.md) | `Task` | Warns, triggers `Completed` |
| [Wait For Event](tasks/wait-for-event.md) | `Event Tag` | Warns, **succeeds** |
| [Ordered Sequence](tasks/ordered-sequence.md) | `Steps` | Warns, **succeeds** |
| [Sub Flow](nodes/sub-flow.md) | `Sub Flow` | Warns, triggers `Completed` |

All four log a warning. Check `LogSimFlow`.

## A task fails immediately

| Task | Cause |
|---|---|
| [Place Object In Zone](tasks/place-object-in-zone.md) | The `Zone` query resolved nothing. Logs *"could not resolve a SimFlow Zone"* |
| [Ordered Sequence](tasks/ordered-sequence.md) | `Event Tag` is empty. Logs and fails |

---

## Events do not reach the flow

**By far the most common first bug:** broadcasting `self` from inside a **UMG
widget** graph. A `UUserWidget` is neither an Actor nor an Actor Component, so it can
never satisfy a payload check. It logs a warning naming the offending class.

**Fix:** broadcast from the owning **actor** with `Payload = self`.

Other causes:

| Check | Detail |
|---|---|
| Does the tag actually match? | Check `Match Child Tags`; a mismatch is silent |
| Is the broadcast running at all? | Put a print node next to it |
| Are you on a client? | Events must reach the authority. Use the component's `Send Event`, which forwards |
| Did you target the right flow? | `Broadcast Flow Event` hits **every** running flow; `Send Flow Event` targets one by `Flow Save Id` |

See [Events](events.md).

---

## Placement and zones

| Symptom | Cause |
|---|---|
| **Objects count as placed while still held** | Held state is not wired. Call `Set Held(true/false)` from your grab logic — attachment alone is unreliable in VR |
| **Nothing is ever tracked** | No collision overlap, or the box is too small. Turn on the zone's `Draw Debug` — silver means nothing tracked |
| **The player pawn is tracked** | `Require Identity Component` was turned off |
| **Objects settle then un-settle repeatedly** | Jitter above `Settle Speed Threshold`. Raise `Settle Time` or the threshold |
| **The wrong zone is used** | Two zones share a tag; tag queries take the **first** world match |

See [Zones](zones.md).

---

## Matching and identity

| Symptom | Cause |
|---|---|
| **The right object is not recognised** | It has no [Identity](identity.md) component, or no tags on it |
| **Everything is "No Match", never a near miss** | Your tag tree is flat. Confusable things must share a parent — see [depth matters](identity.md#depth-matters-more-than-you-would-expect) |
| **`Require All Tags` blocks everything** | With several tags requested, the actor needs **all** of them. Set it to `false` for "any" |
| **Tags on a placed instance do not apply to spawned copies** | The component was added to the level instance, not the Blueprint |
| **Setting `Identity Id` did not help** | **Nothing in SimFlow reads `Identity Id`.** Use Identity Tags — see [Identity](identity.md#identity-id--read-this-before-using-it) |

---

## Conditions and branching

| Symptom | Cause |
|---|---|
| **A branch case never fires** | Its condition slot is empty — an unset condition is **false** |
| **Everything goes to `Default`** | The conditions read a key that does not exist. Missing keys return `Result When Key Missing` (default `false`) |
| **An `All Of` always passes** | It has no children. An empty AND is **true** |
| **The wrong case fires** | Cases are evaluated top to bottom, first match wins. Put the most specific first |
| **Execution stops at a branch** | Nothing matched and `Has Default Pin` is off |

See [Conditions](conditions.md) and [Branch](nodes/branch.md).

---

## Blackboard

| Symptom | Cause |
|---|---|
| **A value is never seen** | Key name mismatch. Typos are silent — print the blackboard with `SimFlow.Debug 1` |
| **A counter became `"111"`** | The value type is `String` with `Add` on, so it concatenated. Use `Int` |
| **An object reference is null after loading** | Object values are **stripped on save**, by design |
| **`Get Score` always returns 0** | `Score On Success` / `Score On Failure` default to `0` on every task. Scoring is opt-in |

See [Blackboard](blackboard.md).

---

## Loops

| Symptom | Cause |
|---|---|
| **The body runs once, then stops** | The `Continue` pin is not wired back. The classic Loop mistake |
| **Log says "hit MaxIterations"** | `Iterations` is 0 and the break condition never passes |
| **The body never runs** | The break condition is already true on entry — it is checked **before** each iteration |
| **The loop stalls partway** | Something in the body ended without returning to `Continue`, often an unwired `Failed` pin |

See [Loop](nodes/loop.md).

## Joins

**The flow stalls at a join** — `Num Inputs` is higher than the number of branches
that actually arrive. In `Wait For All` mode the join then waits forever, silently.
Match `Num Inputs` to the branches you wired. See [Join](nodes/join.md).

---

## Editing and assets

| Symptom | Cause |
|---|---|
| **Editing the flow mid-play changes nothing** | Nodes are duplicated into the instance at start. Stop and restart the flow |
| **Wires moved after adding a Branch case** | Pins are named by index; inserting in the middle renumbers later ones. Add at the end |
| **The editor hangs on Play** | A [Sub Flow](nodes/sub-flow.md) calls itself, directly or in a cycle. There is no recursion guard |
| **No SimFlow entry in the Content Browser** | The plugin is not loaded, or the editor module failed to build. See [Getting Started](getting-started.md) |

---

## Multiplayer

| Symptom | Cause |
|---|---|
| **Clients see nothing** | `Replicate Flow` is on but the owning actor does not replicate. Host on Game State or Player State |
| **Client controls do nothing** | The PlayerController has no **SimFlow Player Component**, so there is no route to the server |
| **Save does nothing** | Save and load are **authority-only**; on a client they log and return |
| **Client UI has no blackboard values** | `Replicate Blackboard` is off |

See [SimFlow Component · Networking](simflow-component.md#field-by-field-breakdown).

---

## Still stuck?

1. `SimFlow.Debug 1` — which node is actually active?
2. Filter the Output Log by `LogSimFlow`.
3. Replace the suspect task with a [Log Message](tasks/log-message.md) — does the
   flow reach that node at all?
4. Check the field the task needs is not empty. **Most silent hangs are an empty
   [Actor Query](actor-query.md) or an empty [condition](conditions.md)**, and
   neither logs anything.

---

*See also: [Getting Started](getting-started.md) · [Glossary](glossary.md) ·
[Documentation index](README.md)*
