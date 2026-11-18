#include "ShiftStoreGameMode.h"
#include "ShiftCustomer.h"
#include "ShiftDirector.h"
#include "ShiftItem.h"
#include "ShiftSaveGame.h"
#include "ShiftTaskTarget.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftGuidance.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Components/RectLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
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
	if (SelfTestElapsed > 40.f)
	{
		FinishSelfTest(false, TEXT("selftest_timeout"));
		return;
	}

	switch (SelfTestStage)
	{
	case ESelfTestStage::Setup:
	{
		if (!CheckSelfTest(TEXT("director_spawned"), IsValid(Director))) { return; }
		// A real night-two checkpoint must not turn the primary title action into an implicit resume.
		Income = 12.5f;
		Losses = 3.f;
		StartNight(1);
		if (!CheckSelfTest(TEXT("resume_fixture_saved"), SaveProgress())) { return; }
		bHasSavedProgress = true;
		Phase = EShiftPhase::Title;
		OnAdvanceScreenKey();
		if (!CheckSelfTest(TEXT("new_game_starts_night_one"), NightIndex == 0 && Income == 0.f && Losses == 0.f)) { return; }
		const UShiftSaveGame* Existing = Cast<UShiftSaveGame>(UGameplayStatics::LoadGameFromSlot(ProgressSlotName(), 0));
		if (!CheckSelfTest(TEXT("new_game_preserves_previous_checkpoint"), Existing && Existing->SavedNightIndex == 1 && Existing->CumulativeIncome == 12.5f)) { return; }
		Phase = EShiftPhase::Title;
		ContinueSavedGame();
		if (!CheckSelfTest(TEXT("explicit_continue_restores_checkpoint"), Phase == EShiftPhase::NightIntro && NightIndex == 1 && Income == 12.5f && Losses == 3.f)) { return; }
		StartNight(0);
		AdvanceSelfTest(ESelfTestStage::IntroPause);
		break;
	}
	case ESelfTestStage::IntroPause:
		if (SelfTestStageTime > 0.35f)
		{
			if (!CheckSelfTest(TEXT("intro_pauses_chore_timers"), ActiveTasks.Num() > 0 && FMath::IsNearlyEqual(ActiveTasks[0]->TimeLeft, CurrentNightSpec().TaskTimeLimit))) { return; }
			Phase = EShiftPhase::Working;
			AdvanceSelfTest(ESelfTestStage::ActiveTimer);
		}
		break;
	case ESelfTestStage::ActiveTimer:
		if (SelfTestStageTime > 0.35f)
		{
			if (!CheckSelfTest(TEXT("working_starts_chore_timers"), ActiveTasks[0]->TimeLeft < CurrentNightSpec().TaskTimeLimit - 0.2f)) { return; }
			const auto& Nights = ShiftData::Nights();
			if (!CheckSelfTest(TEXT("early_nights_have_learning_time"), Nights[0].TaskTimeLimit >= 240.f && Nights[0].TaskTimeLimit > Nights[1].TaskTimeLimit && Nights[1].TaskTimeLimit > Nights[2].TaskTimeLimit && Nights[2].TaskTimeLimit >= 120.f)) { return; }
			StartNight(1);
			Phase = EShiftPhase::Working;
			AShiftPlayerCharacter* Player = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
			FShiftTaskGuidance Guide;
			if (!CheckSelfTest(TEXT("restock_guides_to_warehouse"), GetTaskGuidance(Player, Guide) && Guide.Action == EShiftGuideAction::CollectCarton && Guide.Target == CartonSource)) { return; }
			const TArray<FVector> Route = ShiftGuidance::FindRoute(GetWorld(), Player, Guide.Target.Get(), Guide.Location).Waypoints;
			if (!CheckSelfTest(TEXT("warehouse_route_uses_walkable_aisles"), Route.Num() >= 3 && FVector::Dist2D(Route.Last(), Guide.Location) < 160.f)) { return; }
			FVector PreviousPoint = Player->GetActorLocation();
			for (const FVector& Point : Route)
			{
				if (!CheckSelfTest(TEXT("guide_route_capsule_clear"), ShiftGuidance::CanWalkSegment(GetWorld(), Player, Guide.Target.Get(), PreviousPoint, Point))) { return; }
				PreviousPoint = Point;
			}
			AShiftItem* Carton = GetWorld()->SpawnActor<AShiftItem>(Player->GetActorLocation(), FRotator::ZeroRotator);
			Carton->InitKind(EShiftItemKind::Carton);
			Player->PickupItem(Carton);
			if (!CheckSelfTest(TEXT("carton_pickup_switches_to_restock_shelf"), GetTaskGuidance(Player, Guide) && Guide.Action == EShiftGuideAction::DeliverCarton && Guide.Target == ActiveTasks[0])) { return; }
			Player->ConsumeHeldItem();
			CycleGuidedTask();
			if (!CheckSelfTest(TEXT("guide_can_cycle_chores"), GetTaskGuidance(Player, Guide) && Guide.Action == EShiftGuideAction::Work && Guide.Target == ActiveTasks[1])) { return; }
			CycleGuidedTask();
			SelfTestRestockTarget = ActiveTasks[0];
			Player->SetActorLocation(Route.Last());
			Player->GetController()->SetControlRotation((CartonSource->GetActorLocation() - Player->FirstPersonCamera->GetComponentLocation()).Rotation());
			bSelfTestInteractPressed = false;
			AdvanceSelfTest(ESelfTestStage::RestockPickup);
		}
		break;
	case ESelfTestStage::RestockPickup:
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		AShiftPlayerCharacter* Player = PC ? Cast<AShiftPlayerCharacter>(PC->GetPawn()) : nullptr;
		if (!CheckSelfTest(TEXT("interaction_player_ready"), Player && PC)) { return; }
		if (!bSelfTestInteractPressed && SelfTestStageTime > 0.1f)
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Pressed, 1.f));
			bSelfTestInteractPressed = true;
		}
		if (SelfTestStageTime > 0.4f)
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Released, 0.f));
			if (!CheckSelfTest(TEXT("real_E_picks_up_stock_carton"), Player->HeldItem.IsValid() && Player->HeldItem->Kind == EShiftItemKind::Carton)) { return; }
			FShiftTaskGuidance Guide;
			if (!CheckSelfTest(TEXT("real_pickup_changes_guide_target"), GetTaskGuidance(Player, Guide) && Guide.Action == EShiftGuideAction::DeliverCarton)) { return; }
			const TArray<FVector> Route = ShiftGuidance::FindRoute(GetWorld(), Player, Guide.Target.Get(), Guide.Location).Waypoints;
			if (!CheckSelfTest(TEXT("return_to_shelf_route_exists"), !Route.IsEmpty())) { return; }
			Player->SetActorLocation(Route.Last());
			PC->SetControlRotation((Guide.Location - Player->FirstPersonCamera->GetComponentLocation()).Rotation());
			bSelfTestInteractPressed = false;
			AdvanceSelfTest(ESelfTestStage::RestockWork);
		}
		break;
	}
	case ESelfTestStage::RestockWork:
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		AShiftPlayerCharacter* Player = Cast<AShiftPlayerCharacter>(PC->GetPawn());
		if (!bSelfTestInteractPressed && SelfTestStageTime > 0.15f)
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Pressed, 1.f));
			bSelfTestInteractPressed = true;
		}
		if (!SelfTestRestockTarget.IsValid())
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Released, 0.f));
			if (!CheckSelfTest(TEXT("real_hold_E_completes_restock"), !Player->HeldItem.IsValid() && ActiveTasks.Num() == 1)) { return; }
			AdvanceSelfTest(ESelfTestStage::ServiceSetup);
		}
		else if (SelfTestStageTime > 3.f) { FinishSelfTest(false, TEXT("real_hold_E_completes_restock")); }
		break;
	}
	case ESelfTestStage::ServiceSetup:
	{
		StartNight(1);
		ClearNightActors();
		Phase = EShiftPhase::Working;
		Income = 0.f;
		AShiftPlayerCharacter* Player = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		Player->SetActorLocation(FVector(600.f, 280.f, 100.f));
		AShiftCustomer* Customer = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::CheckoutCustomerSpot, FRotator(0,-90,0));
		if (!CheckSelfTest(TEXT("checkout_customer_spawned"), Customer != nullptr)) { return; }
		Customer->Init(ShiftData::Nights()[0].Customers[0]);
		Customer->State = ECustomerState::WaitingAtCheckout;
		ActiveCustomer = Customer;
		NextCustomerIdx = 1;
		OnCheckoutCompleted(Customer);
		const float PaidIncome = Income;
		OnCheckoutCompleted(Customer);
		if (!CheckSelfTest(TEXT("checkout_cannot_charge_twice"), Income == PaidIncome && Customer->State == ECustomerState::Leaving)) { return; }
		SelfTestDepartingCustomer = Customer;
		SelfTestExitLastLocation = Customer->GetActorLocation();
		SelfTestExitLastYaw = Customer->GetActorRotation().Yaw;
		SelfTestExitMaxYawStep = 0.f;
		AdvanceSelfTest(ESelfTestStage::CustomerExit);
		break;
	}
	case ESelfTestStage::CustomerExit:
	{
		if (AShiftCustomer* Customer = SelfTestDepartingCustomer.Get())
		{
			SelfTestExitLastLocation = Customer->GetActorLocation();
			const float Yaw = Customer->GetActorRotation().Yaw;
			SelfTestExitMaxYawStep = FMath::Max(SelfTestExitMaxYawStep, FMath::Abs(FMath::FindDeltaAngleDegrees(SelfTestExitLastYaw, Yaw)));
			SelfTestExitLastYaw = Yaw;
			if (SelfTestStageTime > 8.f) { FinishSelfTest(false, TEXT("paid_customer_walks_out_without_timeout")); }
			break;
		}
		SelfTestExitSeconds = SelfTestStageTime;
		if (!CheckSelfTest(TEXT("paid_customer_walks_out_without_timeout"), SelfTestExitLastLocation.Y < -150.f && CustomersServed == 1 && !ActiveCustomer.IsValid())) { return; }
		if (!CheckSelfTest(TEXT("departure_has_no_yaw_snapping"), SelfTestExitMaxYawStep < 30.f)) { return; }
		if (!CheckSelfTest(TEXT("first_service_assigns_only_tidy"), ActiveTasks.Num() == 1 && ActiveTasks[0]->Type == EShiftTaskType::Tidy)) { return; }
		OnTaskCompleted(ActiveTasks[0].Get());
		AShiftCustomer* Second = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::CheckoutCustomerSpot, FRotator::ZeroRotator);
		Second->Init(ShiftData::Nights()[0].Customers[0]);
		Second->State = ECustomerState::WaitingAtCheckout;
		ActiveCustomer = Second;
		NextCustomerIdx = 2;
		OnCheckoutCompleted(Second);
		OnCustomerFinished(Second);
		OnCustomerFinished(Second);
		Second->Destroy();
		if (!CheckSelfTest(TEXT("service_finish_callback_is_idempotent"), CustomersServed == 2)) { return; }
		if (!CheckSelfTest(TEXT("second_service_assigns_only_oden"), ActiveTasks.Num() == 1 && ActiveTasks[0]->Type == EShiftTaskType::CookOden)) { return; }
		SelfTestCookTarget = ActiveTasks[0];
		AActor* OdenStation = nullptr;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("Interactive.Oden"))) { OdenStation = *It; break; }
		}
		FVector OdenOrigin = FVector::ZeroVector, OdenExtent = FVector::ZeroVector;
		if (OdenStation) { OdenStation->GetActorBounds(false, OdenOrigin, OdenExtent); }
		if (!CheckSelfTest(TEXT("oden_interaction_above_glass_cover"), OdenStation
			&& FVector::Dist2D(SelfTestCookTarget->GetActorLocation(), OdenOrigin) < 1.f
			&& SelfTestCookTarget->GetActorLocation().Z - SelfTestCookTarget->Hitbox->GetScaledSphereRadius() > OdenOrigin.Z + OdenExtent.Z)) { return; }
		AShiftPlayerCharacter* Player = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		const FVector CustomerView(OdenOrigin.X, OdenOrigin.Y + 120.f, 166.f);
		const FVector CustomerRay = (SelfTestCookTarget->GetActorLocation() - CustomerView).GetSafeNormal();
		FHitResult OdenHit;
		FCollisionQueryParams OdenQuery(TEXT("OdenCustomerInteraction"), false, Player);
		const bool bOdenHit = GetWorld()->SweepSingleByChannel(OdenHit, CustomerView, CustomerView + CustomerRay * 265.f,
			FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(14.f), OdenQuery);
		if (!CheckSelfTest(TEXT("oden_interaction_clear_from_customer_side"), bOdenHit && OdenHit.GetActor() == SelfTestCookTarget.Get())) { return; }
		Player->SetActorLocation(FVector(1250.f, 50.f, 100.f));
		const FShiftRouteResult CookRoute = ShiftGuidance::FindRoute(GetWorld(), Player, SelfTestCookTarget.Get(), SelfTestCookTarget->GetActorLocation());
		if (!CheckSelfTest(TEXT("oden_route_from_cashier_side_exists"), CookRoute.Status != EShiftRouteStatus::Unreachable)) { return; }
		if (!CookRoute.Waypoints.IsEmpty()) { Player->SetActorLocation(CookRoute.Waypoints.Last()); }
		Player->GetController()->SetControlRotation((SelfTestCookTarget->GetActorLocation() - Player->FirstPersonCamera->GetComponentLocation()).Rotation());
		bSelfTestInteractPressed = false;
		AdvanceSelfTest(ESelfTestStage::CookService);
		break;
	}
	case ESelfTestStage::CookService:
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (!bSelfTestInteractPressed && SelfTestStageTime > 0.15f)
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Pressed, 1.f));
			bSelfTestInteractPressed = true;
		}
		if (SelfTestCookTarget.IsValid())
		{
			if (SelfTestStageTime > 8.f) { FinishSelfTest(false, TEXT("real_hold_E_prepares_oden")); }
			break;
		}
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Released, 0.f));
		if (!CheckSelfTest(TEXT("real_hold_E_prepares_oden"), ActiveTasks.IsEmpty())) { return; }
		AShiftCustomer* Third = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::CheckoutCustomerSpot, FRotator::ZeroRotator);
		Third->Init(ShiftData::Nights()[0].Customers[0]);
		Third->State = ECustomerState::WaitingAtCheckout;
		ActiveCustomer = Third;
		NextCustomerIdx = 3;
		OnCheckoutCompleted(Third);
		OnCustomerFinished(Third);
		Third->Destroy();
		if (!CheckSelfTest(TEXT("third_paid_sale_assigns_one_batch_restock"), ActiveTasks.Num() == 1 && ActiveTasks[0]->Type == EShiftTaskType::Restock)) { return; }
		OnTaskCompleted(ActiveTasks[0].Get());
		AShiftCustomer* Last = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::CheckoutCustomerSpot, FRotator::ZeroRotator);
		Last->Init(ShiftData::Nights()[0].Customers[0]);
		Last->State = ECustomerState::WaitingAtCheckout;
		ActiveCustomer = Last;
		NextCustomerIdx = CurrentNightSpec().Customers.Num();
		OnCheckoutCompleted(Last);
		OnCustomerFinished(Last);
		Last->Destroy();
		const bool bOnlyCleanup = ActiveTasks.Num() == CurrentNightSpec().Spills
			&& !ActiveTasks.ContainsByPredicate([](const TWeakObjectPtr<AShiftTaskTarget>& Task) { return !Task.IsValid() || Task->Type != EShiftTaskType::Spill; });
		if (!CheckSelfTest(TEXT("last_customer_goes_directly_to_cleanup"), Phase == EShiftPhase::Cleanup && bOnlyCleanup)) { return; }
		AShiftPlayerCharacter* Player = CastChecked<AShiftPlayerCharacter>(PC->GetPawn());
		Player->SetActorLocation(FVector(600.f, 280.f, 100.f));
		PC->SetControlRotation(FRotator::ZeroRotator);
		// A thin shelf face 48 cm ahead reproduces the former 65 cm through-wall drop.
		AActor* Shelf = GetWorld()->SpawnActor<AActor>();
		UBoxComponent* Face = NewObject<UBoxComponent>(Shelf);
		Shelf->SetRootComponent(Face);
		Face->SetBoxExtent(FVector(2.f, 40.f, 35.f));
		Face->SetCollisionProfileName(TEXT("BlockAll"));
		Face->RegisterComponent();
		const FVector Eye = Player->FirstPersonCamera->GetComponentLocation();
		Shelf->SetActorLocation(Eye + FVector(48.f, 0.f, 0.f));
		Shelf->Tags.Add(TEXT("Fixture.Shelf"));
		Shelf->SetLifeSpan(1.f);
		AShiftItem* Water = GetWorld()->SpawnActor<AShiftItem>();
		Water->InitProduct(TEXT("Water"));
		Player->PickupItem(Water);
		// Align the camera component for the same-frame drop (control rotation updates on tick).
		Player->FirstPersonCamera->SetWorldRotation(FRotator::ZeroRotator);
		Player->DropHeldItem();
		if (!CheckSelfTest(TEXT("product_drop_stays_in_front_of_thin_shelf"), Water->GetActorLocation().X < Shelf->GetActorLocation().X - 2.f)) { return; }
		while (!ActiveTasks.IsEmpty()) { OnTaskCompleted(ActiveTasks.Last().Get()); }
		if (!CheckSelfTest(TEXT("loose_stock_blocks_dawn"), Phase == EShiftPhase::Cleanup && CountLooseMerchandise() == 1)) { return; }
		Player->PickupItem(Water);
		if (!CheckSelfTest(TEXT("held_stock_still_blocks_dawn"), !TryFinishCleanup())) { return; }
		bSelfTestInteractPressed = false;
		AdvanceSelfTest(ESelfTestStage::ReturnStock);
		break;
	}
	case ESelfTestStage::ReturnStock:
	{
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (!bSelfTestInteractPressed && SelfTestStageTime > 0.15f)
		{
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Pressed, 1.f));
			bSelfTestInteractPressed = true;
		}
		if (SelfTestStageTime < 0.4f) { break; }
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E, IE_Released, 0.f));
		if (!CheckSelfTest(TEXT("real_E_returns_last_product_and_reaches_dawn"), CountLooseMerchandise() == 0 && TryFinishCleanup() && Phase == EShiftPhase::Dawn)) { return; }
		AdvanceSelfTest(ESelfTestStage::Regression);
		break;
	}
	case ESelfTestStage::Regression:
	{
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
	Report->SetNumberField(TEXT("normal_customer_exit_seconds"), SelfTestExitSeconds);
	Report->SetNumberField(TEXT("normal_customer_max_yaw_step"), SelfTestExitMaxYawStep);
	Report->SetStringField(TEXT("normal_customer_last_exit_position"), SelfTestExitLastLocation.ToString());
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
