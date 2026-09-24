// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SimFlowScenarioRecord.h"
#include "SimFlowSelectorWidget.generated.h"

class USimFlowAsset;
class USimFlowComponent;

/**
 * Base class for a scenario picker - the panel a trainee sees before a run starts.
 *
 * Reparent your briefing-table widget Blueprint to this, fill Scenarios with the
 * flow assets on offer, and each one becomes an option carrying its display name,
 * description and history. Wire a button to Select Scenario and the flow starts.
 *
 * Convenience, not a requirement: this holds a list and a target flow so you do not
 * have to. A widget on any other base class calls Build Scenario Options, Apply
 * Scenario Records and Start Scenario on SimFlow Statics and keeps the array itself.
 *
 * The during-the-run counterpart is SimFlow Status Widget.
 */
UCLASS(Abstract, Blueprintable, meta = (DisplayName = "SimFlow Selector Widget"))
class SIMFLOWRUNTIME_API USimFlowSelectorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The scenarios on offer. An asset with several Start nodes becomes one option per entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Selector")
	TArray<TObjectPtr<USimFlowAsset>> Scenarios;

	/** Which flow runs the pick. Leave as None to use the primary flow in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Selector")
	FName FlowSaveId = NAME_None;

	/** Where history is read from. Match the component's Scenario Slot Name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Selector")
	FString ScenarioSlotName = TEXT("SimFlowScenarios");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Selector")
	int32 ScenarioUserIndex = 0;

	/**
	 * Point the component's Flow Save Id at the chosen scenario before starting.
	 *
	 * Leave this on unless the project has its own id scheme. One briefing table
	 * running six scenarios through one component otherwise files all six under
	 * that component's single id, and their histories merge.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Selector")
	bool bAssignSaveIdOnSelect = true;

	/** Rebuild the list from Scenarios and re-read the record slot. Called on construct. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Selector")
	void RefreshOptions();

	/** Re-read history without rebuilding the list. Call after a run to update the panel. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Selector")
	void RefreshRecords();

	UFUNCTION(BlueprintPure, Category = "SimFlow|Selector")
	const TArray<FSimFlowScenarioOption>& GetOptions() const { return Options; }

	UFUNCTION(BlueprintPure, Category = "SimFlow|Selector")
	int32 NumOptions() const { return Options.Num(); }

	UFUNCTION(BlueprintPure, Category = "SimFlow|Selector")
	bool GetOption(int32 Index, FSimFlowScenarioOption& OutOption) const;

	/**
	 * Starts the chosen scenario on the target flow.
	 *
	 * In multiplayer a client cannot hand the server a different asset, so this
	 * forwards a start request and the server runs what it has configured.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Selector")
	bool SelectScenario(int32 Index);

	/** The flow this panel drives. Resolved once and cached. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Selector")
	USimFlowComponent* GetTargetFlow();

	/** Drive a specific flow instead of the one Flow Save Id resolves to. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Selector")
	void SetTargetFlow(USimFlowComponent* Component) { TargetFlow = Component; }

	// ---------------------------------------------------------- Design events

	/** The list changed. Rebuild your buttons here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SimFlow|Selector")
	void OnOptionsRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "SimFlow|Selector")
	void OnScenarioSelected(const FSimFlowScenarioOption& Option);

	virtual void NativeConstruct() override;

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SimFlow|Selector")
	TArray<FSimFlowScenarioOption> Options;

private:
	UPROPERTY(Transient)
	TObjectPtr<USimFlowComponent> TargetFlow = nullptr;
};
