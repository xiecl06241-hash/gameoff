#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShiftSettingsWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;

/**
 * Mouse sensitivity page used inside the pause overlay.
 */
UCLASS()
class SHIFTSTORE_API UShiftSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void FocusSlider();

	/** Re-apply the current language to the static settings labels. */
	void RefreshLabels();

	/** Called after Back so the pause menu can restore its main panel. */
	FSimpleDelegate OnClosed;

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USlider> SensitivitySlider;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SensitivityValue;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RestoreButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> BackButton;

	UFUNCTION()
	void OnSensitivityChanged(float Value);

	UFUNCTION()
	void OnRestore();

	UFUNCTION()
	void OnBack();

	void UpdateValueText(float Value);

	TArray<TPair<FString, TWeakObjectPtr<UTextBlock>>> LocalizedLabels;
};
