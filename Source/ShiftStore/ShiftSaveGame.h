#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ShiftSaveGame.generated.h"

UCLASS()
class SHIFTSTORE_API UShiftSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() int32 SavedNightIndex = 0;
	UPROPERTY() float CumulativeIncome = 0.f;
	UPROPERTY() float CumulativeLosses = 0.f;
};
