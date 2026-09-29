# Copyright SimFlow. All Rights Reserved.
"""Registers SimFlow's tools with the engine's MCP server, when there is one.

Needs the experimental ToolsetRegistry plugin. Without it this does nothing and
SimFlow loads exactly as before.
"""

import unreal

if hasattr(unreal, 'ToolsetDefinition') and hasattr(unreal, 'SimFlowEditorLibrary'):
    from simflow_toolset import toolset
    toolset.registration.register()

    # Shows up in the Automation window; "Automation RunTests SimFlow" runs it with the C++ tests.
    if hasattr(unreal, 'PythonTestRunner'):
        from simflow_toolset import tests
        tests._runner = unreal.PythonTestRunner.create(
            'SimFlowToolset.Python',
            unreal.PythonTestRunnerSearchOptions(root_module=tests.__name__))
else:
    unreal.log('SimFlow: the Toolset Registry plugin is not enabled, so SimFlow MCP tools are not registered.')
