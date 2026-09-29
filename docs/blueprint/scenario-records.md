# Scenario records

**Library:** `USimFlowStatics`
**Needs a target?** No.

[Score](../blackboard.md) is a blackboard key, which means it belongs to one run and
dies with it. These nodes keep the best score a flow has ever seen, across runs and
across sessions.

## Where the record lives

High scores go in their own save slot, `SimFlowScenarios` by default — not in the
slot your flow saves to. That separation is the whole point:

| What you do | Run save | High score |
|---|---|---|
| Quick Save, then Quick Load | Restored | Untouched |
| Delete Flow Save | Gone | Survives |
| Start the drill again from scratch | Overwritten on next save | Survives |
| Reset Scenario Record | Untouched | Gone |

Records are keyed by **Flow Save Id**, the same key run saves use, so every drill
keeps its own best. A flow with no `Flow Save Id` set falls back to its owning
actor's name, exactly as saving does.

## The nodes

| Node | Returns | Notes |
|---|---|---|
| Submit High Score (Flow Save Id, Score, Slot Name, User Index) | bool | True only when the record moved |
| Get High Score (Flow Save Id, Slot Name, User Index) | float | `0` when nothing is stored |
| Get Play Count (Flow Save Id, Slot Name, User Index) | int | `0` when nothing is stored |
| Has Scenario Record (Flow Save Id, Slot Name, User Index) | bool | Tells a stored `0` from no record |
| Get Scenario Record (Flow Save Id, Slot Name, User Index) | Struct | Best score, play count, last outcome, last played, runs |
| Get All Scenario Records (Slot Name, User Index) | Map of Name to Struct | The whole table |
| Would Beat High Score (Flow Save Id, Score, Slot Name, User Index) | bool | Asks without writing |
| Reset Scenario Record (Flow Save Id, Slot Name, User Index) | bool | True when there was one to clear |
| Reset All Scenario Records (Slot Name, User Index) | bool | Empties the table |
| Export Scenario Records To CSV (File Name, Slot Name, User Index) | bool, File Path | One row per run. See [below](#run-history-and-export) |
| Export Scenario Records To JSON (File Name, Slot Name, User Index) | bool, File Path | Summary per scenario, runs nested inside |

Leave **Slot Name** empty and you get `SimFlowScenarios`. **User Index** is the
platform user index and defaults to `0`, same as everywhere else in SimFlow.

**Has Scenario Record** is not redundant. `Get High Score` returns `0` for a flow with no
record, and `0` is a real score once a task carries a negative
`Score On Failure`. Check `Has Scenario Record` before showing a number to anyone.

### "High score" and "best score" are the same number

`Get High Score` and `Get Play Count` are shortcuts. Each one calls
`Get Scenario Record` and hands you a single field:

```
Get High Score  ≡  Get Scenario Record ▸ Best Score
Get Play Count  ≡  Get Scenario Record ▸ Play Count
```

Split the struct pin and Unreal names each sub-pin `Return Value <Field>`, so the same
value appears as **Return Value Best Score**. The accessor says "high score", the field
says "best score"; nothing differs but the name.

Which means three nodes off one component — `Get High Score`, `Get Play Count` and
`Get Scenario Record` — are three separate reads of the same file for data the last one
already contains. Call `Get Scenario Record` once into a variable and read its pins.

## Higher replaces, equal does not

A submitted score has to be **strictly higher** than the stored one to take its
place. Submit 200 against a stored 200 and nothing changes: the record keeps its
original timestamp, and `Record Play` returns `false`. That false means "did
not beat it", not "something went wrong".

## Run history and export

Every play `Record Play` counts is also added to the record's **Runs** list: when it
ended (UTC), how it ended, its score, how long the flow ran with pauses left out, and
whether that score was allowed to count towards the best. The list keeps the last 100
runs per scenario. `Play Count` keeps counting past that, and so does the best score.

`Submit High Score` adds no run, since it is not a play.

To get the runs out of the game, call either export node. Any Blueprint can do it:
a results screen button, a debug key, or a check on `Get Play Count` after the last
run of a session.

```
Button ▸ On Clicked ──▶ Export Scenario Records To CSV  (File Name: "Session_A")
                   ──▶ Export Scenario Records To JSON (File Name: "Session_A")
                   ──▶ Set Text ◀── File Path
```

Files land in `Saved/SimFlow/Exports/`: the project's `Saved` folder in the editor,
the game's own `Saved` folder in a packaged build. **File Name** is a plain name. The
extension is added for you, and anything with `/`, `\`, `:` or `..` is refused, so a
Blueprint cannot write outside that folder. Leave it empty for a timestamped name
such as `SimFlowRecords_20260929_143000`. The node returns `false` and writes nothing
when the slot holds no records.

**CSV:** one row per run, sorted by scenario, with the scenario's best score and play
count repeated on each row so a spreadsheet can filter without a lookup:

```
Scenario,Play,EndedAtUtc,Outcome,Score,ElapsedSeconds,ScoreCounted,BestScore,PlayCount
Drill_A,1,2026-09-29T06:10:04.512Z,Completed,80.0,42.5,true,95.0,2
Drill_A,2,2026-09-29T06:12:31.090Z,Failed,95.0,61.2,false,95.0,2
```

`Play` is the play's number across the scenario's whole life, so it stays right after
the oldest runs are dropped. A scenario played only before run history existed still
gets one row, with the run columns empty and its best score and play count filled in.

**JSON:** the same records, one object per scenario with its runs nested inside:

```json
{
  "schemaVersion": 1,
  "exportedAt": "2026-09-29T06:15:00.000Z",
  "scenarios": {
    "Drill_A": {
      "bestScore": 95, "bestScoreAt": "2026-09-29T06:12:31.090Z", "playCount": 2,
      "lastOutcome": "Failed", "lastPlayedAt": "2026-09-29T06:12:31.090Z",
      "runs": [
        { "endedAt": "2026-09-29T06:10:04.512Z", "outcome": "Completed", "score": 80,
          "elapsedSeconds": 42.5, "bScoreCounted": true }
      ]
    }
  }
}
```

Dates are ISO 8601 in UTC. A date that was never set comes out empty in the CSV and
`null` in the JSON.

## From the component

The [SimFlow Component](../simflow-component.md#high-scores) wraps all of this
against its own `Flow Save Id`, so a Blueprint on the flow actor never has to pass
one. It also submits for you when a run ends.

| Field | Default | Meaning |
|---|---|---|
| Scenario Slot Name | `SimFlowScenarios` | |
| Scenario User Index | `0` | |
| Record Play On Finish | `true` | Offer the run score to the record when the flow ends |
| High Score Requires Completion | `true` | Turn off and a failed or aborted run can set one too |

**On New High Score** fires with the new and previous values whenever the record
moves. Previous is `0` when there was no record, so pair it with `Has Scenario Record` if
you want to say "first attempt" rather than "beat 0".

## A debrief screen

The auto-submit runs *before* **On Flow Finished** broadcasts, which means a widget
listening there already reads the new number.

```
On Flow Finished ──▶ Get Score        ──▶ "You scored: {}"
                 ──▶ Get High Score   ──▶ "Best: {}"
                 ──▶ Has Scenario Record   ──▶ Branch ──▶ show the Best line at all
```

For the "NEW RECORD" banner, bind to **On New High Score** instead of comparing
numbers yourself:

```
On New High Score (New Score, Previous Score)
   └─▶ Play banner ──▶ "New record: {New Score}, up from {Previous Score}"
```

And to show a live "you're beating your best" indicator mid-run, poll
**Is Beating High Score** on the component from a timer rather than every frame. See
the warning below.

## Authority

Submitting and resetting are authority-only, like [save and
load](control-and-save.md#save-and-load-across-every-flow). On a client mirror they
log and do nothing. The getters read on any machine, but a client reads that
machine's own save file, which is not the server's. The same goes for export: in a
multiplayer session, run it on the server to get the server's runs.

## Reading costs a file read

Every getter opens the save slot off disk. That is fine when a run ends, and wrong
in a widget binding that ticks. Read once on **On Flow Finished** or
**On New High Score**, store the value in a variable, and bind your UI to that.

## When it misbehaves

**Record Play always returns false.** Either the score is not beating the
stored one, or `Flow Save Id` is `None` and you called the static form directly —
the static needs a real id, the component version fills it in.

**The record vanished after the trainee started a new attempt.** It shouldn't.
Check that nothing in your Blueprint calls `Reset Scenario Record` on start, and that
`Scenario Slot Name` matches between the submit and the read.

**Nothing is stored and the log says the slot holds something else.** Some other
save object already owns that slot name. SimFlow refuses to overwrite it rather than
destroying your data. Pick a different `Scenario Slot Name`.

**A client shows a different best than the server.** Expected. Each machine reads
its own save file. Replicate the number yourself if clients need the server's.

**Scores are recorded for runs that failed.** `High Score Requires Completion` is
off, or something calls `Submit High Score` directly.

**Export returns false.** The slot has no records yet, or File Name contains a path.
The log says which.

**The export has fewer runs than Play Count.** Plays recorded before run history
existed have no entry, and only the last 100 are kept.

*Next: [Control and save](control-and-save.md) · [Blackboard nodes](blackboard-nodes.md) ·
[Status Widget](status-widget.md)*
