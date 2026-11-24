#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftClueActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class SHIFTSTORE_API AShiftClueActor : public AActor
{
	GENERATED_BODY()

public:
	AShiftClueActor();

	void Configure(int32 InClueId, int32 InMinNightIndex, bool bInOtherworldOnly, bool bInNormalOnly, bool bInIsBrassKey, bool bInRequiresBrassKey, bool bInRequiresOfficePhone, const FString& InTitleKey, const FString& InBodyKey);
	bool IsCurrentlyVisible(int32 NightIndex, bool bEffectiveShifted, int32 CollectedMask, bool bHasBrassKey, bool bPhoneAnswered) const;
	void RefreshVisualState(int32 NightIndex, bool bEffectiveShifted, int32 CollectedMask, bool bHasBrassKey, bool bPhoneAnswered);

	int32 ClueId = -1;
	int32 MinNightIndex = 0;
	bool bOtherworldOnly = false;
	bool bNormalOnly = false;
	bool bIsBrassKey = false;
	bool bRequiresBrassKey = false;
	bool bRequiresOfficePhone = false;
	FString TitleKey;
	FString BodyKey;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> AccentMesh;
	UPROPERTY() TObjectPtr<UPointLightComponent> BeaconLight;
	float PulseTime = 0.f;
	bool bVisibleNow = false;
};
