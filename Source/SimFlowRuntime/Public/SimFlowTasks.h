// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SimFlowTask.h"
#include "GameplayTagContainer.h"
#include "SimFlowTasks.generated.h"

class USimFlowCondition;

/** Waits a fixed number of seconds. Respects pause. */
UCLASS(DisplayName = "Delay", meta = (ToolTip = "Waits for a number of seconds, then succeeds."))
class SIMFLOWRUNTIME_API USimFlowTask_Delay : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_Delay();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delay", meta = (ClampMin = "0.0", Units = "s"))
	float Duration = 1.f;

	/** Randomly adds up to this many seconds on top of Duration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delay", meta = (ClampMin = "0.0", Units = "s"), AdvancedDisplay)
	float RandomExtra = 0.f;

	virtual void NativeTaskStart() override;
	virtual void NativeTaskTick(float DeltaTime) override;

private:
	UPROPERTY(Transient)
	float TargetTime = 0.f;
};

/** Prints a message. Great for bringing a flow up before real tasks exist. */
UCLASS(DisplayName = "Log Message")
class SIMFLOWRUNTIME_API USimFlowTask_Log : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_Log();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FString Message = TEXT("SimFlow");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	bool bPrintToScreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log", meta = (ClampMin = "0.0", Units = "s"))
	float ScreenDuration = 3.f;

	virtual void NativeTaskStart() override;
};

/** Writes (or adds to) a blackboard key, then finishes immediately. */
UCLASS(DisplayName = "Set Blackboard Value")
class SIMFLOWRUNTIME_API USimFlowTask_SetBlackboard : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_SetBlackboard();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FSimFlowValue Value;

	/** When true the value is added to whatever is already stored instead of replacing it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	bool bAdd = false;

	virtual void NativeTaskStart() override;
};

/**
 * Blocks until an event tag is raised on the flow.
 * Raise it with USimFlowComponent::SendEvent or USimFlowStatics::SendFlowEvent.
 */
UCLASS(DisplayName = "Wait For Event")
class SIMFLOWRUNTIME_API USimFlowTask_WaitForEvent : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_WaitForEvent();

	/** The tag this task is listening for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	FGameplayTag EventTag;

	/** Also accept child tags, e.g. listening for Sim.Grab accepts Sim.Grab.Extinguisher. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event")
	bool bMatchChildTags = true;

	/** If the tag was already raised earlier in this run, finish straight away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Event", AdvancedDisplay)
	bool bAcceptAlreadyRaised = false;

	virtual void NativeTaskStart() override;
	virtual void NativeTaskEnd(ESimFlowResult Result) override;

private:
	UFUNCTION()
	void HandleEvent(FGameplayTag Tag, UObject* Payload);
};

/**
 * Blocks until a condition becomes true. Useful for "player is holding the drill",
 * "valve rotation > 90 degrees" and similar continuous checks in a VR sim.
 */
UCLASS(DisplayName = "Wait For Condition")
class SIMFLOWRUNTIME_API USimFlowTask_WaitForCondition : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_WaitForCondition();

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Condition")
	TObjectPtr<USimFlowCondition> Condition = nullptr;

	/** Seconds between evaluations. 0 evaluates every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = (ClampMin = "0.0", Units = "s"))
	float CheckInterval = 0.1f;

	/** The condition must hold for this long before the task succeeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = (ClampMin = "0.0", Units = "s"))
	float RequiredHoldTime = 0.f;

	virtual void NativeTaskStart() override;
	virtual void NativeTaskTick(float DeltaTime) override;

private:
	UPROPERTY(Transient) float CheckAccumulator = 0.f;
	UPROPERTY(Transient) float HoldAccumulator = 0.f;
};

/** Blocks until the player pawn reaches a location. The bread and butter of VR tutorials. */
UCLASS(DisplayName = "Go To Location")
class SIMFLOWRUNTIME_API USimFlowTask_GoToLocation : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_GoToLocation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", meta = (MakeEditWidget = "true"))
	FVector TargetLocation = FVector::ZeroVector;

	/** When set, the target is read from this blackboard key instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", AdvancedDisplay)
	FName TargetFromBlackboardKey = NAME_None;

	/** Treat TargetLocation as relative to the flow owner actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	bool bRelativeToFlowOwner = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", meta = (ClampMin = "1.0", Units = "cm"))
	float AcceptanceRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
	bool bIgnoreZ = true;

	/** Draws a debug sphere at the target while the task runs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location", AdvancedDisplay)
	bool bDrawDebugSphere = false;

	UFUNCTION(BlueprintPure, Category = "SimFlow|Task")
	FVector GetResolvedTargetLocation() const;

	virtual void NativeTaskTick(float DeltaTime) override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimFlowQuizPresented, USimFlowTask_Quiz*, Quiz);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSimFlowQuizAnswered, int32, AnswerIndex, bool, bCorrect);

/**
 * A multiple choice question. Bind OnQuizPresented from your VR widget, show the
 * options, then call SubmitAnswer. Wrong answers can retry, fail, or just continue -
 * wire the Failed pin of the Task node to whatever remediation branch you want.
 */
UCLASS(DisplayName = "Quiz")
class SIMFLOWRUNTIME_API USimFlowTask_Quiz : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_Quiz();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz", meta = (MultiLine = "true"))
	FText Question;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	TArray<FText> Options;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz", meta = (ClampMin = "0"))
	int32 CorrectOptionIndex = 0;

	/** Optional: also treat these indices as correct (multi-answer questions). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz", AdvancedDisplay)
	TArray<int32> AdditionalCorrectIndices;

	/** Blackboard key that receives the submitted index. Leave None to skip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz", AdvancedDisplay)
	FName AnswerBlackboardKey = NAME_None;

	/** When true a wrong answer immediately fails the task (drives the Failed pin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	bool bFailOnWrongAnswer = true;

	/** Increments the "Mistakes" blackboard key on a wrong answer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
	bool bCountMistakes = true;

	UPROPERTY(BlueprintAssignable, Category = "SimFlow|Quiz")
	FSimFlowQuizPresented OnQuizPresented;

	UPROPERTY(BlueprintAssignable, Category = "SimFlow|Quiz")
	FSimFlowQuizAnswered OnQuizAnswered;

	/** Call this from your answer buttons. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Quiz")
	void SubmitAnswer(int32 OptionIndex);

	UFUNCTION(BlueprintPure, Category = "SimFlow|Quiz")
	bool IsCorrectIndex(int32 OptionIndex) const;

	virtual void NativeTaskStart() override;
	virtual void NativeTaskEnd(ESimFlowResult Result) override;
};

/**
 * Runs several child tasks at once inside a single node. Handy when you want a
 * small parallel group without cluttering the graph with Parallel/Join nodes.
 */
UCLASS(DisplayName = "Parallel Group")
class SIMFLOWRUNTIME_API USimFlowTask_ParallelGroup : public USimFlowTask
{
	GENERATED_BODY()

public:
	USimFlowTask_ParallelGroup();

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Group")
	TArray<TObjectPtr<USimFlowTask>> Tasks;

	/** When false the group finishes as soon as the first child finishes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
	bool bWaitForAll = true;

	/** Any child failing fails the whole group. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
	bool bFailIfAnyChildFails = true;

	virtual void NativeTaskStart() override;
	virtual void NativeTaskTick(float DeltaTime) override;
	virtual void NativeTaskPause() override;
	virtual void NativeTaskResume() override;
	virtual void NativeTaskEnd(ESimFlowResult Result) override;

private:
	UFUNCTION()
	void HandleChildFinished(ESimFlowResult Result);

	UPROPERTY(Transient) int32 FinishedCount = 0;
	UPROPERTY(Transient) bool bAnyChildFailed = false;
};
