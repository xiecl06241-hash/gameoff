#include "ShiftPlayerCharacter.h"
#include "ShiftLocalization.h"
#include "ShiftItem.h"
#include "ShiftTaskTarget.h"
#include "ShiftCustomer.h"
#include "ShiftStoreGameMode.h"
#include "ShiftHUD.h"
#include "ShiftPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

AShiftPlayerCharacter::AShiftPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 96.f);
	GetCharacterMovement()->MaxWalkSpeed = 260.f;
	GetCharacterMovement()->JumpZVelocity = 0.f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 66.f));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(85.f);

	HoldMount = CreateDefaultSubobject<USceneComponent>(TEXT("HoldMount"));
	HoldMount->SetupAttachment(FirstPersonCamera);
	HoldMount->SetRelativeLocation(FVector(52.f, 22.f, -20.f));
}

void AShiftPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>(); GM && GM->Phase == EShiftPhase::LanguageSelection)
		{
			GM->OpenLanguageSelection();
			return;
		}
		PC->SetShowMouseCursor(false);
		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
	}
}

void AShiftPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AShiftPlayerCharacter::OnGrabbedByMonster(AShiftCustomer* Monster, bool bUnseenAmbush)
{
	GrabbingMonster = Monster;
	StruggleProgress = 0.f;
	if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>())
	{
		if (bUnseenAmbush)
		{
			GM->ModifySanity(-ShiftData::AmbushHit, TEXT("AMBUSHED BY UNSEEN ENTITY!"));
		}
		else
		{
			GM->SetStatusBanner(TEXT("GRABBED! Hold/Mash [E] to struggle free, or [F/RMB] to smash with held item!"), 3.f);
		}
	}
}

void AShiftPlayerCharacter::ReleaseFromGrab()
{
	if (AShiftCustomer* M = GrabbingMonster.Get())
	{
		M->OnReleasedGrab();
	}
	GrabbingMonster.Reset();
	StruggleProgress = 0.f;
}

void AShiftPlayerCharacter::PickupItem(AShiftItem* Item)
{
	if (!Item) { return; }
	if (HeldItem.IsValid())
	{
		DropHeldItem();
	}
	HeldItem = Item;
	Item->SetHeld(true);
	Item->AttachToComponent(HoldMount, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	// The full-size stock carton must sit below the view so the clerk can still follow aisle markers.
	Item->SetActorRelativeLocation(Item->Kind == EShiftItemKind::Carton ? FVector(38.f, 5.f, -45.f) : FVector::ZeroVector);
	Item->SetActorRelativeRotation(FRotator(0.f, -20.f, 0.f));
}

void AShiftPlayerCharacter::DropHeldItem()
{
	if (AShiftItem* Item = HeldItem.Get())
	{
		HeldItem.Reset();
		Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Item->SetActorLocation(FirstPersonCamera->GetComponentLocation() + FirstPersonCamera->GetForwardVector() * 65.f);
		Item->SetHeld(false);
	}
}

void AShiftPlayerCharacter::ConsumeHeldItem()
{
	if (AShiftItem* Item = HeldItem.Get())
	{
		HeldItem.Reset();
		Item->Destroy();
	}
}

FName AShiftPlayerCharacter::ExtractSkuTag(const AActor* Actor) const
{
	if (!Actor) { return NAME_None; }
	for (const FName& Tag : Actor->Tags)
	{
		const FString S = Tag.ToString();
		if (S.StartsWith(TEXT("SKU.")))
		{
			return FName(*S.Mid(4));
		}
	}
	if (const UStaticMeshComponent* SMC = Actor->FindComponentByClass<UStaticMeshComponent>())
	{
		if (const UStaticMesh* SM = SMC->GetStaticMesh())
		{
			const FString Name = SM->GetName();
			if (Name.StartsWith(TEXT("SM_Product_")))
			{
				return FName(*Name.Mid(11));
			}
		}
	}
	return NAME_None;
}

void AShiftPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	if (!PC || !GM) { return; }
	if (AShiftPlayerController* ShiftPC = Cast<AShiftPlayerController>(PC); ShiftPC && ShiftPC->BlocksGameplayInput())
	{
		bPrevInteractDown = false;
		bPrevThrowDown = false;
		return;
	}

	if (GM->Phase == EShiftPhase::LanguageSelection)
	{
		if (PC->WasInputKeyJustPressed(EKeys::Up) || PC->WasInputKeyJustPressed(EKeys::W))
		{
			GM->SelectedLanguage = (GM->SelectedLanguage + ShiftLoc::LanguageCount - 1) % ShiftLoc::LanguageCount;
		}
		if (PC->WasInputKeyJustPressed(EKeys::Down) || PC->WasInputKeyJustPressed(EKeys::S))
		{
			GM->SelectedLanguage = (GM->SelectedLanguage + 1) % ShiftLoc::LanguageCount;
		}
		const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
		for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index)
		{
			if (PC->WasInputKeyJustPressed(NumberKeys[Index])) { GM->SelectedLanguage = Index; }
		}
		FVector2D Mouse;
		bool bClickedChoice = false;
		if (PC->GetMousePosition(Mouse.X, Mouse.Y))
		{
			const bool bClick = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
			const bool bMoved = PreviousMenuMouse.X >= 0.f && FVector2D::DistSquared(Mouse, PreviousMenuMouse) > 1.f;
			int32 Width = 0, Height = 0;
			PC->GetViewportSize(Width, Height);
			if (bMoved || bClick)
			{
				for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index)
				{
					if (AShiftHUD::LanguageButton(Index, Width, Height).IsInside(Mouse))
					{
						GM->SelectedLanguage = Index;
						bClickedChoice = bClick;
					}
				}
			}
			PreviousMenuMouse = Mouse;
		}
		ShiftLoc::SetLanguage(GM->SelectedLanguage, false);
		if (PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::SpaceBar) || bClickedChoice)
		{
			GM->ConfirmLanguage();
			PreviousMenuMouse = FVector2D(-1.f, -1.f);
		}
		return;
	}
	if (GM->Phase == EShiftPhase::Title && PC->WasInputKeyJustPressed(EKeys::L))
	{
		GM->OpenLanguageSelection();
		return;
	}
	if (GM->Phase == EShiftPhase::Title && PC->WasInputKeyJustPressed(EKeys::C))
	{
		GM->ContinueSavedGame();
		return;
	}

	// Title / Intro / Dawn / GameOver / Ending screens consume Space or Enter
	if (GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup)
	{
		if (PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::E))
		{
			GM->OnAdvanceScreenKey();
		}
		if (GM->Phase == EShiftPhase::GameOver && PC->WasInputKeyJustPressed(EKeys::R))
		{
			GM->RestartCurrentNight();
		}
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::Tab)) { GM->CycleGuidedTask(); }

	// Mouse look
	float MouseX = 0.f, MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	if (MouseX != 0.f) { AddControllerYawInput(MouseX * 0.7f); }
	if (MouseY != 0.f) { AddControllerPitchInput(-MouseY * 0.7f); }
	// Arrow key look fallback for accessibility / headless testing
	if (PC->IsInputKeyDown(EKeys::Left))  { AddControllerYawInput(-70.f * DeltaSeconds); }
	if (PC->IsInputKeyDown(EKeys::Right)) { AddControllerYawInput(70.f * DeltaSeconds); }
	if (PC->IsInputKeyDown(EKeys::Up))    { AddControllerPitchInput(-50.f * DeltaSeconds); }
	if (PC->IsInputKeyDown(EKeys::Down))  { AddControllerPitchInput(50.f * DeltaSeconds); }

	// Movement (disabled while grabbed by a monster)
	if (!IsGrabbed())
	{
		FVector Fwd = GetActorForwardVector(); Fwd.Z = 0.f; Fwd.Normalize();
		FVector Right = GetActorRightVector(); Right.Z = 0.f; Right.Normalize();
		if (PC->IsInputKeyDown(EKeys::W)) { AddMovementInput(Fwd, 1.f); }
		if (PC->IsInputKeyDown(EKeys::S)) { AddMovementInput(Fwd, -1.f); }
		if (PC->IsInputKeyDown(EKeys::D)) { AddMovementInput(Right, 1.f); }
		if (PC->IsInputKeyDown(EKeys::A)) { AddMovementInput(Right, -1.f); }
	}

	const bool bInteractDown = PC->IsInputKeyDown(EKeys::E) || PC->IsInputKeyDown(EKeys::LeftMouseButton);
	const bool bInteractJust = bInteractDown && !bPrevInteractDown;
	bPrevInteractDown = bInteractDown;

	const bool bThrowDown = PC->IsInputKeyDown(EKeys::F) || PC->IsInputKeyDown(EKeys::RightMouseButton) || PC->IsInputKeyDown(EKeys::Q);
	const bool bThrowJust = bThrowDown && !bPrevThrowDown;
	bPrevThrowDown = bThrowDown;

	if (PC->WasInputKeyJustPressed(EKeys::G))
	{
		DropHeldItem();
	}

	// Struggle mechanics when grabbed
	if (AShiftCustomer* Monster = GrabbingMonster.Get())
	{
		// Pull camera slightly toward monster
		const FRotator LookAt = (Monster->GetActorLocation() + FVector(0, 0, 55.f) - FirstPersonCamera->GetComponentLocation()).Rotation();
		PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(), LookAt, DeltaSeconds, 5.f));

		// Fast escape: smash monster with held product, then E
		if ((bThrowJust || bInteractJust) && HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product && !HeldItem->bLostItem)
		{
			AShiftItem* Used = HeldItem.Get();
			const FShiftProduct* Prod = ShiftData::FindProduct(Used->Sku);
			const float StunSec = Prod ? Prod->StunSeconds : 2.f;
			GM->OnItemBroken(Used, true);
			ConsumeHeldItem();
			Monster->Stun(StunSec);
			ReleaseFromGrab();
			GM->SetStatusBanner(ShiftLoc::Format(TEXT("Smashed monster with merchandise! Escaped (Stunned %.1fs)"), {FString::Printf(TEXT("%.1f"), StunSec)}), 2.5f);
			return;
		}

		// Slow escape: hold E or mash E
		if (bInteractDown)
		{
			StruggleProgress += DeltaSeconds * (bInteractJust ? 0.55f : 0.36f);
		}
		else
		{
			StruggleProgress = FMath::Max(0.f, StruggleProgress - DeltaSeconds * 0.15f);
		}
		CrosshairPrompt = ShiftLoc::Format(TEXT("GRABBED! Hold/Mash [E] to struggle (%d%%)%s"), {LexToString(FMath::RoundToInt(StruggleProgress * 100.f)), FString((HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product) ? TEXT(" | [F/RMB] Smash with item!") : TEXT(""))});

		if (StruggleProgress >= 1.f)
		{
			ReleaseFromGrab();
			GM->SetStatusBanner(TEXT("Broke free! Keep moving!"), 2.f);
		}
		return;
	}

	UpdateFocusAndInteraction(DeltaSeconds, bInteractJust, bInteractDown, bThrowJust);
}

void AShiftPlayerCharacter::UpdateFocusAndInteraction(float DeltaSeconds, bool bInteractJustPressed, bool bInteractDown, bool bThrowJustPressed)
{
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	CrosshairPrompt.Reset();
	HoldProgress = 0.f;

	// Throwing held item
	if (bThrowJustPressed && HeldItem.IsValid())
	{
		AShiftItem* ToThrow = HeldItem.Get();
		if (ToThrow->CanSimulate())
		{
			HeldItem.Reset();
			ToThrow->SetActorLocation(FirstPersonCamera->GetComponentLocation() + FirstPersonCamera->GetForwardVector() * 65.f);
			ToThrow->Throw(FirstPersonCamera->GetForwardVector() * 980.f + FVector(0, 0, 140.f), this);
			return;
		}
		else
		{
			DropHeldItem();
			return;
		}
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * 265.f;
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("PlayerInteract"), false, this);
	if (HeldItem.IsValid()) { Params.AddIgnoredActor(HeldItem.Get()); }

	const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
		ECC_Visibility, FCollisionShape::MakeSphere(14.f), Params);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;

	// 1. Task target (chore / spill / warehouse stock source)
	if (AShiftTaskTarget* Task = Cast<AShiftTaskTarget>(HitActor))
	{
		FocusedTask = Task;
		if (Task->Type == EShiftTaskType::CartonSource)
		{
			CrosshairPrompt = TEXT("[E] Take Stock Carton");
			if (bInteractJustPressed)
			{
				AShiftItem* Carton = GetWorld()->SpawnActor<AShiftItem>(GetActorLocation(), FRotator::ZeroRotator);
				Carton->InitKind(EShiftItemKind::Carton);
				PickupItem(Carton);
			}
			return;
		}

		EShiftItemKind NeededKind;
		if (Task->NeedsItem(NeededKind))
		{
			if (!HeldItem.IsValid() || HeldItem->Kind != NeededKind)
			{
				const TCHAR* NeededName = (NeededKind == EShiftItemKind::Carton) ? TEXT("Stock Carton (from Warehouse)") : TEXT("Mop (from Warehouse)");
				CrosshairPrompt = ShiftLoc::Format(TEXT("%s — Requires %s"), {FString(Task->Label), FString(NeededName)});
				Task->Progress = 0.f;
				return;
			}
		}

		CrosshairPrompt = ShiftLoc::Format(TEXT("[Hold E] %s (%d%%)"), {FString(Task->Label), LexToString(FMath::RoundToInt(Task->Progress * 100.f))});
		if (bInteractDown)
		{
			Task->Progress += DeltaSeconds / FMath::Max(0.2f, Task->HoldDuration());
			HoldProgress = FMath::Clamp(Task->Progress, 0.f, 1.f);
			if (Task->Progress >= 1.f)
			{
				if (Task->Type == EShiftTaskType::Restock && HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Carton)
				{
					ConsumeHeldItem();
				}
				GM->OnTaskCompleted(Task);
			}
		}
		else
		{
			Task->Progress = FMath::Max(0.f, Task->Progress - DeltaSeconds * 0.5f);
		}
		return;
	}

	// 2. Customer / Monster interaction
	if (AShiftCustomer* Cust = Cast<AShiftCustomer>(HitActor))
	{
		if (Cust->State == ECustomerState::WaitingForHelp)
		{
			const FString WantedLabel = ShiftData::ProductLabel(Cust->Spec.WantedSku);
			if (HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product && HeldItem->Sku == Cust->Spec.WantedSku)
			{
				CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Hand %s to Customer"), {FString(WantedLabel)});
				if (bInteractJustPressed)
				{
					ConsumeHeldItem();
					GM->OnCustomerHelped(Cust);
				}
			}
			else
			{
				CrosshairPrompt = ShiftLoc::Format(TEXT("Customer: \"%s\" (Bring %s)"), {FString(Cust->Spec.Line.IsEmpty() ? TEXT("Can you find this for me?") : *Cust->Spec.Line), FString(WantedLabel)});
			}
			return;
		}
		if (Cust->State == ECustomerState::WaitingAtCheckout)
		{
			CrosshairPrompt = TEXT("[E] Ring Up Customer at Checkout");
			if (bInteractJustPressed)
			{
				GM->OnCheckoutCompleted(Cust);
			}
			return;
		}
		if (Cust->IsMonsterActive())
		{
			CrosshairPrompt = ShiftLoc::Format(TEXT("ENTITY DEMANDS: %s (Deliver to Checkout Register!)"), {FString(ShiftData::ProductLabel(Cust->Spec.MonsterSku))});
			return;
		}
	}

	// 3. Checkout Register / Scanner / Counter
	if (HitActor)
	{
		const FString Label = HitActor->GetName();
		const bool bIsCheckout = HitActor->ActorHasTag(TEXT("Interactive.Scanner"))
			|| FVector::Dist2D(Hit.ImpactPoint, ShiftData::RegisterLocation) < 135.f;
		if (bIsCheckout)
		{
			if (GM->ActiveCustomer.IsValid())
			{
				AShiftCustomer* C = GM->ActiveCustomer.Get();
				if (C->IsMonsterActive())
				{
					const FString Demanded = ShiftData::ProductLabel(C->Spec.MonsterSku);
					if (HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product && HeldItem->Sku == C->Spec.MonsterSku)
					{
						CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Deliver %s to appease the entity!"), {FString(Demanded)});
						if (bInteractJustPressed)
						{
							ConsumeHeldItem();
							GM->OnMonsterDemandDelivered(C);
						}
						return;
					}
					CrosshairPrompt = ShiftLoc::Format(TEXT("Checkout: Bring [%s] here to banish the entity"), {FString(Demanded)});
					return;
				}
				if (C->State == ECustomerState::WaitingAtCheckout)
				{
					CrosshairPrompt = TEXT("[E] Ring Up Customer");
					if (bInteractJustPressed)
					{
						GM->OnCheckoutCompleted(C);
					}
					return;
				}
			}
		}

		// 4. Oden station — take a hot broth cup to throw at the monster
		if (HitActor->ActorHasTag(TEXT("Interactive.Oden")))
		{
			CrosshairPrompt = TEXT("[E] Take Hot Oden Cup (Strong Stun Throwable)");
			if (bInteractJustPressed)
			{
				AShiftItem* Oden = GetWorld()->SpawnActor<AShiftItem>(GetActorLocation(), FRotator::ZeroRotator);
				Oden->InitProduct(TEXT("Noodles"));
				PickupItem(Oden);
			}
			return;
		}

		// 5. Already-spawned AShiftItem
		if (AShiftItem* ShiftItem = Cast<AShiftItem>(HitActor))
		{
			CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Pick up %s"), {FString(ShiftItem->DisplayName())});
			if (bInteractJustPressed)
			{
				PickupItem(ShiftItem);
			}
			return;
		}

		// 6. Mop / Broom props in the level
		if (HitActor->ActorHasTag(TEXT("Tool.Mop")) || HitActor->ActorHasTag(TEXT("Tool.Broom")))
		{
			CrosshairPrompt = TEXT("[E] Equip Mop");
			if (bInteractJustPressed)
			{
				HitActor->SetActorHiddenInGame(true);
				HitActor->SetActorEnableCollision(false);
				AShiftItem* Mop = GetWorld()->SpawnActor<AShiftItem>(GetActorLocation(), FRotator::ZeroRotator);
				Mop->InitKind(EShiftItemKind::Mop);
				PickupItem(Mop);
			}
			return;
		}

		// 7. Hero product props or shelf display stock -> pick up a carryable product
		const FName Sku = ExtractSkuTag(HitActor);
		if (!Sku.IsNone())
		{
			CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Pick up %s"), {FString(ShiftData::ProductLabel(Sku))});
			if (bInteractJustPressed)
			{
				AShiftItem* NewItem = GetWorld()->SpawnActor<AShiftItem>(GetActorLocation(), FRotator::ZeroRotator);
				NewItem->InitProduct(Sku);
				PickupItem(NewItem);
			}
			return;
		}

		// Also allow grabbing products off Gondolas / WallShelves / Refrigerators by nearest shelf SKU
		if (HitActor->ActorHasTag(TEXT("Fixture.Shelf")) || HitActor->ActorHasTag(TEXT("Interactive.FridgeDoor"))
			|| HitActor->GetName().Contains(TEXT("Fridge")) || HitActor->GetName().Contains(TEXT("WallShelf"))
			|| HitActor->GetName().Contains(TEXT("Drinks")))
		{
			// Find the closest instanced product to Hit.ImpactPoint
			FName BestSku = NAME_None;
			float BestDistSq = 85.f * 85.f;
			for (TActorIterator<AActor> It(GetWorld()); It; ++It)
			{
				TArray<UInstancedStaticMeshComponent*> ISMs;
				It->GetComponents(ISMs);
				for (UInstancedStaticMeshComponent* ISM : ISMs)
				{
					const UStaticMesh* SM = ISM->GetStaticMesh();
					if (!SM || !SM->GetName().StartsWith(TEXT("SM_Product_"))) { continue; }
					const FName InstSku(*SM->GetName().Mid(11));
					for (int32 i = 0; i < ISM->GetInstanceCount(); ++i)
					{
						FTransform T;
						ISM->GetInstanceTransform(i, T, true);
						const float D = FVector::DistSquared(T.GetLocation(), Hit.ImpactPoint);
						if (D < BestDistSq)
						{
							BestDistSq = D;
							BestSku = InstSku;
						}
					}
				}
			}
			if (!BestSku.IsNone())
			{
				CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Take %s from shelf"), {FString(ShiftData::ProductLabel(BestSku))});
				if (bInteractJustPressed)
				{
					AShiftItem* NewItem = GetWorld()->SpawnActor<AShiftItem>(GetActorLocation(), FRotator::ZeroRotator);
					NewItem->InitProduct(BestSku);
					PickupItem(NewItem);
				}
				return;
			}
		}
	}
}
