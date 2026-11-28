#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShiftDialogueWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;

UCLASS()
class SHIFTSTORE_API UShiftDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void StartPrologue();
	void HideDialogue();

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SpeakerText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DialogueText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> Portrait;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ClickCatcher;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ContinueText;

	TArray<FString> Speakers;
	TArray<FString> Lines;

	int32 LineIndex = 0;
	int32 VisibleChars = 0;
	float TypeTimer = 0.f;
	bool bActive = false;
	bool bTyping = false;

	void ShowLine();
	void Advance();

	UFUNCTION()
	void OnClick();
};