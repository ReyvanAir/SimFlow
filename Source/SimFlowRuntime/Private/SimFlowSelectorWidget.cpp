// Copyright SimFlow. All Rights Reserved.

#include "SimFlowSelectorWidget.h"
#include "SimFlowAsset.h"
#include "SimFlowComponent.h"
#include "SimFlowStatics.h"
#include "SimFlowRuntimeModule.h"

void USimFlowSelectorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshOptions();
}

void USimFlowSelectorWidget::RefreshOptions()
{
	TArray<USimFlowAsset*> Assets;
	Assets.Reserve(Scenarios.Num());
	for (const TObjectPtr<USimFlowAsset>& Asset : Scenarios)
	{
		Assets.Add(Asset);
	}

	Options = USimFlowStatics::BuildScenarioOptions(Assets);
	USimFlowStatics::ApplyScenarioRecords(Options, ScenarioSlotName, ScenarioUserIndex);
	OnOptionsRefreshed();
}

void USimFlowSelectorWidget::RefreshRecords()
{
	USimFlowStatics::ApplyScenarioRecords(Options, ScenarioSlotName, ScenarioUserIndex);
	OnOptionsRefreshed();
}

bool USimFlowSelectorWidget::GetOption(int32 Index, FSimFlowScenarioOption& OutOption) const
{
	if (!Options.IsValidIndex(Index))
	{
		return false;
	}
	OutOption = Options[Index];
	return true;
}

USimFlowComponent* USimFlowSelectorWidget::GetTargetFlow()
{
	// Cached, because Select Scenario rewrites the component's Flow Save Id and a
	// later lookup by the old id would miss.
	if (!TargetFlow)
	{
		TargetFlow = FlowSaveId.IsNone()
			? USimFlowStatics::GetPrimaryFlow(this)
			: USimFlowStatics::FindFlowById(this, FlowSaveId);
	}
	return TargetFlow;
}

bool USimFlowSelectorWidget::SelectScenario(int32 Index)
{
	if (!Options.IsValidIndex(Index))
	{
		UE_LOG(LogSimFlow, Warning, TEXT("Select Scenario: no option at index %d of %d."), Index, Options.Num());
		return false;
	}

	const FSimFlowScenarioOption& Option = Options[Index];

	if (!USimFlowStatics::StartScenario(GetTargetFlow(), Option, bAssignSaveIdOnSelect))
	{
		return false;
	}

	OnScenarioSelected(Option);
	return true;
}
