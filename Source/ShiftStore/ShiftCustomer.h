#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShiftTypes.h"
#include "ShiftCustomer.generated.h"

class UTextRenderComponent;
class UStaticMeshComponent;
class UPoseableMeshComponent;
class UPointLightComponent;

UENUM()
enum class ECustomerState : uint8
{
	Entering,
	Browsing,
	WaitingForHelp,
	WalkingToCheckout,
	WaitingAtCheckout,
	MonsterStalking,   // revealed unseen -> waits to ambush for extra sanity damage
	MonsterHunting,    // searching / chasing the player while demanding MonsterSku
	MonsterGrabbing,   // holding the player until they struggle free or stun
	MonsterStunned,
	Leaving
};

/**
 * Serves as both normal customer and revealed monster (as designed: you cannot tell
 * them apart beforehand; a monster may reveal while you fetch the item it asked for).
 */
UCLASS()
class SHIFTSTORE_API AShiftCustomer : public ACharacter
{
	GENERATED_BODY()

public:
	AShiftCustomer();

	void Init(const FShiftCustomerSpec& InSpec);

	bool IsMonsterActive() const
	{
		return State == ECustomerState::MonsterStalking
			|| State == ECustomerState::MonsterHunting
			|| State == ECustomerState::MonsterGrabbing
			|| State == ECustomerState::MonsterStunned;
	}

	bool CanSeePlayer(APawn* Player, float MaxDist = 1100.f, float CosHalfFOV = 0.25f) const;
	bool PlayerSeesMe(APawn* Player) const;

	void Stun(float Seconds);
	void OnReleasedGrab();
	void SatisfyAndLeave();
	void BeginLeaving();
	void RevealMonster(APawn* Player);
	void AttractToNoise(const FVector& NoiseLocation);
	void SetOtherworldPreview(bool bShowTrueForm);
	FVector GrabFocusLocation() const;
	bool IsBlockedByPlayer() const { return bBlockedByPlayer; }
	int32 CourtesyLineCount() const { return CourtesyCount; }

	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Speech;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> EyeLight;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> AnomalyMaskLeftEye;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> AnomalyMaskRightEye;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPoseableMeshComponent> MonsterPose;
	float ChaseElapsedSeconds = 0.f;
	bool bAutomatedTest = false;
	float ChaseFlickerTime = 0.f;

	FShiftCustomerSpec Spec;
	ECustomerState State = ECustomerState::Entering;
	FVector TargetPos = FVector::ZeroVector;
	float StateTimer = 0.f;
	float StunTimer = 0.f;
	bool bWitnessedReveal = true;
	bool bSeeingPlayer = false;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool UpdatePlayerBlock(float DeltaSeconds, const FVector& Direction);
	bool bBlockedByPlayer = false;
	float BlockedSeconds = 0.f;
	float ClearSeconds = 0.f;
	float CourtesyTime = 0.f;
	double LastCourtesyTime = -100.;
	int32 CourtesyCount = 0;
	FText SavedSpeech;
	void MoveToward(const FVector& Goal, float Speed, float DeltaSeconds);
	void FollowStoreRoute(const FVector& Goal, float DeltaSeconds);
	void MonsterMoveToward(const FVector& Goal, float DeltaSeconds);
	bool MonsterPathClear(const FVector& From, const FVector& Dir, float Distance) const;
	void PickSearchPoint();
	void ApplyMonsterVisuals();

	int32 PatrolIdx = 0;
	float RepathTimer = 0.f;
	float TellTimer = 3.2f;
	float StareTriggerTime = 0.f;
	float OtherworldPeekTime = 0.f;
	bool bOtherworldPreviewActive = false;
	float RevealThreshold = 13.5f;
	TArray<FVector> StoreRoute;
	FVector StoreRouteGoal = FVector::ZeroVector;
	FVector PreviousRouteLocation = FVector::ZeroVector;
	int32 StoreRouteIndex = 0;
	float StoreRouteRetry = 0.f;
	float RouteStalledTime = 0.f;
	bool bHasStoreRouteGoal = false;
	float StoreRouteGoalTolerance = 1.f;
	float MonsterReplanCooldown = 0.f;
	float MonsterStuckTime = 0.f;
	FVector MonsterLastPosition = FVector::ZeroVector;
	bool bMonsterSteerRight = false;
};
