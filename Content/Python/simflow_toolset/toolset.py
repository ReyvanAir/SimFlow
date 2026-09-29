# Copyright SimFlow. All Rights Reserved.
"""SimFlow tools for the engine's MCP server.

Three groups: read a flow or the project's flow types, edit a flow asset, and drive a
flow running in Play In Editor. The editing work is done by unreal.SimFlowEditorLibrary
and USimFlowAsset; this file only checks input, wraps each edit in one undo step, and
turns the results into JSON.

Tools take and return strings and numbers. Assets are named by path
("/Game/Flows/Fire"), nodes by the guid describe_flow prints, types by name
("Task_Delay") or, for a Blueprint type, by full path.
"""

import json

import unreal

import toolset_registry
from toolset_registry.registration import Registration

_lib = unreal.SimFlowEditorLibrary


def _base_class(kind: str) -> unreal.Class:
    bases = {'node': unreal.SimFlowNode, 'task': unreal.SimFlowTask, 'condition': unreal.SimFlowCondition}
    return bases[kind].static_class()


def _load_flow(asset_path: str) -> unreal.SimFlowAsset:
    if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        raise ValueError(f'No asset at {asset_path}. list_flows shows the flows in the project.')
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(asset, unreal.SimFlowAsset):
        raise ValueError(f'{asset_path} is not a SimFlow Graph asset. list_flows shows the ones in the project.')
    return asset


def _find_node(asset: unreal.SimFlowAsset, node_guid: str) -> unreal.SimFlowNode:
    node = _lib.find_node_by_guid_string(asset, node_guid)
    if not node:
        raise ValueError(f'No node {node_guid} in {asset.get_path_name()}. describe_flow lists the guids.')
    return node


def _resolve_type(kind: str, name: str) -> unreal.Class:
    """The class for a node, task or condition type name, refusing abstract and hidden ones."""
    base = _base_class(kind)
    cls = _lib.find_class(base, name)
    if not cls:
        raise ValueError(
            f'No {kind} type "{name}", or the name fits more than one. '
            f'list_{kind}_types shows what exists; give the full path for a Blueprint type.')
    entry = next((e for e in json.loads(_lib.list_classes(base)) if e['path'] == cls.get_path_name()), {})
    if entry.get('abstract'):
        raise ValueError(f'{name} is abstract; list_{kind}_types shows the concrete ones.')
    if kind == 'node' and not entry.get('placeable', True):
        raise ValueError(f'{name} cannot be placed by hand.')
    return cls


def _sync(asset: unreal.SimFlowAsset) -> None:
    """Rebuild the editor graph from the nodes, or the edit would not show and the next graph edit would erase it."""
    if not _lib.sync_graph_from_asset(asset):
        raise RuntimeError(f'Could not rebuild the graph of {asset.get_path_name()}.')


def _node_json(asset: unreal.SimFlowAsset, node_guid: str) -> str:
    for node in json.loads(_lib.describe_flow(asset))['nodes']:
        if node['guid'] == node_guid:
            return json.dumps(node)
    return json.dumps({'guid': node_guid})


def _pins(asset: unreal.SimFlowAsset, node_guid: str) -> str:
    node = json.loads(_node_json(asset, node_guid))
    return f'inputs {node.get("inputs", [])}, outputs {[o["pin"] for o in node.get("outputs", [])]}'


def _game_world() -> unreal.World:
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError('No game is running. Call start_play_in_editor first.')
    return world


def _components() -> list:
    found = []
    for actor in unreal.GameplayStatics.get_all_actors_of_class(_game_world(), unreal.Actor):
        found.extend(actor.get_components_by_class(unreal.SimFlowComponent))
    return found


def _component(flow_save_id: str) -> unreal.SimFlowComponent:
    components = _components()
    if flow_save_id:
        for component in components:
            if str(component.get_effective_save_id()) == flow_save_id:
                return component
        raise ValueError(f'No running flow with id "{flow_save_id}". list_running_flows shows the ids.')
    if len(components) == 1:
        return components[0]
    raise ValueError('flow_save_id is needed unless exactly one flow is running. list_running_flows shows the ids.')


def _running(component: unreal.SimFlowComponent) -> str:
    return _lib.describe_running_flow(component)


def _set_instanced(asset_path: str, node_guid: str, property_path: str, type_name: str, properties_json: str | None) -> str:
    asset = _load_flow(asset_path)
    node = _find_node(asset, node_guid)
    cls = None
    if type_name:
        slot = property_path.rsplit('.', 1)[-1].split('[')[0]
        kind = 'task' if slot in ('Task', 'Tasks') else 'condition'
        cls = _resolve_type(kind, type_name)
    with unreal.ScopedEditorTransaction('SimFlow: set instanced object'):
        asset.modify()
        error = _lib.set_instanced_property(node, property_path, cls, properties_json or '')
        _sync(asset)
    if error:
        raise ValueError(error)
    return _node_json(asset, node_guid)


@unreal.uclass()
class SimFlowTools(unreal.ToolsetDefinition):
    """Read, edit and run SimFlow scenario graphs (tasks, branches, sub flows).

    Workflow: list_flows, then describe_flow for the guids and pins. list_node_types /
    list_task_types / list_condition_types and describe_type say what can be added and
    what each setting is called. Edits change the asset in memory as one undo step; call
    save_flow to write it to disk. To try a flow: start_play_in_editor, then flow_control
    and list_running_flows.
    """

    # ------------------------------------------------------------------ read

    @toolset_registry.tool_call
    @staticmethod
    def list_flows() -> str:
        """Lists every SimFlow Graph asset in the project.

        Returns:
            JSON array of {path, name}. Pass path to the other tools.
        """
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        assets = registry.get_assets_by_class(unreal.TopLevelAssetPath('/Script/SimFlowRuntime', 'SimFlowAsset'), True)
        return json.dumps([{'path': f'{a.package_name}.{a.asset_name}', 'name': str(a.asset_name)} for a in assets])

    @toolset_registry.tool_call
    @staticmethod
    def describe_flow(asset_path: str) -> str:
        """Returns a whole flow: nodes, pins, links, settings, tasks, and validation problems.

        Args:
            asset_path: Asset path of the flow, e.g. /Game/Flows/FireDrill.

        Returns:
            JSON with entries, nodes (guid, class, title, properties, inputs, outputs with links,
            task) and errors/warnings. A node's properties hold its editable settings; a task
            or condition inside it appears there as {class, classPath, properties}.
        """
        return _lib.describe_flow(_load_flow(asset_path))

    @toolset_registry.tool_call
    @staticmethod
    def validate_flow(asset_path: str) -> str:
        """Checks a flow for a missing Start node, dangling links, empty Task nodes and unreachable nodes.

        Args:
            asset_path: Asset path of the flow.

        Returns:
            JSON {errors, warnings}.
        """
        errors, warnings = _load_flow(asset_path).validate_flow()
        return json.dumps({'errors': list(errors), 'warnings': list(warnings)})

    @toolset_registry.tool_call
    @staticmethod
    def list_node_types() -> str:
        """Lists the node types a flow graph can contain (Task, Branch, Delay, Sub Flow, ...).

        Returns:
            JSON array of {name, path, description, abstract, blueprint, placeable, category}.
            Only entries with placeable true and abstract false can be added.
        """
        return _lib.list_classes(_base_class('node'))

    @toolset_registry.tool_call
    @staticmethod
    def list_task_types() -> str:
        """Lists the task types a Task node can run, including Blueprint tasks in the project.

        Returns:
            JSON array of {name, path, description, abstract, blueprint}.
        """
        return _lib.list_classes(_base_class('task'))

    @toolset_registry.tool_call
    @staticmethod
    def list_condition_types() -> str:
        """Lists the condition types a Branch, Loop, Wait For Condition or abort check can use.

        Returns:
            JSON array of {name, path, description, abstract, blueprint}.
        """
        return _lib.list_classes(_base_class('condition'))

    @toolset_registry.tool_call
    @staticmethod
    def describe_type(kind: str, type_name: str) -> str:
        """Describes one node, task or condition type: what it does and every setting with its type, default and allowed values.

        Args:
            kind: node, task or condition.
            type_name: Type name such as Task_Delay or SimFlowNode_Branch, or the full path for a Blueprint type.

        Returns:
            JSON {name, description, category, properties: [{name, type, tooltip, default, options, instancedOf}]}.
            instancedOf marks a slot that holds a task or condition; fill it with set_instanced.
        """
        if kind not in ('node', 'task', 'condition'):
            raise ValueError('kind must be node, task or condition.')
        return _lib.describe_class(_resolve_type(kind, type_name))

    # ------------------------------------------------------------------ edit

    @toolset_registry.tool_call
    @staticmethod
    def create_flow(asset_path: str) -> str:
        """Creates a new flow with its Start node. Not saved until save_flow.

        Args:
            asset_path: Where to create it, e.g. /Game/Flows/FireDrill. Must not exist yet.

        Returns:
            The new flow, as describe_flow.
        """
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            raise ValueError(f'{asset_path} already exists.')
        folder, _, name = asset_path.rstrip('/').rpartition('/')
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.SimFlowAsset, unreal.SimFlowAssetFactory())
        if not asset:
            raise RuntimeError(f'Could not create {asset_path}.')
        return _lib.describe_flow(asset)

    @toolset_registry.tool_call
    @staticmethod
    def set_flow_properties(asset_path: str, properties_json: str) -> str:
        """Sets the flow's own settings: FlowDisplayName, FlowDescription, InitialBlackboard.

        Args:
            asset_path: Asset path of the flow.
            properties_json: JSON object, e.g. {"FlowDisplayName": "Fire drill"}. InitialBlackboard is an array of
                {"Key": "Attempts", "Value": {"Type": "Int", "IntValue": 3}}.

        Returns:
            The flow, as describe_flow.
        """
        asset = _load_flow(asset_path)
        with unreal.ScopedEditorTransaction('SimFlow: set flow properties'):
            error = _lib.set_properties_from_json(asset, properties_json)
        if error:
            raise ValueError(error)
        return _lib.describe_flow(asset)

    @toolset_registry.tool_call
    @staticmethod
    def add_node(asset_path: str, node_type: str, x: float = 0.0, y: float = 0.0, properties_json: str | None = None) -> str:
        """Adds a node to a flow. It has no links yet; wire it with connect_nodes.

        Args:
            asset_path: Asset path of the flow.
            node_type: Node type name from list_node_types, e.g. SimFlowNode_Task.
            x: Graph position, to the right.
            y: Graph position, downward. Nodes about 300 apart in x read well.
            properties_json: Optional JSON object of settings, e.g. {"TimeLimit": 30}. Names from describe_type.

        Returns:
            The new node as JSON: guid, pins, properties.
        """
        asset = _load_flow(asset_path)
        cls = _resolve_type('node', node_type)
        with unreal.ScopedEditorTransaction('SimFlow: add node'):
            asset.modify()
            node = asset.add_node(cls)
            node_guid = _lib.get_node_guid_string(node)
            if properties_json:
                error = _lib.set_properties_from_json(node, properties_json)
                if error:
                    asset.remove_node(node)
                    raise ValueError(error)
            _lib.set_node_position(node, unreal.Vector2D(x, y))
            _sync(asset)
        return _node_json(asset, node_guid)

    @toolset_registry.tool_call
    @staticmethod
    def remove_node(asset_path: str, node_guid: str) -> str:
        """Removes a node and every link that pointed at it.

        Args:
            asset_path: Asset path of the flow.
            node_guid: Guid from describe_flow.

        Returns:
            The flow, as describe_flow.
        """
        asset = _load_flow(asset_path)
        node = _find_node(asset, node_guid)
        with unreal.ScopedEditorTransaction('SimFlow: remove node'):
            asset.modify()
            asset.remove_node(node)
            _sync(asset)
        return _lib.describe_flow(asset)

    @toolset_registry.tool_call
    @staticmethod
    def set_node_position(asset_path: str, node_guid: str, x: float, y: float) -> str:
        """Moves a node in the graph. Position has no effect on how the flow runs.

        Args:
            asset_path: Asset path of the flow.
            node_guid: Guid from describe_flow.
            x: Graph position, to the right.
            y: Graph position, downward.

        Returns:
            The node as JSON.
        """
        asset = _load_flow(asset_path)
        node = _find_node(asset, node_guid)
        with unreal.ScopedEditorTransaction('SimFlow: move node'):
            _lib.set_node_position(node, unreal.Vector2D(x, y))
            _sync(asset)
        return _node_json(asset, node_guid)

    @toolset_registry.tool_call
    @staticmethod
    def connect_nodes(asset_path: str, from_guid: str, from_pin: str, to_guid: str, to_pin: str | None = None) -> str:
        """Wires an output pin of one node into an input pin of another.

        Args:
            asset_path: Asset path of the flow.
            from_guid: Guid of the node the execution leaves.
            from_pin: Output pin name from describe_flow, e.g. Out or Completed.
            to_guid: Guid of the node it arrives at.
            to_pin: Input pin name. Leave empty for the node's first input.

        Returns:
            The source node as JSON, with its links.
        """
        asset = _load_flow(asset_path)
        source = _find_node(asset, from_guid)
        target = _find_node(asset, to_guid)
        with unreal.ScopedEditorTransaction('SimFlow: connect nodes'):
            asset.modify()
            linked = asset.connect_nodes(source, unreal.Name(from_pin), target, unreal.Name(to_pin or 'None'))
            if not linked:
                raise ValueError(
                    f'Could not connect. Source has {_pins(asset, from_guid)}; target has {_pins(asset, to_guid)}.')
            _sync(asset)
        return _node_json(asset, from_guid)

    @toolset_registry.tool_call
    @staticmethod
    def disconnect_nodes(asset_path: str, from_guid: str, from_pin: str, to_guid: str, to_pin: str | None = None) -> str:
        """Removes one link between two nodes.

        Args:
            asset_path: Asset path of the flow.
            from_guid: Guid of the node the link leaves.
            from_pin: Output pin name.
            to_guid: Guid of the node it arrives at.
            to_pin: Input pin name. Leave empty to remove links into any input of that node.

        Returns:
            The source node as JSON, with its remaining links.
        """
        asset = _load_flow(asset_path)
        source = _find_node(asset, from_guid)
        target = _find_node(asset, to_guid)
        with unreal.ScopedEditorTransaction('SimFlow: disconnect nodes'):
            asset.modify()
            if not asset.disconnect_nodes(source, unreal.Name(from_pin), target, unreal.Name(to_pin or 'None')):
                raise ValueError(f'No such link. Source has {_pins(asset, from_guid)}.')
            _sync(asset)
        return _node_json(asset, from_guid)

    @toolset_registry.tool_call
    @staticmethod
    def set_node_properties(asset_path: str, node_guid: str, properties_json: str) -> str:
        """Changes a node's settings. A node whose pins depend on a setting (Branch cases, Loop count) rebuilds them.

        Args:
            asset_path: Asset path of the flow.
            node_guid: Guid from describe_flow.
            properties_json: JSON object of settings, e.g. {"TimeLimit": 30, "bAutoRetryOnFailure": true}.
                Names and allowed values come from describe_type. Setting an array replaces it whole, and
                that drops any conditions held inside its elements; set the conditions afterwards with
                set_instanced. A task or condition is not set here; use set_task or set_instanced.

        Returns:
            The node as JSON.
        """
        asset = _load_flow(asset_path)
        node = _find_node(asset, node_guid)
        with unreal.ScopedEditorTransaction('SimFlow: set node properties'):
            asset.modify()
            error = _lib.set_properties_from_json(node, properties_json)
            _sync(asset)
        if error:
            raise ValueError(error)
        return _node_json(asset, node_guid)

    @toolset_registry.tool_call
    @staticmethod
    def set_task(asset_path: str, node_guid: str, task_type: str, properties_json: str | None = None) -> str:
        """Puts a task on a Task node, replacing any task already there.

        Args:
            asset_path: Asset path of the flow.
            node_guid: Guid of a Task node.
            task_type: Task type name from list_task_types, e.g. SimFlowTask_Delay, or the full path of a Blueprint task.
            properties_json: Optional JSON object of the task's settings, e.g. {"DisplayName": "Wait", "Instruction": "Wait 5 seconds"}.
                Names from describe_type. A task that holds a condition takes it through set_instanced.

        Returns:
            The node as JSON, with the task under properties.Task.
        """
        return _set_instanced(asset_path, node_guid, 'Task', task_type, properties_json)

    @toolset_registry.tool_call
    @staticmethod
    def set_instanced(asset_path: str, node_guid: str, property_path: str, type_name: str, properties_json: str | None = None) -> str:
        """Creates a task or condition inside a node and stores it in a slot.

        Args:
            asset_path: Asset path of the flow.
            node_guid: Guid of the node that holds the slot.
            property_path: Where it goes, starting at the node: Task, AbortCondition, Condition (Loop),
                Cases[0].Condition (Branch), Task.Condition (a Wait For Condition task), Task.Conditions
                (append to an All Of / Any Of list). Steps go through structs, array elements and existing
                objects. describe_type marks the slots with instancedOf.
            type_name: Task or condition type name. Empty clears the slot.
            properties_json: Optional JSON object of the new object's settings.

        Returns:
            The node as JSON.
        """
        return _set_instanced(asset_path, node_guid, property_path, type_name, properties_json)

    @toolset_registry.tool_call
    @staticmethod
    def save_flow(asset_path: str) -> str:
        """Saves a flow asset to disk.

        Args:
            asset_path: Asset path of the flow.

        Returns:
            JSON {saved, errors, warnings}: the result of validate_flow, so a broken flow is reported when it is saved.
        """
        asset = _load_flow(asset_path)
        saved = unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
        errors, warnings = asset.validate_flow()
        return json.dumps({'saved': bool(saved), 'errors': list(errors), 'warnings': list(warnings)})

    # ------------------------------------------------------------------ run

    @toolset_registry.tool_call
    @staticmethod
    def start_play_in_editor() -> str:
        """Starts Play In Editor. Returns at once; the game is up a moment later, which list_running_flows shows.

        Returns:
            A short confirmation.
        """
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
        return 'Play In Editor requested.'

    @toolset_registry.tool_call
    @staticmethod
    def stop_play_in_editor() -> str:
        """Ends Play In Editor.

        Returns:
            A short confirmation.
        """
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
        return 'Stop requested.'

    @toolset_registry.tool_call
    @staticmethod
    def list_running_flows() -> str:
        """Lists the flows in the running game with their state, current task, score, blackboard and mistakes.

        Returns:
            JSON array, one entry per flow component: flowSaveId, owner, flowAsset, runState, progress, score,
            currentTask, currentInstruction, activeNodes, blackboard, mistakes, scenarioRecord.
        """
        return json.dumps([json.loads(_running(c)) for c in _components()])

    @toolset_registry.tool_call
    @staticmethod
    def flow_control(flow_save_id: str, action: str, entry_name: str | None = None) -> str:
        """Controls a running flow.

        Args:
            flow_save_id: Id from list_running_flows. Empty when only one flow is running.
            action: start, stop, restart, pause, resume, retry_task, skip_task or fail_task.
            entry_name: With start: the Start node to begin from. Empty for the component's default.

        Returns:
            The flow's state afterwards, as list_running_flows shows it.
        """
        component = _component(flow_save_id)
        if action == 'start':
            ok = component.start_flow_from_entry(unreal.Name(entry_name)) if entry_name else component.start_flow()
        elif action == 'restart':
            ok = component.restart_flow()
        elif action == 'retry_task':
            ok = component.retry_current_task()
        elif action == 'skip_task':
            ok = component.skip_current_task()
        elif action == 'fail_task':
            ok = component.fail_current_task()
        elif action in ('stop', 'pause', 'resume'):
            getattr(component, f'{action}_flow')()
            ok = True
        else:
            raise ValueError('action must be start, stop, restart, pause, resume, retry_task, skip_task or fail_task.')
        if ok is False:
            raise RuntimeError(f'{action} was refused. The flow may not be in a state where it applies.')
        return _running(component)

    @toolset_registry.tool_call
    @staticmethod
    def send_event(flow_save_id: str, event_tag: str) -> str:
        """Raises a gameplay-tag event on a running flow, as a Send Event node would. Wakes tasks that wait for it.

        Args:
            flow_save_id: Id from list_running_flows. Empty when only one flow is running.
            event_tag: Registered gameplay tag, e.g. SimFlow.Event.Extinguisher.PickedUp.

        Returns:
            The flow's state afterwards.
        """
        component = _component(flow_save_id)
        if not _lib.send_event_by_name(component, event_tag):
            raise ValueError(f'"{event_tag}" is not a registered gameplay tag.')
        return _running(component)

    @toolset_registry.tool_call
    @staticmethod
    def submit_quiz_answer(flow_save_id: str, option_index: int) -> str:
        """Answers the quiz the flow is asking.

        Args:
            flow_save_id: Id from list_running_flows. Empty when only one flow is running.
            option_index: Zero-based index of the chosen option.

        Returns:
            The flow's state afterwards.
        """
        component = _component(flow_save_id)
        component.submit_quiz_answer(option_index)
        return _running(component)

    @toolset_registry.tool_call
    @staticmethod
    def set_blackboard_value(flow_save_id: str, key: str, value_type: str, value: str) -> str:
        """Writes a value on a running flow's blackboard, for tasks and branches that read it.

        Args:
            flow_save_id: Id from list_running_flows. Empty when only one flow is running.
            key: Blackboard key.
            value_type: bool, int, float, string or name.
            value: The value as text: true/false, 42, 1.5, or any text.

        Returns:
            The flow's state afterwards.
        """
        component = _component(flow_save_id)
        board = component.get_blackboard()
        if not board:
            raise RuntimeError('The flow has no blackboard yet; start it first.')
        name = unreal.Name(key)
        if value_type == 'bool':
            board.set_bool(name, value.strip().lower() in ('true', '1', 'yes'))
        elif value_type == 'int':
            board.set_int(name, int(value))
        elif value_type == 'float':
            board.set_float(name, float(value))
        elif value_type == 'string':
            board.set_string(name, value)
        elif value_type == 'name':
            board.set_name(name, unreal.Name(value))
        else:
            raise ValueError('value_type must be bool, int, float, string or name.')
        return _running(component)

    @toolset_registry.tool_call
    @staticmethod
    def save_flow_state(flow_save_id: str, slot_name: str) -> str:
        """Saves a running flow's progress to a save slot, so it can be resumed.

        Args:
            flow_save_id: Id from list_running_flows. Empty when only one flow is running.
            slot_name: Save slot name.

        Returns:
            A short confirmation.
        """
        if not _component(flow_save_id).save_flow_to_slot(slot_name, 0):
            raise RuntimeError(f'Could not save to slot "{slot_name}".')
        return f'Saved to slot "{slot_name}".'

    @toolset_registry.tool_call
    @staticmethod
    def load_flow_state(flow_save_id: str, slot_name: str, from_last_checkpoint: bool = False) -> str:
        """Restores a running flow from a save slot.

        Args:
            flow_save_id: Id from list_running_flows. Empty when only one flow is running.
            slot_name: Save slot name.
            from_last_checkpoint: True to re-run from the last checkpoint passed; false to restore the exact state.

        Returns:
            The flow's state afterwards.
        """
        component = _component(flow_save_id)
        mode = unreal.SimFlowLoadMode.FROM_LAST_CHECKPOINT if from_last_checkpoint else unreal.SimFlowLoadMode.EXACT_STATE
        if not component.load_flow_from_slot(slot_name, 0, mode):
            raise RuntimeError(f'Nothing loaded from slot "{slot_name}".')
        return _running(component)

    @toolset_registry.tool_call
    @staticmethod
    def reset_scenario_record(flow_save_id: str) -> str:
        """Clears a scenario's play count, best score and run history. Cannot be undone.

        Args:
            flow_save_id: Id from list_running_flows. Empty when only one flow is running.

        Returns:
            The flow's state afterwards.
        """
        component = _component(flow_save_id)
        component.reset_scenario_record()
        return _running(component)


registration = Registration([SimFlowTools])
