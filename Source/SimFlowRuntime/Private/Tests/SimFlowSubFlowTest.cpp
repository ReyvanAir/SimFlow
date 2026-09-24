// Copyright SimFlow. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SimFlowAsset.h"
#include "SimFlowNodes.h"

namespace
{
	USimFlowNode_SubFlow* AddSubFlow(USimFlowAsset* Parent, USimFlowAsset* Child)
	{
		USimFlowNode_SubFlow* Node = Cast<USimFlowNode_SubFlow>(Parent->AddNode(USimFlowNode_SubFlow::StaticClass()));
		Node->SubFlow = Child;
		return Node;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowSubFlowLookupTest,
	"SimFlow.SubFlow.NodeLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowSubFlowLookupTest::RunTest(const FString&)
{
	// Main ─▶ Sub Flow ─▶ Child ─▶ Sub Flow ─▶ Grandchild
	USimFlowAsset* Main = NewObject<USimFlowAsset>();
	USimFlowAsset* Child = NewObject<USimFlowAsset>();
	USimFlowAsset* Grandchild = NewObject<USimFlowAsset>();

	USimFlowNode* MainTask = Main->AddNode(USimFlowNode_Task::StaticClass());
	USimFlowNode* ChildTask = Child->AddNode(USimFlowNode_Task::StaticClass());
	USimFlowNode* DeepTask = Grandchild->AddNode(USimFlowNode_Task::StaticClass());
	AddSubFlow(Main, Child);
	AddSubFlow(Child, Grandchild);

	TestTrue(TEXT("a flow still finds its own node first"),
		Main->FindNodeByGuidInTree(MainTask->NodeGuid) == MainTask);

	TestNull(TEXT("the plain lookup stays shallow, so the scheduler is unaffected"),
		Main->FindNodeByGuid(ChildTask->NodeGuid));

	TestTrue(TEXT("a task inside a sub flow is found from the main flow"),
		Main->FindNodeByGuidInTree(ChildTask->NodeGuid) == ChildTask);

	TestTrue(TEXT("and one nested two sub flows down"),
		Main->FindNodeByGuidInTree(DeepTask->NodeGuid) == DeepTask);

	// An empty Sub Flow node must be skipped, not dereferenced.
	AddSubFlow(Main, nullptr);
	TestNull(TEXT("an empty Sub Flow node does not break the search"),
		Main->FindNodeByGuidInTree(FGuid::NewGuid()));

	// The grandchild calls back to the main flow. Authoring allows it; the search
	// must still end. A hang here fails the test by timing it out.
	AddSubFlow(Grandchild, Main);
	TestNull(TEXT("a node that exists nowhere returns null even when flows form a cycle"),
		Main->FindNodeByGuidInTree(FGuid::NewGuid()));

	TestTrue(TEXT("and a real node is still found through the cycle"),
		Child->FindNodeByGuidInTree(MainTask->NodeGuid) == MainTask);

	return true;
}

#endif
