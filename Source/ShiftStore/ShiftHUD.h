#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShiftHUD.generated.h"

UCLASS()
class SHIFTSTORE_API AShiftHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back);
	void DrawCenteredText(const FString& Str, float Y, FLinearColor Color, float Scale = 1.2f);
};
