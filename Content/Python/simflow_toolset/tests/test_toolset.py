# Copyright SimFlow. All Rights Reserved.
"""Builds and edits a flow through the toolset, the way the MCP server calls it.

Play In Editor tools are not covered here: they need a running game.
"""

import functools
import json
import unittest

import unreal

import toolset_registry

from simflow_toolset.toolset import SimFlowTools

ASSET = '/Game/SimFlowToolsetTest/Flow'


def _raising(test):
    """Run a test with tool errors raised as exceptions instead of script errors."""
    @functools.wraps(test)
    def wrapper(self):
        with toolset_registry.tool_raising_exceptions():
            test(self)
    return wrapper


def _by_guid(flow: dict, guid: str) -> dict:
    return next(n for n in flow['nodes'] if n['guid'] == guid)


class TestSimFlowTools(unittest.TestCase):
    """Edits go through the same entry points the MCP server uses."""

    def setUp(self):
        self._delete()
        self.addCleanup(self._delete)

    @staticmethod
    def _delete():
        if unreal.EditorAssetLibrary.does_asset_exist(ASSET):
            unreal.EditorAssetLibrary.delete_asset(ASSET)

    @_raising
    def test_types_can_be_listed_and_described(self):
        nodes = {n['name']: n for n in json.loads(SimFlowTools.list_node_types())}
        self.assertTrue(nodes['SimFlowNode_Task']['placeable'])
        tasks = {t['name'] for t in json.loads(SimFlowTools.list_task_types())}
        self.assertIn('SimFlowTask_Delay', tasks)
        conditions = {c['name'] for c in json.loads(SimFlowTools.list_condition_types())}
        self.assertIn('SimFlowCondition_Constant', conditions)

        described = json.loads(SimFlowTools.describe_type('task', 'Task_Delay'))
        duration = next(p for p in described['properties'] if p['name'] == 'Duration')
        self.assertEqual(duration['default'], 1.0)

    @_raising
    def test_a_flow_can_be_built_wired_and_read_back(self):
        flow = json.loads(SimFlowTools.create_flow(ASSET))
        start = next(n for n in flow['nodes'] if n['class'] == 'SimFlowNode_Entry')['guid']

        task = json.loads(SimFlowTools.add_node(ASSET, 'SimFlowNode_Task', 300.0, 0.0, '{"TimeLimit": 30}'))
        json.loads(SimFlowTools.set_task(ASSET, task['guid'], 'Task_Delay', '{"Duration": 2, "DisplayName": "Wait"}'))
        finish = json.loads(SimFlowTools.add_node(ASSET, 'SimFlowNode_Finish', 600.0, 0.0, ''))
        SimFlowTools.connect_nodes(ASSET, start, 'Out', task['guid'], '')
        SimFlowTools.connect_nodes(ASSET, task['guid'], 'Completed', finish['guid'], '')

        problems = json.loads(SimFlowTools.validate_flow(ASSET))
        self.assertEqual(problems['errors'], [])

        read_back = json.loads(SimFlowTools.describe_flow(ASSET))
        node = _by_guid(read_back, task['guid'])
        self.assertEqual(node['properties']['timeLimit'], 30)
        self.assertEqual(node['properties']['task']['class'], 'SimFlowTask_Delay')
        self.assertEqual(node['properties']['task']['properties']['duration'], 2)
        self.assertEqual(node['position'], [300.0, 0.0])
        linked = next(o for o in node['outputs'] if o['pin'] == 'Completed')['links']
        self.assertEqual(linked[0]['node'], finish['guid'])

        SimFlowTools.disconnect_nodes(ASSET, task['guid'], 'Completed', finish['guid'], '')
        node = _by_guid(json.loads(SimFlowTools.describe_flow(ASSET)), task['guid'])
        self.assertEqual(next(o for o in node['outputs'] if o['pin'] == 'Completed')['links'], [])

        SimFlowTools.remove_node(ASSET, finish['guid'])
        self.assertEqual(len(json.loads(SimFlowTools.describe_flow(ASSET))['nodes']), 2)

    @_raising
    def test_a_branch_keeps_a_condition_inside_a_case(self):
        SimFlowTools.create_flow(ASSET)
        branch = json.loads(SimFlowTools.add_node(ASSET, 'SimFlowNode_Branch'))
        SimFlowTools.set_node_properties(ASSET, branch['guid'], '{"Cases": [{"Label": "Yes"}, {"Label": "No"}]}')
        SimFlowTools.set_instanced(ASSET, branch['guid'], 'Cases[1].Condition', 'Condition_Constant', '{"bValue": false}')

        node = _by_guid(json.loads(SimFlowTools.describe_flow(ASSET)), branch['guid'])
        self.assertEqual(len(node['outputs']), 3)
        condition = node['properties']['cases'][1]['condition']
        self.assertEqual(condition['class'], 'SimFlowCondition_Constant')
        self.assertFalse(condition['properties']['bValue'])
        self.assertIsNone(node['properties']['cases'][0]['condition'])

    @_raising
    def test_bad_input_is_refused_with_a_reason(self):
        SimFlowTools.create_flow(ASSET)
        task = json.loads(SimFlowTools.add_node(ASSET, 'SimFlowNode_Task'))

        with self.assertRaisesRegex(RuntimeError, 'No node type'):
            SimFlowTools.add_node(ASSET, 'SimFlowNode_NoSuchNode')
        with self.assertRaisesRegex(RuntimeError, 'abstract'):
            SimFlowTools.set_task(ASSET, task['guid'], 'SimFlowTask', '')
        with self.assertRaisesRegex(RuntimeError, 'NoSuchThing'):
            SimFlowTools.set_node_properties(ASSET, task['guid'], '{"NoSuchThing": 1}')
        with self.assertRaisesRegex(RuntimeError, 'No node'):
            SimFlowTools.remove_node(ASSET, '00000000000000000000000000000001')
        with self.assertRaisesRegex(RuntimeError, 'Could not connect'):
            SimFlowTools.connect_nodes(ASSET, task['guid'], 'NoSuchPin', task['guid'], '')
        with self.assertRaisesRegex(RuntimeError, 'No asset at'):
            SimFlowTools.describe_flow('/Game/DoesNotExist')

    @_raising
    def test_running_flows_need_a_game(self):
        with self.assertRaisesRegex(RuntimeError, 'No game is running'):
            SimFlowTools.list_running_flows()
