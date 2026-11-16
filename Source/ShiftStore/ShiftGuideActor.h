#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftGuidance.h"
#include "ShiftTypes.h"
#include "ShiftGuideActor.generated.h"

UCLASS()
class SHIFTSTORE_API AShiftGuideActor : public AActor
{
	GENERATED_BODY()
public:
	AShiftGuideActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	FShiftTaskGuidance Current;
	EShiftRouteStatus Status = EShiftRouteStatus::Unreachable;
	TArray<FVector> Route;
	bool bActive = false;
	int32 ReplanCount = 0;
	float FloorRouteLength = 0.f;
	FVector MarkerLocation() const;
	bool IsReplanning() const { return bNeedsReplan; }
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UInstancedStaticMeshComponent> Arrows;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UInstancedStaticMeshComponent> Highlight;
private:
	UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> HighlightMaterial;
	void BuildGeometry(class AShiftPlayerCharacter* Clerk);
	void AddStroke(class UInstancedStaticMeshComponent* Component, const FVector& A, const FVector& B, float Width);
	double LastPlanTime = -10.;
	double LastGeometryTime = -10.;
	FVector LastPlanPosition = FVector::ZeroVector;
	float LastShiftAlpha = -1.f;
	bool bNeedsReplan = true;
};
