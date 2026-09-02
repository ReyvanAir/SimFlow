# Changelog

All notable changes to SimFlow. Versions follow the plugin's `VersionName`.

## 1.1.2

The object-recognition release. Before this, SimFlow could tell you a trainee
had finished a step, but not *what they did wrong* — every task was a success
detector, and nothing in the plugin could refer to a specific object in the
level. 1.1.2 adds an identity layer, zone-based placement, ordered procedure
validation, and a first-class mistake record.

The design rule it follows: **Blueprint reports neutral facts, the flow asset
decides whether they were correct.** A button broadcasts that it was pressed and
knows nothing about the exercise; the flow holds the answer. That keeps one
level reusable across scenarios without editing any Blueprint.

### Added

**Object identity**

- `USimFlowIdentityComponent` — put it on the item Blueprint, set `Identity Tags`
  (e.g. `Item.Extinguisher.Foam`), optional `Display Name` and `Identity Id`.
  Every placed and spawned copy carries it.
- `FSimFlowActorQuery` — the "which object do I mean" struct used by all new
  tasks. Resolves in order: `Specific Actor` → `Blackboard Key` → `Required Tags`
  → `Required Class` / `Required Actor Tag`.
- `ESimFlowMatchQuality` — `No Match` / `Related` / `Exact`. Because gameplay tags
  nest, a CO2 extinguisher scores `Related` against an expected foam extinguisher
  while a wrench scores `No Match`, so feedback can distinguish a near miss from a
  random object. `Min Related Tag Depth` (default 2) sets the threshold.
- `USimFlowIdentityStatics` — Blueprint access to identity tags, display names and
  query matching. Falls back to `IGameplayTagAssetInterface`, so actors already
  tagged for GAS work without a second component.

**Zones**

- `ASimFlowZone` — a box volume that reports what is inside it *without judging
  it*. Carries a `USimFlowIdentityComponent` rather than its own tagging scheme,
  so one mechanism names both items and zones.
- Settling: `On Actor Entered` fires on overlap, but `On Actor Settled` waits until
  the object is detached from the hand, below `Settle Speed Threshold`, and has
  held still for `Settle Time`. Holding an item over a bin is not placing it.
  Picking it back up restarts the settle timer.
- Optional `Broadcast Flow Events` raises `SimFlow.Event.Placed` /
  `SimFlow.Event.Removed` with the actor as payload, so a plain Wait For Event
  task can use a zone too.
- `Set Held` / `Is Held` on the identity component. A Zone asks the object whether
  it is being held before falling back to guessing from attachment. Call
  `Set Held (true)` where your grab succeeds and `false` on release.

**Tasks**

- **Place Object In Zone** — `Zone`, `Accepted Items`, optional `Rejected Items`
  (explicit decoys always lose, even if they would otherwise pass),
  `Required Count`, `Require Settled`, `Wrong Item Policy`. Fires
  `On Wrong Item Placed` with the match quality. A wrong item is reported once
  until it leaves the zone and is placed again.
- **Ordered Sequence** — an ordered list of steps, each with a target query and
  instruction text. `Out Of Order Policy` is `Ignore` / `Count Mistake` /
  `Restart Sequence` / `Fail Task`. Input belonging to another step of the
  procedure is recorded at `Related` severity; unrelated props at `No Match`, and
  `Unlisted Input Is Mistake` (default off) decides whether they are ignored.

**Mistake record**

- `FSimFlowMistake` — kind tag, description, severity, the object involved and a
  timestamp — recorded on `USimFlowInstance` and persisted in `FSimFlowSaveState`.
- `Record Mistake`, `Get Mistakes`, `Get Mistakes Of Kind`, `Get Mistake Count`,
  `Clear Mistakes`, and the `On Mistake Recorded` delegate.
- Task-level `Record Mistake` and `Apply Mismatch Policy` helpers on
  `USimFlowTask`, available to Blueprint task subclasses.
- New tags: `SimFlow.Mistake` with `.WrongItem`, `.WrongTarget`, `.WrongOrder`,
  `.WrongAnswer`; plus `SimFlow.Event.Placed` and `SimFlow.Event.Removed`.
- New blackboard keys: `WrongAttempts` and `CurrentStep`.

### Changed

- **Wait For Event no longer discards the event payload.** It gained
  `Expected Payload` (a query), `Mismatch Policy`
  (`Ignore` / `Count Mistake` / `Fail Task`), `Payload To Blackboard Key` and an
  `On Payload Rejected` delegate. Ten buttons can now broadcast the same tag while
  only the intended one advances the flow.
- Quiz now writes into the mistake record (`SimFlow.Mistake.WrongAnswer`) instead
  of only incrementing the `Mistakes` blackboard key, so quiz answers appear in
  the same debrief list as everything else. The key is still incremented, so
  existing conditions written against it are unaffected.

### Fixed

- `StartInstance` did not clear the mistake list, so restarting a flow carried the
  previous run's mistakes forward while the blackboard `Mistakes` counter was
  reset to zero by `ClearAll()` — the two records disagreed after any retry.
  Introduced and fixed within this release.
- `Wait For Event` with `Accept Already Raised` enabled would have accepted a
  previously-raised tag without checking the expected payload, since a past event
  retains only its tag. The flag is now ignored (with a verbose log line) when
  `Expected Payload` is set, rather than silently letting the wrong object pass.
- A Zone could not tell that an item was still in the player's hand under any grab
  system that holds objects with a physics constraint rather than by reparenting
  them — which is most of them, VRExpansion's default grip included. `Require
  Detached` therefore passed, and a trainee holding an item steady above a zone
  completed the step without ever letting go. Items now report their own held state
  through `Set Held`, and the Zone checks that before falling back to attachment.
- A payload that is neither an Actor nor an Actor Component — a `UUserWidget` broadcast
  from a UMG button's `OnClicked` is the usual case — could not match an actor query and
  said nothing about it, so the task simply never completed. It now logs a warning naming
  the payload's class and what the query wanted. The match still fails, deliberately:
  resolving a widget through `GetTypedOuter<AActor>()` would report a confident match
  against the owning PlayerController, which is worse than failing.

### Known limitations

None of these are regressions; they are the edges of the new features.

- `Specific Actor` is a soft pointer resolved without a synchronous load. An actor
  in an unloaded World Partition cell or streaming sublevel will not resolve — use
  identity tags or a blackboard key for streamed content.
- Tag and class queries resolve via a linear `TActorIterator` scan. This runs at
  task start, not per frame, and is not cached; on very large levels prefer
  `Specific Actor` or a blackboard key for zones.
- A Zone's fallback "still held" guess is `GetAttachParentActor()`, which only
  works for frameworks that reparent a grabbed actor. Most do not — of
  VRExpansion's twelve `EGripCollisionType` values only `AttachmentGrip` uses
  native attachment, so a gripped object usually looks detached from outside.
  Call `Set Held` on the item's identity component and the guess is never needed;
  without it, a trainee can hold an item steady over a zone and pass the step.
- A mistake's `Involved` object pointer is deliberately stripped when a flow is
  saved. Reloaded runs keep `Involved Name` for display but not a live reference.
- **Place Object In Zone** requires an `ASimFlowZone`; it will not accept an
  arbitrary trigger volume. It fails the task and logs a warning if the `Zone`
  query resolves to nothing.
- Zone overlap needs the item's collision to respond to the zone's
  `OverlapAllDynamic` profile. Items with `NoCollision` are never seen.
- Backward compatible: existing flow assets, saves and Blueprint task subclasses
  keep working. New properties all default to the pre-1.1.2 behaviour — leave
  `Expected Payload` empty and Wait For Event behaves exactly as before.

## 1.1.0

Initial public release. Node graph authoring, sequential and parallel tasks,
conditions, branching, pause/resume, retry/skip/fail, checkpoints, sub-flows,
save/load and multiplayer replication.
