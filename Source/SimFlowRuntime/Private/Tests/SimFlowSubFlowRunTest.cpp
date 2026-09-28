// Copyright SimFlow. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SimFlowAsset.h"
#include "SimFlowBlackboard.h"
#include "SimFlowGameplayTags.h"
#include "SimFlowInstance.h"
#include "SimFlowNodes.h"
#include "SimFlowTasks.h"

// These run real flows, without a component or a world: nothing below needs either.
// Each check calls TickInstance(0) after acting, as a frame would, to let the main
// flow step past a sub flow that just finished.

namespace
{
	FGameplayTag TagA() { return SimFlowTags::Event_Grab; }
	FGameplayTag TagB() { return SimFlowTags::Event_Release; }
	FGameplayTag TagC() { return SimFlowTags::Event_ButtonPressed; }

	USimFlowNode_Task* AddWait(USimFlowAsset* Asset, FGameplayTag Tag, float Score = 0.f, bool bAcceptAlreadyRaised = false)
	{
		USimFlowNode_Task* Node = Cast<USimFlowNode_Task>(Asset->AddNode(USimFlowNode_Task::StaticClass()));
		USimFlowTask_WaitForEvent* Wait = NewObject<USimFlowTask_WaitForEvent>(Node);
		Wait->EventTag = Tag;
		Wait->bMatchChildTags = false;
		Wait->bAcceptAlreadyRaised = bAcceptAlreadyRaised;
		Wait->ScoreOnSuccess = Score;
		Node->Task = Wait;
		return Node;
	}

	// Start -> the given nodes in order -> Finish. Every task pin leads on, so a
	// skipped or failed task moves to the next one like a completed one.
	void Chain(USimFlowAsset* Asset, const TArray<USimFlowNode*>& Nodes)
	{
		USimFlowNode* Previous = Asset->AddNode(USimFlowNode_Entry::StaticClass());
		FName PreviousPin = SimFlowPins::Out;
		TArray<USimFlowNode*> All = Nodes;
		All.Add(Asset->AddNode(USimFlowNode_Finish::StaticClass()));

		for (USimFlowNode* Node : All)
		{
			Asset->ConnectNodes(Previous, PreviousPin, Node, NAME_None);
			if (Cast<USimFlowNode_Task>(Previous))
			{
				Asset->ConnectNodes(Previous, SimFlowPins::Failed, Node, NAME_None);
				Asset->ConnectNodes(Previous, SimFlowPins::Skipped, Node, NAME_None);
			}
			Previous = Node;
			PreviousPin = SimFlowPins::Completed;
		}
	}

	USimFlowNode_SubFlow* AddSubFlow(USimFlowAsset* Asset, USimFlowAsset* Child)
	{
		USimFlowNode_SubFlow* Node = Cast<USimFlowNode_SubFlow>(Asset->AddNode(USimFlowNode_SubFlow::StaticClass()));
		Node->SubFlow = Child;
		return Node;
	}

	USimFlowInstance* Run(USimFlowAsset* Asset)
	{
		USimFlowInstance* Instance = NewObject<USimFlowInstance>();
		Instance->InitializeInstance(Asset, nullptr);
		Instance->StartInstance();
		return Instance;
	}

	USimFlowInstance* RunningSubFlow(const USimFlowInstance* Instance)
	{
		for (USimFlowNode* Node : Instance->GetActiveNodes())
		{
			if (const USimFlowNode_SubFlow* Sub = Cast<USimFlowNode_SubFlow>(Node))
			{
				return Sub->GetChildInstance();
			}
		}
		return nullptr;
	}

	FGameplayTag CurrentWaitTag(const USimFlowInstance* Instance)
	{
		const USimFlowTask_WaitForEvent* Wait = Cast<USimFlowTask_WaitForEvent>(Instance->GetCurrentTask());
		return Wait ? Wait->EventTag : FGameplayTag();
	}

	void AddDefault(USimFlowAsset* Asset, FName Key, const FSimFlowValue& Value)
	{
		FSimFlowBlackboardEntry Entry;
		Entry.Key = Key;
		Entry.Value = Value;
		Asset->InitialBlackboard.Add(Entry);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowMainFlowBaselineTest,
	"SimFlow.SubFlow.MainFlowUnchanged", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowMainFlowBaselineTest::RunTest(const FString&)
{
	// No sub flow anywhere: this is how every flow behaved before, and must still.
	USimFlowAsset* Main = NewObject<USimFlowAsset>();
	Chain(Main, { AddWait(Main, TagA()), AddWait(Main, TagB()) });

	USimFlowInstance* Flow = Run(Main);
	Flow->RaiseEvent(TagB());
	TestEqual(TEXT("a wait ignores a tag it is not waiting for"), CurrentWaitTag(Flow), TagA());
	Flow->RaiseEvent(TagA());
	TestEqual(TEXT("the right tag finishes the task"), CurrentWaitTag(Flow), TagB());
	TestTrue(TEXT("the history holds both tags"), Flow->WasEventRaised(TagA()) && Flow->WasEventRaised(TagB()));
	TestEqual(TEXT("skip moves on to the next step"), Flow->SkipActiveTasks(), 1);
	Flow->TickInstance(0.f);
	TestEqual(TEXT("and the flow completes"), Flow->GetRunState(), ESimFlowRunState::Completed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowSubFlowEventsTest,
	"SimFlow.SubFlow.Events", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowSubFlowEventsTest::RunTest(const FString&)
{
	// The reported case: Main -> Sub Flow -> [Wait For Event A].
	{
		USimFlowAsset* Child = NewObject<USimFlowAsset>();
		Chain(Child, { AddWait(Child, TagA()) });
		USimFlowAsset* Main = NewObject<USimFlowAsset>();
		Chain(Main, { AddSubFlow(Main, Child) });

		USimFlowInstance* Flow = Run(Main);
		TestNotNull(TEXT("the sub flow is running"), RunningSubFlow(Flow));
		Flow->RaiseEvent(TagB());
		Flow->TickInstance(0.f);
		TestEqual(TEXT("a different tag leaves the sub flow waiting"), Flow->GetRunState(), ESimFlowRunState::Running);
		Flow->RaiseEvent(TagA());
		Flow->TickInstance(0.f);
		TestEqual(TEXT("the tag sent to the main flow finishes the task in the sub flow"), Flow->GetRunState(), ESimFlowRunState::Completed);
	}

	// Two levels deep: Main -> Sub Flow -> Sub Flow -> [Wait A].
	{
		USimFlowAsset* Inner = NewObject<USimFlowAsset>();
		Chain(Inner, { AddWait(Inner, TagA()) });
		USimFlowAsset* Middle = NewObject<USimFlowAsset>();
		Chain(Middle, { AddSubFlow(Middle, Inner) });
		USimFlowAsset* Main = NewObject<USimFlowAsset>();
		Chain(Main, { AddSubFlow(Main, Middle) });

		USimFlowInstance* Flow = Run(Main);
		Flow->RaiseEvent(TagA());
		Flow->TickInstance(0.f);
		Flow->TickInstance(0.f);
		TestEqual(TEXT("the tag reaches a sub flow inside a sub flow"), Flow->GetRunState(), ESimFlowRunState::Completed);
	}

	// Accept Already Raised: A is raised in the main flow before the sub flow starts.
	// Main -> [Wait B] -> Sub Flow -> [Wait A, accept already raised].
	{
		USimFlowAsset* Child = NewObject<USimFlowAsset>();
		Chain(Child, { AddWait(Child, TagA(), 0.f, /*bAcceptAlreadyRaised*/ true) });
		USimFlowAsset* Main = NewObject<USimFlowAsset>();
		Chain(Main, { AddWait(Main, TagB()), AddSubFlow(Main, Child) });

		USimFlowInstance* Flow = Run(Main);
		Flow->RaiseEvent(TagA());
		Flow->RaiseEvent(TagB());
		Flow->TickInstance(0.f);
		TestEqual(TEXT("a tag raised earlier in the run counts inside the sub flow"), Flow->GetRunState(), ESimFlowRunState::Completed);
	}

	// A sub flow started by an event does not also receive that same event, exactly
	// as a main-flow task started by it would not. Main -> [Wait A] -> Sub Flow -> [Wait A].
	{
		USimFlowAsset* Child = NewObject<USimFlowAsset>();
		Chain(Child, { AddWait(Child, TagA()) });
		USimFlowAsset* Main = NewObject<USimFlowAsset>();
		Chain(Main, { AddWait(Main, TagA()), AddSubFlow(Main, Child) });

		USimFlowInstance* Flow = Run(Main);
		Flow->RaiseEvent(TagA());
		Flow->TickInstance(0.f);
		TestNotNull(TEXT("one event does not finish a task and the sub flow task it started"), RunningSubFlow(Flow));
		Flow->RaiseEvent(TagA());
		Flow->TickInstance(0.f);
		TestEqual(TEXT("a second one does"), Flow->GetRunState(), ESimFlowRunState::Completed);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowSubFlowControlsTest,
	"SimFlow.SubFlow.TaskControls", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowSubFlowControlsTest::RunTest(const FString&)
{
	// Main -> Sub Flow -> [Wait A] -> [Wait C]. Skip and fail used to end the whole
	// sub flow; they must move from A to C and leave the sub flow running.
	auto MakeMain = []()
	{
		USimFlowAsset* Child = NewObject<USimFlowAsset>();
		Chain(Child, { AddWait(Child, TagA()), AddWait(Child, TagC()) });
		USimFlowAsset* Main = NewObject<USimFlowAsset>();
		Chain(Main, { AddSubFlow(Main, Child) });
		return Main;
	};

	USimFlowInstance* Skipped = Run(MakeMain());
	TestEqual(TEXT("skip reaches the task inside the sub flow"), Skipped->SkipActiveTasks(), 1);
	Skipped->TickInstance(0.f);
	TestEqual(TEXT("skip moves to the sub flow's next task"), CurrentWaitTag(Skipped), TagC());
	TestNotNull(TEXT("and the sub flow is still running"), RunningSubFlow(Skipped));

	USimFlowInstance* Failed = Run(MakeMain());
	Failed->FailActiveTasks();
	Failed->TickInstance(0.f);
	TestEqual(TEXT("fail follows the task's Failed pin inside the sub flow"), CurrentWaitTag(Failed), TagC());
	TestEqual(TEXT("and the main flow keeps running"), Failed->GetRunState(), ESimFlowRunState::Running);

	USimFlowInstance* Retried = Run(MakeMain());
	TestEqual(TEXT("retry reaches the task inside the sub flow"), Retried->RetryActiveTasks(), 1);
	TestEqual(TEXT("and it is still the same task"), CurrentWaitTag(Retried), TagA());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowSubFlowBlackboardTest,
	"SimFlow.SubFlow.Blackboard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowSubFlowBlackboardTest::RunTest(const FString&)
{
	const FName Shared = TEXT("Shared");
	const FName ChildOnly = TEXT("ChildOnly");
	const FName Door = TEXT("DoorOpen");

	// The child's defaults: one key the main flow also has, one it does not. Its one
	// task scores 10.
	auto MakeChild = [&]()
	{
		USimFlowAsset* Child = NewObject<USimFlowAsset>();
		AddDefault(Child, Shared, FSimFlowValue::MakeInt(2));
		AddDefault(Child, ChildOnly, FSimFlowValue::MakeInt(1));
		Chain(Child, { AddWait(Child, TagA(), /*Score*/ 10.f) });
		return Child;
	};
	auto MakeMain = [&](bool bWriteBack)
	{
		USimFlowAsset* Main = NewObject<USimFlowAsset>();
		AddDefault(Main, Shared, FSimFlowValue::MakeInt(9));
		AddDefault(Main, SimFlowKeys::Score, FSimFlowValue::MakeFloat(50.f));
		USimFlowNode_SubFlow* Sub = AddSubFlow(Main, MakeChild());
		Sub->bWriteBackBlackboard = bWriteBack;
		Chain(Main, { Sub });
		return Main;
	};

	// Default settings (inherit + write back): one shared blackboard.
	{
		USimFlowInstance* Flow = Run(MakeMain(true));
		USimFlowInstance* Child = RunningSubFlow(Flow);
		if (!TestNotNull(TEXT("the sub flow is running"), Child))
		{
			return false;
		}
		USimFlowBlackboard* Board = Flow->GetBlackboard();
		TestTrue(TEXT("the sub flow uses the main flow's blackboard"), Child->GetBlackboard() == Board);
		TestEqual(TEXT("the main flow's value wins over the sub flow's default"), Board->GetInt(Shared), 9);
		TestEqual(TEXT("a key only the sub flow defines is added"), Board->GetInt(ChildOnly), 1);
		TestEqual(TEXT("starting the sub flow did not wipe the main flow's score"), Board->GetScore(), 50.f);

		Board->SetBool(Door, true);
		TestTrue(TEXT("a value set from outside mid-sub-flow reaches it"), Child->GetBlackboard()->GetBool(Door));

		Flow->RaiseEvent(TagA());
		Flow->TickInstance(0.f);
		TestEqual(TEXT("the flow completes"), Flow->GetRunState(), ESimFlowRunState::Completed);
		TestEqual(TEXT("the sub flow's score adds to the main flow's (it used to replace it)"), Board->GetScore(), 60.f);
	}

	// Inherit without write back: the sub flow works on its own copy.
	{
		USimFlowInstance* Flow = Run(MakeMain(false));
		USimFlowInstance* Child = RunningSubFlow(Flow);
		if (!TestNotNull(TEXT("the sub flow is running"), Child))
		{
			return false;
		}
		TestTrue(TEXT("without write back the sub flow has its own blackboard"), Child->GetBlackboard() != Flow->GetBlackboard());
		TestEqual(TEXT("the copy starts with the main flow's values (inherit used to be wiped)"), Child->GetBlackboard()->GetInt(Shared), 9);
		TestEqual(TEXT("and the main flow's score"), Child->GetBlackboard()->GetScore(), 50.f);
		TestEqual(TEXT("plus the sub flow's own defaults"), Child->GetBlackboard()->GetInt(ChildOnly), 1);

		Flow->RaiseEvent(TagA());
		Flow->TickInstance(0.f);
		TestEqual(TEXT("nothing is written back"), Flow->GetBlackboard()->GetScore(), 50.f);
		TestFalse(TEXT("not even new keys"), Flow->GetBlackboard()->HasValue(ChildOnly));
	}
	return true;
}

#endif
