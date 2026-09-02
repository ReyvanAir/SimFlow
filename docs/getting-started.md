# Getting Started

Install SimFlow, build a flow that does something, and see it run. About fifteen
minutes end to end.

New to the vocabulary? Keep the [Glossary](glossary.md) open in a second tab.

---

## 1. Install the plugin

SimFlow is a **C++ plugin**, so your project needs a C++ module. That is the one
requirement that catches people out.

1. Copy the `SimFlow` folder into `YourProject/Plugins/SimFlow`.
2. Right-click your `.uproject` → **Generate Visual Studio project files**
   (or run `GenerateProjectFiles`).
3. Build the editor target.
4. Enable **SimFlow** in *Edit → Plugins* if it is not already on.

**If your project is Blueprint-only:** add any empty C++ class once from the editor
(*Tools → New C++ Class → None*). Unreal converts the project to a C++ project and
step 2 becomes possible. You never have to write C++ after that — everything below
is Blueprint and the Details panel.

> **Engine version:** SimFlow 1.1.3 targets **Unreal Engine 5.6.0**.

### Verifying the install

Open the Content Browser, right-click in empty space, and look for a **SimFlow**
section with **SimFlow Graph** in it. If it is there, the plugin is loaded.

---

## 2. The fastest possible look: the sample flow

Before building anything by hand, generate the sample:

**Tools → SimFlow → Create Sample VR Tutorial Flow**

This builds a complete flow that exercises most of the plugin and opens it. Read it
top to bottom — it is the quickest way to see how nodes, tasks and conditions fit
together. Then come back and build your own.

---

## 3. Build your first flow

### Create the asset

**Content Browser → right-click → SimFlow → SimFlow Graph.**

Name it `F_HelloFlow`. Open it. There is already a **Start** node in the graph —
every flow needs one, and this is where execution begins.

### Add a task

1. Right-click the graph → **Tasks → Task**. A Task node appears.
2. Drag from `Start`'s **Out** pin onto the Task node's **In** pin.
3. Select the Task node. In the Details panel, find the **Task** field, click the
   dropdown, and choose **Log Message**.
4. The Task field expands into that task's own settings. Set **Message** to
   `Hello from SimFlow`.

> The `Task` field is empty by default. A Task node with no task assigned logs a
> warning and passes straight through its `Completed` pin — it does not fail. See
> [Task node](nodes/task.md).

### Finish the flow

1. Right-click the graph → **Flow Control → Finish**.
2. Wire the Task node's **Completed** pin into the Finish node's **In** pin.

Your graph now reads: Start → Task (Log Message) → Finish.

```
  ┌───────┐      ┌──────────────────┐          ┌────────┐
  │ Start │ Out──┤In   Task         │Completed─┤In      │
  │       │      │     (Log Message)│          │ Finish │
  └───────┘      │                  │Failed    └────────┘
                 │                  │Skipped
                 │                  │Timed Out
                 └──────────────────┘
```

Save the asset.

### Run it

1. In your level, select an actor to host the flow — the Game Mode, a level actor,
   or your VR pawn all work. (Which one to pick, and why, is covered in
   [SimFlow Component](simflow-component.md).)
2. **Add Component → SimFlow Component**.
3. Set **Flow Asset** to `F_HelloFlow`.
4. Set **Start Mode** to **Auto - On First Tick**.
5. Press Play.

You should see `Hello from SimFlow` printed on screen.

> **Why "On First Tick" rather than "On Begin Play"?** On Begin Play, other actors
> in the level may not have begun play yet, so a flow that immediately looks for a
> zone or an item can fail to find it. On First Tick is the safer default. See
> [Start Mode](simflow-component.md#start-mode).

### See what it is doing

Open the console (`~`) and type:

```
SimFlow.Debug 1
```

Live flow state draws on screen: which node is active, the current task, elapsed
time, and the blackboard. This is the single most useful debugging tool in the
plugin — reach for it before anything else.

---

## 4. Make it react to the world

A flow that logs a message is not yet interesting. The next step is having the flow
wait for the trainee to *do* something.

Replace the Log Message task with **Wait For Event**:

1. Select the Task node, change **Task** to **Wait For Event**.
2. Set **Event Tag** to `SimFlow.Event.Interact`.

Now the flow blocks until something raises that tag. From any Blueprint in your
level — a button, a grabbable object, an animation notify — call:

**Broadcast Flow Event** with `Event Tag` = `SimFlow.Event.Interact` and
`Payload` = `self`.

Press Play, trigger the Blueprint, and the flow advances.

> **Always pass `self` as the payload from the owning actor**, not from a widget.
> A UMG widget is neither an Actor nor an Actor Component and can never satisfy an
> object check. This is a common first bug — see
> [Wait For Event · Pitfalls](tasks/wait-for-event.md#common-pitfalls).

---

## 5. Where to go next

| If you want to… | Read |
|---|---|
| Understand what an object *is* so tasks can recognise it | [Identity System](identity.md) |
| Say "put the extinguisher in the bay" | [Place Object In Zone](tasks/place-object-in-zone.md) |
| Say "press these three buttons in order" | [Ordered Sequence](tasks/ordered-sequence.md) |
| Branch on state, loop, run things in parallel | [Node Reference](nodes/README.md) |
| Remember things between tasks | [Blackboard](blackboard.md) |
| See complete worked examples | [Example Workflows](workflows/README.md) |
| Fix something that is not working | [Troubleshooting](troubleshooting.md) |

---

*See also: [Documentation index](README.md) · [Glossary](glossary.md)*
