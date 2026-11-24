#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShiftHUDWidget.generated.h"

class UFont;
class UImage;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class AShiftPlayerCharacter;
class AShiftStoreGameMode;

/**
 * Gameplay HUD built with UMG widgets.
 *
 * All player-facing strings still come from ShiftLoc, so the existing
 * author_localization.py / check_localization.py workflow keeps working.
 */
class UButton;
class UWidget;

UCLASS()
class SHIFTSTORE_API UShiftHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UShiftHUDWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	void Refresh(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player);

	UFUNCTION()
	void ToggleRules();


private:
	// --- Top-left status strip ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NightText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SanityText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> SanityBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LedgerText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> LossBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CarryText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> StaminaBar;

	// --- Bottom-left Shift Rules memo ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RulesTitle;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RuleText1;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RuleText2;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RuleText3;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CorruptionText;

	// --- Top-right objective panel ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ObjectiveTitle;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UVerticalBox> ObjectiveRows;

	// --- Center / bottom ---
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> HoldBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BannerText;

	// Optional: add these to the WBP only if you want the dot/vignette.
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> CrosshairDot;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UImage> HeartbeatOverlay;

	// Fonts are reused from the original Canvas HUD assets.
	UPROPERTY() TObjectPtr<UFont> ChineseFont;
	UPROPERTY() TObjectPtr<UFont> JapaneseFont;
	UPROPERTY() TObjectPtr<UFont> TraditionalChineseFont;

	UPROPERTY() TArray<TObjectPtr<UTextBlock>> ObjectiveRowPool;

	// rules button
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RulesButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> RulesCard;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RulesButtonLabel;

	bool bRulesExpanded = false;

	
	
	//rules button

	int32 LastLanguage = -1;

	void BuildObjectiveRows();
	void ApplyFonts();
	void ApplyFontTo(UTextBlock* Text, UFont* Font) const;
	void RefreshObjective(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player);
	void RefreshRules(AShiftStoreGameMode* GM, bool bShifted);
	void SetObjectiveRows(const TArray<FString>& Lines, const TArray<FLinearColor>& Colors);

	static void SetTextIfChanged(UTextBlock* Block, const FString& Value);
};
