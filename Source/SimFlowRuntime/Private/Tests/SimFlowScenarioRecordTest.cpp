// Copyright SimFlow. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SimFlowScenarioRecord.h"
#include "SimFlowStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowRunHistoryExportTest,
	"SimFlow.RunHistoryExport",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowRunHistoryExportTest::RunTest(const FString&)
{
	const FString Slot = TEXT("SimFlowRunHistoryTest");
	const FName History = TEXT("History");
	const FName Legacy = TEXT("Legacy");
	UGameplayStatics::DeleteGameInSlot(Slot, 0);

	FString Path;
	TestFalse(TEXT("nothing to export from an empty slot"),
		USimFlowStatics::ExportScenarioRecordsToCsv(TEXT("RunHistoryTest"), Path, Slot, 0));

	// A record written before run history existed: a count and a best, no runs.
	{
		USimFlowScenarioSave* Save = Cast<USimFlowScenarioSave>(
			UGameplayStatics::CreateSaveGameObject(USimFlowScenarioSave::StaticClass()));
		FSimFlowScenarioRecord& Old = Save->Records.Add(Legacy);
		Old.PlayCount = 3;
		Old.BestScore = 7.f;
		Old.BestScoreAt = FDateTime::UtcNow();
		UGameplayStatics::SaveGameToSlot(Save, Slot, 0);
	}

	// Ten plays scoring 10..100, the last one failed and not allowed to score.
	for (int32 Play = 1; Play <= 10; ++Play)
	{
		const bool bLast = Play == 10;
		USimFlowStatics::RecordPlay(History, Play * 10.f,
			bLast ? ESimFlowRunState::Failed : ESimFlowRunState::Completed, Slot, 0, !bLast, Play + 0.5f);
	}
	USimFlowStatics::SubmitHighScore(History, 1000.f, Slot, 0);

	const FSimFlowScenarioRecord Record = USimFlowStatics::GetScenarioRecord(History, Slot, 0);
	TestEqual(TEXT("one run per play, none for Submit High Score"), Record.Runs.Num(), 10);
	if (Record.Runs.Num() == 10)
	{
		TestEqual(TEXT("oldest first"), Record.Runs[0].Score, 10.f);
		TestEqual(TEXT("elapsed time is kept"), Record.Runs[0].ElapsedSeconds, 1.5f);
		TestTrue(TEXT("the run is stamped"), Record.Runs[0].EndedAt > FDateTime(0));
		TestEqual(TEXT("the outcome is kept"), Record.Runs[9].Outcome, ESimFlowRunState::Failed);
		TestFalse(TEXT("a play that may not score is flagged"), Record.Runs[9].bScoreCounted);
	}

	TestFalse(TEXT("a path is refused"),
		USimFlowStatics::ExportScenarioRecordsToCsv(TEXT("../Escape"), Path, Slot, 0));
	TestTrue(TEXT("and gives back no path"), Path.IsEmpty());

	// CSV: header, ten run rows, and the legacy record as a summary-only row.
	TestTrue(TEXT("CSV export writes"),
		USimFlowStatics::ExportScenarioRecordsToCsv(TEXT("RunHistoryTest"), Path, Slot, 0));
	TestTrue(TEXT("into Saved/SimFlow/Exports"), Path.EndsWith(TEXT("SimFlow/Exports/RunHistoryTest.csv")));

	FString Csv;
	FFileHelper::LoadFileToString(Csv, *Path);
	TArray<FString> Lines;
	Csv.ParseIntoArrayLines(Lines);
	TestEqual(TEXT("header, ten runs, one summary row"), Lines.Num(), 12);
	if (Lines.Num() == 12)
	{
		TestEqual(TEXT("header"), Lines[0],
			FString(TEXT("Scenario,Play,EndedAtUtc,Outcome,Score,ElapsedSeconds,ScoreCounted,BestScore,PlayCount")));
		TestTrue(TEXT("first run row"), Lines[1].StartsWith(TEXT("History,1,"))
			&& Lines[1].EndsWith(TEXT(",Completed,10.0,1.5,true,1000.0,10")));
		TestTrue(TEXT("last run row"), Lines[10].StartsWith(TEXT("History,10,"))
			&& Lines[10].EndsWith(TEXT(",Failed,100.0,10.5,false,1000.0,10")));
		TestEqual(TEXT("a record with no runs still exports its summary"), Lines[11],
			FString(TEXT("Legacy,,,,,,,7.0,3")));
	}
	IFileManager::Get().Delete(*Path);

	// JSON: the same data, runs nested under each scenario.
	TestTrue(TEXT("JSON export writes"),
		USimFlowStatics::ExportScenarioRecordsToJson(TEXT("RunHistoryTest"), Path, Slot, 0));

	FString Json;
	FFileHelper::LoadFileToString(Json, *Path);
	TSharedPtr<FJsonObject> Root;
	TestTrue(TEXT("JSON parses"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) && Root.IsValid());
	if (Root.IsValid())
	{
		const TSharedPtr<FJsonObject> Scenario = Root->GetObjectField(TEXT("scenarios"))->GetObjectField(TEXT("History"));
		TestEqual(TEXT("play count"), Scenario->GetNumberField(TEXT("playCount")), 10.0);

		const TArray<TSharedPtr<FJsonValue>> Runs = Scenario->GetArrayField(TEXT("runs"));
		TestEqual(TEXT("runs are nested"), Runs.Num(), 10);
		if (Runs.Num() == 10)
		{
			const TSharedPtr<FJsonObject> Last = Runs[9]->AsObject();
			TestEqual(TEXT("outcome by name"), Last->GetStringField(TEXT("outcome")), FString(TEXT("Failed")));
			TestEqual(TEXT("elapsed seconds"), Last->GetNumberField(TEXT("elapsedSeconds")), 10.5);
			TestTrue(TEXT("dates are ISO 8601 UTC"), Last->GetStringField(TEXT("endedAt")).EndsWith(TEXT("Z")));
		}
	}
	IFileManager::Get().Delete(*Path);

	// The cap keeps the newest runs.
	for (int32 Play = 11; Play <= SimFlowScenarioDefaults::MaxRunsKept + 5; ++Play)
	{
		USimFlowStatics::RecordPlay(History, Play * 10.f, ESimFlowRunState::Completed, Slot, 0);
	}
	const FSimFlowScenarioRecord Capped = USimFlowStatics::GetScenarioRecord(History, Slot, 0);
	TestEqual(TEXT("history stops at the cap"), Capped.Runs.Num(), SimFlowScenarioDefaults::MaxRunsKept);
	TestEqual(TEXT("the oldest runs went first"), Capped.Runs[0].Score, 60.f);
	TestEqual(TEXT("the play count is not capped"), Capped.PlayCount, SimFlowScenarioDefaults::MaxRunsKept + 5);

	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	return true;
}

#endif
