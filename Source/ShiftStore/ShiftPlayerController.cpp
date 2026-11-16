#include "ShiftPlayerController.h"
#include "ShiftHUD.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftStoreGameMode.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"

void AShiftPlayerController::BeginPlay()
{
	Super::BeginPlay();
	float Saved = 1.f;
	GConfig->GetFloat(TEXT("ShiftStore.Input"), TEXT("MouseSensitivity"), Saved, GGameUserSettingsIni);
	Sensitivity = FMath::IsFinite(Saved) ? FMath::Clamp(Saved, MinSensitivity, MaxSensitivity) : 1.f;
}

void AShiftPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	SaveMouseSensitivity();
	Super::EndPlay(Reason);
}

void AShiftPlayerController::SetMouseSensitivity(float Value, bool bPersist)
{
	const float Safe = FMath::IsFinite(Value) ? Value : 1.f;
	const float Next = FMath::Clamp(FMath::RoundToFloat(Safe * 20.f) / 20.f, MinSensitivity, MaxSensitivity);
	if (!FMath::IsNearlyEqual(Next, Sensitivity)) { Sensitivity = Next; bSensitivityDirty = true; }
	if (bPersist) { SaveMouseSensitivity(); }
}

void AShiftPlayerController::SaveMouseSensitivity()
{
	if (!bSensitivityDirty) { return; }
	GConfig->SetFloat(TEXT("ShiftStore.Input"), TEXT("MouseSensitivity"), Sensitivity, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	bSensitivityDirty = false;
}

void AShiftPlayerController::CloseMouseSettings()
{
	ReleaseMenuMouse();
	SaveMouseSensitivity();
	bMouseSettingsOpen = false;
	SelectedPauseAction = 1;
	MenuTransitionFrame = GFrameCounter;
}

void AShiftPlayerController::DecreaseSensitivity()
{
	if (bPauseMenuOpen && bMouseSettingsOpen && SelectedMouseSetting == 0) { SetMouseSensitivity(Sensitivity - 0.05f); }
}

void AShiftPlayerController::IncreaseSensitivity()
{
	if (bPauseMenuOpen && bMouseSettingsOpen && SelectedMouseSetting == 0) { SetMouseSensitivity(Sensitivity + 0.05f); }
}

void AShiftPlayerController::UpdateSensitivityFromMouse(float X, float Width)
{
	const float Fraction = FMath::Clamp((X - Width * 0.30f) / (Width * 0.40f), 0.f, 1.f);
	SetMouseSensitivity(FMath::Lerp(MinSensitivity, MaxSensitivity, Fraction), false);
}

void AShiftPlayerController::ReleaseMenuMouse()
{
	if (bDraggingSensitivity) { bDraggingSensitivity = false; SaveMouseSensitivity(); }
}

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
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AShiftPlayerController::ReleaseMenuMouse).bExecuteWhenPaused = true;
	for (const FKey Key : {EKeys::Left, EKeys::A})
	{
		InputComponent->BindKey(Key, IE_Pressed, this, &AShiftPlayerController::DecreaseSensitivity).bExecuteWhenPaused = true;
	}
	for (const FKey Key : {EKeys::Right, EKeys::D})
	{
		InputComponent->BindKey(Key, IE_Pressed, this, &AShiftPlayerController::IncreaseSensitivity).bExecuteWhenPaused = true;
	}
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
		ReleaseMenuMouse();
		SaveMouseSensitivity();
		bMouseSettingsOpen = false;
		SetInputMode(FInputModeGameOnly());
	}
}

void AShiftPlayerController::TogglePauseMenu()
{
	const AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	if (!GM || GM->Phase == EShiftPhase::LanguageSelection || GM->Phase == EShiftPhase::Title) { return; }
	if (bMouseSettingsOpen) { CloseMouseSettings(); return; }
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
	if (!bPauseMenuOpen) { return; }
	if (bMouseSettingsOpen) { SelectedMouseSetting = (SelectedMouseSetting + 2) % 3; }
	else { SelectedPauseAction = (SelectedPauseAction + 3) % 4; }
}

void AShiftPlayerController::NextPauseAction()
{
	if (!bPauseMenuOpen) { return; }
	if (bMouseSettingsOpen) { SelectedMouseSetting = (SelectedMouseSetting + 1) % 3; }
	else { SelectedPauseAction = (SelectedPauseAction + 1) % 4; }
}

void AShiftPlayerController::ConfirmPauseAction()
{
	if (!bPauseMenuOpen) { return; }
	if (bMouseSettingsOpen)
	{
		if (SelectedMouseSetting == 1) { SetMouseSensitivity(1.f); }
		else if (SelectedMouseSetting == 2) { CloseMouseSettings(); }
		return;
	}
	const int32 Action = SelectedPauseAction;
	if (Action == 1)
	{
		bMouseSettingsOpen = true;
		SelectedMouseSetting = 0;
		GetMousePosition(PreviousMenuMouse.X, PreviousMenuMouse.Y);
		return;
	}
	if (Action == 3)
	{
		SaveMouseSensitivity();
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
		return;
	}
	SetPauseMenuOpen(false);
	if (bPauseMenuOpen) { return; }
	if (Action == 2)
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
	if (bMouseSettingsOpen)
	{
		if (AShiftHUD::SensitivitySlider(Width, Height).IsInside(Mouse))
		{
			SelectedMouseSetting = 0;
			bDraggingSensitivity = true;
			UpdateSensitivityFromMouse(Mouse.X, Width);
			return;
		}
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (AShiftHUD::SensitivityButton(Index, Width, Height).IsInside(Mouse))
			{
				SelectedMouseSetting = Index + 1;
				ConfirmPauseAction();
				return;
			}
		}
		return;
	}
	for (int32 Index = 0; Index < 4; ++Index)
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
			int32 Width = 0, Height = 0;
			GetViewportSize(Width, Height);
			if (bMouseSettingsOpen && bDraggingSensitivity)
			{
				if (IsInputKeyDown(EKeys::LeftMouseButton)) { UpdateSensitivityFromMouse(Mouse.X, Width); }
				else { ReleaseMenuMouse(); }
			}
			if (PreviousMenuMouse.X >= 0.f && FVector2D::DistSquared(Mouse, PreviousMenuMouse) > 1.f)
			{
				if (bMouseSettingsOpen && !bDraggingSensitivity)
				{
					if (AShiftHUD::SensitivitySlider(Width, Height).IsInside(Mouse)) { SelectedMouseSetting = 0; }
					for (int32 Index = 0; Index < 2; ++Index)
					{
						if (AShiftHUD::SensitivityButton(Index, Width, Height).IsInside(Mouse)) { SelectedMouseSetting = Index + 1; }
					}
				}
				for (int32 Index = 0; !bMouseSettingsOpen && Index < 4; ++Index)
				{
					if (AShiftHUD::PauseButton(Index, Width, Height).IsInside(Mouse)) { SelectedPauseAction = Index; }
				}
			}
			PreviousMenuMouse = Mouse;
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ShiftPauseTest"))) { TickPauseTest(); }
	if (FParse::Param(FCommandLine::Get(), TEXT("ShiftSensitivityTest"))) { TickSensitivityTest(); }
}
