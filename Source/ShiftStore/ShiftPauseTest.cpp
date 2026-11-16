#include "ShiftPlayerController.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftStoreGameMode.h"
#include "ShiftTaskTarget.h"
#include "ShiftCustomer.h"
#include "ShiftDirector.h"
#include "ShiftHUD.h"
#include "ShiftItem.h"
#include "ShiftLocalization.h"
#include "InputKeyEventArgs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void AShiftPlayerController::TickPauseTest()
{
	// This probe is opt-in and uses real time because world time stops in the menu.
	const double Now = FPlatformTime::Seconds();
	if (PauseTestNextTime == 0.) { PauseTestNextTime = Now + 1.; }
	if (Now < PauseTestNextTime) { return; }
	PauseTestNextTime = Now + 0.30;
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	AShiftPlayerCharacter* Clerk = Cast<AShiftPlayerCharacter>(GetPawn());
	if (!GM || !Clerk) { return; }
	const auto Check = [this](const TCHAR* Name, bool bPassed) {
		PauseTestChecks.Add(Name, bPassed);
		UE_LOG(LogTemp, Display, TEXT("PAUSE_CHECK %s = %d"), Name, bPassed);
	};
	const auto Key = [this](FKey InKey, bool bDown) {
		InputKey(FInputKeyEventArgs::CreateSimulated(InKey, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f));
	};
	switch (PauseTestStep++)
	{
	case 0:
		Key(EKeys::Escape, true);
		break;
	case 1:
		Check(TEXT("escape_does_not_bypass_language_picker"), !IsPaused() && GM->Phase == EShiftPhase::LanguageSelection);
		Key(EKeys::Escape, false);
		GM->Phase = EShiftPhase::Title;
		GM->OnAdvanceScreenKey();
		Key(EKeys::Escape, true);
		break;
	case 2:
		Check(TEXT("escape_pauses_night_intro"), IsPaused() && bPauseMenuOpen && bShowMouseCursor);
		Key(EKeys::Escape, false);
		Key(EKeys::E, true);
		break;
	case 3:
		Check(TEXT("pause_interaction_cannot_advance_intro"), GM->Phase == EShiftPhase::NightIntro && IsPaused());
		Key(EKeys::E, false);
		Key(EKeys::Escape, true);
		break;
	case 4:
	{
		Check(TEXT("escape_resumes_without_advancing_intro"), !IsPaused() && !bPauseMenuOpen && !bShowMouseCursor && GM->Phase == EShiftPhase::NightIntro);
		Key(EKeys::Escape, false);
		GM->Phase = EShiftPhase::Working;
		GM->Sanity = 80.f;
		GM->ActiveTasks[0]->TimeLeft = 20.f;
		GM->ActiveTasks[1]->TimeLeft = 0.f;
		AShiftCustomer* Monster = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::CheckoutCustomerSpot, FRotator::ZeroRotator);
		if (!Monster)
		{
			Check(TEXT("monster_fixture_spawned"), false);
			FinishPauseTest();
			FPlatformMisc::RequestExitWithStatus(true, 1);
			return;
		}
		FShiftCustomerSpec Spec;
		Spec.bMonster = true;
		Monster->Init(Spec);
		Monster->State = ECustomerState::MonsterHunting;
		GM->ActiveCustomer = Monster;
		GM->Director->SetShifted(true);
		AShiftItem* Item = GetWorld()->SpawnActor<AShiftItem>(FVector(800.f, 150.f, 200.f), FRotator::ZeroRotator);
		Item->InitProduct(ShiftData::Products()[0].Sku);
		Clerk->PickupItem(Item);
		break;
	}
	case 5:
		Key(EKeys::Escape, true);
		break;
	case 6:
		Check(TEXT("escape_opens_gameplay_pause_menu"), IsPaused() && bPauseMenuOpen && bShowMouseCursor && SelectedPauseAction == 0);
		PauseTestTimeLeft = GM->ActiveTasks[0]->TimeLeft;
		PauseTestSanity = GM->Sanity;
		PauseTestWorldTime = GetWorld()->GetTimeSeconds();
		PauseTestMonsterTimer = GM->ActiveCustomer->StateTimer;
		PauseTestMonsterLocation = GM->ActiveCustomer->GetActorLocation();
		PauseTestPlayerLocation = Clerk->GetActorLocation();
		PauseTestShiftAlpha = GM->Director->Alpha();
		Key(EKeys::Escape, false);
		Key(EKeys::E, true);
		Key(EKeys::F, true);
		Key(EKeys::W, true);
		{
			const FString Dir = FPaths::ProjectSavedDir() / TEXT("PauseMenu") / ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage());
			IFileManager::Get().MakeDirectory(*Dir, true);
			FScreenshotRequest::RequestScreenshot(Dir / TEXT("Pause.png"), true, false);
		}
		break;
	case 7:
		Check(TEXT("paused_world_time_stays_fixed"), GetWorld()->GetTimeSeconds() == PauseTestWorldTime);
		Check(TEXT("paused_chore_timer_stays_fixed"), GM->ActiveTasks[0]->TimeLeft == PauseTestTimeLeft);
		Check(TEXT("paused_sanity_stays_fixed"), GM->Sanity == PauseTestSanity);
		Check(TEXT("paused_monster_stays_fixed"), GM->ActiveCustomer->StateTimer == PauseTestMonsterTimer && GM->ActiveCustomer->GetActorLocation().Equals(PauseTestMonsterLocation, 0.01f));
		Check(TEXT("paused_shift_animation_stays_fixed"), GM->Director->Alpha() == PauseTestShiftAlpha);
		Check(TEXT("paused_input_cannot_move_or_throw"), Clerk->GetActorLocation().Equals(PauseTestPlayerLocation, 0.01f) && Clerk->HeldItem.IsValid() && GM->Losses == 0.f);
		Key(EKeys::E, false);
		Key(EKeys::F, false);
		Key(EKeys::W, false);
		Key(EKeys::Escape, true);
		break;
	case 8:
		Check(TEXT("escape_resumes_gameplay"), !IsPaused() && !bPauseMenuOpen && !bShowMouseCursor);
		Check(TEXT("timers_and_sanity_resume"), GM->ActiveTasks[0]->TimeLeft < PauseTestTimeLeft && GM->Sanity < PauseTestSanity);
		Key(EKeys::Escape, false);
		Key(EKeys::Escape, true);
		break;
	case 9:
	{
		Key(EKeys::Escape, false);
		int32 Width = 0, Height = 0;
		GetViewportSize(Width, Height);
		const FVector2D Center = AShiftHUD::PauseButton(0, Width, Height).GetCenter();
		SetMouseLocation(FMath::RoundToInt(Center.X), FMath::RoundToInt(Center.Y));
		Key(EKeys::LeftMouseButton, true);
		break;
	}
	case 10:
		Check(TEXT("mouse_click_resumes_gameplay"), !IsPaused() && !bPauseMenuOpen && !bShowMouseCursor);
		Check(TEXT("resume_click_does_not_use_held_item"), Clerk->HeldItem.IsValid() && GM->Losses == 0.f);
		Key(EKeys::LeftMouseButton, false);
		Key(EKeys::Escape, true);
		break;
	case 11:
		Key(EKeys::Escape, false);
		Key(EKeys::Down, true);
		break;
	case 12:
		Check(TEXT("keyboard_selects_return_to_title"), bPauseMenuOpen && SelectedPauseAction == 1);
		Key(EKeys::Down, false);
		Key(EKeys::Enter, true);
		break;
	case 13:
		Check(TEXT("return_to_title_unpauses_and_cleans_night"), !IsPaused() && !bPauseMenuOpen && GM->Phase == EShiftPhase::Title
			&& GM->ActiveTasks.IsEmpty() && !GM->ActiveCustomer.IsValid() && !Clerk->HeldItem.IsValid() && !GM->Director->IsShifted());
		Key(EKeys::Enter, false);
		break;
	case 14:
		Check(TEXT("menu_confirm_does_not_start_new_game"), GM->Phase == EShiftPhase::Title);
		GM->OnAdvanceScreenKey();
		Key(EKeys::Escape, true);
		break;
	case 15:
		Key(EKeys::Escape, false);
		Key(EKeys::Down, true);
		break;
	case 16:
		Key(EKeys::Down, false);
		break;
	case 17:
		Key(EKeys::Down, true);
		break;
	case 18:
		Key(EKeys::Down, false);
		Check(TEXT("keyboard_selects_quit_game"), bPauseMenuOpen && SelectedPauseAction == 2);
		FinishPauseTest();
		Key(EKeys::Enter, true);
		break;
	default:
		break;
	}
}

void AShiftPlayerController::FinishPauseTest()
{
	bool bPassed = !PauseTestChecks.IsEmpty();
	TSharedRef<FJsonObject> Checks = MakeShared<FJsonObject>();
	for (const auto& Pair : PauseTestChecks) { Checks->SetBoolField(Pair.Key, Pair.Value); bPassed &= Pair.Value; }
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("passed"), bPassed);
	Report->SetObjectField(TEXT("checks"), Checks);
	Report->SetStringField(TEXT("language"), ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage()));
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("PauseMenu") / ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage());
	IFileManager::Get().MakeDirectory(*Dir, true);
	FFileHelper::SaveStringToFile(Json, *(Dir / TEXT("report.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
