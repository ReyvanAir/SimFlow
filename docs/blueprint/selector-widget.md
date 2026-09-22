# Scenario selector

**Classes:** `USimFlowSelectorWidget`, `USimFlowStatics`
**Needs a target?** No, for the three statics.

[Status Widget](status-widget.md) covers the panel shown *during* a run. This is the
panel shown before it: a list of what can be played, and a button that starts one.

## Three nodes do the work

| Node | Takes | Does |
|---|---|---|
| Build Scenario Options | An array of flow assets | One option per Start node, carrying display name, description and record key. History left empty |
| Apply Scenario Records | The options, a slot name | Fills in best score, play count, last outcome and last played — one read of the slot for the whole list |
| Start Scenario | A flow component, one option | Ends whatever was running, points the flow at the pick, starts it |

They are static, so any Blueprint can call them. The widget class below is a
convenience wrapper, not a requirement.

## One asset per scenario, or many entries in one

Both authoring shapes work and there is no setting to choose between them.
`Build Scenario Options` expands every [Start node](../nodes/start.md) it finds:

- An asset with a single `Default` entry becomes **one** option, labelled with the
  asset's `Flow Display Name`.
- An asset holding three Start nodes becomes **three** options, each labelled with
  its entry name.

An asset with no Start node is skipped with a warning, because a button for it could
never start anything.

## What an option carries

`FSimFlowScenarioOption`:

| Field | Holds |
|---|---|
| Flow Asset | The asset this option starts |
| Entry Name | Which Start node to begin from |
| Display Name | Button label — the asset's display name, or the entry name when one asset holds several |
| Description | The asset's `Flow Description` |
| Save Id | The record key: the asset's name, or `Asset.Entry` for a multi-entry asset |
| Record | Best score, play count, last outcome, last played. All zero until `Apply Scenario Records` runs |
| Has Record | Tells a stored `0` from never played. Check this before showing a number |

## Without the base class

```
Event Construct
  └─▶ Build Scenario Options ( your flow assets )   ──▶  [ Options ]
        └─▶ Apply Scenario Records ( Options, Slot = "" )
              └─▶ For Each ──▶ one button per option
                               label        = Display Name
                               subtitle     = Description
                               best / plays = Record.Best Score, Record.Play Count

On button clicked
  └─▶ Get Primary Flow  ──▶  Start Scenario ( Flow, Option )
```

Leave `Slot Name` empty and it resolves to `SimFlowScenarios`. Keep the options in a
variable rather than rebuilding them every frame: each `Apply Scenario Records` opens
the save file.

## With the base class

Reparent your briefing-table widget Blueprint to **SimFlow Selector Widget** and it
holds the list and the target flow for you.

| Member | Does |
|---|---|
| Scenarios | The flow assets on offer. Set it in the Details panel |
| Flow Save Id | Which flow runs the pick. None uses the primary flow |
| Refresh Options | Rebuild the list and re-read history. Called for you on construct |
| Refresh Records | Re-read history without rebuilding. Call after a run ends |
| Select Scenario | Starts the option at that index. Wire it to your buttons |
| Get Options / Get Option / Num Options | Read the built list |
| On Options Refreshed | Design event — rebuild your buttons here |
| On Scenario Selected | Design event — fires after a pick starts successfully |

## Each pick gets its own history

Scenario records are keyed by the component's **Flow Save Id**. One briefing table
running six scenarios through one component would file all six plays under that
component's single id, and their histories would merge into one meaningless total.

So `Start Scenario` writes the option's `Save Id` to the component first.
**Assign Save Id** is on by default; turn it off only if your project keys records
another way.

The key comes from the asset's name, so renaming `TrafficDrill` to `TrafficScenario`
reads as a scenario nobody has played. Rename before you ship, or key them yourself.

## Authority

Building a list and reading history work on any machine. Starting does not: with
**Replicate Flow** on, a client cannot hand the server a different asset, so
`Start Scenario` logs a warning and forwards a plain start request, and the server
runs whatever it has configured. Pick the scenario server-side, or give each trainee
an unreplicated flow.

Records are written server-side too — see [Scenario records](scenario-records.md).

## When it misbehaves

| Symptom | Cause |
|---|---|
| A scenario is missing from the list | Its asset has no [Start node](../nodes/start.md). The log names it |
| Every option shows a best score of 0 | `Apply Scenario Records` was never called, or the slot name does not match the component's **Scenario Slot Name** |
| A never-played scenario shows "Best: 0" | Read `Has Record` first — a stored `0` and never played both read as `0` |
| Every scenario shares one play count | **Assign Save Id** is off, so all picks file under the component's id |
| Select Scenario returns false, nothing starts | No flow to run it on. Place a [SimFlow Component](../simflow-component.md), or set `Flow Save Id` |
| The panel never updates after a run | Options cache their history. Call `Refresh Records` on **On Flow Finished** |

## Related

- [Scenario records](scenario-records.md) — the history the options display
- [Status Widget](status-widget.md) — the panel for during the run
- [Driving any widget](any-widget.md) — when you do not own the widget's base class
- [Start node](../nodes/start.md) — what each option maps to
- [Flow access](flow-access.md) — getting the component to hand `Start Scenario`
