#include "ShiftLanguagePopup.h"

#include "../ShiftLocalization.h"
#include "../ShiftStoreGameMode.h"

#include "Components/Button.h"

void UShiftLanguagePopup::NativeConstruct()
{
	Super::NativeConstruct();

	if (EnglishButton)
	{
		EnglishButton->OnClicked.AddDynamic(this, &UShiftLanguagePopup::OnEnglish);
	}
	if (JapaneseButton)
	{
		JapaneseButton->OnClicked.AddDynamic(this, &UShiftLanguagePopup::OnJapanese);
	}
	if (TraditionalChineseButton)
	{
		TraditionalChineseButton->OnClicked.AddDynamic(
			this, &UShiftLanguagePopup::OnTraditionalChinese);
	}
	if (SimplifiedChineseButton)
	{
		SimplifiedChineseButton->OnClicked.AddDynamic(
			this, &UShiftLanguagePopup::OnSimplifiedChinese);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UShiftLanguagePopup::ShowPopup()
{
	SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();
}

void UShiftLanguagePopup::Choose(int32 Index)
{
	if (!ShiftLoc::SetLanguage(Index, true))
	{
		return;
	}

	if (AShiftStoreGameMode* GM = GetWorld()
		? GetWorld()->GetAuthGameMode<AShiftStoreGameMode>()
		: nullptr)
	{
		GM->SelectedLanguage = Index;
	}

	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.ExecuteIfBound();
}

void UShiftLanguagePopup::OnEnglish()
{
	Choose(0);
}

void UShiftLanguagePopup::OnJapanese()
{
	Choose(1);
}

void UShiftLanguagePopup::OnTraditionalChinese()
{
	Choose(2);
}

void UShiftLanguagePopup::OnSimplifiedChinese()
{
	Choose(3);
}