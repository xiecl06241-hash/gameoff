#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShiftHUD.generated.h"

UCLASS()
class SHIFTSTORE_API AShiftHUD : public AHUD
{
	GENERATED_BODY()

public:
	AShiftHUD();
	virtual void DrawHUD() override;
	static FBox2D LanguageButton(int32 Index, float Width, float Height);
	bool HasRuntimeFonts() const;

private:
	void DrawTaskGuidance(class AShiftStoreGameMode* GM, class AShiftPlayerCharacter* Player);
	TWeakObjectPtr<AActor> GuideActor;
	FVector GuideDestination = FVector::ZeroVector;
	TArray<FVector> GuideRoute;
	float GuideRouteCheckedAt = -10.f;
	UPROPERTY() TObjectPtr<UFont> ChineseFont;
	UPROPERTY() TObjectPtr<UFont> JapaneseFont;
	UPROPERTY() TObjectPtr<UFont> TraditionalChineseFont;
	UFont* InterfaceFont(int32 Language = -1) const;
	void DrawLocalizedText(const FString& Str, FLinearColor Color, float X, float Y, UFont* Font, float Scale, float MaxWidth = 0.f);
	void DrawBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back);
	void DrawCenteredText(const FString& Str, float Y, FLinearColor Color, float Scale = 1.2f);
};
