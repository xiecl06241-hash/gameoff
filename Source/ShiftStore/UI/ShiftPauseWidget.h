#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShiftPauseWidget.generated.h"

class UButton;
class UWidget;
class UShiftSettingsWidget;

/**
 * Pause overlay. Buttons call the PlayerController's public UI_* methods instead
 * of recreating the old Canvas hit-testing logic.
 */
UCLASS()
class SHIFTSTORE_API UShiftPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** Focus the Resume button when the menu opens (keyboard/gamepad). */
	void FocusFirstButton();

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> MainPanel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShiftSettingsWidget> SettingsPanel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> TitleButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> QuitButton;

	UFUNCTION()
	void OnResume();

	UFUNCTION()
	void OnSettings();

	UFUNCTION()
	void OnTitle();

	UFUNCTION()
	void OnQuit();

	void HandleSettingsClosed();
};
