// Copyright SimFlow. All Rights Reserved.

#include "SimFlowTasks.h"
#include "SimFlowInstance.h"
#include "SimFlowBlackboard.h"
#include "SimFlowCondition.h"
#include "SimFlowRuntimeModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "SimFlowTasks"

// ---------------------------------------------------------------------- Delay

USimFlowTask_Delay::USimFlowTask_Delay()
{
	DisplayName = LOCTEXT("DelayName", "Delay");
	bAllowRetry = false;
}

void USimFlowTask_Delay::NativeTaskStart()
{
	TargetTime = Duration + (RandomExtra > 0.f ? FMath::FRandRange(0.f, RandomExtra) : 0.f);
	if (TargetTime <= 0.f)
	{
		FinishTask(ESimFlowResult::Succeeded);
	}
}

void USimFlowTask_Delay::NativeTaskTick(float /*DeltaTime*/)
{
	if (ElapsedTime >= TargetTime)
	{
		FinishTask(ESimFlowResult::Succeeded);
	}
}

// ------------------------------------------------------------------------ Log

USimFlowTask_Log::USimFlowTask_Log()
{
	DisplayName = LOCTEXT("LogName", "Log Message");
	bAllowRetry = false;
}

void USimFlowTask_Log::NativeTaskStart()
{
	UE_LOG(LogSimFlow, Log, TEXT("[SimFlow] %s"), *Message);

	if (bPrintToScreen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, ScreenDuration, FColor::Cyan, FString::Printf(TEXT("[SimFlow] %s"), *Message));
	}

	FinishTask(ESimFlowResult::Succeeded);
}

// -------------------------------------------------------------- SetBlackboard

USimFlowTask_SetBlackboard::USimFlowTask_SetBlackboard()
{
	DisplayName = LOCTEXT("SetBlackboardName", "Set Blackboard Value");
	bAllowRetry = false;
}

void USimFlowTask_SetBlackboard::NativeTaskStart()
{
	if (USimFlowBlackboard* Blackboard = GetBlackboard())
	{
		if (bAdd)
		{
			Blackboard->AddToValue(Key, Value);
		}
		else
		{
			Blackboard->SetValue(Key, Value);
		}
	}
	FinishTask(ESimFlowResult::Succeeded);
}

// -------------------------------------------------------------- WaitForEvent

USimFlowTask_WaitForEvent::USimFlowTask_WaitForEvent()
{
	DisplayName = LOCTEXT("WaitForEventName", "Wait For Event");
}

void USimFlowTask_WaitForEvent::NativeTaskStart()
{
	if (!EventTag.IsValid())
	{
		UE_LOG(LogSimFlow, Warning, TEXT("WaitForEvent task has no tag set - finishing immediately."));
		FinishTask(ESimFlowResult::Succeeded);
		return;
	}

	if (bAcceptAlreadyRaised && FlowInstance && FlowInstance->WasEventRaised(EventTag))
	{
		FinishTask(ESimFlowResult::Succeeded);
		return;
	}

	if (FlowInstance)
	{
		FlowInstance->OnEventRaised.AddDynamic(this, &USimFlowTask_WaitForEvent::HandleEvent);
	}
}

void USimFlowTask_WaitForEvent::NativeTaskEnd(ESimFlowResult /*Result*/)
{
	if (FlowInstance)
	{
		FlowInstance->OnEventRaised.RemoveDynamic(this, &USimFlowTask_WaitForEvent::HandleEvent);
	}
}

void USimFlowTask_WaitForEvent::HandleEvent(FGameplayTag Tag, UObject* /*Payload*/)
{
	if (!bIsRunning || bIsPaused)
	{
		return;
	}

	const bool bMatches = bMatchChildTags ? Tag.MatchesTag(EventTag) : (Tag == EventTag);
	if (bMatches)
	{
		FinishTask(ESimFlowResult::Succeeded);
	}
}

// ---------------------------------------------------------- WaitForCondition

USimFlowTask_WaitForCondition::USimFlowTask_WaitForCondition()
{
	DisplayName = LOCTEXT("WaitForConditionName", "Wait For Condition");
}

void USimFlowTask_WaitForCondition::NativeTaskStart()
{
	CheckAccumulator = 0.f;
	HoldAccumulator = 0.f;

	if (!Condition)
	{
		UE_LOG(LogSimFlow, Warning, TEXT("WaitForCondition task has no condition - finishing immediately."));
		FinishTask(ESimFlowResult::Succeeded);
	}
}

void USimFlowTask_WaitForCondition::NativeTaskTick(float DeltaTime)
{
	if (!Condition)
	{
		return;
	}

	CheckAccumulator += DeltaTime;
	if (CheckAccumulator < CheckInterval)
	{
		return;
	}

	const float Slice = CheckAccumulator;
	CheckAccumulator = 0.f;

	if (Condition->Evaluate(FlowInstance))
	{
		HoldAccumulator += Slice;
		if (HoldAccumulator >= RequiredHoldTime)
		{
			FinishTask(ESimFlowResult::Succeeded);
		}
	}
	else
	{
		HoldAccumulator = 0.f;
	}
}

// ------------------------------------------------------------- GoToLocation

USimFlowTask_GoToLocation::USimFlowTask_GoToLocation()
{
	DisplayName = LOCTEXT("GoToLocationName", "Go To Location");
}

FVector USimFlowTask_GoToLocation::GetResolvedTargetLocation() const
{
	FVector Target = TargetLocation;

	if (!TargetFromBlackboardKey.IsNone())
	{
		if (USimFlowBlackboard* Blackboard = GetBlackboard())
		{
			Target = Blackboard->GetVector(TargetFromBlackboardKey, TargetLocation);
			return Target;
		}
	}

	if (bRelativeToFlowOwner)
	{
		if (const AActor* Owner = GetFlowOwner())
		{
			Target = Owner->GetActorTransform().TransformPosition(TargetLocation);
		}
	}

	return Target;
}

void USimFlowTask_GoToLocation::NativeTaskTick(float /*DeltaTime*/)
{
	const APawn* Pawn = GetPlayerPawn();
	if (!Pawn)
	{
		return;
	}

	const FVector Target = GetResolvedTargetLocation();

#if ENABLE_DRAW_DEBUG
	if (bDrawDebugSphere)
	{
		if (const UWorld* World = GetWorld())
		{
			DrawDebugSphere(World, Target, AcceptanceRadius, 16, FColor::Green, false, -1.f, 0, 2.f);
		}
	}
#endif

	FVector Delta = Pawn->GetActorLocation() - Target;
	if (bIgnoreZ)
	{
		Delta.Z = 0.f;
	}

	if (Delta.SizeSquared() <= FMath::Square(AcceptanceRadius))
	{
		FinishTask(ESimFlowResult::Succeeded);
	}
}

// ------------------------------------------------------------------------ Quiz

USimFlowTask_Quiz::USimFlowTask_Quiz()
{
	DisplayName = LOCTEXT("QuizName", "Quiz");
	bAllowRetry = true;
}

void USimFlowTask_Quiz::NativeTaskStart()
{
	OnQuizPresented.Broadcast(this);

	if (FlowInstance)
	{
		FlowInstance->NotifyQuizPresented(this);
	}
}

void USimFlowTask_Quiz::NativeTaskEnd(ESimFlowResult /*Result*/)
{
	// Nothing to unbind - UI listens to OnQuizPresented and calls SubmitAnswer.
}

bool USimFlowTask_Quiz::IsCorrectIndex(int32 OptionIndex) const
{
	return OptionIndex == CorrectOptionIndex || AdditionalCorrectIndices.Contains(OptionIndex);
}

void USimFlowTask_Quiz::SubmitAnswer(int32 OptionIndex)
{
	if (!bIsRunning)
	{
		UE_LOG(LogSimFlow, Warning, TEXT("SubmitAnswer called on a quiz that is not running."));
		return;
	}

	const bool bCorrect = IsCorrectIndex(OptionIndex);

	if (USimFlowBlackboard* Blackboard = GetBlackboard())
	{
		Blackboard->SetInt(SimFlowKeys::LastAnswerIndex, OptionIndex);
		Blackboard->SetBool(SimFlowKeys::LastAnswerCorrect, bCorrect);

		if (!AnswerBlackboardKey.IsNone())
		{
			Blackboard->SetInt(AnswerBlackboardKey, OptionIndex);
		}
		if (!bCorrect && bCountMistakes)
		{
			Blackboard->AddToValue(SimFlowKeys::Mistakes, FSimFlowValue::MakeInt(1));
		}
	}

	OnQuizAnswered.Broadcast(OptionIndex, bCorrect);

	if (bCorrect)
	{
		FinishTask(ESimFlowResult::Succeeded);
	}
	else
	{
		FinishTask(bFailOnWrongAnswer ? ESimFlowResult::Failed : ESimFlowResult::Succeeded);
	}
}

// -------------------------------------------------------------- ParallelGroup

USimFlowTask_ParallelGroup::USimFlowTask_ParallelGroup()
{
	DisplayName = LOCTEXT("ParallelGroupName", "Parallel Group");
}

void USimFlowTask_ParallelGroup::NativeTaskStart()
{
	FinishedCount = 0;
	bAnyChildFailed = false;

	if (Tasks.Num() == 0)
	{
		FinishTask(ESimFlowResult::Succeeded);
		return;
	}

	// Snapshot: children may finish synchronously inside StartTask.
	TArray<TObjectPtr<USimFlowTask>> Snapshot = Tasks;
	for (const TObjectPtr<USimFlowTask>& Child : Snapshot)
	{
		if (!Child)
		{
			FinishedCount++;
			continue;
		}
		Child->InitializeTask(FlowInstance, nullptr);
		Child->OnTaskFinished.AddDynamic(this, &USimFlowTask_ParallelGroup::HandleChildFinished);
		Child->StartTask();

		if (!bIsRunning)
		{
			// A child finished the group already.
			return;
		}
	}
}

void USimFlowTask_ParallelGroup::NativeTaskTick(float DeltaTime)
{
	for (const TObjectPtr<USimFlowTask>& Child : Tasks)
	{
		if (Child && Child->bIsRunning)
		{
			Child->TickTask(DeltaTime);
			if (!bIsRunning)
			{
				return;
			}
		}
	}
}

void USimFlowTask_ParallelGroup::NativeTaskPause()
{
	for (const TObjectPtr<USimFlowTask>& Child : Tasks)
	{
		if (Child)
		{
			Child->PauseTask();
		}
	}
}

void USimFlowTask_ParallelGroup::NativeTaskResume()
{
	for (const TObjectPtr<USimFlowTask>& Child : Tasks)
	{
		if (Child)
		{
			Child->ResumeTask();
		}
	}
}

void USimFlowTask_ParallelGroup::NativeTaskEnd(ESimFlowResult /*Result*/)
{
	for (const TObjectPtr<USimFlowTask>& Child : Tasks)
	{
		if (Child)
		{
			Child->OnTaskFinished.RemoveDynamic(this, &USimFlowTask_ParallelGroup::HandleChildFinished);
			if (Child->bIsRunning)
			{
				Child->AbortTask();
			}
		}
	}
}

void USimFlowTask_ParallelGroup::HandleChildFinished(ESimFlowResult Result)
{
	FinishedCount++;

	if (Result == ESimFlowResult::Failed || Result == ESimFlowResult::TimedOut)
	{
		bAnyChildFailed = true;
	}

	const bool bDone = bWaitForAll ? (FinishedCount >= Tasks.Num()) : true;
	if (bDone && bIsRunning)
	{
		const bool bFailed = bAnyChildFailed && bFailIfAnyChildFails;
		FinishTask(bFailed ? ESimFlowResult::Failed : ESimFlowResult::Succeeded);
	}
}

#undef LOCTEXT_NAMESPACE
