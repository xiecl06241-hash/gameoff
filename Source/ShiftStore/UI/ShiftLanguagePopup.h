#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShiftLanguagePopup.generated.h"

class UButton;

UCLASS()
class SHIFTSTORE_API UShiftLanguagePopup : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void ShowPopup();

	FSimpleDelegate OnClosed;

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> EnglishButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> JapaneseButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> TraditionalChineseButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SimplifiedChineseButton;

	UFUNCTION()
	void OnEnglish();

	UFUNCTION()
	void OnJapanese();

	UFUNCTION()
	void OnTraditionalChinese();

	UFUNCTION()
	void OnSimplifiedChinese();

	void Choose(int32 Index);
};