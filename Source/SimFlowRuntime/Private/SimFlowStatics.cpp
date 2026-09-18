// Copyright SimFlow. All Rights Reserved.

#include "SimFlowStatics.h"
#include "SimFlowComponent.h"
#include "SimFlowSubsystem.h"
#include "SimFlowPlayerComponent.h"
#include "SimFlowRuntimeModule.h"
#include "SimFlowScenarioRecord.h"
#include "Kismet/GameplayStatics.h"
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
	bool UpdateRecord(FName FlowSaveId, float Score, ESimFlowRunState Outcome,
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

		// First score ever takes it; after that it has to beat the best, ties do not.
		const bool bNewBest = bScoreCounts
			&& (Record.BestScoreAt == FDateTime(0) || Score > Record.BestScore);
		if (bNewBest)
		{
			Record.BestScore = Score;
			Record.BestScoreAt = FDateTime::UtcNow();
		}

		if (bCountPlay)
		{
			Record.PlayCount++;
			Record.LastOutcome = Outcome;
			Record.LastPlayedAt = FDateTime::UtcNow();
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

bool USimFlowStatics::RecordPlay(FName FlowSaveId, float Score, ESimFlowRunState Outcome, const FString& SlotName, int32 UserIndex, bool bScoreCounts)
{
	return UpdateRecord(FlowSaveId, Score, Outcome, true, bScoreCounts, SlotName, UserIndex);
}

bool USimFlowStatics::SubmitHighScore(FName FlowSaveId, float Score, const FString& SlotName, int32 UserIndex)
{
	return UpdateRecord(FlowSaveId, Score, ESimFlowRunState::NotStarted, false, true, SlotName, UserIndex);
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
