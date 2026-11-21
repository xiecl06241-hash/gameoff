#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftDirector.generated.h"

class ULightComponent;
class UPointLightComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class UStaticMeshComponent;
class APostProcessVolume;
class UExponentialHeightFogComponent;

/**
 * Runtime version of the Normal -> Shift store transformation that used to live in two maps.
 * Uses the same rules as Tools/generate_store.py: the middle gondolas slide +-30 cm (with their
 * stock, tags and aisle label), main lights drop out, cold cases / exit / emergency lights stay.
 */
UCLASS()
class SHIFTSTORE_API AShiftDirector : public AActor
{
	GENERATED_BODY()

public:
	AShiftDirector();

	void SetShifted(bool bInShift);
	bool IsShifted() const { return bShift; }
	/** True when the store is shifted and the emergency breaker is not overriding it back to normal. */
	bool IsEffectiveShifted() const { return bShift && BreakerOverrideLeft <= 0.f; }
	float Alpha() const { return CurrentAlpha; }

	/** Temporarily force normal lighting & shelf positions during a Shift (returns false if on cooldown or not shifted). */
	bool TriggerBreakerOverride(float Duration = 10.f);
	bool IsBreakerOverridden() const { return BreakerOverrideLeft > 0.f; }
	float BreakerOverrideRemaining() const { return BreakerOverrideLeft; }
	float BreakerCooldownRemaining() const { return BreakerCooldownLeft; }

	/** Toggle a localized pre-shift anomaly (0 = warped welcome sign, 1 = half-shifted front gondola). */
	void SetMicroShift(int32 AnomalyIndex, bool bActive);
	void ClearMicroShifts();

	/** Brief flicker of the main lights (night 1 foreshadowing). */
	void Flicker(float Duration);
	bool ValidateState(bool bExpectedShifted, FString& OutError) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FMovedActor { TWeakObjectPtr<AActor> Actor; FVector Base; FVector Offset; };
	struct FMovedInstance { TWeakObjectPtr<UInstancedStaticMeshComponent> Comp; int32 Index; FTransform Base; FVector Offset; };
	struct FLightState { TWeakObjectPtr<ULightComponent> Light; float Normal; float Shift; FLinearColor NormalColor; FLinearColor ShiftColor; };
	struct FAccentLightState { TWeakObjectPtr<UPointLightComponent> Light; float Normal; float Shift; FLinearColor NormalColor; FLinearColor ShiftColor; };

	void Collect();
	void Apply(float A);
	static FVector OffsetFor(const FVector& P);

	TArray<FMovedActor> Moved;
	TArray<FMovedInstance> MovedInstances;
	TArray<FLightState> Lights;
	TArray<FAccentLightState> AccentLights;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Fixtures;
	TArray<TWeakObjectPtr<UMaterialInterface>> FixtureNormalMats;
	TWeakObjectPtr<UStaticMeshComponent> WelcomeSign;
	TWeakObjectPtr<APostProcessVolume> PostProcess;
	TWeakObjectPtr<UExponentialHeightFogComponent> HorrorFog;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> WelcomeMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> NoticeMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> DarkLens;

	bool bShift = false;
	float CurrentAlpha = 0.f;
	float AppliedAlpha = -1.f;
	float FlickerLeft = 0.f;
	float BreakerOverrideLeft = 0.f;
	float BreakerCooldownLeft = 0.f;
	bool bMicroSignWarped = false;
	bool bMicroGondolaShifted = false;
};
