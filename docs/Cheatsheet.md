# SimFlow cheat sheet

## Component API (Blueprint)

```
// Lifecycle
StartFlow()                    StartFlowFromEntry(Name)
StopFlow()                     RestartFlow()
PauseFlow()   ResumeFlow()     TogglePause()

// Player controls
RetryCurrentTask()  SkipCurrentTask()  FailCurrentTask()

// Events
SendEvent(GameplayTag, Payload)

// Save / load
QuickSave()  QuickLoad()
SaveFlowToSlot(Slot, UserIndex)
LoadFlowFromSlot(Slot, UserIndex, LoadMode)
HasSaveInSlot(Slot, UserIndex)
SaveFlowState() -> FSimFlowSaveState
LoadFlowState(State, LoadMode)

// Queries
GetRunState()  IsFlowRunning()  IsFlowPaused()
GetCurrentTask()  GetCurrentTaskName()  GetCurrentInstruction()
GetProgress()  GetScore()  GetBlackboard()  GetDebugText()
```

## Component events

`OnFlowStarted` · `OnFlowPaused` · `OnFlowResumed` · `OnFlowFinished(State)`
`OnTaskStarted(Node, Task)` · `OnTaskFinished(Node, Task, Result)` · `OnTaskRetried(Node, Task)`
`OnCheckpointReached(Checkpoint)` · `OnQuizPresented(Quiz)`

## Writing a task (Blueprint)

1. New Blueprint → parent class `SimFlowTask`.
2. Set `Display Name` and `Instruction` in Class Defaults.
3. Implement **On Task Start** — spawn markers, bind delegates, highlight objects.
4. Call **Finish Task** (Succeeded / Failed) when the player is done.
5. Implement **On Task End** to clean up. It runs on skip, fail and abort too.

Useful inside a task: `Get Blackboard`, `Get Flow Owner`, `Get Player Pawn`,
`Get Saved Value` / `Set Saved Value` (survives save/load).

## Writing a condition (Blueprint)

1. New Blueprint → parent class `SimFlowCondition`.
2. Implement **Evaluate** → return a bool.
3. Drop an instance into a Branch case, a Wait For Condition task, or a Task
   node's Abort Condition slot.

## Talking to the flow from the world

```
// In your grab component / button / anim notify:
Broadcast Flow Event (WorldContext, Tag = SimFlow.Event.Grab, Payload = self)
```

A `Wait For Event` task listening for `SimFlow.Event.Grab` (or any parent tag,
with Match Child Tags on) completes.

## Blackboard keys the built-ins use

| Key | Written by |
|---|---|
| `Score` | `ScoreOnSuccess` / `ScoreOnFailure` on any task, `AddScore` |
| `Mistakes` | Quiz task on a wrong answer |
| `LastResult` | every task when it finishes |
| `LastAnswerIndex`, `LastAnswerCorrect` | Quiz task |


## Multiplayer

```
Component:  bReplicateFlow = true, on a REPLICATED actor (GameState / PlayerState)
PlayerController BP:  add a SimFlow Player Component
```

Client-safe calls (they forward to the server themselves):

```
StartFlow / StopFlow / RestartFlow
PauseFlow / ResumeFlow / TogglePause
RetryCurrentTask / SkipCurrentTask / FailCurrentTask
SendEvent(Tag, Payload)
SubmitQuizAnswer(Index)
Broadcast Flow Event (statics)
Request Flow Control (statics)   <- instructor panels
```

Server-only: `SaveFlowToSlot`, `LoadFlowFromSlot`, `QuickSave`, `QuickLoad`.

Client-side reads (all resolve from the replicated state + the local flow asset):

```
GetRunState / IsFlowRunning / IsFlowPaused
GetCurrentTask / GetCurrentTaskName / GetCurrentInstruction
GetCurrentQuiz / GetCurrentTaskRemainingTime
GetProgress / GetScore / GetBlackboard
GetNetState / IsClientMirror / HasFlowAuthority
OnNetStateChanged   <- fires when the server's state arrives
```

Gate instructor-only controls by overriding **Is Request Authorised** on the
SimFlow Player Component. Client requests are never trusted; they all pass
through it on the server.

## Console

```
SimFlow.Debug 1     on-screen status overlay for every running flow
SimFlow.Debug 0     off
```

## Gotchas

* A Task node's unwired `Failed` / `Skipped` / `TimedOut` pins fall through to
  `Completed`. Turn off `bFallbackToCompleted` if you want a hard stop instead.
* `Join` in *Wait For Any* mode stays alive after firing so it can absorb the
  losing branch. The losing branch's task keeps running until the flow ends — for
  a timeout on a single task, prefer the Task node's own `TimeLimit`.
* Object references in the blackboard are not saved. Re-resolve them in
  **On Load Task State**.
* The editor graph is stripped from packaged builds; only the node data ships.
  Never store gameplay state on the graph node — put it on the runtime node.
* In multiplayer, `GetCurrentTask()` on a client returns the **authored template**
  task, not a live one. Its text is correct; its runtime counters are not — read
  those from `GetNetState()` instead.
