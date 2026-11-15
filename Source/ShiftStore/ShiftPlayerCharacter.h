#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShiftTypes.h"
#include "ShiftPlayerCharacter.generated.h"

class UCameraComponent;
class AShiftItem;
class AShiftTaskTarget;
class AShiftCustomer;

UCLASS()
class SHIFTSTORE_API AShiftPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AShiftPlayerCharacter();

	void OnGrabbedByMonster(AShiftCustomer* Monster, bool bUnseenAmbush);
	void ReleaseFromGrab();
	bool IsGrabbed() const { return GrabbingMonster.IsValid(); }

	void PickupItem(AShiftItem* Item);
	void DropHeldItem();
	void ConsumeHeldItem();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FirstPersonCamera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> HoldMount;

	TWeakObjectPtr<AShiftItem> HeldItem;
	TWeakObjectPtr<AShiftTaskTarget> FocusedTask;
	TWeakObjectPtr<AShiftCustomer> GrabbingMonster;

	FString CrosshairPrompt;
	float HoldProgress = 0.f;      // 0..1 for current chore
	float StruggleProgress = 0.f;  // 0..1 when grabbed

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void UpdateFocusAndInteraction(float DeltaSeconds, bool bInteractJustPressed, bool bInteractDown, bool bThrowJustPressed);
	FName ExtractSkuTag(const AActor* Actor) const;

	bool bPrevInteractDown = false;
	bool bPrevThrowDown = false;
};
