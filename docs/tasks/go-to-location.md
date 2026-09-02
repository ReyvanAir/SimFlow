# Go To Location

**Class:** `USimFlowTask_GoToLocation`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Go To Location**

---

## Overview / Purpose

Blocks until the **player pawn reaches a location**. The bread and butter of VR
tutorials: "walk to the control panel", "move to the muster point".

It watches the local player pawn — in a VR project, the VR pawn — and succeeds when
it is within `Acceptance Radius` of the target.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Target Location** | Vector | `0,0,0` | **Yes** | The destination. Has a **movable widget in the viewport** — drag it rather than typing coordinates. |
| **Target From Blackboard Key** | Name | `None` | No | *Advanced.* When set, the target is read from this [blackboard](../blackboard.md) vector key instead. |
| **Relative To Flow Owner** | Bool | `false` | No | Treat `Target Location` as relative to the flow owner actor. |
| **Acceptance Radius** | Float (cm, min 1) | `150.0` | No | How close counts as arrived. |
| **Ignore Z** | Bool | `true` | No | Ignore the vertical axis. |
| **Draw Debug Sphere** | Bool | `false` | No | *Advanced.* Draws a sphere at the target while the task runs. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Target Location left at `0,0,0`** | The target is the world origin. The trainee must walk there — usually not what you meant, and it looks like the task is broken. |
| **Target From Blackboard Key set but the key is missing** | The key reads as a zero vector, so the target becomes the origin (or the owner's location with `Relative To Flow Owner` on). |
| **No player pawn** | The distance check cannot run and the task waits. |
| **Acceptance Radius too small** | In VR the pawn's origin may never get close enough. `150` cm is a sensible floor. |
| **Ignore Z off with a target at floor level** | The HMD is ~170 cm above the floor, so the 3D distance never drops below a small radius. **This is why `Ignore Z` defaults to on.** |

The origin-target case is the common one: an unset `Target Location` is a *valid*
location, so nothing warns you.

---

## Ignore Z and VR

`Ignore Z` defaults to **on** for a reason. A pawn's location in VR sits at the
play-space floor or at the HMD depending on your rig, and the HMD height varies with
the person wearing it. Measuring 3D distance to a floor-level target then depends on
the trainee's height, which makes the radius unreliable.

Leave it on unless you genuinely need a vertical component — different floors of a
building, say.

---

## Resolving the target

Precedence:

1. **Target From Blackboard Key**, if set — read the vector from the blackboard.
2. Otherwise **Target Location**.
3. If **Relative To Flow Owner** is on, the result is offset by the flow owner
   actor's transform.

`Get Resolved Target Location` returns the final world position, which is useful for
driving a waypoint marker in your UI.

---

## Dependencies

| Depends on | Why |
|---|---|
| A player pawn | The thing being measured |
| [Blackboard](../blackboard.md) | Only for `Target From Blackboard Key` |

---

## Example use case: walk to the control panel

**Goal:** the trainee must move to the panel before the next step.

1. Add a [Task node](../nodes/task.md), **Task** = **Go To Location**.
2. Select the Task node. In the viewport, **drag the Target Location widget** to the
   spot in front of the panel — no need to type coordinates.
3. Set **Acceptance Radius** to `200` — generous enough for a room-scale play space.
4. Leave **Ignore Z** on.
5. Tick **Draw Debug Sphere** while testing so you can see the target.
6. Set **Instruction** to `Walk to the control panel`.
7. On the Task node, set **Time Limit** to `90` and wire `Timed Out` to a hint that
   highlights the panel.

### Variant: a destination chosen at runtime

For a target picked during the run — say a randomly chosen fault location:

1. Have your Blueprint write the vector with `Set Vector` on the flow's blackboard,
   key `FaultLocation`.
2. Set **Target From Blackboard Key** to `FaultLocation`.
3. `Target Location` is then ignored.

---

## Common pitfalls

**The trainee is standing on the spot and nothing happens.**
`Acceptance Radius` is too small, or `Ignore Z` was turned off and the HMD height is
being counted. Turn on `Draw Debug Sphere` to see where the target actually is.

**The task wants them to walk to the middle of the map.**
`Target Location` is `0,0,0` and was never set — or a blackboard key was named but
never written, which reads as a zero vector.

**The task completes immediately.**
The target is already within the radius. With `Relative To Flow Owner` on and a
target of `0,0,0`, the target *is* the owner.

**It works in the editor but not in VR.**
Check `Ignore Z`, and make sure you are measuring to the pawn, not the camera.

**The waypoint marker is in the wrong place.**
Use `Get Resolved Target Location` rather than reading `Target Location`, so
relative and blackboard forms are accounted for.

---

*See also: [Wait For Condition](wait-for-condition.md) ·
[Player Near Location condition](../conditions.md#player-near-location) ·
[Task Reference](README.md)*
