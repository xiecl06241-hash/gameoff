#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShiftTypes.h"
#include "ShiftCustomer.generated.h"

class UTextRenderComponent;
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

	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Speech;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> EyeLight;

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
	void MoveToward(const FVector& Goal, float Speed, float DeltaSeconds);
	void RevealMonster(APawn* Player);
	void PickSearchPoint();
	void ApplyMonsterVisuals();

	int32 PatrolIdx = 0;
	float RepathTimer = 0.f;
};
