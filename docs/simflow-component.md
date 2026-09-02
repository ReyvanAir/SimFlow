# SimFlow Component

**Component:** `SimFlow Component` (`USimFlowComponent`)
**Add via:** Add Component → SimFlow Component
**Class group:** SimFlow

---

## Overview / Purpose

The SimFlow Component **runs a flow**. It is the entire designer-facing runtime API:
start, pause, resume, retry, skip, fail, save, load.

Drop it on any actor — a Game Mode, a level actor, the Game State, or the VR pawn —
point it at a [flow asset](glossary.md), and choose when it starts.

### Which actor should host it?

| Host | Good for |
|---|---|
| **Game Mode** | A single-player scenario. Simple, and it exists before the level's actors do. |
| **A level actor** | A flow tied to one room or machine. |
| **Game State** | Multiplayer: **one shared scenario** everyone sees. |
| **Player State** | Multiplayer: **a flow per trainee**. |
| **VR pawn** | Flows that follow the player between levels — but note the pawn may respawn. |

For replication the host actor must itself replicate, which is why Game State and
Player State are the recommended homes there.

---

## Field-by-field breakdown

### SimFlow

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Flow Asset** | SimFlow Asset | *null* | **Yes** | The flow this component runs. |
| **Entry Name** | Name | `Default` | No | Which [Start](nodes/start.md) node to begin from. |

### SimFlow \| Start

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Start Mode** | Enum | `Manual` | When the flow begins — see below. |
| **Auto Start Delay** | Float (s, min 0) | `1.0` | Only shown when Start Mode is `Auto - After Delay`. |

### SimFlow \| Network

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Replicate Flow** | Bool | `false` | Run server-authoritatively and mirror to clients. Read-only at runtime. |
| **Net Refresh Interval** | Float (s, min 0.1) | `1.0` | How often the server refreshes the elapsed-time field. Structural changes replicate immediately. |
| **Replicate Blackboard** | Bool | `true` | Send the blackboard to clients so their UI can read score and answers. |

### SimFlow \| Save

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Flow Save Id** | Name | `None` | Identifies this flow inside a save file, and addresses it over the network. |
| **Default Save Slot Name** | String | `SimFlowSave` | Slot used by Quick Save / Quick Load. |
| **Default Save User Index** | Int | `0` | Platform user index. |
| **Auto Resume From Save On Begin Play** | Bool | `false` | When a save exists on BeginPlay, resume from it instead of starting fresh. |
| **Default Load Mode** | Enum | `Exact State` | `Exact State` or `From Last Checkpoint`. |

### SimFlow \| Debug / Advanced

| Field | Type | Default | Meaning |
|---|---|---|---|
| **Show Debug HUD** | Bool | `false` | Draw this flow's status on screen. Also toggled globally by `SimFlow.Debug`. |
| **Follow Game Pause** | Bool | `true` | Pause the flow when the game itself pauses. Authority only. |

---

## Start Mode

| Mode | Behaviour |
|---|---|
| **Manual** | Nothing happens until you call `Start Flow`. |
| **Auto - On Begin Play** | Starts on BeginPlay. |
| **Auto - On First Tick** | Starts on the first tick after BeginPlay. |
| **Auto - After Delay** | Starts `Auto Start Delay` seconds later. |

> **Prefer `Auto - On First Tick` over `On Begin Play`.** On BeginPlay, other actors
> in the level may not have begun play yet, so a flow that immediately looks for a
> [zone](zones.md) or an item can fail to find it. On First Tick every actor has
> begun play, which makes it the safer default.

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Flow Asset is null** | Nothing runs. `Start Flow` returns false. |
| **Entry Name does not match any Start node** | The flow cannot find an entry and does not start. Check the [Start](nodes/start.md) node's `Entry Name` — both default to `Default`. |
| **Flow Save Id is None** | Save/load still works via the slot name, but `Find Flow By Id` and client control routing cannot address this flow. **Set it if you use replication or more than one flow.** |
| **Replicate Flow on, owning actor does not replicate** | The flow runs on the server but clients never receive state. |
| **Save/load called on a client** | Logs and does nothing — save and load are authority-only. |
| **Start Mode = Manual and nothing calls Start Flow** | The flow sits at `Not Started` forever. A common "nothing happens" cause. |

---

## Controls

All of these are safe to call from either side. On a client with `Replicate Flow`
on, they forward to the server through the local **SimFlow Player Component** —
which the PlayerController must have for the forwarding to work.

| Function | Notes |
|---|---|
| **Start Flow** | Starts from `Entry Name` |
| **Start Flow From Entry** (Name) | Starts from a named [Start](nodes/start.md) node |
| **Stop Flow** | |
| **Restart Flow** | |
| **Pause Flow** / **Resume Flow** / **Toggle Pause** | |
| **Retry Current Task** | Honours the task's `Allow Retry` and `Max Retries` |
| **Skip Current Task** | Honours the task's `Allow Skip` |
| **Fail Current Task** | Drives the Task node's `Failed` pin |
| **Send Event** (Tag, Payload) | Raises an event tag on **this** flow |
| **Submit Quiz Answer** (Index) | Answers the [quiz](tasks/quiz.md) currently on screen |

---

## Save and load

Save and load are **authority-only**.

| Function | Notes |
|---|---|
| **Save Flow State** | Returns a save-state struct |
| **Load Flow State** (State, Load Mode) | Restores from a struct |
| **Save Flow To Slot** (Slot, User Index) | |
| **Load Flow From Slot** (Slot, User Index, Load Mode) | |
| **Quick Save** / **Quick Load** | Uses `Default Save Slot Name` and `Default Save User Index` |
| **Has Save In Slot** (Slot, User Index) | |

### Load modes

| Mode | Behaviour |
|---|---|
| **Exact State** | Restores the exact set of active nodes, including their elapsed times. |
| **From Last Checkpoint** | Restores the blackboard, then re-runs the flow from the last [checkpoint](nodes/checkpoint.md) passed. |

Object references in the blackboard are **stripped on save** — see
[Blackboard](blackboard.md#values-and-types).

---

## Queries

These work on clients too, reading replicated state and resolving names,
instructions and quiz content from the flow asset every machine already has.

| Function | Returns |
|---|---|
| **Get Flow Instance** / **Get Blackboard** | The live objects |
| **Get Run State** | `Not Started`, `Running`, `Paused`, `Completed`, `Failed`, `Aborted` |
| **Is Flow Running** / **Is Flow Paused** | |
| **Get Current Task** / **Get Current Task Name** / **Get Current Instruction** | For tutorial UI |
| **Get Progress** | 0–1 |
| **Get Score** | The `Score` blackboard key |
| **Get Current Task Remaining Time** | Seconds left on the task's time limit, or `-1` when it has none |
| **Get Current Quiz** | The quiz being asked, or null. On clients this is the authored template. |
| **Is Client Mirror** / **Has Flow Authority** | Which side am I on? |
| **Get Debug Text** | Multi-line status, ready for a world-space VR debug widget |

---

## Events

| Delegate | Fires when |
|---|---|
| **On Flow Started / Paused / Resumed** | |
| **On Flow Finished** (Final State) | |
| **On Task Started / Finished / Retried** | Drive your tutorial UI from these |
| **On Checkpoint Reached** | |
| **On Quiz Presented** | Show your quiz widget here |
| **On Net State Changed** | Fires on clients whenever replicated state changes; the server fires it too |

---

## Dependencies

| Depends on | Why |
|---|---|
| A [flow asset](glossary.md) | Nothing to run without one |
| **SimFlow Player Component** on the PlayerController | Only for client → server control forwarding when replicating |
| A replicating owner actor | Only when `Replicate Flow` is on |

---

## Example use case: a flow on the Game Mode

**Goal:** run a tutorial flow as soon as the level is ready, with a debug HUD while
building it.

1. Open your Game Mode Blueprint.
2. **Add Component → SimFlow Component**.
3. Set **Flow Asset** to your flow.
4. Set **Start Mode** to **Auto - On First Tick**.
5. Set **Flow Save Id** to something stable, e.g. `MainTutorial`.
6. Tick **Show Debug HUD** while you are building.
7. Press Play.

To let a button in the level advance the flow, from that Blueprint call
**Broadcast Flow Event** with the tag and `Payload = self`. See
[Events](events.md).

---

## Common pitfalls

**Nothing happens on Play.**
`Start Mode` is `Manual` and nothing calls `Start Flow`. Or `Flow Asset` is empty.

**The flow starts but immediately cannot find a zone or item.**
`Start Mode` is `Auto - On Begin Play`. Switch to **On First Tick**.

**The flow does not start and `Entry Name` looks fine.**
The [Start](nodes/start.md) node's own `Entry Name` was renamed. They must match.

**Client controls do nothing in multiplayer.**
The PlayerController has no **SimFlow Player Component**, so there is no route to
the server. Add it.

**Save does nothing in a multiplayer session.**
Save and load are authority-only; on a client they log and return.

**Editing the flow asset mid-play changes nothing.**
Nodes are duplicated into the instance when the flow starts. Stop and restart to
pick up authoring changes. See [Glossary · Asset vs. Instance](glossary.md).

**Two flows both respond to a `Broadcast Flow Event`.**
`Broadcast Flow Event` raises the tag on **every** running flow. To target one, use
`Send Flow Event` with that flow's `Flow Save Id`.

---

*See also: [Getting Started](getting-started.md) · [Events](events.md) ·
[Blackboard](blackboard.md) · [Troubleshooting](troubleshooting.md) ·
[Documentation index](README.md)*
