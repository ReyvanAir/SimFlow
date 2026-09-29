// Copyright SimFlow. All Rights Reserved.

#include "SimFlowStatics.h"
#include "SimFlowAsset.h"
#include "SimFlowComponent.h"
#include "SimFlowSubsystem.h"
#include "SimFlowPlayerComponent.h"
#include "SimFlowRuntimeModule.h"
#include "SimFlowScenarioRecord.h"
#include "Kismet/GameplayStatics.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

USimFlowComponent* USimFlowStatics::GetPrimaryFlow(const UObject* WorldContextObject)
{
	USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject);
	return Subsystem ? Subsystem->GetPrimaryFlow() : nullptr;
}

USimFlowComponent* USimFlowStatics::FindFlowById(const UObject* WorldContextObject, FName FlowSaveId)
{
	USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject);
	return Subsystem ? Subsystem->FindFlowById(FlowSaveId) : nullptr;
}

USimFlowComponent* USimFlowStatics::GetFlowFromActor(AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<USimFlowComponent>() : nullptr;
}

void USimFlowStatics::BroadcastFlowEvent(const UObject* WorldContextObject, FGameplayTag EventTag, UObject* Payload)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;

	// On a client one request covers every flow the server is running, so send a
	// single RPC rather than one per local mirror.
	if (World && World->GetNetMode() == NM_Client)
	{
		if (USimFlowPlayerComponent* Player = USimFlowPlayerComponent::GetLocal(WorldContextObject))
		{
			Player->RequestEvent(NAME_None, EventTag, Payload);
		}
		else
		{
			UE_LOG(LogSimFlow, Warning,
				TEXT("Event '%s' was raised on a client with no SimFlow Player Component on the PlayerController, so it never reached the server."),
				*EventTag.ToString());
		}
		return;
	}

	USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		return;
	}

	for (USimFlowComponent* Component : Subsystem->GetAllFlowComponents())
	{
		Component->SendEvent(EventTag, Payload);
	}
}

void USimFlowStatics::RequestFlowControl(const UObject* WorldContextObject, FName FlowSaveId, ESimFlowControlRequest Request)
{
	if (USimFlowPlayerComponent* Player = USimFlowPlayerComponent::GetLocal(WorldContextObject))
	{
		Player->RequestControl(FlowSaveId, Request);
		return;
	}

	// No player component: fall back to acting directly, which is correct in
	// single player and on the server.
	if (USimFlowComponent* Component = FindFlowById(WorldContextObject, FlowSaveId))
	{
		switch (Request)
		{
		case ESimFlowControlRequest::Start:			Component->StartFlow();			break;
		case ESimFlowControlRequest::Stop:			Component->StopFlow();			break;
		case ESimFlowControlRequest::Restart:		Component->RestartFlow();		break;
		case ESimFlowControlRequest::Pause:			Component->PauseFlow();			break;
		case ESimFlowControlRequest::Resume:		Component->ResumeFlow();		break;
		case ESimFlowControlRequest::TogglePause:	Component->TogglePause();		break;
		case ESimFlowControlRequest::Retry:			Component->RetryCurrentTask();	break;
		case ESimFlowControlRequest::Skip:			Component->SkipCurrentTask();	break;
		case ESimFlowControlRequest::Fail:			Component->FailCurrentTask();	break;
		default:																	break;
		}
	}
}

void USimFlowStatics::SendFlowEvent(const UObject* WorldContextObject, FName FlowSaveId, FGameplayTag EventTag, UObject* Payload)
{
	if (USimFlowComponent* Component = FindFlowById(WorldContextObject, FlowSaveId))
	{
		Component->SendEvent(EventTag, Payload);
	}
}

void USimFlowStatics::PauseAllFlows(const UObject* WorldContextObject)
{
	if (USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject))
	{
		Subsystem->PauseAllFlows();
	}
}

void USimFlowStatics::ResumeAllFlows(const UObject* WorldContextObject)
{
	if (USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject))
	{
		Subsystem->ResumeAllFlows();
	}
}

bool USimFlowStatics::SaveAllFlows(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex)
{
	USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject);
	return Subsystem ? Subsystem->SaveAllFlowsToSlot(SlotName, UserIndex) : false;
}

void USimFlowStatics::StopAllFlows(const UObject* WorldContextObject)
{
	if (USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject))
	{
		Subsystem->StopAllFlows();
	}
}

int32 USimFlowStatics::LoadAllFlows(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex, ESimFlowLoadMode LoadMode)
{
	USimFlowSubsystem* Subsystem = USimFlowSubsystem::Get(WorldContextObject);
	return Subsystem ? Subsystem->LoadAllFlowsFromSlot(SlotName, UserIndex, LoadMode) : 0;
}

bool USimFlowStatics::DeleteFlowSave(const FString& SlotName, int32 UserIndex)
{
	return USimFlowSubsystem::DeleteSaveSlot(SlotName, UserIndex);
}

// --------------------------------------------------------------- Scenario record

namespace
{
	FString ResolveScenarioSlot(const FString& SlotName)
	{
		return SlotName.IsEmpty() ? FString(SimFlowScenarioDefaults::SlotName) : SlotName;
	}

	/**
	 * Reads the record table out of a slot. Null when the slot holds something else:
	 * better to refuse than to overwrite a save that is not ours.
	 */
	USimFlowScenarioSave* OpenScenarioSave(const FString& Slot, int32 UserIndex, bool bCreateWhenMissing)
	{
		if (UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
		{
			USimFlowScenarioSave* Existing = Cast<USimFlowScenarioSave>(UGameplayStatics::LoadGameFromSlot(Slot, UserIndex));
			if (!Existing)
			{
				UE_LOG(LogSimFlow, Warning,
					TEXT("Slot '%s' exists but does not hold SimFlow scenario records. Refusing to overwrite it."), *Slot);
			}
			return Existing;
		}

		if (!bCreateWhenMissing)
		{
			return nullptr;
		}
		return Cast<USimFlowScenarioSave>(UGameplayStatics::CreateSaveGameObject(USimFlowScenarioSave::StaticClass()));
	}

	bool WriteScenarioSave(USimFlowScenarioSave* Save, const FString& Slot, int32 UserIndex)
	{
		if (UGameplayStatics::SaveGameToSlot(Save, Slot, UserIndex))
		{
			return true;
		}
		UE_LOG(LogSimFlow, Warning, TEXT("Could not write scenario records to slot '%s'."), *Slot);
		return false;
	}

	/** Shared by RecordPlay and SubmitHighScore. bCountPlay separates the two. */
	bool UpdateRecord(FName FlowSaveId, float Score, ESimFlowRunState Outcome, float ElapsedSeconds,
		bool bCountPlay, bool bScoreCounts, const FString& SlotName, int32 UserIndex)
	{
		if (FlowSaveId.IsNone())
		{
			UE_LOG(LogSimFlow, Warning, TEXT("Scenario records need a flow save id. Set FlowSaveId on the component, or pass one."));
			return false;
		}

		const FString Slot = ResolveScenarioSlot(SlotName);
		USimFlowScenarioSave* Save = OpenScenarioSave(Slot, UserIndex, true);
		if (!Save)
		{
			return false;
		}

		FSimFlowScenarioRecord& Record = Save->Records.FindOrAdd(FlowSaveId);
		const FDateTime Now = FDateTime::UtcNow();

		// First score ever takes it; after that it has to beat the best, ties do not.
		const bool bNewBest = bScoreCounts
			&& (Record.BestScoreAt == FDateTime(0) || Score > Record.BestScore);
		if (bNewBest)
		{
			Record.BestScore = Score;
			Record.BestScoreAt = Now;
		}

		if (bCountPlay)
		{
			Record.PlayCount++;
			Record.LastOutcome = Outcome;
			Record.LastPlayedAt = Now;

			FSimFlowRunEntry& Run = Record.Runs.AddDefaulted_GetRef();
			Run.EndedAt = Now;
			Run.Outcome = Outcome;
			Run.Score = Score;
			Run.ElapsedSeconds = ElapsedSeconds;
			Run.bScoreCounted = bScoreCounts;

			const int32 Excess = Record.Runs.Num() - SimFlowScenarioDefaults::MaxRunsKept;
			if (Excess > 0)
			{
				Record.Runs.RemoveAt(0, Excess);
			}
		}
		else if (!bNewBest)
		{
			return false;   // score-only submission that beat nothing: no write
		}

		if (!WriteScenarioSave(Save, Slot, UserIndex))
		{
			return false;
		}

		UE_LOG(LogSimFlow, Log, TEXT("Scenario '%s': play %d, %s, best %g."),
			*FlowSaveId.ToString(), Record.PlayCount, bNewBest ? TEXT("new best") : TEXT("no change"), Record.BestScore);
		return bNewBest;
	}
}

bool USimFlowStatics::RecordPlay(FName FlowSaveId, float Score, ESimFlowRunState Outcome, const FString& SlotName, int32 UserIndex, bool bScoreCounts, float ElapsedSeconds)
{
	return UpdateRecord(FlowSaveId, Score, Outcome, ElapsedSeconds, true, bScoreCounts, SlotName, UserIndex);
}

bool USimFlowStatics::SubmitHighScore(FName FlowSaveId, float Score, const FString& SlotName, int32 UserIndex)
{
	return UpdateRecord(FlowSaveId, Score, ESimFlowRunState::NotStarted, 0.f, false, true, SlotName, UserIndex);
}

FSimFlowScenarioRecord USimFlowStatics::GetScenarioRecord(FName FlowSaveId, const FString& SlotName, int32 UserIndex)
{
	if (const USimFlowScenarioSave* Save = OpenScenarioSave(ResolveScenarioSlot(SlotName), UserIndex, false))
	{
		if (const FSimFlowScenarioRecord* Record = Save->Records.Find(FlowSaveId))
		{
			return *Record;
		}
	}
	return FSimFlowScenarioRecord();
}

bool USimFlowStatics::HasScenarioRecord(FName FlowSaveId, const FString& SlotName, int32 UserIndex)
{
	const USimFlowScenarioSave* Save = OpenScenarioSave(ResolveScenarioSlot(SlotName), UserIndex, false);
	return Save && Save->Records.Contains(FlowSaveId);
}

float USimFlowStatics::GetHighScore(FName FlowSaveId, const FString& SlotName, int32 UserIndex)
{
	return GetScenarioRecord(FlowSaveId, SlotName, UserIndex).BestScore;
}

int32 USimFlowStatics::GetPlayCount(FName FlowSaveId, const FString& SlotName, int32 UserIndex)
{
	return GetScenarioRecord(FlowSaveId, SlotName, UserIndex).PlayCount;
}

TMap<FName, FSimFlowScenarioRecord> USimFlowStatics::GetAllScenarioRecords(const FString& SlotName, int32 UserIndex)
{
	if (const USimFlowScenarioSave* Save = OpenScenarioSave(ResolveScenarioSlot(SlotName), UserIndex, false))
	{
		return Save->Records;
	}
	return TMap<FName, FSimFlowScenarioRecord>();
}

bool USimFlowStatics::WouldBeatHighScore(FName FlowSaveId, float Score, const FString& SlotName, int32 UserIndex)
{
	const USimFlowScenarioSave* Save = OpenScenarioSave(ResolveScenarioSlot(SlotName), UserIndex, false);
	if (!Save)
	{
		return true;
	}

	const FSimFlowScenarioRecord* Record = Save->Records.Find(FlowSaveId);
	return !Record || Score > Record->BestScore;
}

bool USimFlowStatics::ResetScenarioRecord(FName FlowSaveId, const FString& SlotName, int32 UserIndex)
{
	const FString Slot = ResolveScenarioSlot(SlotName);
	USimFlowScenarioSave* Save = OpenScenarioSave(Slot, UserIndex, false);
	if (!Save || Save->Records.Remove(FlowSaveId) == 0)
	{
		return false;
	}
	return WriteScenarioSave(Save, Slot, UserIndex);
}

bool USimFlowStatics::ResetHighScore(FName FlowSaveId, const FString& SlotName, int32 UserIndex)
{
	const FString Slot = ResolveScenarioSlot(SlotName);
	USimFlowScenarioSave* Save = OpenScenarioSave(Slot, UserIndex, false);
	if (!Save)
	{
		return false;
	}

	FSimFlowScenarioRecord* Record = Save->Records.Find(FlowSaveId);
	if (!Record || Record->BestScoreAt == FDateTime(0))
	{
		return false;
	}

	Record->BestScore = 0.f;
	Record->BestScoreAt = FDateTime(0);
	return WriteScenarioSave(Save, Slot, UserIndex);
}

bool USimFlowStatics::ResetAllScenarioRecords(const FString& SlotName, int32 UserIndex)
{
	const FString Slot = ResolveScenarioSlot(SlotName);
	USimFlowScenarioSave* Save = OpenScenarioSave(Slot, UserIndex, false);
	if (!Save)
	{
		// Nothing of ours to clear. OpenScenarioSave has already warned if the slot is foreign.
		return !UGameplayStatics::DoesSaveGameExist(Slot, UserIndex);
	}

	Save->Records.Empty();
	return WriteScenarioSave(Save, Slot, UserIndex);
}

// ---------------------------------------------------------------------- Export

namespace
{
	FString IsoOrBlank(const FDateTime& Time)
	{
		return Time == FDateTime(0) ? FString() : Time.ToIso8601();
	}

	FString OutcomeName(ESimFlowRunState Outcome)
	{
		return StaticEnum<ESimFlowRunState>()->GetNameStringByValue(static_cast<int64>(Outcome));
	}

	FString CsvField(const FString& Value)
	{
		if (Value.Contains(TEXT(",")) || Value.Contains(TEXT("\"")) || Value.Contains(TEXT("\n")))
		{
			return TEXT("\"") + Value.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
		}
		return Value;
	}

	/** One row per run, each carrying its scenario's best score and play count. */
	FString RecordsToCsv(const TMap<FName, FSimFlowScenarioRecord>& Records)
	{
		FString Csv = TEXT("Scenario,Play,EndedAtUtc,Outcome,Score,ElapsedSeconds,ScoreCounted,BestScore,PlayCount\n");

		for (const TPair<FName, FSimFlowScenarioRecord>& Pair : Records)
		{
			const FSimFlowScenarioRecord& Record = Pair.Value;
			const FString Id = CsvField(Pair.Key.ToString());
			const FString Summary = FString::Printf(TEXT("%s,%d"), *FString::SanitizeFloat(Record.BestScore), Record.PlayCount);

			if (Record.Runs.IsEmpty())
			{
				// Played before run history existed: the summary is all there is.
				Csv += FString::Printf(TEXT("%s,,,,,,,%s\n"), *Id, *Summary);
				continue;
			}

			// Plays before history began, or past the cap, have no entry; number from the end.
			const int32 FirstPlay = FMath::Max(1, Record.PlayCount - Record.Runs.Num() + 1);
			for (int32 Index = 0; Index < Record.Runs.Num(); ++Index)
			{
				const FSimFlowRunEntry& Run = Record.Runs[Index];
				Csv += FString::Printf(TEXT("%s,%d,%s,%s,%s,%s,%s,%s\n"),
					*Id, FirstPlay + Index, *IsoOrBlank(Run.EndedAt), *OutcomeName(Run.Outcome),
					*FString::SanitizeFloat(Run.Score), *FString::SanitizeFloat(Run.ElapsedSeconds),
					Run.bScoreCounted ? TEXT("true") : TEXT("false"), *Summary);
			}
		}
		return Csv;
	}

	FString RecordsToJson(const TMap<FName, FSimFlowScenarioRecord>& Records)
	{
		// ISO 8601 instead of Unreal's own date text, so any JSON reader can parse them.
		FJsonObjectConverter::CustomExportCallback IsoDates;
		IsoDates.BindLambda([](FProperty* Property, const void* Value) -> TSharedPtr<FJsonValue>
		{
			const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			if (!StructProperty || StructProperty->Struct != TBaseStructure<FDateTime>::Get())
			{
				return nullptr;
			}
			const FDateTime& Time = *static_cast<const FDateTime*>(Value);
			return Time == FDateTime(0)
				? StaticCastSharedRef<FJsonValue>(MakeShared<FJsonValueNull>())
				: StaticCastSharedRef<FJsonValue>(MakeShared<FJsonValueString>(Time.ToIso8601()));
		});

		const TSharedRef<FJsonObject> Scenarios = MakeShared<FJsonObject>();
		for (const TPair<FName, FSimFlowScenarioRecord>& Pair : Records)
		{
			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSimFlowScenarioRecord::StaticStruct(), &Pair.Value, Entry, 0, 0, &IsoDates);
			Scenarios->SetObjectField(Pair.Key.ToString(), Entry);
		}

		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("schemaVersion"), 1);
		Root->SetStringField(TEXT("exportedAt"), FDateTime::UtcNow().ToIso8601());
		Root->SetObjectField(TEXT("scenarios"), Scenarios);

		FString Json;
		FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
		return Json;
	}

	bool ExportRecords(const FString& FileName, const TCHAR* Extension, const FString& SlotName, int32 UserIndex,
		FString& OutFilePath, FFileHelper::EEncodingOptions Encoding,
		TFunctionRef<FString(const TMap<FName, FSimFlowScenarioRecord>&)> Format)
	{
		OutFilePath.Reset();

		FString Name = FileName.IsEmpty()
			? FDateTime::Now().ToString(TEXT("SimFlowRecords_%Y%m%d_%H%M%S"))
			: FileName;

		// A plain name only, so whatever a Blueprint passes stays inside the exports folder.
		if (Name.Contains(TEXT("..")) || Name.Contains(TEXT("/")) || Name.Contains(TEXT("\\")) || Name.Contains(TEXT(":")))
		{
			UE_LOG(LogSimFlow, Warning, TEXT("Export file name '%s' must be a plain name, not a path."), *FileName);
			return false;
		}
		if (!Name.EndsWith(Extension))
		{
			Name += Extension;
		}

		TMap<FName, FSimFlowScenarioRecord> Records = USimFlowStatics::GetAllScenarioRecords(SlotName, UserIndex);
		if (Records.IsEmpty())
		{
			UE_LOG(LogSimFlow, Warning, TEXT("Slot '%s' has no scenario records to export."), *ResolveScenarioSlot(SlotName));
			return false;
		}
		Records.KeySort(FNameLexicalLess());

		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SimFlow/Exports") / Name);
		if (!FFileHelper::SaveStringToFile(Format(Records), *Path, Encoding))
		{
			UE_LOG(LogSimFlow, Warning, TEXT("Could not write '%s'."), *Path);
			return false;
		}

		UE_LOG(LogSimFlow, Log, TEXT("Exported %d scenario records to '%s'."), Records.Num(), *Path);
		OutFilePath = Path;
		return true;
	}
}

bool USimFlowStatics::ExportScenarioRecordsToCsv(const FString& FileName, FString& FilePath, const FString& SlotName, int32 UserIndex)
{
	// With a BOM, so Excel reads non-ASCII scenario ids correctly.
	return ExportRecords(FileName, TEXT(".csv"), SlotName, UserIndex, FilePath, FFileHelper::EEncodingOptions::ForceUTF8, RecordsToCsv);
}

bool USimFlowStatics::ExportScenarioRecordsToJson(const FString& FileName, FString& FilePath, const FString& SlotName, int32 UserIndex)
{
	return ExportRecords(FileName, TEXT(".json"), SlotName, UserIndex, FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, RecordsToJson);
}

TArray<FSimFlowScenarioOption> USimFlowStatics::BuildScenarioOptions(const TArray<USimFlowAsset*>& Assets)
{
	TArray<FSimFlowScenarioOption> Result;

	for (USimFlowAsset* Asset : Assets)
	{
		if (!Asset)
		{
			continue;
		}

		const TArray<FName> Entries = Asset->GetEntryNames();
		if (Entries.Num() == 0)
		{
			// No Start node means nothing to start. A button for it would always fail.
			UE_LOG(LogSimFlow, Warning,
				TEXT("Flow '%s' has no Start node, so it cannot be offered as a scenario."),
				*Asset->GetName());
			continue;
		}

		// One asset per scenario is the common shape; several Start nodes in one asset
		// is the other. Expanding every entry covers both without a switch.
		const bool bManyEntries = Entries.Num() > 1;

		for (const FName Entry : Entries)
		{
			FSimFlowScenarioOption& Option = Result.AddDefaulted_GetRef();
			Option.FlowAsset = Asset;
			Option.EntryName = Entry;
			Option.Description = Asset->FlowDescription;
			Option.DisplayName = bManyEntries ? FText::FromName(Entry) : Asset->GetDisplayNameText();
			Option.SaveId = bManyEntries
				? FName(*FString::Printf(TEXT("%s.%s"), *Asset->GetName(), *Entry.ToString()))
				: Asset->GetFName();
		}
	}

	return Result;
}

void USimFlowStatics::ApplyScenarioRecords(TArray<FSimFlowScenarioOption>& Options, const FString& SlotName, int32 UserIndex)
{
	// One read of the slot for the whole list, not one per option.
	const TMap<FName, FSimFlowScenarioRecord> All = GetAllScenarioRecords(SlotName, UserIndex);

	for (FSimFlowScenarioOption& Option : Options)
	{
		const FSimFlowScenarioRecord* Found = All.Find(Option.SaveId);
		Option.Record = Found ? *Found : FSimFlowScenarioRecord();
		Option.bHasRecord = Found != nullptr;
	}
}

bool USimFlowStatics::StartScenario(USimFlowComponent* Flow, const FSimFlowScenarioOption& Option, bool bAssignSaveId)
{
	if (!Flow)
	{
		UE_LOG(LogSimFlow, Warning,
			TEXT("Start Scenario: no flow to run '%s' on. Get Primary Flow or Find Flow By Id first."),
			*Option.SaveId.ToString());
		return false;
	}

	if (Flow->IsClientMirror())
	{
		// The asset and the save id live on the server. Writing them here would only
		// desync the mirror, so forward the start and let the server use its own.
		UE_LOG(LogSimFlow, Warning,
			TEXT("Start Scenario picked '%s', but flow '%s' is server-authoritative: the server starts the scenario it has configured. "
				 "Choose the scenario server-side, or give each trainee an unreplicated flow."),
			*Option.SaveId.ToString(), *Flow->GetEffectiveSaveId().ToString());

		return Flow->StartFlow();
	}

	if (!Option.FlowAsset)
	{
		UE_LOG(LogSimFlow, Warning, TEXT("Start Scenario: option '%s' has no flow asset."), *Option.SaveId.ToString());
		return false;
	}

	// Swapping the asset under a running flow would leave the old instance mid-task,
	// so end it first. The stop counts as an Aborted play.
	if (Flow->IsFlowRunning())
	{
		Flow->StopFlow();
	}

	if (bAssignSaveId)
	{
		Flow->FlowSaveId = Option.SaveId;
	}
	Flow->FlowAsset = Option.FlowAsset;
	Flow->EntryName = Option.EntryName;

	return Flow->StartFlowFromEntry(Option.EntryName);
}

FSimFlowValue USimFlowStatics::MakeFlowBool(bool Value)					{ return FSimFlowValue::MakeBool(Value); }
FSimFlowValue USimFlowStatics::MakeFlowInt(int32 Value)					{ return FSimFlowValue::MakeInt(Value); }
FSimFlowValue USimFlowStatics::MakeFlowFloat(float Value)				{ return FSimFlowValue::MakeFloat(Value); }
FSimFlowValue USimFlowStatics::MakeFlowString(const FString& Value)		{ return FSimFlowValue::MakeString(Value); }
FSimFlowValue USimFlowStatics::MakeFlowName(FName Value)					{ return FSimFlowValue::MakeName(Value); }
FSimFlowValue USimFlowStatics::MakeFlowVector(FVector Value)				{ return FSimFlowValue::MakeVector(Value); }
FSimFlowValue USimFlowStatics::MakeFlowObject(UObject* Value)				{ return FSimFlowValue::MakeObject(Value); }

bool USimFlowStatics::FlowValueToBool(const FSimFlowValue& Value)			{ return Value.AsBool(); }
int32 USimFlowStatics::FlowValueToInt(const FSimFlowValue& Value)			{ return FMath::RoundToInt(Value.AsNumber()); }
float USimFlowStatics::FlowValueToFloat(const FSimFlowValue& Value)		{ return static_cast<float>(Value.AsNumber()); }
FString USimFlowStatics::FlowValueToString(const FSimFlowValue& Value)	{ return Value.AsString(); }

FText USimFlowStatics::ResultToText(ESimFlowResult Result)
{
	return StaticEnum<ESimFlowResult>()->GetDisplayNameTextByValue(static_cast<int64>(Result));
}

FText USimFlowStatics::RunStateToText(ESimFlowRunState State)
{
	return StaticEnum<ESimFlowRunState>()->GetDisplayNameTextByValue(static_cast<int64>(State));
}

FText USimFlowStatics::FormatSeconds(float Seconds)
{
	const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
	const int32 Minutes = Total / 60;
	const int32 Rest = Total % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Rest));
}
