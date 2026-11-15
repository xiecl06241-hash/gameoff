#include "ShiftStoreGameMode.h"
#include "ShiftPlayerCharacter.h"
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

const FString AShiftStoreGameMode::PlayerSaveSlotName(TEXT("ShiftStoreSlot"));

AShiftStoreGameMode::AShiftStoreGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AShiftPlayerCharacter::StaticClass();
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

	Director = GetWorld()->SpawnActor<AShiftDirector>(FVector::ZeroVector, FRotator::ZeroRotator);

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
	LoadProgress();
	if (FParse::Param(*CmdLine, TEXT("ShiftAutoStart")))
	{
		StartNight(NightIndex);
		Phase = EShiftPhase::Working;
	}
	else
	{
		Phase = EShiftPhase::Title;
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
	ClearNightActors();
	NightIndex = FMath::Clamp(InNightIndex, 0, ShiftData::Nights().Num() - 1);
	CheckpointNight = NightIndex;
	CheckpointIncome = Income;
	CheckpointLosses = Losses;
	bHasNightCheckpoint = true;
	NextCustomerIdx = 0;
	CustomersServed = 0;
	Sanity = ShiftData::MaxSanity;
	CustomerSpawnCooldown = 1.5f;
	EerieFlickerTimer = 12.f;

	if (Director) { Director->SetShifted(false); }

	if (AShiftPlayerCharacter* P = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		P->ReleaseFromGrab();
		P->ConsumeHeldItem();
		P->SetActorLocationAndRotation(FVector(800.f, 150.f, 100.f), FRotator(0.f, 90.f, 0.f));
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
	SetStatusBanner(FString::Printf(TEXT("%s — Complete opening chores to let customers in."), *Spec.Title), 4.f);
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
		StartNight(NightIndex);
	}
	else if (Phase == EShiftPhase::NightIntro)
	{
		Phase = EShiftPhase::Working;
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
			T->Setup(EShiftTaskType::Spill, FString::Printf(TEXT("Mop floor residue #%d"), i + 1), 75.f);
			ActiveTasks.Add(T);
		}
	}
}

void AShiftStoreGameMode::OnTaskCompleted(AShiftTaskTarget* Task)
{
	if (!Task) { return; }
	const bool bWasOverdue = Task->IsOverdue();
	ActiveTasks.RemoveAll([Task](const TWeakObjectPtr<AShiftTaskTarget>& P) { return !P.IsValid() || P.Get() == Task; });
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
		: FString::Printf(TEXT("Chore completed! (%d remaining)"), ActiveTasks.Num()), 2.5f);
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
		SetStatusBanner(TEXT("All customers served! Equip the Mop in the back corridor and clean the floor before 06:00."), 5.f);
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
	SetStatusBanner(FString::Printf(TEXT("Customer asks for help finding [%s]! Pick it up from a shelf and press [E] on them."),
		*ShiftData::ProductLabel(Customer->Spec.WantedSku)), 4.5f);
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
	SetStatusBanner(TEXT("Customer is waiting at the Checkout Register. Press [E] to ring them up."), 3.5f);
}

void AShiftStoreGameMode::OnCheckoutCompleted(AShiftCustomer* Customer)
{
	if (!Customer) { return; }
	const FShiftProduct* Prod = ShiftData::FindProduct(Customer->Spec.WantedSku);
	const float SalePrice = Prod ? Prod->Price : 4.5f;
	Income += SalePrice;

	Customer->Speech->SetText(FText::FromString(FString::Printf(TEXT("Paid +$%.2f"), SalePrice)));
	Customer->State = ECustomerState::Leaving;
	Customer->TargetPos = ShiftData::OutsideDoor;
	Customer->StateTimer = 0.f;
	PlaySynthesizedCue(820.f, 0.18f, 0.28f);
	SetStatusBanner(FString::Printf(TEXT("Sale complete (+$%.2f)! Restock sold merchandise."), SalePrice), 3.f);
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
		SetStatusBanner(FString::Printf(TEXT("THE STORE SHIFTED! Find [%s] (Red Glow) and deliver it to the Checkout Register!"), *ItemName), 5.f);
	}
	else
	{
		SetStatusBanner(FString::Printf(TEXT("Lights died behind you... An unseen entity is stalking! Find [%s] -> Checkout Register!"), *ItemName), 5.f);
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
	if (ActiveCustomer.Get() == Customer)
	{
		ActiveCustomer.Reset();
	}
	CustomersServed++;
	CustomerSpawnCooldown = 2.0f;

	// After serving a customer: prioritize restocking, then reset timers on any remaining tasks
	for (TWeakObjectPtr<AShiftTaskTarget>& TP : ActiveTasks)
	{
		if (AShiftTaskTarget* T = TP.Get())
		{
			T->RestartTimer(CurrentNightSpec().TaskTimeLimit);
		}
	}
	SpawnRestockAfterService();
	// Also assign one additional chore if the night still has customers waiting
	if (NextCustomerIdx < CurrentNightSpec().Customers.Num())
	{
		const int32 ExtraSpot = (NightIndex * 3 + CustomersServed * 2) % ShiftData::TaskSpots().Num();
		SpawnTaskFromSpot(ExtraSpot);
	}
}

void AShiftStoreGameMode::OnItemHitCustomer(AShiftItem* Item, AShiftCustomer* Customer)
{
	if (!Item || !Customer) { return; }
	const FShiftProduct* Prod = ShiftData::FindProduct(Item->Sku);
	const float StunSec = Prod ? Prod->StunSeconds : 2.0f;
	if (Customer->IsMonsterActive())
	{
		Customer->Stun(StunSec);
		SetStatusBanner(FString::Printf(TEXT("Direct hit! Entity stunned for %.1fs (-$%.2f cost loss)"), StunSec, Prod ? Prod->Cost : 2.f), 3.f);
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
		SetStatusBanner(FString::Printf(TEXT("Damaged %s (-$%.2f wholesale loss)"), *ShiftData::ProductLabel(Item->Sku), Cost), 2.5f);
	}
	if (Losses >= ShiftData::LossLimit)
	{
		TriggerGameOver(FString::Printf(TEXT("Store losses reached -$%.2f (Limit: -$%.0f). You were fired."), Losses, ShiftData::LossLimit));
	}
}

void AShiftStoreGameMode::ModifySanity(float Delta, const FString& Reason)
{
	Sanity = FMath::Clamp(Sanity + Delta, 0.f, ShiftData::MaxSanity);
	if (!Reason.IsEmpty())
	{
		SetStatusBanner(Reason, 3.f);
	}
	if (Sanity <= 0.f && Phase != EShiftPhase::GameOver)
	{
		TriggerGameOver(TEXT("Your sanity collapsed during the night shift."));
	}
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
