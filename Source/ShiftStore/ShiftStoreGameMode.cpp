#include "ShiftStoreGameMode.h"
#include "ShiftLocalization.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftPlayerController.h"
#include "ShiftGuideActor.h"
#include "ShiftFeedbackTest.h"
#include "ShiftHUD.h"
#include "ShiftDirector.h"
#include "ShiftTaskTarget.h"
#include "ShiftCustomer.h"
#include "ShiftItem.h"
#include "ShiftSaveGame.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Components/TextRenderComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GameFramework/PlayerController.h"

const FString AShiftStoreGameMode::PlayerSaveSlotName(TEXT("ShiftStoreSlot"));

AShiftStoreGameMode::AShiftStoreGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AShiftPlayerCharacter::StaticClass();
	PlayerControllerClass = AShiftPlayerController::StaticClass();
	HUDClass = AShiftHUD::StaticClass();
}

const FShiftNightSpec& AShiftStoreGameMode::CurrentNightSpec() const
{
	const TArray<FShiftNightSpec>& List = ShiftData::Nights();
	return List[FMath::Clamp(NightIndex, 0, List.Num() - 1)];
}

void AShiftStoreGameMode::BeginPlay()
{
	Super::BeginPlay();
	ShiftLoc::Initialize();
	SelectedLanguage = ShiftLoc::CurrentLanguage();
	FString PreviewLanguage;
	if (FParse::Value(FCommandLine::Get(), TEXT("ShiftLanguage="), PreviewLanguage))
	{
		for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index)
		{
			if (PreviewLanguage == ShiftLoc::LanguageCode(Index))
			{
				ShiftLoc::SetLanguage(Index, false);
				SelectedLanguage = Index;
				break;
			}
		}
	}

	Director = GetWorld()->SpawnActor<AShiftDirector>(FVector::ZeroVector, FRotator::ZeroRotator);
	GuideRenderer = GetWorld()->SpawnActor<AShiftGuideActor>(FVector::ZeroVector, FRotator::ZeroRotator);

	// Permanent warehouse stock carton dispenser
	AShiftTaskTarget* Source = GetWorld()->SpawnActor<AShiftTaskTarget>(FVector(400.f, 1380.f, 95.f), FRotator::ZeroRotator);
	if (Source)
	{
		Source->Setup(EShiftTaskType::CartonSource, TEXT("Take Stock Carton"), 0.f);
		CartonSource = Source;
	}

	SetupProceduralAudio();

	// If launched with -ShiftAutoStart or -ShiftSelfTest, jump straight into working state
	const FString CmdLine = FCommandLine::Get();
	if (FParse::Param(*CmdLine, TEXT("ShiftSelfTest")))
	{
		ShiftRunSelfTest();
		return;
	}
	if (FParse::Param(*CmdLine, TEXT("ShiftLocalizationTest")))
	{
		NightIndex = 0;
		OpenLanguageSelection();
		return;
	}
	bHasSavedProgress = LoadProgress();
	if (FParse::Param(*CmdLine, TEXT("ShiftAutoStart")))
	{
		StartNight(NightIndex);
		Phase = EShiftPhase::Working;
	}
	else
	{
		OpenLanguageSelection();
	}
	if (FParse::Param(*CmdLine, TEXT("ShiftFeedbackTest"))) { GetWorld()->SpawnActor<AShiftFeedbackTest>(); }
}

void AShiftStoreGameMode::OpenLanguageSelection()
{
	Phase = EShiftPhase::LanguageSelection;
	SelectedLanguage = ShiftLoc::CurrentLanguage();
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void AShiftStoreGameMode::ConfirmLanguage()
{
	if (Phase != EShiftPhase::LanguageSelection || !ShiftLoc::SetLanguage(SelectedLanguage)) { return; }
	Phase = EShiftPhase::Title;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AShiftStoreGameMode::SetupProceduralAudio()
{
	// Attach fridge hum to AudioAnchor_Refrigeration if present
	FVector FridgePos(350.f, 1000.f, 170.f);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Audio.Refrigeration")))
		{
			FridgePos = It->GetActorLocation();
			break;
		}
	}

	USoundWaveProcedural* HumWave = NewObject<USoundWaveProcedural>(this);
	HumWave->SetSampleRate(22050);
	HumWave->NumChannels = 1;
	HumWave->Duration = INDEFINITELY_LOOPING_DURATION;
	HumWave->SoundGroup = SOUNDGROUP_Effects;
	HumWave->bLooping = true;

	// Pre-fill 2 seconds of warm low-frequency refrigeration hum (60Hz + 120Hz harmonic)
	const int32 Samples = 22050 * 2;
	TArray<int16> Pcm;
	Pcm.SetNumUninitialized(Samples);
	for (int32 i = 0; i < Samples; ++i)
	{
		const float T = static_cast<float>(i) / 22050.f;
		const float Val = 0.55f * FMath::Sin(2.f * PI * 60.f * T) + 0.30f * FMath::Sin(2.f * PI * 120.f * T)
			+ 0.10f * FMath::Sin(2.f * PI * 180.f * T);
		Pcm[i] = static_cast<int16>(FMath::Clamp(Val * 2600.f, -32000.f, 32000.f));
	}
	HumWave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
	FridgeHumComp = UGameplayStatics::SpawnSoundAtLocation(this, HumWave, FridgePos, FRotator::ZeroRotator, 0.45f, 1.f, 0.f);
}

void AShiftStoreGameMode::PlaySynthesizedCue(float FreqHz, float DurationSec, float Volume, bool bNoise)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(22050);
	Wave->NumChannels = 1;
	Wave->Duration = DurationSec;
	const int32 Samples = FMath::Max(256, FMath::RoundToInt(22050.f * DurationSec));
	TArray<int16> Pcm;
	Pcm.SetNumUninitialized(Samples);
	for (int32 i = 0; i < Samples; ++i)
	{
		const float T = static_cast<float>(i) / 22050.f;
		const float Env = FMath::Clamp(1.f - (static_cast<float>(i) / Samples), 0.f, 1.f);
		float Signal = FMath::Sin(2.f * PI * FreqHz * T);
		if (bNoise)
		{
			Signal = 0.5f * Signal + 0.5f * FMath::FRandRange(-1.f, 1.f);
		}
		Pcm[i] = static_cast<int16>(FMath::Clamp(Signal * Env * Volume * 24000.f, -32000.f, 32000.f));
	}
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
	UGameplayStatics::PlaySound2D(this, Wave);
}

void AShiftStoreGameMode::ClearNightActors()
{
	for (TWeakObjectPtr<AShiftTaskTarget>& T : ActiveTasks)
	{
		if (AShiftTaskTarget* Ptr = T.Get()) { Ptr->Destroy(); }
	}
	ActiveTasks.Empty();
	if (AShiftCustomer* C = ActiveCustomer.Get()) { C->Destroy(); }
	ActiveCustomer.Reset();
	if (AShiftItem* L = ActiveLostItem.Get()) { L->Destroy(); }
	ActiveLostItem.Reset();
}

void AShiftStoreGameMode::StartNight(int32 InNightIndex)
{
	ResetDialogue();
	ClearNightActors();
	NightIndex = FMath::Clamp(InNightIndex, 0, ShiftData::Nights().Num() - 1);
	CheckpointNight = NightIndex;
	CheckpointIncome = Income;
	CheckpointLosses = Losses;
	bHasNightCheckpoint = true;
	NextCustomerIdx = 0;
	GuidedTaskIndex = 0;
	SalesSinceRestock = 0;
	ServiceTaskRotation = 0;
	CustomersServed = 0;
	Sanity = ShiftData::MaxSanity;
	CustomerSpawnCooldown = 1.5f;
	EerieFlickerTimer = 12.f;

	if (Director) { Director->SetShifted(false); }

	if (AShiftPlayerCharacter* P = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		P->ReleaseFromGrab(false);
		P->ConsumeHeldItem();
		P->SetActorLocationAndRotation(FVector(800.f, 150.f, 100.f), FRotator(0.f, 90.f, 0.f));
		if (AController* C = P->GetController()) { C->SetControlRotation(FRotator(0.f, 90.f, 0.f)); }
	}

	// Re-show any hidden mop/broom props so they can be picked up again
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Tool.Mop")) || It->ActorHasTag(TEXT("Tool.Broom")))
		{
			It->SetActorHiddenInGame(false);
			It->SetActorEnableCollision(true);
		}
	}

	const FShiftNightSpec& Spec = CurrentNightSpec();
	for (int32 SpotIdx : Spec.OpeningTasks)
	{
		SpawnTaskFromSpot(SpotIdx);
	}
	Phase = EShiftPhase::NightIntro;
	BannerText.Empty();
	BannerTimer = 0.f;
}

void AShiftStoreGameMode::RestartCurrentNight()
{
	if (bHasNightCheckpoint)
	{
		NightIndex = CheckpointNight;
		Income = CheckpointIncome;
		Losses = CheckpointLosses;
	}
	else
	{
		LoadProgress();
	}
	StartNight(NightIndex);
	Phase = EShiftPhase::Working;
}

void AShiftStoreGameMode::OnAdvanceScreenKey()
{
	if (Phase == EShiftPhase::Title)
	{
		// The primary title action always starts a new run. Resume is explicit [C].
		Income = 0.f;
		Losses = 0.f;
		StartNight(0);
	}
	else if (Phase == EShiftPhase::NightIntro)
	{
		Phase = EShiftPhase::Working;
		if (NightIndex == 0) { RequestDialogue(EShiftDialogueEvent::WorkStart); }
		PlaySynthesizedCue(520.f, 0.18f, 0.25f);
	}
	else if (Phase == EShiftPhase::Dawn)
	{
		if (NightIndex + 1 < ShiftData::Nights().Num())
		{
			StartNight(NightIndex + 1);
		}
		else
		{
			Phase = EShiftPhase::Ending;
		}
	}
	else if (Phase == EShiftPhase::GameOver)
	{
		RestartCurrentNight();
	}
	else if (Phase == EShiftPhase::Ending)
	{
		NightIndex = 0;
		Income = 0.f;
		Losses = 0.f;
		SaveProgress();
		StartNight(0);
	}
}

void AShiftStoreGameMode::ContinueSavedGame()
{
	if (Phase == EShiftPhase::Title && bHasSavedProgress && LoadProgress())
	{
		StartNight(NightIndex);
	}
}

void AShiftStoreGameMode::ReturnToTitle()
{
	if (AShiftPlayerCharacter* Player = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->ReleaseFromGrab(false);
		Player->ConsumeHeldItem();
		Player->CrosshairPrompt.Empty();
		Player->HoldProgress = 0.f;
	}
	ClearNightActors();
	for (TActorIterator<AShiftItem> It(GetWorld()); It; ++It) { It->Destroy(); }
	if (Director) { Director->SetShifted(false); }
	BannerText.Empty();
	BannerTimer = 0.f;
	GameOverReason.Empty();
	bHasNightCheckpoint = false;
	// Returning to the title must not overwrite the last dawn checkpoint.
	bHasSavedProgress = LoadProgress();
	Phase = EShiftPhase::Title;
	ResetDialogue();
}

void AShiftStoreGameMode::CycleGuidedTask()
{
	if (!ActiveTasks.IsEmpty()) { GuidedTaskIndex = (GuidedTaskIndex + 1) % ActiveTasks.Num(); }
}

bool AShiftStoreGameMode::GetTaskGuidance(const AShiftPlayerCharacter* Player, FShiftTaskGuidance& Out) const
{
	Out = FShiftTaskGuidance();
	// Clerk guidance never reveals a monster or the position of its hidden demand item.
	if (!Player || ActiveCustomer.IsValid() || ActiveTasks.IsEmpty()
		|| (Phase != EShiftPhase::Working && Phase != EShiftPhase::Cleanup)) { return false; }
	AShiftTaskTarget* Task = ActiveTasks[FMath::Clamp(GuidedTaskIndex, 0, ActiveTasks.Num() - 1)].Get();
	if (!Task) { return false; }
	Out.Target = Task;
	Out.Task = Task;
	Out.Location = Task->GetActorLocation();
	Out.TaskLabel = Task->Label;
	if (Task->Type == EShiftTaskType::Restock)
	{
		if (Player->HeldItem.IsValid() && Player->HeldItem->Kind == EShiftItemKind::Carton)
		{
			Out.Action = EShiftGuideAction::DeliverCarton;
		}
		else if (AShiftTaskTarget* Source = CartonSource.Get())
		{
			Out.Action = EShiftGuideAction::CollectCarton;
			Out.Target = Source;
			Out.Location = Source->GetActorLocation();
		}
	}
	else if (Task->Type == EShiftTaskType::Spill)
	{
		Out.Action = EShiftGuideAction::MopFloor;
		if (!Player->HeldItem.IsValid() || Player->HeldItem->Kind != EShiftItemKind::Mop)
		{
			Out.Action = EShiftGuideAction::CollectMop;
			for (TActorIterator<AShiftItem> It(GetWorld()); It; ++It)
			{
				if (It->Kind == EShiftItemKind::Mop && !It->IsHidden())
				{
					Out.Target = *It;
					Out.Location = It->GetActorLocation() + FVector(0.f, 0.f, 20.f);
					return true;
				}
			}
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				if (It->ActorHasTag(TEXT("Tool.Mop")) && !It->IsHidden())
				{
					Out.Action = EShiftGuideAction::CollectMop;
					Out.Target = *It;
					Out.Location = It->GetActorLocation() + FVector(0.f, 0.f, 70.f);
					break;
				}
			}
		}
	}
	if (Task->Type == EShiftTaskType::CookOden) { Out.Action = EShiftGuideAction::CookOden; }
	return true;
}

void AShiftStoreGameMode::SpawnTaskFromSpot(int32 SpotIdx)
{
	const TArray<FShiftTaskSpot>& Spots = ShiftData::TaskSpots();
	if (!Spots.IsValidIndex(SpotIdx)) { return; }
	const FShiftTaskSpot& S = Spots[SpotIdx];
	AShiftTaskTarget* T = GetWorld()->SpawnActor<AShiftTaskTarget>(S.Location, FRotator::ZeroRotator);
	if (T)
	{
		T->Setup(S.Type, S.Label, CurrentNightSpec().TaskTimeLimit);
		ActiveTasks.Add(T);
	}
}

void AShiftStoreGameMode::SpawnRestockAfterService()
{
	const TArray<FVector>& Spots = ShiftData::RestockSpots();
	const FVector Pos = Spots[CustomersServed % Spots.Num()];
	AShiftTaskTarget* T = GetWorld()->SpawnActor<AShiftTaskTarget>(Pos, FRotator::ZeroRotator);
	if (T)
	{
		T->Setup(EShiftTaskType::Restock, TEXT("Restock sold merchandise"), CurrentNightSpec().TaskTimeLimit);
		ActiveTasks.Insert(T, 0);
	}
}

void AShiftStoreGameMode::SpawnCleanupSpills(int32 Count)
{
	static const FVector SpillLocs[] = {
		FVector(800.f, 320.f, 40.f),
		FVector(600.f, 620.f, 40.f),
		FVector(1050.f, 780.f, 40.f),
		FVector(1220.f, 260.f, 40.f)
	};
	for (int32 i = 0; i < Count; ++i)
	{
		AShiftTaskTarget* T = GetWorld()->SpawnActor<AShiftTaskTarget>(SpillLocs[i % 4], FRotator::ZeroRotator);
		if (T)
		{
			T->Setup(EShiftTaskType::Spill, ShiftLoc::Format(TEXT("Mop floor residue #%d"), {LexToString(i + 1)}), CurrentNightSpec().TaskTimeLimit);
			ActiveTasks.Add(T);
		}
	}
}

void AShiftStoreGameMode::SpawnPostServiceTask()
{
	if (!ActiveTasks.IsEmpty()) { return; }
	if (SalesSinceRestock >= 3)
	{
		SpawnRestockAfterService();
		if (!ActiveTasks.IsEmpty()) { SalesSinceRestock = 0; }
	}
	else
	{
		// One brief chore between customers; alternate shelf facing and hot-food preparation.
		const int32 Spot = ServiceTaskRotation % 2 == 0 ? 5 + (ServiceTaskRotation / 2) % 2 : 10;
		SpawnTaskFromSpot(Spot);
	}
	++ServiceTaskRotation;
	if (!ActiveTasks.IsEmpty())
	{
		BannerText.Empty();
		BannerTimer = 0.f;
	}
}

void AShiftStoreGameMode::OnTaskCompleted(AShiftTaskTarget* Task)
{
	if (!Task) { return; }
	const bool bWasOverdue = Task->IsOverdue();
	ActiveTasks.RemoveAll([Task](const TWeakObjectPtr<AShiftTaskTarget>& P) { return !P.IsValid() || P.Get() == Task; });
	GuidedTaskIndex = 0;
	Task->Destroy();

	// Sanity recovery: overdue tasks stop draining and partially recover; on-time tasks give a small boost
	ModifySanity(bWasOverdue ? ShiftData::OverdueRecover : ShiftData::OnTimeRecover);
	PlaySynthesizedCue(660.f, 0.14f, 0.22f);

	if (Phase == EShiftPhase::Cleanup && ActiveTasks.Num() == 0)
	{
		Phase = EShiftPhase::Dawn;
		SaveProgress();
		PlaySynthesizedCue(880.f, 0.35f, 0.30f);
		return;
	}

	SetStatusBanner(ActiveTasks.Num() == 0
		? TEXT("All chores complete. Ready for next customer.")
		: ShiftLoc::Format(TEXT("Chore completed! (%d remaining)"), {LexToString(ActiveTasks.Num())}), 2.5f);
}

void AShiftStoreGameMode::TrySpawnNextCustomer()
{
	if (Phase != EShiftPhase::Working || ActiveCustomer.IsValid() || ActiveTasks.Num() > 0)
	{
		return;
	}
	const FShiftNightSpec& Spec = CurrentNightSpec();
	if (NextCustomerIdx >= Spec.Customers.Num())
	{
		Phase = EShiftPhase::Cleanup;
		SpawnCleanupSpills(Spec.Spills);
		SetStatusBanner(TEXT("All customers served! Equip the Mop in the warehouse and clean the floor before 06:00."), 5.f);
		PlaySynthesizedCue(580.f, 0.25f, 0.25f);
		return;
	}

	const FShiftCustomerSpec& CSpec = Spec.Customers[NextCustomerIdx++];
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AShiftCustomer* Cust = GetWorld()->SpawnActor<AShiftCustomer>(ShiftData::OutsideDoor, FRotator(0.f, 90.f, 0.f), SP);
	if (Cust)
	{
		Cust->Init(CSpec);
		ActiveCustomer = Cust;
		PlaySynthesizedCue(740.f, 0.22f, 0.25f); // Door chime
		SetStatusBanner(TEXT("Door chime: A customer has entered the store. Prioritize customer service!"), 3.5f);
	}
}

void AShiftStoreGameMode::OnCustomerNeedsHelp(AShiftCustomer* Customer)
{
	if (!Customer) { return; }
	PlaySynthesizedCue(480.f, 0.16f, 0.22f);
	BannerText.Empty();
	BannerTimer = 0.f;
}

void AShiftStoreGameMode::OnCustomerHelped(AShiftCustomer* Customer)
{
	if (!Customer) { return; }
	Customer->Speech->SetText(FText::FromString(TEXT("Thanks! Heading to checkout.")));
	Customer->State = ECustomerState::WalkingToCheckout;
	Customer->TargetPos = ShiftData::CheckoutCustomerSpot;
	Customer->StateTimer = 0.f;
	PlaySynthesizedCue(620.f, 0.15f, 0.22f);
}

void AShiftStoreGameMode::OnCustomerAtCheckout(AShiftCustomer* Customer)
{
	BannerText.Empty();
	BannerTimer = 0.f;
}

void AShiftStoreGameMode::OnCheckoutCompleted(AShiftCustomer* Customer)
{
	if (Phase != EShiftPhase::Working || !Customer || ActiveCustomer.Get() != Customer || Customer->State != ECustomerState::WaitingAtCheckout) { return; }
	const FShiftProduct* Prod = ShiftData::FindProduct(Customer->Spec.WantedSku);
	const float SalePrice = Prod ? Prod->Price : 4.5f;
	Income += SalePrice;
	++SalesSinceRestock;

	Customer->Speech->SetText(FText::FromString(ShiftLoc::Format(TEXT("Paid +$%.2f"), {FString::Printf(TEXT("%.2f"), SalePrice)})));
	Customer->BeginLeaving();
	PlaySynthesizedCue(820.f, 0.18f, 0.28f);
	SetStatusBanner(ShiftLoc::Format(TEXT("Sale complete (+$%.2f). The customer is leaving."), {FString::Printf(TEXT("%.2f"), SalePrice)}), 3.f);
}

void AShiftStoreGameMode::OnMonsterRevealed(AShiftCustomer* Monster, bool bWitnessed)
{
	if (!Monster) { return; }
	if (Director) { Director->SetShifted(true); }

	// Spawn the displaced item the monster demands at one of the backroom/corner spots
	if (AShiftItem* Old = ActiveLostItem.Get()) { Old->Destroy(); }
	const TArray<FVector>& Spots = ShiftData::LostItemSpots();
	const FVector LostPos = Spots[(NightIndex + NextCustomerIdx) % Spots.Num()];
	AShiftItem* Lost = GetWorld()->SpawnActor<AShiftItem>(LostPos, FRotator::ZeroRotator);
	if (Lost)
	{
		Lost->InitProduct(Monster->Spec.MonsterSku);
		Lost->bLostItem = true;
		Lost->SetHeld(false);
		ActiveLostItem = Lost;
	}

	PlaySynthesizedCue(110.f, 0.65f, 0.40f, true);
	const FString ItemName = ShiftData::ProductLabel(Monster->Spec.MonsterSku);
	if (bWitnessed)
	{
		RequestDialogue(EShiftDialogueEvent::WitnessReveal);
		SetStatusBanner(ShiftLoc::Format(TEXT("THE STORE SHIFTED! Find [%s] (Red Glow) and deliver it to the Checkout Register!"), {FString(ItemName)}), 5.f);
	}
	else
	{
		SetStatusBanner(ShiftLoc::Format(TEXT("Lights died behind you... An unseen entity is stalking! Find [%s] -> Checkout Register!"), {FString(ItemName)}), 5.f);
	}
}

void AShiftStoreGameMode::OnMonsterDemandDelivered(AShiftCustomer* Monster)
{
	if (!Monster) { return; }
	if (Director) { Director->SetShifted(false); }
	if (AShiftItem* L = ActiveLostItem.Get()) { L->Destroy(); ActiveLostItem.Reset(); }

	Monster->SatisfyAndLeave();
	ModifySanity(10.f);
	PlaySynthesizedCue(700.f, 0.30f, 0.30f);
	SetStatusBanner(TEXT("Abnormal customer appeased! Store shifting back to normal..."), 4.f);
}

void AShiftStoreGameMode::OnCustomerFinished(AShiftCustomer* Customer)
{
	if (!Customer || ActiveCustomer.Get() != Customer) { return; }
	ActiveCustomer.Reset();
	if (Phase != EShiftPhase::Working) { return; }
	CustomersServed++;
	CustomerSpawnCooldown = 2.0f;

	// Existing chores are reassigned, but no extra work is inserted after the last customer.
	for (TWeakObjectPtr<AShiftTaskTarget>& TP : ActiveTasks)
	{
		if (AShiftTaskTarget* T = TP.Get())
		{
			T->RestartTimer(CurrentNightSpec().TaskTimeLimit);
		}
	}
	if (NextCustomerIdx >= CurrentNightSpec().Customers.Num())
	{
		TrySpawnNextCustomer();
		return;
	}
	SpawnPostServiceTask();
}

void AShiftStoreGameMode::OnItemHitCustomer(AShiftItem* Item, AShiftCustomer* Customer)
{
	if (!Item || !Customer) { return; }
	const FShiftProduct* Prod = ShiftData::FindProduct(Item->Sku);
	const float StunSec = Prod ? Prod->StunSeconds : 2.0f;
	if (Customer->IsMonsterActive())
	{
		Customer->Stun(StunSec);
		SetStatusBanner(ShiftLoc::Format(TEXT("Direct hit! Entity stunned for %.1fs (-$%.2f cost loss)"), {FString::Printf(TEXT("%.1f"), StunSec), FString::Printf(TEXT("%.2f"), Prod ? Prod->Cost : 2.f)}), 3.f);
	}
	if (!Item->bLostItem)
	{
		OnItemBroken(Item, true);
		Item->Destroy();
	}
}

void AShiftStoreGameMode::OnItemBroken(AShiftItem* Item, bool bUsedOnMonster)
{
	if (!Item || Item->Kind != EShiftItemKind::Product || Item->bLostItem) { return; }
	const FShiftProduct* Prod = ShiftData::FindProduct(Item->Sku);
	const float Cost = Prod ? Prod->Cost : 2.5f;
	Losses += Cost;
	PlaySynthesizedCue(180.f, 0.18f, 0.30f, true);
	if (!bUsedOnMonster)
	{
		SetStatusBanner(ShiftLoc::Format(TEXT("Damaged %s (-$%.2f wholesale loss)"), {FString(ShiftData::ProductLabel(Item->Sku)), FString::Printf(TEXT("%.2f"), Cost)}), 2.5f);
	}
	if (Losses >= ShiftData::LossLimit)
	{
		TriggerGameOver(ShiftLoc::Format(TEXT("Store losses reached -$%.2f (Limit: -$%.0f). You were fired."), {FString::Printf(TEXT("%.2f"), Losses), FString::Printf(TEXT("%.0f"), ShiftData::LossLimit)}));
	}
}

void AShiftStoreGameMode::ModifySanity(float Delta, const FString& Reason)
{
	const float PreviousSanity = Sanity;
	Sanity = FMath::Clamp(Sanity + Delta, 0.f, ShiftData::MaxSanity);
	if (!Reason.IsEmpty())
	{
		SetStatusBanner(Reason, 3.f);
	}
	if (Sanity <= 0.f && Phase != EShiftPhase::GameOver)
	{
		TriggerGameOver(TEXT("Your sanity collapsed during the night shift."));
	}
	else if (PreviousSanity >= 35.f && Sanity < 35.f) { RequestDialogue(EShiftDialogueEvent::LowSanity); }
}

void AShiftStoreGameMode::SetStatusBanner(const FString& InText, float Seconds)
{
	BannerText = InText;
	BannerTimer = Seconds;
}

void AShiftStoreGameMode::TriggerGameOver(const FString& Reason)
{
	GameOverReason = Reason;
	Phase = EShiftPhase::GameOver;
	ResetDialogue();
	if (AShiftPlayerCharacter* P = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) { P->ReleaseFromGrab(false); }
	if (Director) { Director->SetShifted(false); }
	PlaySynthesizedCue(95.f, 0.8f, 0.45f, true);
}

const FString& AShiftStoreGameMode::ProgressSlotName() const
{
	return bSelfTestRunning ? SelfTestSaveSlotName : PlayerSaveSlotName;
}

bool AShiftStoreGameMode::SaveProgress()
{
	if (UShiftSaveGame* SG = Cast<UShiftSaveGame>(UGameplayStatics::CreateSaveGameObject(UShiftSaveGame::StaticClass())))
	{
		SG->SavedNightIndex = FMath::Min(NightIndex + (Phase == EShiftPhase::Dawn ? 1 : 0), ShiftData::Nights().Num() - 1);
		SG->CumulativeIncome = Income;
		SG->CumulativeLosses = Losses;
		return UGameplayStatics::SaveGameToSlot(SG, ProgressSlotName(), 0);
	}
	return false;
}

bool AShiftStoreGameMode::LoadProgress()
{
	NightIndex = 0;
	Income = 0.f;
	Losses = 0.f;
	if (UGameplayStatics::DoesSaveGameExist(ProgressSlotName(), 0))
	{
		if (UShiftSaveGame* SG = Cast<UShiftSaveGame>(UGameplayStatics::LoadGameFromSlot(ProgressSlotName(), 0)))
		{
			NightIndex = FMath::Clamp(SG->SavedNightIndex, 0, ShiftData::Nights().Num() - 1);
			Income = SG->CumulativeIncome;
			Losses = SG->CumulativeLosses;
			return true;
		}
	}
	return false;
}

void AShiftStoreGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickDialogue(DeltaSeconds);
	if (FParse::Param(FCommandLine::Get(), TEXT("ShiftLocalizationTest")))
	{
		TickLocalizationTest(DeltaSeconds);
		return;
	}
	if (bSelfTestRunning)
	{
		TickSelfTest(DeltaSeconds);
		return;
	}

	if (BannerTimer > 0.f) { BannerTimer = FMath::Max(0.f, BannerTimer - DeltaSeconds); }

	// Keep procedural refrigeration hum buffer fed
	AudioFeedTimer += DeltaSeconds;
	if (AudioFeedTimer >= 1.0f && FridgeHumComp && FridgeHumComp->IsPlaying())
	{
		AudioFeedTimer = 0.f;
		if (USoundWaveProcedural* Wave = Cast<USoundWaveProcedural>(FridgeHumComp->Sound))
		{
			if (Wave->GetAvailableAudioByteCount() < 22050 * 2)
			{
				const int32 Samples = 22050;
				TArray<int16> Pcm;
				Pcm.SetNumUninitialized(Samples);
				for (int32 i = 0; i < Samples; ++i)
				{
					const float T = static_cast<float>(i) / 22050.f;
					const float Val = 0.55f * FMath::Sin(2.f * PI * 60.f * T) + 0.30f * FMath::Sin(2.f * PI * 120.f * T);
					Pcm[i] = static_cast<int16>(Val * 2600.f);
				}
				Wave->QueueAudio(reinterpret_cast<const uint8*>(Pcm.GetData()), Pcm.Num() * sizeof(int16));
			}
		}
	}

	if (Phase != EShiftPhase::Working && Phase != EShiftPhase::Cleanup) { return; }

	// 1. Overdue task sanity drain (stacks across overdue tasks)
	int32 OverdueCount = 0;
	for (const TWeakObjectPtr<AShiftTaskTarget>& TP : ActiveTasks)
	{
		if (const AShiftTaskTarget* T = TP.Get())
		{
			if (T->IsOverdue()) { ++OverdueCount; }
		}
	}
	if (OverdueCount > 0)
	{
		ModifySanity(-ShiftData::OverdueDrainPerSec * OverdueCount * DeltaSeconds);
	}

	// 2. Monster line-of-sight and grab sanity drain
	if (AShiftCustomer* C = ActiveCustomer.Get())
	{
		if (C->State == ECustomerState::MonsterGrabbing)
		{
			ModifySanity(-ShiftData::GrabbedDrainPerSec * DeltaSeconds);
		}
		else if (C->IsMonsterActive() && C->bSeeingPlayer)
		{
			ModifySanity(-ShiftData::SeenDrainPerSec * DeltaSeconds);
		}
	}

	// 3. Night 1 eerie foreshadowing flickers
	if (CurrentNightSpec().bEerieEvents && Director && !Director->IsShifted())
	{
		EerieFlickerTimer -= DeltaSeconds;
		if (EerieFlickerTimer <= 0.f)
		{
			EerieFlickerTimer = FMath::FRandRange(16.f, 26.f);
			Director->Flicker(0.65f);
			PlaySynthesizedCue(90.f, 0.25f, 0.18f, true);
		}
	}

	// 4. Spawn next customer when all chores are done and store is clear
	if (Phase == EShiftPhase::Working && !ActiveCustomer.IsValid() && ActiveTasks.Num() == 0)
	{
		CustomerSpawnCooldown -= DeltaSeconds;
		if (CustomerSpawnCooldown <= 0.f)
		{
			TrySpawnNextCustomer();
		}
	}
}

void AShiftStoreGameMode::ShiftForceState(int32 bShifted)
{
	if (Director)
	{
		Director->SetShifted(bShifted != 0);
	}
}

void AShiftStoreGameMode::ShiftSkipNight()
{
	ClearNightActors();
	Phase = EShiftPhase::Dawn;
	SaveProgress();
}
