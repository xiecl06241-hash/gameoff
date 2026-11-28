#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/EngineTypes.h"
#include "ShiftUIRoot.generated.h"

class UWidgetSwitcher;
class UShiftHUDWidget;
class UShiftPauseWidget;
class UShiftTitleWidget;
class UShiftDialogueWidget;

UCLASS()
class SHIFTSTORE_API UShiftRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void Refresh();
	void ToggleRules();

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShiftHUDWidget> HUD;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShiftPauseWidget> PauseMenu;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UShiftTitleWidget> Title;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UShiftDialogueWidget> DialogueLayer;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UUserWidget> LogoLayer;

	FTimerHandle LogoHideTimer;

	float RefreshTimer = 0.f;
	int32 LastScreen = INDEX_NONE;
	bool bLastPaused = false;
	bool bPrologueShown = false;

	void HideLogo();
};