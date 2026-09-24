// Copyright SimFlow. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SimFlowEditorLibrary.h"
#include "SimFlowGraph.h"
#include "SimFlowGraphNode.h"
#include "SimFlowAsset.h"
#include "SimFlowNodes.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowEditorLibraryTest,
	"SimFlow.Editor.ScriptedEdit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowEditorLibraryTest::RunTest(const FString&)
{
	// An asset that has already been opened: it has a graph, and a comment box in it.
	USimFlowAsset* Asset = NewObject<USimFlowAsset>();
	USimFlowNode* Entry = Asset->AddNode(USimFlowNode_Entry::StaticClass());
	TestTrue(TEXT("first sync creates the graph"), USimFlowEditorLibrary::SyncGraphFromAsset(Asset));
	USimFlowGraph* Graph = Cast<USimFlowGraph>(Asset->EdGraph);
	UEdGraphNode_Comment* Comment = NewObject<UEdGraphNode_Comment>(Graph);
	Graph->AddNode(Comment, false, false);

	// The script's edit: a task wired after the entry.
	USimFlowNode* Task = Asset->AddNode(USimFlowNode_Task::StaticClass());
	Asset->ConnectNodes(Entry, SimFlowPins::Out, Task, SimFlowPins::In);
	USimFlowEditorLibrary::SetNodePosition(Task, FVector2D(300, 0));
	USimFlowEditorLibrary::SyncGraphFromAsset(Asset);

	USimFlowGraphNode* EntryGraphNode = Graph->FindGraphNodeForRuntimeNode(Entry);
	USimFlowGraphNode* TaskGraphNode = Graph->FindGraphNodeForRuntimeNode(Task);
	TestNotNull(TEXT("the scripted node shows in the graph"), TaskGraphNode);
	TestTrue(TEXT("the comment box survives the sync"), Graph->Nodes.Contains(Comment));
	if (!EntryGraphNode || !TaskGraphNode)
	{
		return false;
	}
	TestEqual(TEXT("the node sits where the script put it"), TaskGraphNode->NodePosX, 300);

	UEdGraphPin* Out = EntryGraphNode->FindPinByRuntimeName(SimFlowPins::Out, EGPD_Output);
	TestTrue(TEXT("the scripted link is a wire in the graph"),
		Out && Out->LinkedTo.ContainsByPredicate([TaskGraphNode](const UEdGraphPin* Pin) { return Pin->GetOwningNode() == TaskGraphNode; }));

	// A later graph edit writes the graph back over the nodes. Without the sync the
	// scripted node would be erased here.
	Graph->CompileToAsset();
	TestTrue(TEXT("a graph edit after the sync keeps the scripted node"), Asset->Nodes.Contains(Task));

	TSharedPtr<FJsonObject> Json;
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(USimFlowEditorLibrary::DescribeFlow(Asset)), Json);
	if (!TestTrue(TEXT("DescribeFlow returns JSON"), Json.IsValid()))
	{
		return false;
	}
	TestEqual(TEXT("DescribeFlow lists both nodes"), Json->GetArrayField(TEXT("nodes")).Num(), 2);

	FString LinkedTo;
	for (const TSharedPtr<FJsonValue>& Node : Json->GetArrayField(TEXT("nodes")))
	{
		if (Node->AsObject()->GetStringField(TEXT("guid")) == Entry->NodeGuid.ToString())
		{
			LinkedTo = Node->AsObject()->GetArrayField(TEXT("outputs"))[0]->AsObject()
				->GetArrayField(TEXT("links"))[0]->AsObject()->GetStringField(TEXT("node"));
		}
	}
	TestEqual(TEXT("DescribeFlow reports the entry's link to the task"), LinkedTo, Task->NodeGuid.ToString());

	return true;
}

#endif
