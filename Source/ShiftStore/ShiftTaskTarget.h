#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftTypes.h"
#include "ShiftTaskTarget.generated.h"

class USphereComponent;
class UTextRenderComponent;
class UStaticMeshComponent;

/** A chore location: hold E (optionally while carrying a carton / the mop) to complete it. */
UCLASS()
class SHIFTSTORE_API AShiftTaskTarget : public AActor
{
	GENERATED_BODY()

public:
	AShiftTaskTarget();

	void Setup(EShiftTaskType InType, const FString& InLabel, float TimeLimit);

	/** Seconds of holding E required. */
	float HoldDuration() const;
	/** The item kind that must be carried, if any. */
	bool NeedsItem(EShiftItemKind& OutKind) const;

	bool IsOverdue() const { return Deadline > 0.f && TimeLeft <= 0.f; }
	bool IsPermanent() const { return Type == EShiftTaskType::CartonSource || Type == EShiftTaskType::BreakerBox; }
	int32 AnomalyIndex = INDEX_NONE;

	/** Reset the countdown (tasks are re-assigned after each customer). */
	void RestartTimer(float TimeLimit);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Hitbox;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Marker;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SpillMesh;

	EShiftTaskType Type = EShiftTaskType::Count;
	FString Label;
	float Deadline = 0.f;   // 0 = no countdown
	float TimeLeft = 0.f;
	float Progress = 0.f;   // 0..1 hold progress

protected:
	virtual void Tick(float DeltaSeconds) override;
};
