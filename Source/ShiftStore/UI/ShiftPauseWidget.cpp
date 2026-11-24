#include "ShiftPauseWidget.h"

#include "../ShiftPlayerController.h"
#include "ShiftSettingsWidget.h"

#include "Components/Button.h"

void UShiftPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UShiftPauseWidget::OnResume);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UShiftPauseWidget::OnSettings);
	}
	if (TitleButton)
	{
		TitleButton->OnClicked.AddDynamic(this, &UShiftPauseWidget::OnTitle);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UShiftPauseWidget::OnQuit);
	}

	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
		SettingsPanel->OnClosed.BindUObject(this, &UShiftPauseWidget::HandleSettingsClosed);
	}
	if (MainPanel)
	{
		MainPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UShiftPauseWidget::FocusFirstButton()
{
	if (ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

void UShiftPauseWidget::OnResume()
{
	if (AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer()))
	{
		PC->UIResume();
	}
}

void UShiftPauseWidget::OnSettings()
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

void UShiftPauseWidget::OnTitle()
{
	if (AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer()))
	{
		PC->UIReturnToTitle();
	}
}

void UShiftPauseWidget::OnQuit()
{
	if (AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer()))
	{
		PC->UIQuit();
	}
}

void UShiftPauseWidget::HandleSettingsClosed()
{
	if (MainPanel)
	{
		MainPanel->SetVisibility(ESlateVisibility::Visible);
	}
	FocusFirstButton();
}
