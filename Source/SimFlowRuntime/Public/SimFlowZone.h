// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SimFlowIdentity.h"
#include "SimFlowZone.generated.h"

class UBoxComponent;
class ASimFlowZone;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSimFlowZoneActorSignature, ASimFlowZone*, Zone, AActor*, Actor);

/**
 * A named volume a flow can ask questions about: "what is in the parts bin?".
 *
 * Drop one in the level, size the box, give it a ZoneId or some ZoneTags so a
 * task can find it. It tracks what is inside, and - because a trainee holding an
 * object over the bin has not put it down yet - it also tracks whether each
 * object has actually settled.
 */
UCLASS(Blueprintable, ClassGroup = SimFlow, meta = (DisplayName = "SimFlow Zone"))
class SIMFLOWRUNTIME_API ASimFlowZone : public AActor
{
	GENERATED_BODY()

public:
	ASimFlowZone();

	/** The volume itself. Resize it in the level, not in the Blueprint defaults. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TObjectPtr<UBoxComponent> Box;

	/**
	 * Names the zone, exactly the way items are named. Set its IdentityTags to
	 * something like Zone.PartsBin and a task's FSimFlowActorQuery finds it by tag -
	 * one identity mechanism for the whole plugin rather than a second one for zones.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TObjectPtr<USimFlowIdentityComponent> Identity;

	/**
	 * Only actors passing this are tracked at all. Leave it empty to track anything
	 * carrying a SimFlow Identity component, which is usually what you want - the
	 * zone stays neutral about right and wrong, and the task does the judging.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	FSimFlowActorQuery TrackFilter;

	/** When true (the default) untagged actors such as the player pawn are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	bool bRequireIdentityComponent = true;

	/** An object still attached to something (a VR hand) is not considered placed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone|Settling")
	bool bRequireDetached = true;

	/** How long an object must sit still inside the zone before it counts as placed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone|Settling", meta = (ClampMin = "0.0", Units = "s"))
	float SettleTime = 0.35f;

	/** Simulating objects must also drop below this speed. 0 skips the check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone|Settling", meta = (ClampMin = "0.0"))
	float SettleSpeedThreshold = 20.f;

	/**
	 * Also raise SimFlow.Event.Placed / SimFlow.Event.Removed on every flow, with the
	 * actor as the payload, so a plain Wait For Event task can use this zone too.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone|Events")
	bool bBroadcastFlowEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone|Debug")
	bool bDrawDebug = false;

	// ------------------------------------------------------------- Delegates

	/** Fires the moment an actor overlaps, before it has settled. */
	UPROPERTY(BlueprintAssignable, Category = "SimFlow|Zone")
	FSimFlowZoneActorSignature OnActorEntered;

	UPROPERTY(BlueprintAssignable, Category = "SimFlow|Zone")
	FSimFlowZoneActorSignature OnActorExited;

	/** Fires once the actor is genuinely put down. This is the one tasks listen to. */
	UPROPERTY(BlueprintAssignable, Category = "SimFlow|Zone")
	FSimFlowZoneActorSignature OnActorSettled;

	// ------------------------------------------------------------ Queries

	/** Everything overlapping, settled or not. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Zone")
	TArray<AActor*> GetContainedActors() const;

	/** Everything that has been put down and left alone. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Zone")
	TArray<AActor*> GetSettledActors() const;

	UFUNCTION(BlueprintPure, Category = "SimFlow|Zone")
	bool ContainsActor(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "SimFlow|Zone")
	bool IsActorSettled(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "SimFlow|Zone")
	FText GetDisplayNameText() const;

	// ------------------------------------------------------------- AActor

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaTime) override;

protected:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

private:
	/** Per actor settling bookkeeping. */
	struct FTrackedActor
	{
		TWeakObjectPtr<AActor> Actor;
		float StillTime = 0.f;
		bool bSettled = false;
	};

	bool ShouldTrack(const AActor* Actor) const;
	bool IsActorAtRest(const AActor* Actor) const;
	int32 IndexOf(const AActor* Actor) const;

	TArray<FTrackedActor> Tracked;
};
