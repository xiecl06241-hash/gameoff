#include "ShiftPlayerController.h"
#include "ShiftHUD.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftStoreGameMode.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void AShiftPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// The controller still processes these bindings while the world is paused.
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AShiftPlayerController::TogglePauseMenu).bExecuteWhenPaused = true;
	for (const FKey Key : {EKeys::Up, EKeys::W})
	{
		InputComponent->BindKey(Key, IE_Pressed, this, &AShiftPlayerController::PreviousPauseAction).bExecuteWhenPaused = true;
	}
	for (const FKey Key : {EKeys::Down, EKeys::S})
	{
		InputComponent->BindKey(Key, IE_Pressed, this, &AShiftPlayerController::NextPauseAction).bExecuteWhenPaused = true;
	}
	for (const FKey Key : {EKeys::Enter, EKeys::SpaceBar})
	{
		InputComponent->BindKey(Key, IE_Pressed, this, &AShiftPlayerController::ConfirmPauseAction).bExecuteWhenPaused = true;
	}
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AShiftPlayerController::ClickPauseAction).bExecuteWhenPaused = true;
}

void AShiftPlayerController::SetPauseMenuOpen(bool bOpen)
{
	if (bOpen == bPauseMenuOpen || !SetPause(bOpen)) { return; }
	bPauseMenuOpen = bOpen;
	MenuTransitionFrame = GFrameCounter;
	bWaitForGameplayRelease = true;
	SetShowMouseCursor(bOpen);
	if (bOpen)
	{
		SelectedPauseAction = 0;
		GetMousePosition(PreviousMenuMouse.X, PreviousMenuMouse.Y);
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		if (AShiftPlayerCharacter* Clerk = Cast<AShiftPlayerCharacter>(GetPawn()))
		{
			Clerk->GetCharacterMovement()->StopMovementImmediately();
		}
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void AShiftPlayerController::TogglePauseMenu()
{
	const AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	if (!GM || GM->Phase == EShiftPhase::LanguageSelection || GM->Phase == EShiftPhase::Title) { return; }
	SetPauseMenuOpen(!bPauseMenuOpen);
}

bool AShiftPlayerController::BlocksGameplayInput()
{
	if (bPauseMenuOpen || MenuTransitionFrame == GFrameCounter) { return true; }
	if (bWaitForGameplayRelease)
	{
		// A menu click or held interaction must not become a throw, chore or screen advance.
		for (const FKey Key : {EKeys::E, EKeys::LeftMouseButton, EKeys::F, EKeys::RightMouseButton,
			EKeys::Q, EKeys::G, EKeys::Enter, EKeys::SpaceBar})
		{
			if (IsInputKeyDown(Key)) { return true; }
		}
		bWaitForGameplayRelease = false;
	}
	return false;
}

void AShiftPlayerController::PreviousPauseAction()
{
	if (bPauseMenuOpen) { SelectedPauseAction = (SelectedPauseAction + 2) % 3; }
}

void AShiftPlayerController::NextPauseAction()
{
	if (bPauseMenuOpen) { SelectedPauseAction = (SelectedPauseAction + 1) % 3; }
}

void AShiftPlayerController::ConfirmPauseAction()
{
	if (!bPauseMenuOpen) { return; }
	const int32 Action = SelectedPauseAction;
	if (Action == 2)
	{
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		return;
	}
	SetPauseMenuOpen(false);
	if (bPauseMenuOpen) { return; }
	if (Action == 1)
	{
		if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>()) { GM->ReturnToTitle(); }
	}
}

void AShiftPlayerController::ClickPauseAction()
{
	if (!bPauseMenuOpen) { return; }
	FVector2D Mouse;
	int32 Width = 0, Height = 0;
	GetViewportSize(Width, Height);
	if (!GetMousePosition(Mouse.X, Mouse.Y)) { return; }
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (AShiftHUD::PauseButton(Index, Width, Height).IsInside(Mouse))
		{
			SelectedPauseAction = Index;
			ConfirmPauseAction();
			return;
		}
	}
}

void AShiftPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPauseMenuOpen)
	{
		FVector2D Mouse;
		if (GetMousePosition(Mouse.X, Mouse.Y))
		{
			if (PreviousMenuMouse.X >= 0.f && FVector2D::DistSquared(Mouse, PreviousMenuMouse) > 1.f)
			{
				int32 Width = 0, Height = 0;
				GetViewportSize(Width, Height);
				for (int32 Index = 0; Index < 3; ++Index)
				{
					if (AShiftHUD::PauseButton(Index, Width, Height).IsInside(Mouse)) { SelectedPauseAction = Index; }
				}
			}
			PreviousMenuMouse = Mouse;
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ShiftPauseTest"))) { TickPauseTest(); }
}
