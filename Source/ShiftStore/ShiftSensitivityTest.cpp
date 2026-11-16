#include "ShiftPlayerController.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftStoreGameMode.h"
#include "ShiftTaskTarget.h"
#include "ShiftLocalization.h"
#include "ShiftHUD.h"
#include "Camera/CameraComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void AShiftPlayerController::TickSensitivityTest()
{
	const double Now = FPlatformTime::Seconds();
	if (SensitivityTestNextTime == 0.) { SensitivityTestNextTime = Now + 1.; }
	if (Now < SensitivityTestNextTime) { return; }
	SensitivityTestNextTime = Now + 0.30;
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	AShiftPlayerCharacter* Clerk = Cast<AShiftPlayerCharacter>(GetPawn());
	if (!GM || !Clerk) { return; }
	const auto Check = [this](const TCHAR* Name, bool bPassed) {
		SensitivityTestChecks.Add(Name, bPassed);
		UE_LOG(LogTemp, Display, TEXT("SENSITIVITY_CHECK %s = %d"), Name, bPassed);
	};
	float Readback = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("ShiftSensitivityReadback="), Readback))
	{
		Check(TEXT("sensitivity_restored_after_relaunch"), FMath::IsNearlyEqual(Sensitivity, Readback, 0.001f));
		FinishSensitivityTest();
		return;
	}
	const auto Key = [this](FKey InKey, bool bDown) {
		InputKey(FInputKeyEventArgs::CreateSimulated(InKey, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f));
	};
	const auto MouseClick = [this, &Key](const FBox2D& Rect) {
		const FVector2D Center = Rect.GetCenter();
		SetMouseLocation(FMath::RoundToInt(Center.X), FMath::RoundToInt(Center.Y));
		Key(EKeys::LeftMouseButton, true);
	};
	const auto SliderPosition = [this](float Value, int32 Width, int32 Height) {
		const float Fraction = (Value - MinSensitivity) / (MaxSensitivity - MinSensitivity);
		SetMouseLocation(FMath::RoundToInt(Width * (0.30f + 0.40f * Fraction)), FMath::RoundToInt(Height * 0.485f));
	};
	const auto MeasureLook = [this, Clerk](bool bOriginalInput = false) {
		RotationInput = FRotator::ZeroRotator;
		SetControlRotation(FRotator::ZeroRotator);
		if (bOriginalInput)
		{
			Clerk->AddControllerYawInput(20.f * 0.7f);
			Clerk->AddControllerPitchInput(-10.f * 0.7f);
		}
		else { Clerk->ApplyMouseLook(20.f, 10.f); }
		UpdateRotation(1.f / 60.f);
		const FVector2D Motion(FMath::UnwindDegrees(GetControlRotation().Yaw), FMath::UnwindDegrees(GetControlRotation().Pitch));
		RotationInput = FRotator::ZeroRotator;
		return Motion;
	};
	const auto SavedSensitivity = []() {
		FConfigFile Disk;
		Disk.Read(ShiftLoc::SettingsFile());
		float Value = -1.f;
		Disk.GetFloat(TEXT("ShiftStore.Input"), TEXT("MouseSensitivity"), Value);
		return Value;
	};
	int32 Width = 0, Height = 0;
	GetViewportSize(Width, Height);
	switch (SensitivityTestStep++)
	{
	case 0:
		GM->Phase = EShiftPhase::Title;
		GM->OnAdvanceScreenKey();
		GM->OnAdvanceScreenKey();
		SetMouseSensitivity(1.f, false);
		SensitivityTestBaseline = MeasureLook(true);
		Check(TEXT("default_mouse_look_matches_previous_speed"), !SensitivityTestBaseline.IsNearlyZero() && MeasureLook().Equals(SensitivityTestBaseline, 0.01f));
		Key(EKeys::Escape, true);
		break;
	case 1:
		Check(TEXT("pause_menu_open"), bPauseMenuOpen && IsPaused());
		Key(EKeys::Escape, false);
		Key(EKeys::Down, true);
		break;
	case 2:
		Check(TEXT("keyboard_selects_mouse_settings_entry"), SelectedPauseAction == 1);
		Key(EKeys::Down, false);
		Key(EKeys::Enter, true);
		break;
	case 3:
	{
		Check(TEXT("mouse_settings_remain_paused"), bMouseSettingsOpen && bPauseMenuOpen && IsPaused());
		Key(EKeys::Enter, false);
		SensitivityTestTimer = GM->ActiveTasks[0]->TimeLeft;
		SensitivityTestSanity = GM->Sanity;
		Key(EKeys::Right, true);
		break;
	}
	case 4:
		Key(EKeys::Right, false);
		Check(TEXT("right_key_increases_sensitivity"), FMath::IsNearlyEqual(Sensitivity, 1.05f, 0.001f));
		Check(TEXT("settings_freeze_timers_and_sanity"), GM->ActiveTasks[0]->TimeLeft == SensitivityTestTimer && GM->Sanity == SensitivityTestSanity);
		Key(EKeys::Left, true);
		break;
	case 5:
		Key(EKeys::Left, false);
		Check(TEXT("left_key_decreases_sensitivity"), FMath::IsNearlyEqual(Sensitivity, 1.f, 0.001f));
		SliderPosition(1.5f, Width, Height);
		Key(EKeys::LeftMouseButton, true);
		break;
	case 6:
		Check(TEXT("slider_click_updates_value"), FMath::IsNearlyEqual(Sensitivity, 1.5f, 0.001f) && bDraggingSensitivity);
		SliderPosition(MaxSensitivity + 0.5f, Width, Height);
		break;
	case 7:
		Check(TEXT("slider_drag_clamps_maximum"), Sensitivity == MaxSensitivity);
		SliderPosition(MinSensitivity - 0.5f, Width, Height);
		break;
	case 8:
		Check(TEXT("slider_drag_clamps_minimum"), Sensitivity == MinSensitivity);
		SliderPosition(1.5f, Width, Height);
		break;
	case 9:
		Key(EKeys::LeftMouseButton, false);
		break;
	case 10:
		Check(TEXT("mouse_release_saves_setting"), FMath::IsNearlyEqual(SavedSensitivity(), 1.5f, 0.001f));
		MouseClick(AShiftHUD::SensitivityButton(0, Width, Height));
		break;
	case 11:
		Check(TEXT("restore_default_button"), Sensitivity == 1.f && FMath::IsNearlyEqual(SavedSensitivity(), 1.f));
		Key(EKeys::LeftMouseButton, false);
		break;
	case 12:
		SliderPosition(1.5f, Width, Height);
		Key(EKeys::LeftMouseButton, true);
		break;
	case 13:
	{
		Key(EKeys::LeftMouseButton, false);
		const FString Dir = FPaths::ProjectSavedDir() / TEXT("Sensitivity") / ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage());
		IFileManager::Get().MakeDirectory(*Dir, true);
		FScreenshotRequest::RequestScreenshot(Dir / TEXT("Settings.png"), true, false);
		break;
	}
	case 14:
		MouseClick(AShiftHUD::SensitivityButton(1, Width, Height));
		break;
	case 15:
		Check(TEXT("back_returns_to_paused_menu"), !bMouseSettingsOpen && bPauseMenuOpen && IsPaused());
		Key(EKeys::LeftMouseButton, false);
		Key(EKeys::Escape, true);
		break;
	case 16:
	{
		Check(TEXT("resume_restores_game_input"), !bMouseSettingsOpen && !bPauseMenuOpen && !IsPaused() && !bShowMouseCursor);
		Key(EKeys::Escape, false);
		const FVector2D Motion = MeasureLook();
		Check(TEXT("sensitivity_scales_actual_yaw_and_pitch"), Motion.Equals(SensitivityTestBaseline * 1.5f, 0.01f));
		GM->RestartCurrentNight();
		Check(TEXT("sensitivity_survives_night_restart"), Sensitivity == 1.5f);
		GM->ReturnToTitle();
		GM->OnAdvanceScreenKey();
		GM->OnAdvanceScreenKey();
		Check(TEXT("sensitivity_survives_new_game"), Sensitivity == 1.5f);
		Key(EKeys::Escape, true);
		break;
	}
	case 17:
		Key(EKeys::Escape, false);
		MouseClick(AShiftHUD::PauseButton(1, Width, Height));
		break;
	case 18:
		Check(TEXT("mouse_opens_settings_entry"), bMouseSettingsOpen && IsPaused());
		Key(EKeys::LeftMouseButton, false);
		Key(EKeys::Escape, true);
		break;
	case 19:
		Check(TEXT("escape_returns_to_paused_menu"), !bMouseSettingsOpen && bPauseMenuOpen && IsPaused());
		Key(EKeys::Escape, false);
		FinishSensitivityTest();
		break;
	default: break;
	}
}

void AShiftPlayerController::FinishSensitivityTest()
{
	bool bPassed = !SensitivityTestChecks.IsEmpty();
	TSharedRef<FJsonObject> Checks = MakeShared<FJsonObject>();
	for (const auto& Pair : SensitivityTestChecks) { Checks->SetBoolField(Pair.Key, Pair.Value); bPassed &= Pair.Value; }
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("passed"), bPassed);
	Report->SetNumberField(TEXT("baseline_yaw_degrees"), SensitivityTestBaseline.X);
	Report->SetNumberField(TEXT("baseline_pitch_degrees"), SensitivityTestBaseline.Y);
	Report->SetObjectField(TEXT("checks"), Checks);
	Report->SetStringField(TEXT("language"), ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage()));
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Sensitivity") / ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage());
	IFileManager::Get().MakeDirectory(*Dir, true);
	FFileHelper::SaveStringToFile(Json, *(Dir / TEXT("report.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FPlatformMisc::RequestExitWithStatus(!bPassed, bPassed ? 0 : 1);
}
