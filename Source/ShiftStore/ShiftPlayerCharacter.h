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
	void ReleaseFromGrab(bool bEscaped = true);
	void ResetGrabCamera();
	void ApplyMouseLook(float DeltaX, float DeltaY);
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
	float Stamina = 100.f;         // 0..100 for [Shift] sprint
	bool bIsSprinting = false;
	float SprintRecoverDelay = 0.f;
	float PhaseSnapFovKick = 0.f;  // 0..1 dimensional camera punch on [Q] Phase Shift
	float FootstepTimer = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	FVector ReleaseLocation(const AShiftItem& Item) const;
	void UpdateGrabCamera(float DeltaSeconds);
	bool bGrabCameraActive = false;
	float GrabCameraTime = 0.f;
	FRotator GrabStartRotation = FRotator::ZeroRotator;
	void UpdateFocusAndInteraction(float DeltaSeconds, bool bInteractJustPressed, bool bInteractDown, bool bThrowJustPressed, bool bRefuseJustPressed);
	FName ExtractSkuTag(const AActor* Actor) const;

	bool bPrevInteractDown = false;
	bool bPrevThrowDown = false;
	FVector2D PreviousMenuMouse = FVector2D(-1.f, -1.f);
};
