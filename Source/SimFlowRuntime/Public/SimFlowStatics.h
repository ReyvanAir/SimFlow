// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SimFlowTypes.h"
#include "SimFlowNetTypes.h"
#include "SimFlowScenarioRecord.h"
#include "SimFlowStatics.generated.h"

class USimFlowComponent;
class USimFlowInstance;
class USimFlowTask;
class AActor;

/** Blueprint helpers so gameplay code rarely needs a direct component reference. */
UCLASS()
class SIMFLOWRUNTIME_API USimFlowStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The first running flow in the game instance. */
	UFUNCTION(BlueprintPure, Category = "SimFlow", meta = (WorldContext = "WorldContextObject"))
	static USimFlowComponent* GetPrimaryFlow(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "SimFlow", meta = (WorldContext = "WorldContextObject"))
	static USimFlowComponent* FindFlowById(const UObject* WorldContextObject, FName FlowSaveId);

	/** Finds the flow component on an actor, if it has one. */
	UFUNCTION(BlueprintPure, Category = "SimFlow")
	static USimFlowComponent* GetFlowFromActor(AActor* Actor);

	/**
	 * Raises an event on every running flow. This is the usual way for a grabbable
	 * object, a button or an animation notify to talk to whatever flow is running.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow", meta = (WorldContext = "WorldContextObject"))
	static void BroadcastFlowEvent(const UObject* WorldContextObject, FGameplayTag EventTag, UObject* Payload = nullptr);

	/** Raises an event on one specific flow. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow", meta = (WorldContext = "WorldContextObject"))
	static void SendFlowEvent(const UObject* WorldContextObject, FName FlowSaveId, FGameplayTag EventTag, UObject* Payload = nullptr);

	/**
	 * Asks for a control action on a flow, taking the right route automatically:
	 * straight through on the server or in single player, via a Server RPC on a
	 * client. Ideal for instructor panels and pause menus.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Control", meta = (WorldContext = "WorldContextObject"))
	static void RequestFlowControl(const UObject* WorldContextObject, FName FlowSaveId, ESimFlowControlRequest Request);

	UFUNCTION(BlueprintCallable, Category = "SimFlow|Control", meta = (WorldContext = "WorldContextObject"))
	static void PauseAllFlows(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SimFlow|Control", meta = (WorldContext = "WorldContextObject"))
	static void ResumeAllFlows(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SimFlow|Control", meta = (WorldContext = "WorldContextObject"))
	static void StopAllFlows(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "SimFlow|Save", meta = (WorldContext = "WorldContextObject"))
	static bool SaveAllFlows(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "SimFlow|Save", meta = (WorldContext = "WorldContextObject"))
	static int32 LoadAllFlows(const UObject* WorldContextObject, const FString& SlotName, int32 UserIndex = 0,
		ESimFlowLoadMode LoadMode = ESimFlowLoadMode::ExactState);

	UFUNCTION(BlueprintCallable, Category = "SimFlow|Save")
	static bool DeleteFlowSave(const FString& SlotName, int32 UserIndex = 0);

	// --------------------------------------------------------- Scenario record

	// Per-scenario history - play count, last outcome, last played, best score -
	// in its own slot, so deleting a run save leaves it standing. Keyed by Flow
	// Save Id. Leave SlotName empty to use "SimFlowScenarios".
	//
	// Every one of these opens the slot off disk. Read once and cache; do not bind
	// one to a widget that ticks.

	/**
	 * One call at the end of an attempt: counts the play, stores how it ended and
	 * when, and takes the score if it beats the stored best. Returns true only when
	 * the best moved.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Record")
	static bool RecordPlay(FName FlowSaveId, float Score, ESimFlowRunState Outcome, const FString& SlotName, int32 UserIndex = 0, bool bScoreCounts = true);

	/** Score only, no play counted. Returns true when the best moved; a tie does not. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Record")
	static bool SubmitHighScore(FName FlowSaveId, float Score, const FString& SlotName, int32 UserIndex = 0);

	/** Everything stored for one scenario. All fields zero when there is no record. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Record")
	static FSimFlowScenarioRecord GetScenarioRecord(FName FlowSaveId, const FString& SlotName, int32 UserIndex = 0);

	/** Tells a stored 0 from never played. Check this before showing a number. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Record")
	static bool HasScenarioRecord(FName FlowSaveId, const FString& SlotName, int32 UserIndex = 0);

	/** Best score, or 0 when there is no record. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Record")
	static float GetHighScore(FName FlowSaveId, const FString& SlotName, int32 UserIndex = 0);

	/** How many finished attempts, or 0 when there is no record. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Record")
	static int32 GetPlayCount(FName FlowSaveId, const FString& SlotName, int32 UserIndex = 0);

	/** Every record in the slot. This is what a scenario selector reads. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Record")
	static TMap<FName, FSimFlowScenarioRecord> GetAllScenarioRecords(const FString& SlotName, int32 UserIndex = 0);

	/** Would this score take the best? Writes nothing. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Record")
	static bool WouldBeatHighScore(FName FlowSaveId, float Score, const FString& SlotName, int32 UserIndex = 0);

	/** Clears one scenario's history. True when there was one to clear. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Record")
	static bool ResetScenarioRecord(FName FlowSaveId, const FString& SlotName, int32 UserIndex = 0);

	/** Clears the best score but keeps the play count and last outcome. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Record")
	static bool ResetHighScore(FName FlowSaveId, const FString& SlotName, int32 UserIndex = 0);

	/** Empties the whole table. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Record")
	static bool ResetAllScenarioRecords(const FString& SlotName, int32 UserIndex = 0);

	// ------------------------------------------------------- Scenario picker
	//
	// A scenario list for any widget. SimFlow Selector Widget is these three nodes
	// with a list held for you; a Blueprint on some other base class calls them
	// directly and keeps the array itself.

	/**
	 * Expands flow assets into one option per Start node, so a single-entry asset
	 * gives one scenario and an asset holding several gives one each. Leaves Record
	 * empty - pass the result through Apply Scenario Records to fill it.
	 */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Selector")
	static TArray<FSimFlowScenarioOption> BuildScenarioOptions(const TArray<USimFlowAsset*>& Assets);

	/** Stamps each option with its stored history, in one read of the slot. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Selector")
	static void ApplyScenarioRecords(UPARAM(ref) TArray<FSimFlowScenarioOption>& Options, const FString& SlotName, int32 UserIndex = 0);

	/**
	 * Points a flow at the chosen scenario and starts it, ending whatever it was
	 * running first.
	 *
	 * With bAssignSaveId on, the component's Flow Save Id becomes the option's, so
	 * each scenario keeps its own history instead of every pick filing under the
	 * one component. On a client mirror this only forwards a start request: the
	 * asset lives on the server.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Selector")
	static bool StartScenario(USimFlowComponent* Flow, const FSimFlowScenarioOption& Option, bool bAssignSaveId = true);

	// ------------------------------------------------------------ Value makers

	UFUNCTION(BlueprintPure, Category = "SimFlow|Value", meta = (DisplayName = "Make SimFlow Value (Bool)"))	static FSimFlowValue MakeFlowBool(bool Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value", meta = (DisplayName = "Make SimFlow Value (Int)"))	static FSimFlowValue MakeFlowInt(int32 Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value", meta = (DisplayName = "Make SimFlow Value (Float)"))	static FSimFlowValue MakeFlowFloat(float Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value", meta = (DisplayName = "Make SimFlow Value (String)"))	static FSimFlowValue MakeFlowString(const FString& Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value", meta = (DisplayName = "Make SimFlow Value (Name)"))	static FSimFlowValue MakeFlowName(FName Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value", meta = (DisplayName = "Make SimFlow Value (Vector)"))	static FSimFlowValue MakeFlowVector(FVector Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value", meta = (DisplayName = "Make SimFlow Value (Object)"))	static FSimFlowValue MakeFlowObject(UObject* Value);

	UFUNCTION(BlueprintPure, Category = "SimFlow|Value") static bool		FlowValueToBool(const FSimFlowValue& Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value") static int32	FlowValueToInt(const FSimFlowValue& Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value") static float	FlowValueToFloat(const FSimFlowValue& Value);
	UFUNCTION(BlueprintPure, Category = "SimFlow|Value") static FString	FlowValueToString(const FSimFlowValue& Value);

	/** Formats a result enum for UI, e.g. "Timed Out". */
	UFUNCTION(BlueprintPure, Category = "SimFlow")
	static FText ResultToText(ESimFlowResult Result);

	UFUNCTION(BlueprintPure, Category = "SimFlow")
	static FText RunStateToText(ESimFlowRunState State);

	/** Seconds -> "01:23", for countdown widgets. */
	UFUNCTION(BlueprintPure, Category = "SimFlow")
	static FText FormatSeconds(float Seconds);
};
