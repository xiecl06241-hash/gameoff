#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShiftUIRoot.generated.h"

class UWidgetSwitcher;
class UShiftHUDWidget;
class UShiftPauseWidget;

/**
 * Root UMG widget.
 *
 * Owned by AShiftHUD and added to the viewport once. It maps the game phase to
 * the matching screen and forwards the 10 Hz refresh to the gameplay HUD.
 */
UCLASS()
class SHIFTSTORE_API UShiftRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Rebuild the visible screen and refresh the current child widget. */
	void Refresh();
	void ToggleRules();

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShiftHUDWidget> HUD;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShiftPauseWidget> PauseMenu;

	float RefreshTimer = 0.f;
	int32 LastScreen = INDEX_NONE;
	bool bLastPaused = false;
};
