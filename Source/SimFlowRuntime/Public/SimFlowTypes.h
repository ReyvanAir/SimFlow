// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "SimFlowTypes.generated.h"

/** How a flow is kicked off by a USimFlowComponent. */
UENUM(BlueprintType)
enum class ESimFlowStartMode : uint8
{
	/** Nothing happens until StartFlow() is called from Blueprint / C++. */
	Manual				UMETA(DisplayName = "Manual"),
	/** Starts on BeginPlay. */
	AutoOnBeginPlay		UMETA(DisplayName = "Auto - On Begin Play"),
	/** Starts on the first tick after BeginPlay (safer: all actors have begun play). */
	AutoOnFirstTick		UMETA(DisplayName = "Auto - On First Tick"),
	/** Starts after AutoStartDelay seconds. */
	AutoAfterDelay		UMETA(DisplayName = "Auto - After Delay")
};

/** Overall state of a running flow. */
UENUM(BlueprintType)
enum class ESimFlowRunState : uint8
{
	NotStarted	UMETA(DisplayName = "Not Started"),
	Running		UMETA(DisplayName = "Running"),
	Paused		UMETA(DisplayName = "Paused"),
	Completed	UMETA(DisplayName = "Completed"),
	Failed		UMETA(DisplayName = "Failed"),
	Aborted		UMETA(DisplayName = "Aborted")
};

/** Result of a single task / node. */
UENUM(BlueprintType)
enum class ESimFlowResult : uint8
{
	Succeeded	UMETA(DisplayName = "Succeeded"),
	Failed		UMETA(DisplayName = "Failed"),
	Skipped		UMETA(DisplayName = "Skipped"),
	TimedOut	UMETA(DisplayName = "Timed Out"),
	Aborted		UMETA(DisplayName = "Aborted")
};

/** Result the Finish node reports for the whole flow. */
UENUM(BlueprintType)
enum class ESimFlowFinishMode : uint8
{
	Complete	UMETA(DisplayName = "Complete (Success)"),
	Fail		UMETA(DisplayName = "Fail"),
	Abort		UMETA(DisplayName = "Abort")
};

/** Value types the blackboard can hold. */
UENUM(BlueprintType)
enum class ESimFlowValueType : uint8
{
	None	UMETA(DisplayName = "None"),
	Bool	UMETA(DisplayName = "Bool"),
	Int		UMETA(DisplayName = "Int"),
	Float	UMETA(DisplayName = "Float"),
	String	UMETA(DisplayName = "String"),
	Name	UMETA(DisplayName = "Name"),
	Vector	UMETA(DisplayName = "Vector"),
	Object	UMETA(DisplayName = "Object")
};

/** Comparison operators used by conditions. */
UENUM(BlueprintType)
enum class ESimFlowCompareOp : uint8
{
	Equal			UMETA(DisplayName = "=="),
	NotEqual		UMETA(DisplayName = "!="),
	Less			UMETA(DisplayName = "<"),
	LessOrEqual		UMETA(DisplayName = "<="),
	Greater			UMETA(DisplayName = ">"),
	GreaterOrEqual	UMETA(DisplayName = ">=")
};

/** How a Join node decides it is satisfied. */
UENUM(BlueprintType)
enum class ESimFlowJoinMode : uint8
{
	/** Fires once every connected input has been triggered (classic "wait for all"). */
	WaitForAll	UMETA(DisplayName = "Wait For All"),
	/** Fires as soon as any input is triggered, then ignores the rest (race / timeout pattern). */
	WaitForAny	UMETA(DisplayName = "Wait For Any (Race)")
};

/** How LoadFlowState restores a flow. */
UENUM(BlueprintType)
enum class ESimFlowLoadMode : uint8
{
	/** Restore the exact set of nodes that were active, including their elapsed times. */
	ExactState			UMETA(DisplayName = "Exact State"),
	/** Restore the blackboard, then re-run the flow from the last checkpoint that was passed. */
	FromLastCheckpoint	UMETA(DisplayName = "From Last Checkpoint")
};

/**
 * A small tagged union used for blackboard values, condition operands and
 * save data. Kept as a plain USTRUCT so it survives SaveGame serialisation.
 */
USTRUCT(BlueprintType)
struct SIMFLOWRUNTIME_API FSimFlowValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value")
	ESimFlowValueType Type = ESimFlowValueType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value", meta = (EditCondition = "Type == ESimFlowValueType::Bool", EditConditionHides))
	bool BoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value", meta = (EditCondition = "Type == ESimFlowValueType::Int", EditConditionHides))
	int32 IntValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value", meta = (EditCondition = "Type == ESimFlowValueType::Float", EditConditionHides))
	float FloatValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value", meta = (EditCondition = "Type == ESimFlowValueType::String", EditConditionHides))
	FString StringValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value", meta = (EditCondition = "Type == ESimFlowValueType::Name", EditConditionHides))
	FName NameValue = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value", meta = (EditCondition = "Type == ESimFlowValueType::Vector", EditConditionHides))
	FVector VectorValue = FVector::ZeroVector;

	/** Object references are runtime only - they are cleared when a flow is saved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow|Value", meta = (EditCondition = "Type == ESimFlowValueType::Object", EditConditionHides))
	TObjectPtr<UObject> ObjectValue = nullptr;

	FSimFlowValue() = default;

	static FSimFlowValue MakeBool(bool In);
	static FSimFlowValue MakeInt(int32 In);
	static FSimFlowValue MakeFloat(float In);
	static FSimFlowValue MakeString(const FString& In);
	static FSimFlowValue MakeName(FName In);
	static FSimFlowValue MakeVector(const FVector& In);
	static FSimFlowValue MakeObject(UObject* In);

	bool IsSet() const { return Type != ESimFlowValueType::None; }

	/** Best-effort numeric interpretation (bool -> 0/1, string -> Atof). */
	double AsNumber() const;
	bool AsBool() const;
	FString AsString() const;

	/** Adds Other to this value where that makes sense (numbers, strings, vectors). */
	void Add(const FSimFlowValue& Other);

	bool Compare(ESimFlowCompareOp Op, const FSimFlowValue& Other) const;

	FString ToDisplayString() const;

	/** Drops any hard object reference. Called before serialising into a SaveGame. */
	void StripObjectReferences();

	bool operator==(const FSimFlowValue& Other) const;
	bool operator!=(const FSimFlowValue& Other) const { return !(*this == Other); }
};

/** A single named blackboard entry, used for both runtime and save data. */
USTRUCT(BlueprintType)
struct SIMFLOWRUNTIME_API FSimFlowBlackboardEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimFlow")
	FSimFlowValue Value;

	FSimFlowBlackboardEntry() = default;
	FSimFlowBlackboardEntry(FName InKey, const FSimFlowValue& InValue) : Key(InKey), Value(InValue) {}
};

/** Identifies one output pin link target inside a flow asset. */
USTRUCT()
struct SIMFLOWRUNTIME_API FSimFlowPinLink
{
	GENERATED_BODY()

	/** Guid of the node this link points at. */
	UPROPERTY()
	FGuid NodeGuid;

	/** Name of the input pin on that node. */
	UPROPERTY()
	FName PinName = NAME_None;

	FSimFlowPinLink() = default;
	FSimFlowPinLink(const FGuid& InGuid, FName InPin) : NodeGuid(InGuid), PinName(InPin) {}

	bool IsValid() const { return NodeGuid.IsValid(); }

	bool operator==(const FSimFlowPinLink& Other) const
	{
		return NodeGuid == Other.NodeGuid && PinName == Other.PinName;
	}
};

/** One output pin on a flow node, plus everything it is wired to. */
USTRUCT()
struct SIMFLOWRUNTIME_API FSimFlowOutputPin
{
	GENERATED_BODY()

	UPROPERTY()
	FName PinName = NAME_None;

	/** Optional friendlier label shown in the graph editor. */
	UPROPERTY()
	FString DisplayName;

	UPROPERTY()
	TArray<FSimFlowPinLink> Links;

	FSimFlowOutputPin() = default;
	explicit FSimFlowOutputPin(FName InName, const FString& InDisplay = FString())
		: PinName(InName), DisplayName(InDisplay) {}
};

/** One input pin on a flow node. */
USTRUCT()
struct SIMFLOWRUNTIME_API FSimFlowInputPin
{
	GENERATED_BODY()

	UPROPERTY()
	FName PinName = NAME_None;

	UPROPERTY()
	FString DisplayName;

	FSimFlowInputPin() = default;
	explicit FSimFlowInputPin(FName InName, const FString& InDisplay = FString())
		: PinName(InName), DisplayName(InDisplay) {}
};

/** Well known pin names, so graph code and node code cannot drift apart. */
namespace SimFlowPins
{
	SIMFLOWRUNTIME_API extern const FName In;
	SIMFLOWRUNTIME_API extern const FName Out;
	SIMFLOWRUNTIME_API extern const FName Completed;
	SIMFLOWRUNTIME_API extern const FName Failed;
	SIMFLOWRUNTIME_API extern const FName Skipped;
	SIMFLOWRUNTIME_API extern const FName TimedOut;
	SIMFLOWRUNTIME_API extern const FName Default;
	SIMFLOWRUNTIME_API extern const FName LoopBody;
}

/** Well known blackboard keys the built-in tasks use. */
namespace SimFlowKeys
{
	SIMFLOWRUNTIME_API extern const FName Score;
	SIMFLOWRUNTIME_API extern const FName Mistakes;
	SIMFLOWRUNTIME_API extern const FName LastResult;
	SIMFLOWRUNTIME_API extern const FName LastAnswerIndex;
	SIMFLOWRUNTIME_API extern const FName LastAnswerCorrect;
}
