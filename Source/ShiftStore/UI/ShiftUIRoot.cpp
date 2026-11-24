#include "ShiftUIRoot.h"
#include "ShiftHUDWidget.h"
#include "ShiftPauseWidget.h"
#include "../ShiftPlayerCharacter.h"
#include "../ShiftPlayerController.h"
#include "../ShiftStoreGameMode.h"

#include "Components/WidgetSwitcher.h"

void UShiftRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UShiftRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The HUD does not need to rebuild at full frame rate. 10 Hz keeps text,
	// timers and bars readable without creating a new FText every frame.
	RefreshTimer += InDeltaTime;
	if (RefreshTimer < 0.1f)
	{
		return;
	}
	RefreshTimer = 0.f;
	Refresh();
}

void UShiftRootWidget::Refresh()
{
	AShiftStoreGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AShiftStoreGameMode>() : nullptr;
	AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer());
	if (!GM || !PC || !ScreenSwitcher)
	{
		return;
	}

	// Keep this order in sync with the WBP_ShiftRoot WidgetSwitcher children:
	// 0 Language, 1 Title, 2 NightIntro, 3 HUD, 4 Dawn, 5 GameOver, 6 Ending.
	int32 Screen = 3;
	switch (GM->Phase)
	{
	case EShiftPhase::LanguageSelection: Screen = 0; break;
	case EShiftPhase::Title:             Screen = 1; break;
	case EShiftPhase::NightIntro:        Screen = 2; break;
	case EShiftPhase::Working:
	case EShiftPhase::Cleanup:           Screen = 3; break;
	case EShiftPhase::Dawn:              Screen = 4; break;
	case EShiftPhase::GameOver:          Screen = 5; break;
	case EShiftPhase::Ending:            Screen = 6; break;
	default: break;
	}

	if (Screen != LastScreen)
	{
		ScreenSwitcher->SetActiveWidgetIndex(Screen);
		LastScreen = Screen;
	}

	const bool bPaused = PC->IsPauseMenuOpen();
	if (PauseMenu)
	{
		PauseMenu->SetVisibility(bPaused
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);

		if (bPaused && !bLastPaused)
		{
			PauseMenu->FocusFirstButton();
		}
	}
	bLastPaused = bPaused;

	if (HUD && Screen == 3)
	{
		HUD->Refresh(GM, Cast<AShiftPlayerCharacter>(PC->GetPawn()));
	}

	PC->UpdateUIInputMode();
}

void UShiftRootWidget::ToggleRules()
{
    if (HUD)
    {
        HUD->ToggleRules();
    }
}