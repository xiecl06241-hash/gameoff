#include "ShiftTitleWidget.h"

#include "../ShiftLocalization.h"
#include "../ShiftStoreGameMode.h"
#include "ShiftLanguagePopup.h"
#include "ShiftSettingsWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UShiftTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &UShiftTitleWidget::OnStart);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UShiftTitleWidget::OnSettings);
	}
	if (LanguageButton)
	{
		LanguageButton->OnClicked.AddDynamic(this, &UShiftTitleWidget::OnLanguage);
	}

	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
		SettingsPanel->OnClosed.BindUObject(this, &UShiftTitleWidget::CloseSettings);
	}

	if (LanguagePopup)
	{
		LanguagePopup->SetVisibility(ESlateVisibility::Collapsed);
		LanguagePopup->OnClosed.BindUObject(this, &UShiftTitleWidget::OnLanguageChanged);
	}

	RefreshTexts();
}

void UShiftTitleWidget::RefreshTexts()
{
	if (StartLabel)
	{
		StartLabel->SetText(FText::FromString(ShiftLoc::Text(TEXT("START GAME"))));
	}
	if (SettingsLabel)
	{
		SettingsLabel->SetText(FText::FromString(ShiftLoc::Text(TEXT("SETTINGS"))));
	}
	if (LanguageLabel)
	{
		LanguageLabel->SetText(FText::FromString(ShiftLoc::Text(TEXT("LANGUAGE"))));
	}
}

void UShiftTitleWidget::ShowLanguagePopupOnce()
{
	if (bLanguagePromptShown)
	{
		return;
	}
	bLanguagePromptShown = true;

	if (LanguagePopup)
	{
		LanguagePopup->ShowPopup();
	}
}

void UShiftTitleWidget::OnStart()
{
	if (AShiftStoreGameMode* GM = GetWorld()
		? GetWorld()->GetAuthGameMode<AShiftStoreGameMode>()
		: nullptr)
	{
		if (GM->Phase == EShiftPhase::Title)
		{
			GM->OnAdvanceScreenKey();
		}
	}
}

void UShiftTitleWidget::OnSettings()
{
	if (MainPanel)
	{
		MainPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Visible);
		SettingsPanel->FocusSlider();
	}
}

void UShiftTitleWidget::CloseSettings()
{
	if (MainPanel)
	{
		MainPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UShiftTitleWidget::OnLanguage()
{
	if (LanguagePopup)
	{
		LanguagePopup->ShowPopup();
	}
}

void UShiftTitleWidget::OnLanguageChanged()
{
	RefreshTexts();
}