#include "ShiftStoreGameMode.h"
#include "ShiftLocalization.h"
#include "ShiftHUD.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftCustomer.h"
#include "ShiftTaskTarget.h"
#include "ShiftItem.h"
#include "Camera/CameraComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void AShiftStoreGameMode::CheckLocalization(const FString& Name, bool bPassed)
{
	LocalizationChecks.Add(Name, bPassed);
	UE_LOG(LogTemp, Display, TEXT("LOCALIZATION_CHECK %s = %d"), *Name, bPassed);
}

void AShiftStoreGameMode::TickLocalizationTest(float DeltaSeconds)
{
	LocalizationTime += DeltaSeconds;
	if (LocalizationTime < 1.f) { return; }
	LocalizationTime = 0.f;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AShiftPlayerCharacter* Player = PC ? Cast<AShiftPlayerCharacter>(PC->GetPawn()) : nullptr;
	if (!PC || !Player) { CheckLocalization(TEXT("player_exists"), false); FinishLocalizationTest(); return; }
	FString ExpectedSavedCode;
	if (FParse::Value(FCommandLine::Get(), TEXT("ShiftLocalizationReadback="), ExpectedSavedCode))
	{
		CheckLocalization(TEXT("saved_language_restored_after_relaunch"), ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage()) == ExpectedSavedCode && SelectedLanguage == ShiftLoc::CurrentLanguage());
		FinishLocalizationTest();
		return;
	}
	const auto Key = [PC](FKey InKey, bool bDown) {
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(InKey, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f));
	};
	FString Code;
	FParse::Value(FCommandLine::Get(), TEXT("ShiftLanguage="), Code);
	int32 Target = 0;
	for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index) { if (Code == ShiftLoc::LanguageCode(Index)) { Target = Index; } }
	const FKey Numbers[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	const auto Screenshot = [this, &Code](const FString& Name) {
		const FString Dir = FPaths::ProjectSavedDir() / TEXT("Localization") / Code;
		IFileManager::Get().MakeDirectory(*Dir, true);
		FScreenshotRequest::RequestScreenshot(Dir / (Name + TEXT(".png")), true, false);
	};
	switch (LocalizationStep++)
	{
	case 0:
	{
		FString Error;
		CheckLocalization(TEXT("packaged_catalogues"), ShiftLoc::Validate(Error));
		CheckLocalization(TEXT("language_order"), ShiftLoc::LanguageCount == 4
			&& ShiftLoc::LanguageCode(0) == TEXT("en") && ShiftLoc::LanguageCode(1) == TEXT("ja")
			&& ShiftLoc::LanguageCode(2) == TEXT("zh-Hant") && ShiftLoc::LanguageCode(3) == TEXT("zh-Hans"));
		AShiftHUD* HUD = Cast<AShiftHUD>(PC->GetHUD());
		CheckLocalization(TEXT("packaged_runtime_fonts"), HUD && HUD->HasRuntimeFonts());
		CheckLocalization(TEXT("language_before_title"), Phase == EShiftPhase::LanguageSelection && PC->bShowMouseCursor);
		CheckLocalization(TEXT("invalid_locale_preserves_selection"), !ShiftLoc::SetLanguage(-1, false) && ShiftLoc::CurrentLanguage() == Target);
		CheckLocalization(TEXT("unknown_text_fallback"), ShiftLoc::Text(TEXT("UNLISTED BRAND")) == TEXT("UNLISTED BRAND"));
		Key(EKeys::R, true);
		break;
	}
	case 1:
		CheckLocalization(TEXT("restart_cannot_bypass_picker"), Phase == EShiftPhase::LanguageSelection);
		Key(EKeys::R, false);
		Key(EKeys::Two, true);
		break;
	case 2:
		CheckLocalization(TEXT("number_2_selects_japanese"), SelectedLanguage == 1 && ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage()) == TEXT("ja"));
		Key(EKeys::Two, false);
		Key(EKeys::Up, true);
		break;
	case 3:
		CheckLocalization(TEXT("arrow_selects_english"), SelectedLanguage == 0);
		Key(EKeys::Up, false);
		Key(Numbers[Target], true);
		break;
	case 4:
		CheckLocalization(TEXT("target_language_selected"), SelectedLanguage == Target && ShiftLoc::CurrentLanguage() == Target);
		Key(Numbers[Target], false);
		Screenshot(TEXT("Language"));
		break;
	case 5:
		Key(EKeys::Enter, true);
		break;
	case 6:
	{
		CheckLocalization(TEXT("confirm_opens_title"), Phase == EShiftPhase::Title && !PC->bShowMouseCursor);
		Key(EKeys::Enter, false);
		FConfigFile Persisted;
		Persisted.Read(ShiftLoc::SettingsFile());
		FString PersistedCode;
		Persisted.GetString(TEXT("ShiftStore.Language"), TEXT("Code"), PersistedCode);
		CheckLocalization(TEXT("language_written_to_settings"), PersistedCode == ShiftLoc::LanguageCode(Target));
		Screenshot(TEXT("Title"));
		break;
	}
	case 7:
		Key(EKeys::L, true);
		break;
	case 8:
		CheckLocalization(TEXT("title_can_reopen_picker"), Phase == EShiftPhase::LanguageSelection && SelectedLanguage == Target);
		Key(EKeys::L, false);
		Key(EKeys::Enter, true);
		break;
	case 9:
		Key(EKeys::Enter, false);
		Key(EKeys::E, true);
		break;
	case 10:
		CheckLocalization(TEXT("start_opens_night_intro"), Phase == EShiftPhase::NightIntro);
		CheckLocalization(TEXT("new_game_is_night_one"), NightIndex == 0 && Income == 0.f && Losses == 0.f);
		CheckLocalization(TEXT("reading_intro_keeps_full_task_budget"), !ActiveTasks.IsEmpty() && FMath::IsNearlyEqual(ActiveTasks[0]->TimeLeft, CurrentNightSpec().TaskTimeLimit));
		Key(EKeys::E, false);
		Screenshot(TEXT("NightIntro"));
		break;
	case 11:
		Key(EKeys::E, true);
		break;
	case 12:
		CheckLocalization(TEXT("intro_enters_gameplay"), Phase == EShiftPhase::Working && !Player->HeldItem.IsValid());
		Key(EKeys::E, false);
		Screenshot(TEXT("HUD"));
		break;
	case 13:
	{
		Player->SetActorLocation(FVector(800.f, 180.f, 96.f));
		PC->SetControlRotation(FRotator::ZeroRotator);
		AShiftCustomer* Customer = GetWorld()->SpawnActor<AShiftCustomer>(FVector(1050.f, 180.f, 88.f), FRotator::ZeroRotator);
		if (Customer)
		{
			Customer->Init(ShiftData::Nights()[0].Customers[1]);
			Customer->State = ECustomerState::WaitingForHelp;
			Customer->Speech->SetText(FText::FromString(ShiftLoc::Format(TEXT("[Need: %s]"), {ShiftData::ProductLabel(Customer->Spec.WantedSku)})));
			ActiveCustomer = Customer;
			BannerTimer = 0.f;
		}
		break;
	}
	case 14:
		CheckLocalization(TEXT("localized_customer_dialogue"), ActiveCustomer.IsValid() && ActiveCustomer->Speech->Text.ToString().Contains(ShiftLoc::Text(TEXT("MIDNIGHT Noodle Cup"))));
		CheckLocalization(TEXT("customer_subtitle_visible"), ActiveCustomer.IsValid() && PC->LineOfSightTo(ActiveCustomer.Get()));
		Screenshot(TEXT("Customer"));
		break;
	case 15:
		Phase = EShiftPhase::GameOver;
		GameOverReason = TEXT("Your sanity collapsed during the night shift.");
		Screenshot(TEXT("GameOver"));
		break;
	case 16:
		Phase = EShiftPhase::Dawn;
		Screenshot(TEXT("Dawn"));
		break;
	case 17:
		Phase = EShiftPhase::Ending;
		Screenshot(TEXT("Ending"));
		break;
	case 18:
		StartNight(1);
		Phase = EShiftPhase::Working;
		break;
	case 19:
	{
		FShiftTaskGuidance Guide;
		CheckLocalization(TEXT("restock_take_step"), GetTaskGuidance(Player, Guide) && Guide.Action == EShiftGuideAction::CollectCarton);
		Screenshot(TEXT("RestockTake"));
		break;
	}
	case 20:
	{
		AShiftItem* Carton = GetWorld()->SpawnActor<AShiftItem>(Player->GetActorLocation(), FRotator::ZeroRotator);
		Carton->InitKind(EShiftItemKind::Carton);
		Player->PickupItem(Carton);
		break;
	}
	case 21:
	{
		FShiftTaskGuidance Guide;
		CheckLocalization(TEXT("restock_return_step"), GetTaskGuidance(Player, Guide) && Guide.Action == EShiftGuideAction::DeliverCarton);
		Screenshot(TEXT("RestockReturn"));
		break;
	}
	case 22:
		CycleGuidedTask();
		break;
	case 23:
		Screenshot(TEXT("TaskSwitch"));
		break;
	case 24:
	{
		StartNight(0);
		ClearNightActors();
		Phase = EShiftPhase::Working;
		Player->SetActorLocation(FVector(650.f, 170.f, 100.f));
		AShiftCustomer* Customer = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::CheckoutCustomerSpot, FRotator(0,-90,0));
		Customer->Init(ShiftData::Nights()[0].Customers[0]);
		Customer->State = ECustomerState::WaitingAtCheckout;
		ActiveCustomer = Customer;
		NextCustomerIdx = 1;
		PC->SetControlRotation((Customer->GetActorLocation() + FVector(0,0,60) - Player->FirstPersonCamera->GetComponentLocation()).Rotation());
		OnCheckoutCompleted(Customer);
		break;
	}
	case 25:
		if (ActiveCustomer.IsValid())
		{
			PC->SetControlRotation((ActiveCustomer->GetActorLocation() + FVector(0,0,60) - Player->FirstPersonCamera->GetComponentLocation()).Rotation());
		}
		Screenshot(TEXT("DepartureEarly"));
		break;
	case 26:
		if (ActiveCustomer.IsValid())
		{
			PC->SetControlRotation((ActiveCustomer->GetActorLocation() + FVector(0,0,60) - Player->FirstPersonCamera->GetComponentLocation()).Rotation());
		}
		Screenshot(TEXT("DepartureDoorway"));
		break;
	case 27:
	case 28:
		break;
	case 29:
	{
		CheckLocalization(TEXT("normal_customer_exits_after_payment"), !ActiveCustomer.IsValid() && CustomersServed == 1);
		CheckLocalization(TEXT("service_creates_one_tidy_task"), ActiveTasks.Num() == 1 && ActiveTasks[0]->Type == EShiftTaskType::Tidy);
		if (!ActiveTasks.IsEmpty()) { OnTaskCompleted(ActiveTasks[0].Get()); }
		AShiftCustomer* Second = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::CheckoutCustomerSpot, FRotator::ZeroRotator);
		Second->Init(ShiftData::Nights()[0].Customers[0]);
		Second->State = ECustomerState::WaitingAtCheckout;
		ActiveCustomer = Second;
		NextCustomerIdx = 2;
		OnCheckoutCompleted(Second);
		OnCustomerFinished(Second);
		Second->Destroy();
		break;
	}
	case 30:
		CheckLocalization(TEXT("service_creates_one_oden_task"), ActiveTasks.Num() == 1 && ActiveTasks[0]->Type == EShiftTaskType::CookOden);
		PC->SetControlRotation(FRotator(0.f, 15.f, 0.f));
		Screenshot(TEXT("CookOden"));
		break;
	default:
		FinishLocalizationTest();
		break;
	}
}

void AShiftStoreGameMode::FinishLocalizationTest()
{
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	TSharedRef<FJsonObject> Checks = MakeShared<FJsonObject>();
	bool bPassed = !LocalizationChecks.IsEmpty();
	for (const auto& Pair : LocalizationChecks) { Checks->SetBoolField(Pair.Key, Pair.Value); bPassed &= Pair.Value; }
	Report->SetBoolField(TEXT("passed"), bPassed);
	Report->SetStringField(TEXT("language"), ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage()));
	Report->SetStringField(TEXT("settings_file"), ShiftLoc::SettingsFile());
	Report->SetObjectField(TEXT("checks"), Checks);
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Localization") / ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage());
	IFileManager::Get().MakeDirectory(*Dir, true);
	FFileHelper::SaveStringToFile(Json, *(Dir / TEXT("report.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FPlatformMisc::RequestExitWithStatus(!bPassed, bPassed ? 0 : 1);
}
