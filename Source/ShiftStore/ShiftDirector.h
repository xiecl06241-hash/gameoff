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
	bool IsShifted() const { return bShift || bPlayerPhaseShift; }
	/** True when a monster has forced a full Store Shift (distinct from player's [Q] Otherworld peek). */
	bool IsMonsterShifted() const { return bShift; }
	/** True when the store is effectively in the Otherworld (either monster Shift or player [Q] phase shift, and not overridden). */
	bool IsEffectiveShifted() const { return (bShift && BreakerOverrideLeft <= 0.f && PhaseBurstLeft <= 0.f) || (!bShift && bPlayerPhaseShift); }
	float Alpha() const { return CurrentAlpha; }

	/** Player-triggered [Q] Phase Shift: toggles Otherworld during normal play, or forces a 2.5s Normal snap during a monster chase. */
	bool TogglePlayerPhaseShift();
	bool IsPlayerPhaseShifted() const { return !bShift && bPlayerPhaseShift; }
	bool IsPhaseBurstActive() const { return bShift && PhaseBurstLeft > 0.f; }
	float PlayerPhaseCooldownRemaining() const { return PlayerPhaseCooldown; }
	void ClearPlayerPhaseShift();

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
	bool bPlayerPhaseShift = false;
	bool bSnappyTransition = false;
	float CurrentAlpha = 0.f;
	float AppliedAlpha = -1.f;
	float FlickerLeft = 0.f;
	float BreakerOverrideLeft = 0.f;
	float BreakerCooldownLeft = 0.f;
	float PlayerPhaseCooldown = 0.f;
	float PhaseBurstLeft = 0.f;
	bool bMicroSignWarped = false;
	bool bMicroGondolaShifted = false;
};
