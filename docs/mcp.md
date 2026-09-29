# Controlling SimFlow from Claude (MCP)

An MCP client such as Claude Code can read, edit and run SimFlow flows in the editor.
It does this through the engine's own MCP server. SimFlow registers a set of tools
with it; there is no separate server to run.

This is editor only. Nothing here reaches a packaged game.

## What you need

Unreal Engine 5.8, with these plugins on in the project:

| Plugin | Why |
|---|---|
| Unreal MCP (`ModelContextProtocol`) | The server itself. Experimental. |
| Toolset Registry (`ToolsetRegistry`) | Lets a plugin add tools to that server. Experimental. |
| Python Editor Script Plugin | SimFlow's tools are written in Python. |
| Editor Scripting Utilities | Asset load and save from Python. |

All four are Epic's, and the first two are marked experimental and may change in a
later engine version. SimFlow does not require any of them. Without Toolset Registry
its tools simply are not registered, and the output log says so once at startup.

Then turn the server on: **Edit > Project Settings > Model Context Protocol**, tick
*Auto Start Server*, restart the editor. It listens on `http://localhost:8000/mcp`
unless you changed the port or path there. Tell Claude Code where it is:

```
claude mcp add --transport http unreal http://localhost:8000/mcp
```

The server has no sign-in. It accepts any process on the machine that can reach the
port, so leave it off on a shared computer.

By default the server lists three tools (`list_toolsets`, `describe_toolset`,
`call_tool`) and the client looks up the rest as it needs them, so SimFlow's 28 tools
cost nothing until they are used. The toolset's name is
`simflow_toolset.toolset.SimFlowTools`; a client passes that as `toolset_name` to
`describe_toolset` and `call_tool`.

## The tools

Assets are named by path (`/Game/Flows/FireDrill`). Nodes are named by the guid
`describe_flow` prints. Types are named the way `list_node_types` and its siblings
print them (`SimFlowNode_Task`, or just `Node_Task`); a Blueprint type needs its full
path.

### Read

| Tool | What it returns |
|---|---|
| `list_flows` | Every SimFlow Graph asset in the project. |
| `describe_flow` | One flow: nodes, pins, links, each node's settings, the task or condition inside it, and validation errors and warnings. |
| `validate_flow` | Only the errors and warnings. |
| `list_node_types`, `list_task_types`, `list_condition_types` | What can be added, Blueprint types included. |
| `describe_type` | One type: what it does, and every setting with its type, tooltip, default and allowed values. |

### Edit

Each edit changes the asset in memory as one undo step and rebuilds the graph view, so
it shows in an open editor tab. Nothing reaches disk until `save_flow`.

| Tool | What it does |
|---|---|
| `create_flow` | New flow with its Start node. |
| `set_flow_properties` | Display name, description, initial blackboard. |
| `add_node` | Adds a node at a position, optionally with settings. |
| `remove_node`, `set_node_position` | |
| `connect_nodes`, `disconnect_nodes` | Wire one node's output pin to another's input. |
| `set_node_properties` | Changes a node's settings from a JSON object. |
| `set_task` | Puts a task on a Task node. |
| `set_instanced` | Puts a task or condition into any slot: `AbortCondition`, `Condition`, `Cases[1].Condition`, `Task.Condition`. |
| `save_flow` | Writes the asset and reports its validation result. |

Settings are JSON objects keyed by the names `describe_type` gives:
`{"TimeLimit": 30, "bAutoRetryOnFailure": true}`. A name that is not an editable setting
refuses the whole call before anything changes, and the reply lists what was wrong.

A task or condition is not a plain setting. It is created in its slot, by class, with
`set_task` or `set_instanced`, and its own settings go in the same call.

Setting an array replaces it whole. On a Branch that drops the conditions held in the
old cases, so set the cases first and their conditions after.

### Run

These need Play In Editor. `start_play_in_editor` returns at once; the game is up a
moment later.

| Tool | What it does |
|---|---|
| `start_play_in_editor`, `stop_play_in_editor` | |
| `list_running_flows` | Each flow component: state, progress, score, current task, active nodes, blackboard, mistakes, scenario record. |
| `flow_control` | start, stop, restart, pause, resume, retry, skip or fail the current task. |
| `send_event` | Raises a gameplay-tag event, as a Send Event node would. |
| `submit_quiz_answer` | Answers the quiz being asked. |
| `set_blackboard_value` | Writes a bool, int, float, string or name. |
| `save_flow_state`, `load_flow_state` | Progress to and from a save slot. |
| `reset_scenario_record` | Clears a scenario's history. It cannot be undone. |

A flow is addressed by its `Flow Save Id`. Leave it empty when only one flow is running.

## A short session

Ask Claude: "Make a flow at /Game/Flows/Drill that starts, waits 5 seconds with the
instruction 'Stand by the exit', and finishes." It will call, in order, `create_flow`,
`add_node` (Task, then Finish), `set_task` with the Delay type and its settings,
`connect_nodes` twice, `validate_flow` and `save_flow`. `describe_flow` then shows the
result, and the flow is open in the graph editor with the nodes in place.

## Limits

- Pin names are the ones `describe_flow` lists. A wrong pin name is refused with the
  pins that exist.
- Only settings marked editable are reachable. Runtime-only state is not.
- A Blueprint task's settings appear when its variables are Instance Editable.
- Array elements are edited by replacing the array, except for the conditions and tasks
  inside them, which have their own path syntax (`Cases[0].Condition`).
- Edits to an asset that is open in the editor rebuild its graph. Comment boxes are kept.

## When something is off

| Symptom | Cause |
|---|---|
| No SimFlow tools in the list | Toolset Registry is not enabled, or Python is not. The log has "SimFlow: the Toolset Registry plugin is not enabled". |
| "No node type ..." | The name is wrong or matches more than one type. Use the full path from the list. |
| "No game is running" | The run tools need Play In Editor. |
| A change shows in `describe_flow` but not on disk | Call `save_flow`. |
