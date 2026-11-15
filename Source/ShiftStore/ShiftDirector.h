#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftDirector.generated.h"

class ULightComponent;
class UInstancedStaticMeshComponent;
class UTextRenderComponent;
class UStaticMeshComponent;
class APostProcessVolume;

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
	float Alpha() const { return CurrentAlpha; }

	/** Brief flicker of the main lights (night 1 foreshadowing). */
	void Flicker(float Duration);
	bool ValidateState(bool bExpectedShifted, FString& OutError) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FMovedActor { TWeakObjectPtr<AActor> Actor; FVector Base; FVector Offset; };
	struct FMovedInstance { TWeakObjectPtr<UInstancedStaticMeshComponent> Comp; int32 Index; FTransform Base; FVector Offset; };
	struct FLightState { TWeakObjectPtr<ULightComponent> Light; float Normal; float Shift; };

	void Collect();
	void Apply(float A);
	static FVector OffsetFor(const FVector& P);

	TArray<FMovedActor> Moved;
	TArray<FMovedInstance> MovedInstances;
	TArray<FLightState> Lights;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Fixtures;
	TArray<TWeakObjectPtr<UMaterialInterface>> FixtureNormalMats;
	TWeakObjectPtr<UStaticMeshComponent> WelcomeSign;
	TWeakObjectPtr<APostProcessVolume> PostProcess;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> WelcomeMesh;
	UPROPERTY(Transient) TObjectPtr<UStaticMesh> NoticeMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> DarkLens;

	bool bShift = false;
	float CurrentAlpha = 0.f;
	float AppliedAlpha = -1.f;
	float FlickerLeft = 0.f;
};
