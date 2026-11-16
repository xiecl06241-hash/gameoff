#include "ShiftFeedbackTest.h"
#include "ShiftStoreGameMode.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftGuideActor.h"
#include "ShiftDirector.h"
#include "ShiftItem.h"
#include "ShiftTaskTarget.h"
#include "ShiftLocalization.h"
#include "ShiftHUD.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

AShiftFeedbackTest::AShiftFeedbackTest()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AShiftFeedbackTest::Check(const FString& Name, bool bPassed)
{
	Checks.Add(Name, bPassed);
	UE_LOG(LogTemp, Display, TEXT("FEEDBACK_CHECK %s = %d"), *Name, bPassed);
}

void AShiftFeedbackTest::Screenshot(const FString& Name)
{
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Feedback") / ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage());
	IFileManager::Get().MakeDirectory(*Dir, true);
	FScreenshotRequest::RequestScreenshot(Dir / (Name + TEXT(".png")), true, false);
}

void AShiftFeedbackTest::SetupBlockedCustomer()
{
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AShiftPlayerCharacter* Clerk = Cast<AShiftPlayerCharacter>(PC->GetPawn());
	if (Customer.IsValid()) { Customer->Destroy(); }
	Clerk->SetActorLocation(FVector(900.f, 50.f, 100.f));
	PC->SetControlRotation(FRotator(0.f, 180.f, 0.f));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Customer = GetWorld()->SpawnActor<AShiftCustomer>(FVector(800.f, 50.f, 100.f), FRotator::ZeroRotator, Spawn);
	Customer->Init(ShiftData::Nights()[0].Customers[0]);
	const ECustomerState States[] = {ECustomerState::Entering, ECustomerState::Browsing, ECustomerState::WalkingToCheckout, ECustomerState::Leaving};
	Customer->State = States[BlockCase];
	Customer->TargetPos = FVector(1150.f, 50.f, 100.f);
	Customer->StateTimer = 6.4f;
	SavedState = Customer->State;
	SavedLocation = Customer->GetActorLocation();
	GM->ActiveCustomer = Customer;
}

void AShiftFeedbackTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Now = FPlatformTime::Seconds();
	if (StartedAt == 0.) { StartedAt = Now; NextStepTime = Now + 1.; }
	if (Now - StartedAt > 75.) { Check(TEXT("probe_timeout"), false); Finish(); return; }
	if (Now < NextStepTime) { return; }
	NextStepTime = Now + 0.35;
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AShiftPlayerCharacter* Clerk = PC ? Cast<AShiftPlayerCharacter>(PC->GetPawn()) : nullptr;
	AShiftHUD* HUD = PC ? Cast<AShiftHUD>(PC->GetHUD()) : nullptr;
	if (!GM || !Clerk || !GM->GuideRenderer || !HUD) { Check(TEXT("probe_world_ready"), false); Finish(); return; }
	AShiftGuideActor* Guide = GM->GuideRenderer;
	const auto Key = [PC](FKey InKey, bool bDown) {
		PC->InputKey(FInputKeyEventArgs::CreateSimulated(InKey, bDown ? IE_Pressed : IE_Released, bDown ? 1.f : 0.f));
	};
	const auto ClearTasks = [GM]() {
		for (const TWeakObjectPtr<AShiftTaskTarget>& Task : GM->ActiveTasks) { if (Task.IsValid()) { Task->Destroy(); } }
		GM->ActiveTasks.Empty();
	};
	switch (Step++)
	{
	case 0:
		GM->Phase = EShiftPhase::Title;
		GM->OnAdvanceScreenKey();
		GM->OnAdvanceScreenKey();
		Check(TEXT("opening_subtitle_triggered"), GM->SubtitleText == TEXT("Let's get the store ready."));
		Check(TEXT("opening_subtitle_not_repeated"), !GM->RequestDialogue(EShiftDialogueEvent::WorkStart));
		NextStepTime = Now + 0.7;
		break;
	case 1:
	{
		FShiftTaskGuidance G;
		GM->GetTaskGuidance(Clerk, G);
		Check(TEXT("selected_task_shared_with_guide"), G.Task == GM->ActiveTasks[0] && Guide->Current.Task == G.Task);
		Check(TEXT("reachable_route_has_floor_arrows"), Guide->Status == EShiftRouteStatus::Reachable && Guide->Arrows->GetInstanceCount() > 0);
		Check(TEXT("floor_route_limited_to_five_metres"), Guide->FloorRouteLength > 0.f && Guide->FloorRouteLength <= 500.f);
		Check(TEXT("one_objective_row_per_task"), HUD->ObjectiveTaskRows == GM->ActiveTasks.Num());
		Check(TEXT("objective_panel_fits_viewport"), HUD->bObjectiveFitsViewport);
		Check(TEXT("subtitle_is_at_most_two_lines"), HUD->SubtitleLines > 0 && HUD->SubtitleLines <= 2);
		Screenshot(TEXT("Work"));
		PC->SetControlRotation(FRotator(0.f, -90.f, 0.f));
		break;
	}
	case 2:
		Screenshot(TEXT("Behind"));
		GM->StartNight(1);
		GM->Phase = EShiftPhase::Working;
		NextStepTime = Now + 0.7;
		break;
	case 3:
		Check(TEXT("restock_first_step_targets_carton_source"), Guide->Current.Action == EShiftGuideAction::CollectCarton && Guide->Current.Target == GM->CartonSource);
		if (Guide->Route.IsEmpty()) { Check(TEXT("warehouse_route_exists"), false); Finish(); return; }
		Clerk->SetActorLocation(Guide->Route.Last());
		PC->SetControlRotation((GM->CartonSource->GetActorLocation() - Clerk->FirstPersonCamera->GetComponentLocation()).Rotation());
		NextStepTime = Now + 0.7;
		break;
	case 4:
		Check(TEXT("arrival_replaces_arrows_with_target_highlight"), Guide->Status == EShiftRouteStatus::Arrived && Guide->Arrows->GetInstanceCount() == 0 && Guide->Highlight->GetInstanceCount() > 0);
		Screenshot(TEXT("Target"));
		Key(EKeys::E, true);
		break;
	case 5:
		Key(EKeys::E, false);
		Check(TEXT("real_pickup_takes_carton"), Clerk->HeldItem.IsValid() && Clerk->HeldItem->Kind == EShiftItemKind::Carton);
		NextStepTime = Now + 0.7;
		break;
	case 6:
		Check(TEXT("pickup_immediately_changes_destination"), Guide->Current.Action == EShiftGuideAction::DeliverCarton && Guide->Current.Target == Guide->Current.Task);
		Screenshot(TEXT("Deliver"));
		Item = Clerk->HeldItem;
		Clerk->DropHeldItem();
		NextStepTime = Now + 0.7;
		break;
	case 7:
		Check(TEXT("dropped_carton_restores_collection_step"), Guide->Current.Action == EShiftGuideAction::CollectCarton && Guide->Current.Target == GM->CartonSource);
		if (Item.IsValid()) { Item->Destroy(); }
		GM->CycleGuidedTask();
		NextStepTime = Now + 0.7;
		break;
	case 8:
		Check(TEXT("tab_selection_updates_task_and_route"), Guide->Current.Task == GM->ActiveTasks[1]);
		ClearTasks();
		{
			AShiftTaskTarget* Spill = GetWorld()->SpawnActor<AShiftTaskTarget>(FVector(800.f, 320.f, 40.f), FRotator::ZeroRotator);
			Spill->Setup(EShiftTaskType::Spill, ShiftLoc::Format(TEXT("Mop floor residue #%d"), {TEXT("1")}), 240.f);
			GM->ActiveTasks.Add(Spill);
		}
		NextStepTime = Now + 0.7;
		break;
	case 9:
		Check(TEXT("spill_guides_to_mop_first"), Guide->Current.Action == EShiftGuideAction::CollectMop);
		Item = GetWorld()->SpawnActor<AShiftItem>(Clerk->GetActorLocation(), FRotator::ZeroRotator);
		Item->InitKind(EShiftItemKind::Mop);
		Clerk->PickupItem(Item.Get());
		NextStepTime = Now + 0.7;
		break;
	case 10:
		Check(TEXT("held_mop_switches_to_spill"), Guide->Current.Action == EShiftGuideAction::MopFloor && Guide->Current.Target == Guide->Current.Task);
		Clerk->DropHeldItem();
		NextStepTime = Now + 0.7;
		break;
	case 11:
		Check(TEXT("dropped_mop_becomes_collection_target"), Guide->Current.Action == EShiftGuideAction::CollectMop && Guide->Current.Target == Item);
		if (Item.IsValid()) { Item->Destroy(); }
		GM->ActiveTasks[0]->SetActorLocation(FVector(0.f, 800.f, 160.f));
		Clerk->SetActorLocation(FVector(800.f, 300.f, 100.f));
		NextStepTime = Now + 0.7;
		break;
	case 12:
	{
		const FShiftRouteResult Invalid = ShiftGuidance::FindRoute(GetWorld(), Clerk, GM->ActiveTasks[0].Get(), GM->ActiveTasks[0]->GetActorLocation());
		Check(TEXT("unreachable_not_confused_with_arrival"), Invalid.Status == EShiftRouteStatus::Unreachable && Invalid.Waypoints.IsEmpty());
		// Use a work task so that the invalid task, rather than the mop source, is the destination.
		GM->ActiveTasks[0]->Setup(EShiftTaskType::Count, ShiftData::TaskSpots()[0].Label, 240.f);
		NextStepTime = Now + 0.7;
		break;
	}
	case 13:
		Check(TEXT("unreachable_has_no_misleading_arrows"), Guide->Status == EShiftRouteStatus::Unreachable && Guide->Route.IsEmpty() && Guide->Arrows->GetInstanceCount() == 0 && Guide->Highlight->GetInstanceCount() == 0);
		Screenshot(TEXT("BlockedRoute"));
		GM->StartNight(1);
		GM->Phase = EShiftPhase::Working;
		GM->Director->SetShifted(true);
		SavedPlanCount = Guide->ReplanCount;
		NextStepTime = Now + 1.6;
		break;
	case 14:
	{
		Check(TEXT("shift_layout_replans_without_spam"), Guide->ReplanCount > SavedPlanCount && Guide->ReplanCount - SavedPlanCount <= 4);
		Check(TEXT("shift_layout_route_exists"), Guide->Status == EShiftRouteStatus::Reachable && !Guide->Route.IsEmpty());
		FVector Previous = Clerk->GetActorLocation();
		bool bClear = true;
		for (const FVector& Point : Guide->Route) { bClear &= ShiftGuidance::CanWalkSegment(GetWorld(), Clerk, Guide->Current.Target.Get(), Previous, Point); Previous = Point; }
		Check(TEXT("shift_layout_route_does_not_cross_shelves"), bClear);
		Screenshot(TEXT("ShiftRoute"));
		GM->StartNight(0);
		GM->Phase = EShiftPhase::Working;
		ClearTasks();
		SetupBlockedCustomer();
		NextStepTime = Now + 1.15;
		break;
	}
	case 15:
	{
		const FString Prefix = FString::Printf(TEXT("block_state_%d_"), BlockCase);
		Check(Prefix + TEXT("stops_and_keeps_goal"), Customer->IsBlockedByPlayer() && Customer->State == SavedState && Customer->TargetPos.Equals(FVector(1150.f, 50.f, 100.f)));
		Check(Prefix + TEXT("stops_walk_animation"), Customer->GetVelocity().Size2D() < 1.f && FVector::Dist2D(Customer->GetActorLocation(), SavedLocation) < 3.f);
		Check(Prefix + TEXT("pauses_state_timeout"), FMath::IsNearlyEqual(Customer->StateTimer, 6.4f, 0.05f));
		Check(Prefix + TEXT("courtesy_once"), Customer->CourtesyLineCount() == 1 && Customer->Speech->Text.ToString() == TEXT("Excuse me, could I get through?"));
		if (BlockCase == 0) { Screenshot(TEXT("Courtesy")); }
		SavedCourtesyCount = Customer->CourtesyLineCount();
		NextStepTime = Now + 0.8;
		break;
	}
	case 16:
		Check(FString::Printf(TEXT("block_state_%d_no_courtesy_spam"), BlockCase), Customer->CourtesyLineCount() == SavedCourtesyCount);
		Clerk->SetActorLocation(FVector(600.f, 50.f, 100.f));
		NextStepTime = Now + 0.12;
		break;
	case 17:
		Check(FString::Printf(TEXT("block_state_%d_clear_hysteresis"), BlockCase), Customer->IsBlockedByPlayer());
		NextStepTime = Now + 0.45;
		break;
	case 18:
		Check(FString::Printf(TEXT("block_state_%d_resumes_original_route"), BlockCase), !Customer->IsBlockedByPlayer() && FVector::Dist2D(Customer->GetActorLocation(), SavedLocation) > 8.f);
		if (++BlockCase < 4) { SetupBlockedCustomer(); Step = 15; NextStepTime = Now + 1.15; }
		else
		{
			Customer->Destroy();
			GM->StartNight(0);
			GM->Phase = EShiftPhase::Working;
			Clerk->SetActorLocation(FVector(800.f, 130.f, 100.f));
			PC->SetControlRotation(FRotator(0.f, -90.f, 0.f));
			Customer = GetWorld()->SpawnActor<AShiftCustomer>(FVector(800.f, 210.f, 100.f), FRotator(0.f, -90.f, 0.f));
			if (!Customer.IsValid()) { Check(TEXT("grab_fixture_spawned"), false); Finish(); return; }
			FShiftCustomerSpec Spec = ShiftData::Nights()[1].Customers.Last();
			Spec.bMonster = true;
			Spec.MonsterSku = ShiftData::Products()[0].Sku;
			Customer->Init(Spec);
			Customer->State = ECustomerState::WaitingForHelp;
			Customer->StateTimer = 4.3f;
			GM->ActiveCustomer = Customer;
			SavedLocation = Clerk->GetActorLocation();
			NextStepTime = Now + 0.5;
		}
		break;
	case 19:
	{
		const FVector Focus = Customer->GrabFocusLocation();
		Check(TEXT("grab_focus_uses_head_bone"), Customer->GetMesh()->GetBoneIndex(TEXT("head")) != INDEX_NONE && Focus.Z > Customer->GetActorLocation().Z + 55.f);
		Check(TEXT("grab_fov_reaches_seventy"), FMath::IsNearlyEqual(Clerk->FirstPersonCamera->FieldOfView, 70.f, 0.1f));
		Check(TEXT("grab_camera_faces_head"), FVector::DotProduct(Clerk->FirstPersonCamera->GetForwardVector(), (Focus - Clerk->FirstPersonCamera->GetComponentLocation()).GetSafeNormal()) > 0.98f);
		Check(TEXT("grab_does_not_teleport_player_or_camera"), FVector::Dist2D(SavedLocation, Clerk->GetActorLocation()) < 0.1f && Clerk->FirstPersonCamera->GetRelativeLocation().Equals(FVector(0.f, 0.f, 66.f), 0.1f));
		Screenshot(TEXT("Grab"));
		Key(EKeys::E, true);
		NextStepTime = Now + 3.05;
		break;
	}
	case 20:
		Key(EKeys::E, false);
		Check(TEXT("hold_E_escapes_during_grab_camera"), !Clerk->IsGrabbed() && Clerk->FirstPersonCamera->FieldOfView == 85.f);
		Check(TEXT("escape_subtitle_triggered"), GM->SubtitleText == TEXT("I need some distance..."));
		GM->Sanity = 100.f;
		Customer->State = ECustomerState::MonsterGrabbing;
		Clerk->OnGrabbedByMonster(Customer.Get(), false);
		Item = GetWorld()->SpawnActor<AShiftItem>(Clerk->GetActorLocation(), FRotator::ZeroRotator);
		Item->InitProduct(ShiftData::Products()[0].Sku);
		Clerk->PickupItem(Item.Get());
		SavedLosses = GM->Losses;
		Key(EKeys::F, true);
		break;
	case 21:
		Key(EKeys::F, false);
		Check(TEXT("item_escape_during_camera_transition"), !Clerk->IsGrabbed() && Clerk->FirstPersonCamera->FieldOfView == 85.f && Customer->State == ECustomerState::MonsterStunned && GM->Losses > SavedLosses);
		GM->Sanity = 100.f;
		Customer->State = ECustomerState::MonsterGrabbing;
		Clerk->OnGrabbedByMonster(Customer.Get(), false);
		Key(EKeys::Escape, true);
		break;
	case 22:
		Key(EKeys::Escape, false);
		SavedFOV = Clerk->FirstPersonCamera->FieldOfView;
		SavedSanity = GM->Sanity;
		break;
	case 23:
		Check(TEXT("pause_freezes_grab_camera_and_sanity"), PC->IsPaused() && Clerk->FirstPersonCamera->FieldOfView == SavedFOV && GM->Sanity == SavedSanity);
		Key(EKeys::Escape, true);
		NextStepTime = Now + 0.5;
		break;
	case 24:
		Key(EKeys::Escape, false);
		Check(TEXT("resume_continues_grab_camera"), !PC->IsPaused() && FMath::IsNearlyEqual(Clerk->FirstPersonCamera->FieldOfView, 70.f, 0.1f));
		Customer->Destroy();
		break;
	case 25:
		Check(TEXT("destroyed_monster_restores_camera"), !Clerk->IsGrabbed() && Clerk->FirstPersonCamera->FieldOfView == 85.f);
		GM->StartNight(0);
		GM->Phase = EShiftPhase::Working;
		GM->RequestDialogue(EShiftDialogueEvent::WorkStart);
		Check(TEXT("urgent_dialogue_interrupts_work"), GM->RequestDialogue(EShiftDialogueEvent::Grabbed) && GM->SubtitleText == TEXT("Let go of me!"));
		Check(TEXT("lower_priority_dialogue_discarded"), !GM->RequestDialogue(EShiftDialogueEvent::LowSanity));
		Check(TEXT("same_event_cooldown"), !GM->RequestDialogue(EShiftDialogueEvent::Grabbed));
		Check(TEXT("escape_interrupts_grab_dialogue"), GM->RequestDialogue(EShiftDialogueEvent::Escaped));
		NextStepTime = Now + 3.2;
		break;
	case 26:
		Check(TEXT("expired_dialogue_has_no_backlog"), GM->SubtitleText.IsEmpty() && GM->SubtitleTime == 0.f);
		GM->ModifySanity(-66.f);
		Check(TEXT("low_sanity_crossing_triggers_once"), GM->SubtitleText == TEXT("Stay calm. Don't panic."));
		GM->SubtitleTime = 1.f;
		GM->ModifySanity(20.f);
		GM->ModifySanity(-25.f);
		Check(TEXT("low_sanity_recovery_does_not_repeat_line"), GM->SubtitleTime == 1.f);
		GM->StartNight(0);
		GM->Phase = EShiftPhase::Working;
		Customer = GetWorld()->SpawnActor<AShiftCustomer>(FVector(800.f, 240.f, 100.f), FRotator(0.f, -90.f, 0.f));
		if (!Customer.IsValid()) { Check(TEXT("reveal_fixture_spawned"), false); Finish(); return; }
		{
			FShiftCustomerSpec Spec = ShiftData::Nights()[1].Customers.Last();
			Spec.bMonster = true;
			Customer->Init(Spec);
			Customer->State = ECustomerState::MonsterStalking;
			GM->ActiveCustomer = Customer;
			GM->OnMonsterRevealed(Customer.Get(), false);
			Check(TEXT("unseen_reveal_has_no_spoiler_dialogue"), GM->SubtitleText.IsEmpty());
			GM->OnMonsterRevealed(Customer.Get(), true);
			Check(TEXT("witnessed_reveal_triggers_reaction"), GM->SubtitleText == TEXT("What... was that?"));
			Customer->State = ECustomerState::MonsterGrabbing;
			Clerk->OnGrabbedByMonster(Customer.Get(), false);
		}
		NextStepTime = Now + 0.4;
		break;
	case 27:
		GM->ModifySanity(-100.f);
		Check(TEXT("death_restores_grab_camera"), GM->Phase == EShiftPhase::GameOver && !Clerk->IsGrabbed() && Clerk->FirstPersonCamera->FieldOfView == 85.f && GM->SubtitleText.IsEmpty());
		GM->RestartCurrentNight();
		Check(TEXT("restart_clears_camera_and_dialogue"), Clerk->FirstPersonCamera->FieldOfView == 85.f && GM->SubtitleText.IsEmpty());
		GM->ReturnToTitle();
		break;
	case 28:
		Check(TEXT("title_clears_guidance_and_subtitles"), GM->Phase == EShiftPhase::Title && !Guide->bActive && Guide->Arrows->GetInstanceCount() == 0 && GM->SubtitleText.IsEmpty() && Clerk->FirstPersonCamera->FieldOfView == 85.f);
		Finish();
		break;
	default: break;
	}
}

void AShiftFeedbackTest::Finish()
{
	bool bPassed = !Checks.IsEmpty();
	TSharedRef<FJsonObject> Results = MakeShared<FJsonObject>();
	for (const auto& Pair : Checks) { Results->SetBoolField(Pair.Key, Pair.Value); bPassed &= Pair.Value; }
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("passed"), bPassed);
	Report->SetObjectField(TEXT("checks"), Results);
	Report->SetStringField(TEXT("language"), ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage()));
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Feedback") / ShiftLoc::LanguageCode(ShiftLoc::CurrentLanguage());
	IFileManager::Get().MakeDirectory(*Dir, true);
	FFileHelper::SaveStringToFile(Json, *(Dir / TEXT("report.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FPlatformMisc::RequestExitWithStatus(!bPassed, bPassed ? 0 : 1);
	SetActorTickEnabled(false);
}
