// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SimFlowTypes.h"
#include "SimFlowScenarioRecord.generated.h"

class USimFlowAsset;

/** Raised when a recorded score beats the stored best. PreviousBest is 0 when there was none. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSimFlowHighScoreSignature, float, NewScore, float, PreviousBest);

/**
 * What one scenario has done across every attempt: how often, how it ended last
 * time, when that was, and the best score it has ever produced.
 *
 * Deliberately not part of FSimFlowSaveState. That is a resume snapshot of one
 * run and gets cleared with the save; this outlives every run.
 */
USTRUCT(BlueprintType)
struct SIMFLOWRUNTIME_API FSimFlowScenarioRecord
{
	GENERATED_BODY()

	/** Highest score this scenario has ever produced. */
	UPROPERTY(BlueprintReadWrite, Category = "SimFlow|Record")
	float BestScore = 0.f;

	/** UTC, when BestScore was set. Zero when no score has been recorded. */
	UPROPERTY(BlueprintReadWrite, Category = "SimFlow|Record")
	FDateTime BestScoreAt = FDateTime(0);

	/** Every finished attempt counts, however it ended. */
	UPROPERTY(BlueprintReadWrite, Category = "SimFlow|Record")
	int32 PlayCount = 0;

	/** How the most recent attempt ended: Completed, Failed, Aborted. */
	UPROPERTY(BlueprintReadWrite, Category = "SimFlow|Record")
	ESimFlowRunState LastOutcome = ESimFlowRunState::NotStarted;

	/** UTC, when that attempt ended. */
	UPROPERTY(BlueprintReadWrite, Category = "SimFlow|Record")
	FDateTime LastPlayedAt = FDateTime(0);
};

/**
 * One pickable scenario: a flow asset, the Start node to begin from, and whatever
 * that scenario has done before.
 *
 * Built by Build Scenario Options. SimFlow Selector Widget holds a list of these,
 * but nothing about them needs that widget - any Blueprint can build the list.
 */
USTRUCT(BlueprintType)
struct SIMFLOWRUNTIME_API FSimFlowScenarioOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SimFlow|Selector")
	TObjectPtr<USimFlowAsset> FlowAsset = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "SimFlow|Selector")
	FName EntryName = TEXT("Default");

	/** Button label: the asset's display name, or the entry name when one asset holds several. */
	UPROPERTY(BlueprintReadOnly, Category = "SimFlow|Selector")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "SimFlow|Selector")
	FText Description;

	/** Record key for this scenario. The asset name, or Asset.Entry when the asset has several entries. */
	UPROPERTY(BlueprintReadOnly, Category = "SimFlow|Selector")
	FName SaveId = NAME_None;

	/** History as of the last refresh. All zero when never played - check bHasRecord first. */
	UPROPERTY(BlueprintReadOnly, Category = "SimFlow|Selector")
	FSimFlowScenarioRecord Record;

	/** Tells a stored 0 from never played. */
	UPROPERTY(BlueprintReadOnly, Category = "SimFlow|Selector")
	bool bHasRecord = false;
};

/**
 * Scenario history, keyed by the same Flow Save Id run saves use. Its own slot, so
 * Delete Flow Save and a fresh attempt both leave it standing.
 *
 * A selector widget can read this without loading a flow or touching the level.
 */
UCLASS(BlueprintType)
class SIMFLOWRUNTIME_API USimFlowScenarioSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "SimFlow|Record")
	TMap<FName, FSimFlowScenarioRecord> Records;

	UPROPERTY(BlueprintReadWrite, Category = "SimFlow|Record")
	int32 SaveVersion = 1;

	UFUNCTION(BlueprintPure, Category = "SimFlow|Record")
	bool HasRecord(FName FlowSaveId) const { return Records.Contains(FlowSaveId); }
};

/** The slot scenario records go to when nothing else is named. */
namespace SimFlowScenarioDefaults
{
	inline const TCHAR* const SlotName = TEXT("SimFlowScenarios");
}
