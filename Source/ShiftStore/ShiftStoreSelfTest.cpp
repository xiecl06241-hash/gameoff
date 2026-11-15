#include "ShiftStoreGameMode.h"
#include "ShiftCustomer.h"
#include "ShiftDirector.h"
#include "ShiftItem.h"
#include "ShiftSaveGame.h"
#include "ShiftTaskTarget.h"
#include "Components/RectLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/GarbageCollection.h"

void AShiftStoreGameMode::ShiftRunSelfTest()
{
	// The suite owns the world while running, so it must use a dedicated process.
	if (bSelfTestRunning || !FParse::Param(FCommandLine::Get(), TEXT("ShiftSelfTest")))
	{
		UE_LOG(LogTemp, Warning, TEXT("Run tests in a separate process with -ShiftSelfTest -ShiftSelfTestExit"));
		return;
	}
	bFormalSaveExistedBeforeSelfTest = UGameplayStatics::DoesSaveGameExist(PlayerSaveSlotName, 0);
	if (bFormalSaveExistedBeforeSelfTest && !UGameplayStatics::LoadDataFromSlot(FormalSaveBeforeSelfTest, PlayerSaveSlotName, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot snapshot the player save; refusing to start self-test"));
		FPlatformMisc::RequestExitWithStatus(true, 1);
		return;
	}
	SelfTestSaveSlotName = TEXT("ShiftStoreSelfTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	bSelfTestRunning = true;
	SelfTestElapsed = 0.f;
	SelfTestChecks.Empty();
	AdvanceSelfTest(ESelfTestStage::Setup);
}

void AShiftStoreGameMode::AdvanceSelfTest(ESelfTestStage Stage)
{
	SelfTestStage = Stage;
	SelfTestStageTime = 0.f;
}

bool AShiftStoreGameMode::CheckSelfTest(const FString& Name, bool bSuccess, const FString& Detail)
{
	SelfTestChecks.Add(Name, bSuccess);
	if (!bSuccess) { FinishSelfTest(false, Detail.IsEmpty() ? Name : Name + TEXT(": ") + Detail); }
	return bSuccess;
}

void AShiftStoreGameMode::TickSelfTest(float DeltaSeconds)
{
	SelfTestElapsed += DeltaSeconds;
	SelfTestStageTime += DeltaSeconds;
	if (SelfTestElapsed > 20.f)
	{
		FinishSelfTest(false, TEXT("selftest_timeout"));
		return;
	}

	switch (SelfTestStage)
	{
	case ESelfTestStage::Setup:
	{
		if (!CheckSelfTest(TEXT("director_spawned"), IsValid(Director))) { return; }
		Income = 0.f;
		Losses = 0.f;
		StartNight(0);
		Income = 12.f;
		Losses = ShiftData::LossLimit;
		TriggerGameOver(TEXT("First-night checkpoint regression test"));
		RestartCurrentNight();
		if (!CheckSelfTest(TEXT("first_night_restart"), NightIndex == 0 && Income == 0.f && Losses == 0.f && Sanity == ShiftData::MaxSanity)) { return; }

		Income = 12.5f;
		Losses = 3.f;
		StartNight(1);
		Income = 90.f;
		Losses = 35.f;
		ModifySanity(-ShiftData::MaxSanity);
		RestartCurrentNight();
		if (!CheckSelfTest(TEXT("later_night_restart"), NightIndex == 1 && Income == 12.5f && Losses == 3.f && Sanity == ShiftData::MaxSanity)) { return; }

		Income = 0.f;
		Losses = 0.f;
		StartNight(1);
		Phase = EShiftPhase::Working;
		SelfTestInitialTasks = ActiveTasks.Num();
		if (!CheckSelfTest(TEXT("opening_tasks"), SelfTestInitialTasks > 0)) { return; }
		ModifySanity(-25.f);
		const float BeforeRecovery = Sanity;
		while (!ActiveTasks.IsEmpty())
		{
			AShiftTaskTarget* Task = ActiveTasks[0].Get();
			if (!CheckSelfTest(TEXT("valid_task_targets"), IsValid(Task))) { return; }
			OnTaskCompleted(Task);
		}
		if (!CheckSelfTest(TEXT("sanity_recovery"), Sanity > BeforeRecovery && Sanity < ShiftData::MaxSanity)) { return; }
		TrySpawnNextCustomer();
		AShiftCustomer* Customer = ActiveCustomer.Get();
		if (!CheckSelfTest(TEXT("customer_spawned"), IsValid(Customer))) { return; }
		if (!CheckSelfTest(TEXT("cooked_customer_assets"), Customer->GetMesh()->GetSkeletalMeshAsset() && Customer->GetMesh()->GetAnimClass())) { return; }
		Customer->SetActorTickEnabled(false);
		Customer->SetActorLocation(FVector(800.f, 310.f, 100.f));
		Customer->State = ECustomerState::MonsterHunting;

		AShiftItem* Projectile = GetWorld()->SpawnActor<AShiftItem>(FVector(800.f, 180.f, 145.f), FRotator::ZeroRotator);
		if (!CheckSelfTest(TEXT("projectile_spawned"), IsValid(Projectile))) { return; }
		SelfTestProjectile = Projectile;
		Projectile->InitProduct(TEXT("Milk"));
		if (!CheckSelfTest(TEXT("projectile_collision"), Projectile->CanSimulate())) { return; }
		Projectile->Throw(FVector(0.f, 600.f, 0.f), UGameplayStatics::GetPlayerPawn(this, 0));
		AdvanceSelfTest(ESelfTestStage::Projectile);
		break;
	}
	case ESelfTestStage::Projectile:
	{
		AShiftCustomer* Customer = ActiveCustomer.Get();
		if (!CheckSelfTest(TEXT("projectile_target_alive"), IsValid(Customer))) { return; }
		if (Customer->State != ECustomerState::MonsterStunned)
		{
			if (SelfTestStageTime > 3.f) { FinishSelfTest(false, TEXT("physics_projectile_stun")); }
			return;
		}
		if (!CheckSelfTest(TEXT("physics_projectile_stun"), !SelfTestProjectile.IsValid())) { return; }
		const FShiftProduct* Milk = ShiftData::FindProduct(TEXT("Milk"));
		if (!CheckSelfTest(TEXT("ledger_cost"), Milk && FMath::IsNearlyEqual(Losses, Milk->Cost))) { return; }
		CollectGarbage(RF_NoFlags, true);
		if (!CheckSelfTest(TEXT("gc_retains_shift_assets"), FindObject<UStaticMesh>(nullptr, TEXT("/Game/ShiftStore/Meshes/SM_Sign_ShiftNotice.SM_Sign_ShiftNotice")) != nullptr)) { return; }

		const FShiftCustomerSpec* Monster = CurrentNightSpec().Customers.FindByPredicate([](const FShiftCustomerSpec& C) { return C.bMonster; });
		if (!CheckSelfTest(TEXT("monster_spec"), Monster != nullptr)) { return; }
		Customer->Init(*Monster);
		Customer->State = ECustomerState::WaitingForHelp;
		Customer->StateTimer = 4.3f;
		Customer->SetActorTickEnabled(true);
		AdvanceSelfTest(ESelfTestStage::Reveal);
		break;
	}
	case ESelfTestStage::Reveal:
		if (Director->IsShifted() && ActiveLostItem.IsValid())
		{
			SelfTestChecks.Add(TEXT("monster_reveal"), true);
			if (AShiftCustomer* C = ActiveCustomer.Get()) { C->SetActorTickEnabled(false); }
			AdvanceSelfTest(ESelfTestStage::Shift);
		}
		else if (SelfTestStageTime > 2.f) { FinishSelfTest(false, TEXT("monster_reveal")); }
		break;
	case ESelfTestStage::Shift:
		if (Director->Alpha() >= 1.f)
		{
			FString Failure;
			const bool bValid = Director->ValidateState(true, Failure);
			if (!CheckSelfTest(TEXT("shift_actual_transform"), bValid, Failure)) { return; }
			OnMonsterDemandDelivered(ActiveCustomer.Get());
			AdvanceSelfTest(ESelfTestStage::Restore);
		}
		break;
	case ESelfTestStage::Restore:
		if (Director->Alpha() <= 0.f)
		{
			FString Failure;
			const bool bValid = Director->ValidateState(false, Failure);
			if (!CheckSelfTest(TEXT("restore_actual_transform"), bValid, Failure)) { return; }
			if (!CheckSelfTest(TEXT("demand_item_removed"), !ActiveLostItem.IsValid())) { return; }
			// Force the last flicker frame dark, then let the real director tick restore it.
			Director->Flicker(0.001f);
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				TArray<URectLightComponent*> Lights;
				It->GetComponents(Lights);
				for (URectLightComponent* L : Lights) { L->SetIntensity(0.f); }
			}
			AdvanceSelfTest(ESelfTestStage::Flicker);
		}
		break;
	case ESelfTestStage::Flicker:
		if (SelfTestStageTime > 0.1f)
		{
			FString Failure;
			const bool bValid = Director->ValidateState(false, Failure);
			if (!CheckSelfTest(TEXT("flicker_restores_lights"), bValid, Failure)) { return; }
			if (!CheckSelfTest(TEXT("isolated_save_written"), SaveProgress())) { return; }
			const UShiftSaveGame* Saved = Cast<UShiftSaveGame>(UGameplayStatics::LoadGameFromSlot(ProgressSlotName(), 0));
			if (!CheckSelfTest(TEXT("isolated_save_roundtrip"), Saved && Saved->SavedNightIndex == NightIndex && Saved->CumulativeIncome == Income && Saved->CumulativeLosses == Losses)) { return; }
			FinishSelfTest(true);
		}
		break;
	default:
		break;
	}
}

void AShiftStoreGameMode::FinishSelfTest(bool bSuccess, const FString& Failure)
{
	if (!Failure.IsEmpty()) { SelfTestChecks.Add(Failure, false); }
	TArray<uint8> FormalSaveAfter;
	const bool bSaveStillExists = UGameplayStatics::DoesSaveGameExist(PlayerSaveSlotName, 0);
	const bool bSamePlayerSave = bSaveStillExists == bFormalSaveExistedBeforeSelfTest &&
		(!bSaveStillExists || (UGameplayStatics::LoadDataFromSlot(FormalSaveAfter, PlayerSaveSlotName, 0) && FormalSaveAfter == FormalSaveBeforeSelfTest));
	SelfTestChecks.Add(TEXT("formal_save_unchanged"), bSamePlayerSave);
	if (UGameplayStatics::DoesSaveGameExist(SelfTestSaveSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(SelfTestSaveSlotName, 0);
	}
	SelfTestChecks.Add(TEXT("test_slot_cleaned"), !UGameplayStatics::DoesSaveGameExist(SelfTestSaveSlotName, 0));
	if (FParse::Param(FCommandLine::Get(), TEXT("ShiftSelfTestForceFailure")))
	{
		SelfTestChecks.Add(TEXT("forced_failure"), false);
	}
	TSharedRef<FJsonObject> Checks = MakeShared<FJsonObject>();
	FString FailureReason = Failure;
	for (const auto& Check : SelfTestChecks)
	{
		Checks->SetBoolField(Check.Key, Check.Value);
		bSuccess &= Check.Value;
		if (!Check.Value && FailureReason.IsEmpty()) { FailureReason = Check.Key; }
	}
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("passed"), bSuccess);
	Report->SetNumberField(TEXT("initial_tasks"), SelfTestInitialTasks);
	Report->SetObjectField(TEXT("checks"), Checks);
	Report->SetStringField(TEXT("test_save_slot"), SelfTestSaveSlotName);
	Report->SetStringField(TEXT("failure"), FailureReason);
	Report->SetNumberField(TEXT("elapsed_world_seconds"), SelfTestElapsed);
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	const bool bReportWritten = FFileHelper::SaveStringToFile(Json, *(FPaths::ProjectSavedDir() / TEXT("gameplay_selftest.json")));
	bSuccess &= bReportWritten;
	if (bSuccess) { UE_LOG(LogTemp, Display, TEXT("SHIFTSTORE_SELFTEST_PASSED: %s"), *Json); }
	else { UE_LOG(LogTemp, Error, TEXT("SHIFTSTORE_SELFTEST_FAILED: %s"), *Json); }

	if (AShiftItem* Projectile = SelfTestProjectile.Get()) { Projectile->Destroy(); }
	ClearNightActors();
	bSelfTestRunning = false;
	SelfTestStage = ESelfTestStage::Finished;
	bHasNightCheckpoint = false;
	LoadProgress();
	Phase = EShiftPhase::Title;
	if (Director) { Director->SetShifted(false); }
	if (FParse::Param(FCommandLine::Get(), TEXT("ShiftSelfTestExit")))
	{
		// Windows' normal game-loop shutdown discards PostQuitMessage's status.
		// The isolated failure process exits explicitly after its report and cleanup.
		FPlatformMisc::RequestExitWithStatus(!bSuccess, bSuccess ? 0 : 1);
	}
}
