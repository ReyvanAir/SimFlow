// Copyright SimFlow. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SimFlowAsset.h"
#include "SimFlowNodes.h"
#include "SimFlowScenarioRecord.h"
#include "SimFlowStatics.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FString TestSlot = TEXT("SimFlowSelectorTest");

	USimFlowAsset* MakeFlow(const TArray<FName>& EntryNames)
	{
		USimFlowAsset* Asset = NewObject<USimFlowAsset>();
		for (const FName Entry : EntryNames)
		{
			USimFlowNode_Entry* Node = Cast<USimFlowNode_Entry>(Asset->AddNode(USimFlowNode_Entry::StaticClass()));
			Node->EntryName = Entry;
		}
		return Asset;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowSelectorTest,
	"SimFlow.Selector", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowSelectorTest::RunTest(const FString&)
{
	UGameplayStatics::DeleteGameInSlot(TestSlot, 0);

	USimFlowAsset* Single = MakeFlow({ TEXT("Default") });
	Single->FlowDisplayName = FText::FromString(TEXT("Traffic Scenario"));

	USimFlowAsset* Many = MakeFlow({ TEXT("Fire"), TEXT("Flood") });
	USimFlowAsset* Empty = MakeFlow({});

	// The empty asset logs a warning on the way past. Warnings do not fail a test.
	TArray<FSimFlowScenarioOption> Options =
		USimFlowStatics::BuildScenarioOptions({ Single, nullptr, Many, Empty });

	// Single-entry asset: one option, labelled and keyed by the asset.
	// Multi-entry asset: one option per Start node, labelled and keyed by the entry.
	// A null asset and one with no Start node contribute nothing.
	if (!TestEqual(TEXT("three options from two usable assets"), Options.Num(), 3))
	{
		return false;
	}

	TestEqual(TEXT("a single-entry asset is labelled by its display name"),
		Options[0].DisplayName.ToString(), FString(TEXT("Traffic Scenario")));
	TestEqual(TEXT("and keyed by the asset"), Options[0].SaveId, Single->GetFName());
	TestEqual(TEXT("and starts from its only entry"), Options[0].EntryName, FName(TEXT("Default")));

	TestEqual(TEXT("a multi-entry asset is labelled by the entry"),
		Options[1].DisplayName.ToString(), FString(TEXT("Fire")));
	TestEqual(TEXT("and keyed Asset.Entry so the two entries do not share a history"),
		Options[1].SaveId, FName(*FString::Printf(TEXT("%s.Fire"), *Many->GetName())));
	TestEqual(TEXT("the second entry gets its own option"),
		Options[2].SaveId, FName(*FString::Printf(TEXT("%s.Flood"), *Many->GetName())));

	// Records: only the scenario that has been played reports one.
	TestFalse(TEXT("nothing has a record before anything is played"), Options[0].bHasRecord);

	USimFlowStatics::RecordPlay(Options[1].SaveId, 42.f, ESimFlowRunState::Completed, TestSlot, 0);
	USimFlowStatics::ApplyScenarioRecords(Options, TestSlot, 0);

	TestTrue(TEXT("the played scenario reports a record"), Options[1].bHasRecord);
	TestEqual(TEXT("with its best score"), Options[1].Record.BestScore, 42.f);
	TestEqual(TEXT("and one play"), Options[1].Record.PlayCount, 1);
	TestFalse(TEXT("the other entry of the same asset stays untouched"), Options[2].bHasRecord);
	TestFalse(TEXT("and so does the other asset"), Options[0].bHasRecord);

	UGameplayStatics::DeleteGameInSlot(TestSlot, 0);
	return true;
}

#endif
