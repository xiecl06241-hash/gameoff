#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShiftTitleWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UShiftSettingsWidget;
class UShiftLanguagePopup;

UCLASS()
class SHIFTSTORE_API UShiftTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void ShowLanguagePopupOnce();
	void RefreshTexts();

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> MainPanel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> LanguageButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> StartLabel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SettingsLabel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> LanguageLabel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShiftSettingsWidget> SettingsPanel;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UShiftLanguagePopup> LanguagePopup;

	UFUNCTION()
	void OnStart();

	UFUNCTION()
	void OnSettings();

	UFUNCTION()
	void OnLanguage();

	void CloseSettings();
	void OnLanguageChanged();

	bool bLanguagePromptShown = false;
};