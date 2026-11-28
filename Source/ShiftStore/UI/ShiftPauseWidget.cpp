#include "ShiftPauseWidget.h"

#include "../ShiftLocalization.h"
#include "../ShiftPlayerController.h"
#include "ShiftSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

namespace
{
	bool IsPauseLabelKey(const FString& Key)
	{
		static const TCHAR* Keys[] =
		{
			TEXT("PAUSED"),
			TEXT("Resume Game"),
			TEXT("Mouse Sensitivity"),
			TEXT("Return to Main Menu"),
			TEXT("Quit Game"),
			TEXT("Only dawn checkpoints are saved. Returning to the menu discards this night's progress.")
		};

		for (const TCHAR* Candidate : Keys)
		{
			if (Key == Candidate)
			{
				return true;
			}
		}
		return false;
	}
}

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

	LocalizedLabels.Reset();
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			if (UTextBlock* Block = Cast<UTextBlock>(Widget))
			{
				const FString Key = Block->GetText().ToString();
				if (IsPauseLabelKey(Key))
				{
					LocalizedLabels.Emplace(Key, Block);
				}
			}
		});
	}
	RefreshLabels();
}

void UShiftPauseWidget::RefreshLabels()
{
	for (const TPair<FString, TWeakObjectPtr<UTextBlock>>& Pair : LocalizedLabels)
	{
		if (UTextBlock* Block = Pair.Value.Get())
		{
			Block->SetText(FText::FromString(ShiftLoc::Text(Pair.Key)));
		}
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
		SettingsPanel->RefreshLabels();
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
