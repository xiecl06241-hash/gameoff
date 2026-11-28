#include "ShiftUIRoot.h"

#include "ShiftHUDWidget.h"
#include "ShiftPauseWidget.h"
#include "ShiftTitleWidget.h"
#include "ShiftDialogueWidget.h"

#include "../ShiftPlayerCharacter.h"
#include "../ShiftPlayerController.h"
#include "../ShiftStoreGameMode.h"

#include "Components/WidgetSwitcher.h"
#include "TimerManager.h"

void UShiftRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LogoLayer)
	{
		LogoLayer->SetVisibility(ESlateVisibility::Visible);
		GetWorld()->GetTimerManager().SetTimer(
			LogoHideTimer, this, &UShiftRootWidget::HideLogo, 3.2f, false);
	}
	else if (Title)
	{
		Title->ShowLanguagePopupOnce();
	}

	Refresh();
}

void UShiftRootWidget::HideLogo()
{
	if (LogoLayer)
	{
		LogoLayer->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Title)
	{
		Title->ShowLanguagePopupOnce();
	}
}

void UShiftRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

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
	AShiftStoreGameMode* GM = GetWorld()
		? GetWorld()->GetAuthGameMode<AShiftStoreGameMode>()
		: nullptr;

	AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetOwningPlayer());

	if (!GM || !PC || !ScreenSwitcher)
	{
		return;
	}

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

	if (GM->Phase == EShiftPhase::Title)
	{
		bPrologueShown = false;
		if (DialogueLayer)
		{
			DialogueLayer->HideDialogue();
		}
	}

	if (GM->Phase == EShiftPhase::NightIntro
		&& GM->NightIndex == 0
		&& DialogueLayer
		&& !bPrologueShown)
	{
		bPrologueShown = true;
		DialogueLayer->StartPrologue();
	}
	else if (GM->Phase != EShiftPhase::NightIntro && DialogueLayer)
	{
		DialogueLayer->HideDialogue();
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