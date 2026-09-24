// Copyright SimFlow. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SimFlowScenarioRecord.h"
#include "SimFlowStatics.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FString TestSlot = TEXT("SimFlowScenarioRecordTest");
	const FName TestId = TEXT("TestScenario");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowScenarioRecordTest,
	"SimFlow.ScenarioRecord", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowScenarioRecordTest::RunTest(const FString&)
{
	UGameplayStatics::DeleteGameInSlot(TestSlot, 0);

	TestFalse(TEXT("no record before anything is written"),
		USimFlowStatics::HasScenarioRecord(TestId, TestSlot, 0));

	TestTrue(TEXT("first play sets the best"),
		USimFlowStatics::RecordPlay(TestId, 100.f, ESimFlowRunState::Completed, TestSlot, 0));

	TestFalse(TEXT("an equal score does not replace"),
		USimFlowStatics::RecordPlay(TestId, 100.f, ESimFlowRunState::Completed, TestSlot, 0));

	TestFalse(TEXT("a lower score does not replace"),
		USimFlowStatics::RecordPlay(TestId, 99.f, ESimFlowRunState::Completed, TestSlot, 0));

	TestTrue(TEXT("a higher score replaces"),
		USimFlowStatics::RecordPlay(TestId, 101.f, ESimFlowRunState::Failed, TestSlot, 0));

	// bScoreCounts off: the play still counts, the best must not move.
	TestFalse(TEXT("a play that may not score returns false"),
		USimFlowStatics::RecordPlay(TestId, 500.f, ESimFlowRunState::Failed, TestSlot, 0, false));

	const FSimFlowScenarioRecord Record = USimFlowStatics::GetScenarioRecord(TestId, TestSlot, 0);
	TestEqual(TEXT("every attempt was counted"), Record.PlayCount, 5);
	TestEqual(TEXT("the best is the highest scoring attempt"), Record.BestScore, 101.f);
	TestEqual(TEXT("the last outcome is the last attempt's"), Record.LastOutcome, ESimFlowRunState::Failed);
	TestTrue(TEXT("last played was stamped"), Record.LastPlayedAt > FDateTime(0));

	TestTrue(TEXT("reset clears the record"), USimFlowStatics::ResetScenarioRecord(TestId, TestSlot, 0));
	TestFalse(TEXT("and it is gone"), USimFlowStatics::HasScenarioRecord(TestId, TestSlot, 0));

	UGameplayStatics::DeleteGameInSlot(TestSlot, 0);
	return true;
}

#endif
