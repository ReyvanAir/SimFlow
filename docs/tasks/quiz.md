# Quiz

**Class:** `USimFlowTask_Quiz`
**Select via:** [Task node](../nodes/task.md) → **Task** dropdown → **Quiz**

---

## Overview / Purpose

A **multiple-choice question**.

The task holds the question, the options and the answer key; **you build the
widget**. Bind `On Quiz Presented` from your VR widget, show the options, and call
`Submit Answer` when the trainee picks one.

Wrong answers can retry, fail, or just continue — wire the [Task node](../nodes/task.md)'s
`Failed` pin to whatever remediation you want.

---

## Field-by-field breakdown

| Field | Type | Default | Required | Meaning |
|---|---|---|---|---|
| **Question** | Text (multi-line) | *empty* | **Yes** | The question text. |
| **Options** | Array of Text | *empty* | **Yes** | The choices, in display order. |
| **Correct Option Index** | Int (min 0) | `0` | **Yes** | Which option is right. **0-based.** |
| **Additional Correct Indices** | Array of Int | *empty* | No | *Advanced.* Also treat these as correct (multi-answer questions). |
| **Answer Blackboard Key** | Name | `None` | No | *Advanced.* Receives the submitted index. |
| **Fail On Wrong Answer** | Bool | `true` | No | A wrong answer immediately fails the task, driving the `Failed` pin. |
| **Count Mistakes** | Bool | `true` | No | Increments the `Mistakes` [blackboard](../blackboard.md) key on a wrong answer. |

Plus the [fields every task has](README.md#fields-every-task-has).

---

## Behaviour when left empty or misconfigured

| Situation | What happens |
|---|---|
| **Options is empty** | The task presents a question with nothing to pick. It waits forever — no submission is possible. |
| **Correct Option Index out of range** | No submitted answer can ever be correct. Every answer is wrong. |
| **`Submit Answer` with an out-of-range index** | Judged against the answer key like any other index — it will be wrong unless the key is equally out of range. |
| **Nothing calls `Submit Answer`** | The task waits forever. Set a `Time Limit` on the [Task node](../nodes/task.md) if the question should expire. |
| **Answer Blackboard Key is `None`** | Nothing extra is stored — but `LastAnswerIndex` and `LastAnswerCorrect` are written regardless. |

**The index is 0-based.** The first option is `0`. Setting `Correct Option Index` to
`1` when you meant the first choice is the most common configuration error here.

---

## Blackboard keys written

Whether or not you set `Answer Blackboard Key`, a submission writes:

| Key | Type | Meaning |
|---|---|---|
| `LastAnswerIndex` | Int | The index submitted |
| `LastAnswerCorrect` | Bool | Whether it was right |
| `Mistakes` | Int | Incremented on a wrong answer, when `Count Mistakes` is on |

These are the well-known keys — see [Blackboard](../blackboard.md#well-known-keys).

---

## Right / wrong handling

On `Submit Answer`:

1. The index is checked against `Correct Option Index` **and**
   `Additional Correct Indices`.
2. `On Quiz Answered` fires with the index and whether it was correct.
3. The blackboard keys above are written.
4. **Correct** → the task finishes with `Succeeded`.
   **Wrong** → a `SimFlow.Mistake.WrongAnswer` mistake is recorded, and:
   - `Fail On Wrong Answer` **on** → finishes with `Failed`
   - `Fail On Wrong Answer` **off** → finishes with `Succeeded`, so the flow
     continues; branch on `LastAnswerCorrect` afterwards if you care

---

## Delegates and functions

| Member | Signature | Use for |
|---|---|---|
| **On Quiz Presented** | (Quiz) | Show your widget and populate it from the quiz |
| **On Quiz Answered** | (Answer Index, Correct) | Feedback before the flow moves on |
| **Submit Answer** | (Option Index) | **Call this from your answer buttons** |
| **Is Correct Index** | (Option Index) → Bool | Check an index without submitting |

The [SimFlow Component](../simflow-component.md) also surfaces this: its own
`On Quiz Presented` delegate, `Get Current Quiz`, and `Submit Quiz Answer` — which
**works from clients** in multiplayer, forwarding to the server. Prefer the
component's route when you replicate.

---

## Dependencies

| Depends on | Why |
|---|---|
| A widget you build | The task has no UI of its own |
| [Blackboard](../blackboard.md) | For the answer keys |

---

## Example use case: a knowledge check with remediation

**Goal:** ask which extinguisher suits an electrical fire. A wrong answer routes to
a short teaching section, then re-asks.

**The task**

1. Add a [Task node](../nodes/task.md), **Task** = **Quiz**.
2. **Question** = `Which extinguisher is safe on an electrical fire?`
3. **Options**: `Water`, `Foam`, `CO2`.
4. **Correct Option Index** = `2` (CO2 — remember, 0-based).
5. Leave **Fail On Wrong Answer** on and **Count Mistakes** on.
6. Set **Score On Success** to `20` and **Score On Failure** to `-5`.

**The widget**

7. Bind the component's **On Quiz Presented**. In the handler, read `Question` and
   `Options` from the quiz and build your buttons.
8. Each button calls **Submit Quiz Answer** on the component with its index.

**The routing**

9. Wire **Completed** onward to the next section.
10. Wire **Failed** into a teaching section, and loop that back into this Task
    node's `In` pin to re-ask.

```
            ┌──────────────┐ Completed ──▶ next section
   ────────▶┤ Task (Quiz)  │
        ┌──▶│              │ Failed ─────▶ teaching section ──┐
        │   └──────────────┘                                  │
        └─────────────────────────────────────────────────────┘
```

To ask again without a hard fail, turn **Fail On Wrong Answer** off and put a
[Branch](../nodes/branch.md) on `LastAnswerCorrect` after the task instead.

---

## Common pitfalls

**The correct answer is marked wrong.**
`Correct Option Index` is 0-based. The third option is index `2`.

**The quiz never appears.**
Nothing is bound to `On Quiz Presented`, or your widget is not being created. The
task itself draws nothing.

**The flow hangs at the quiz.**
Nothing calls `Submit Answer`. Check that your buttons are wired, and add a
`Time Limit` if the question should expire.

**A wrong answer continues as if correct.**
`Fail On Wrong Answer` is off, which finishes with `Succeeded` by design. Branch on
`LastAnswerCorrect`.

**Client answers do nothing in multiplayer.**
Call the component's `Submit Quiz Answer`, not the task's `Submit Answer` — the
component forwards to the server.

**Mistakes are not counted.**
`Count Mistakes` is off.

---

*See also: [Blackboard](../blackboard.md) · [Branch node](../nodes/branch.md) ·
[SimFlow Component](../simflow-component.md) · [Task Reference](README.md)*
