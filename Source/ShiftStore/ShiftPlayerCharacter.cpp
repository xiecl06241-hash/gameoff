#include "ShiftPlayerCharacter.h"
#include "ShiftLocalization.h"
#include "ShiftItem.h"
#include "ShiftClueActor.h"
#include "ShiftTaskTarget.h"
#include "ShiftCustomer.h"
#include "ShiftDirector.h"
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
	bGrabCameraActive = true;
	GrabCameraTime = 0.f;
	GrabStartRotation = GetController() ? GetController()->GetControlRotation() : GetActorRotation();
	if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>())
	{
		GM->RequestDialogue(EShiftDialogueEvent::Grabbed);
		if (bUnseenAmbush)
		{
			GM->ModifySanity(-ShiftData::AmbushHit, TEXT("AMBUSHED BY UNSEEN ENTITY!"));
		}
		else
		{
			// The crosshair already displays the escape controls; keep the face unobstructed.
			GM->BannerText.Empty();
			GM->BannerTimer = 0.f;
		}
	}
}

void AShiftPlayerCharacter::ReleaseFromGrab(bool bEscaped)
{
	const bool bWasGrabbed = IsGrabbed() || bGrabCameraActive;
	if (AShiftCustomer* M = GrabbingMonster.Get())
	{
		M->OnReleasedGrab();
	}
	GrabbingMonster.Reset();
	StruggleProgress = 0.f;
	ResetGrabCamera();
	if (bWasGrabbed && bEscaped)
	{
		if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>()) { GM->RequestDialogue(EShiftDialogueEvent::Escaped); }
	}
}

void AShiftPlayerCharacter::ResetGrabCamera()
{
	bGrabCameraActive = false;
	GrabCameraTime = 0.f;
	if (AController* C = GetController())
	{
		FRotator Rotation = C->GetControlRotation();
		Rotation.Roll = 0.f;
		C->SetControlRotation(Rotation);
	}
	FirstPersonCamera->SetFieldOfView(85.f);
}

void AShiftPlayerCharacter::UpdateGrabCamera(float DeltaSeconds)
{
	const AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	const AShiftCustomer* Monster = GrabbingMonster.Get();
	if (!Monster || !GM || (GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup))
	{
		if (bGrabCameraActive) { GrabbingMonster.Reset(); StruggleProgress = 0.f; ResetGrabCamera(); }
		return;
	}
	GrabCameraTime += DeltaSeconds;
	const float Alpha = FMath::SmoothStep(0.f, 0.25f, GrabCameraTime);
	FirstPersonCamera->SetFieldOfView(FMath::Lerp(85.f, 70.f, Alpha));
	if (AController* C = GetController())
	{
		FRotator LookAt = (Monster->GrabFocusLocation() - FirstPersonCamera->GetComponentLocation()).Rotation();
		if (!GM->IsAutomatedSession())
		{
			LookAt.Pitch += FMath::Sin(GrabCameraTime * 38.f) * 1.8f;
			LookAt.Yaw += FMath::Cos(GrabCameraTime * 44.f) * 2.2f;
			LookAt.Roll = FMath::Sin(GrabCameraTime * 27.f) * 4.5f;
		}
		C->SetControlRotation(FMath::Lerp(GrabStartRotation.Quaternion(), LookAt.Quaternion(), Alpha).Rotator());
	}
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
	if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>(); GM && !GM->IsAutomatedSession())
	{
		GM->PlayExternalOrSynthesizedCue(TEXT("cloth_rustle.wav"), 320.f, 0.08f, 0.30f);
	}
}

FVector AShiftPlayerCharacter::ReleaseLocation(const AShiftItem& Item) const
{
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector Direction = FirstPersonCamera->GetForwardVector();
	const FVector End = Start + Direction * 65.f;
	if (Item.Kind != EShiftItemKind::Product) { return End; }

	// Store meshes can have an off-centre pivot. Keep the entire product on the player's side of the surface.
	const float Radius = Item.Mesh->Bounds.SphereRadius + FVector::Dist(Item.GetActorLocation(), Item.Mesh->Bounds.Origin);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProductRelease), false, this);
	Params.AddIgnoredActor(&Item);
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_PhysicsBody,
		FCollisionShape::MakeSphere(FMath::Max(1.f, Radius)), Params))
	{
		return Start + Direction * FMath::Max(0.f, Hit.Distance - 1.f);
	}
	return End;
}

void AShiftPlayerCharacter::DropHeldItem()
{
	if (AShiftItem* Item = HeldItem.Get())
	{
		HeldItem.Reset();
		Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Item->SetActorLocation(ReleaseLocation(*Item));
		Item->SetHeld(false);
		if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>(); GM && !GM->IsAutomatedSession())
		{
			GM->PlayExternalOrSynthesizedCue(TEXT("item_drop.wav"), 180.f, 0.09f, 0.40f);
		}
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

void AShiftPlayerCharacter::ApplyMouseLook(float DeltaX, float DeltaY)
{
	const AShiftPlayerController* PC = Cast<AShiftPlayerController>(GetController());
	const float Scale = 0.7f * (PC ? PC->MouseSensitivity() : 1.f);
	AddControllerYawInput(DeltaX * Scale);
	AddControllerPitchInput(-DeltaY * Scale);
}

void AShiftPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	if (!PC || !GM) { return; }
	UpdateGrabCamera(DeltaSeconds);
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

	// Archive paper reader overlay: dismiss with [E] / [Space] / [Tab] / [LMB]
	if (GM->bArchiveReaderOpen)
	{
		if (PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::SpaceBar)
			|| PC->WasInputKeyJustPressed(EKeys::Tab) || PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			GM->CloseArchiveReader();
			bPrevInteractDown = PC->IsInputKeyDown(EKeys::E) || PC->IsInputKeyDown(EKeys::LeftMouseButton);
		}
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::Tab)) { GM->ToggleRuleNote(); }
	if (PC->WasInputKeyJustPressed(EKeys::V)) { GM->CycleGuidedTask(); }

	// Mouse look
	if (!IsGrabbed())
	{
	float MouseX = 0.f, MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	ApplyMouseLook(MouseX, MouseY);
	// Arrow key look fallback for accessibility / headless testing
	if (PC->IsInputKeyDown(EKeys::Left))  { AddControllerYawInput(-70.f * DeltaSeconds); }
	if (PC->IsInputKeyDown(EKeys::Right)) { AddControllerYawInput(70.f * DeltaSeconds); }
	if (PC->IsInputKeyDown(EKeys::Up))    { AddControllerPitchInput(-50.f * DeltaSeconds); }
	if (PC->IsInputKeyDown(EKeys::Down))  { AddControllerPitchInput(50.f * DeltaSeconds); }
	}

	// Movement & [Shift] Sprint (disabled while grabbed by a monster)
	if (!IsGrabbed())
	{
		FVector Fwd = GetActorForwardVector(); Fwd.Z = 0.f; Fwd.Normalize();
		FVector Right = GetActorRightVector(); Right.Z = 0.f; Right.Normalize();
		bool bMovingInput = false;
		if (PC->IsInputKeyDown(EKeys::W)) { AddMovementInput(Fwd, 1.f); bMovingInput = true; }
		if (PC->IsInputKeyDown(EKeys::S)) { AddMovementInput(Fwd, -1.f); bMovingInput = true; }
		if (PC->IsInputKeyDown(EKeys::D)) { AddMovementInput(Right, 1.f); bMovingInput = true; }
		if (PC->IsInputKeyDown(EKeys::A)) { AddMovementInput(Right, -1.f); bMovingInput = true; }

		const bool bShiftKey = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
		bIsSprinting = bShiftKey && bMovingInput && Stamina > 1.f;
		if (bIsSprinting)
		{
			Stamina = FMath::Max(0.f, Stamina - 24.f * DeltaSeconds);
			SprintRecoverDelay = 0.55f;
		}
		else
		{
			SprintRecoverDelay = FMath::Max(0.f, SprintRecoverDelay - DeltaSeconds);
			if (SprintRecoverDelay <= 0.f)
			{
				Stamina = FMath::Min(100.f, Stamina + 34.f * DeltaSeconds);
			}
		}
		const float BaseSpeed = GM->IsAutomatedSession() ? 260.f : 285.f;
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? 435.f : BaseSpeed;
		if (GetVelocity().Size2D() > 75.f)
		{
			FootstepTimer -= DeltaSeconds;
			if (FootstepTimer <= 0.f)
			{
				FootstepTimer = bIsSprinting ? 0.30f : 0.46f;
				GM->PlayFootstepCue(nullptr, bIsSprinting ? 0.36f : 0.20f);
			}
		}
		else
		{
			FootstepTimer = FMath::Min(FootstepTimer, 0.12f);
		}
		if (!bGrabCameraActive)
		{
			PhaseSnapFovKick = FMath::Max(0.f, PhaseSnapFovKick - DeltaSeconds * 4.5f);
			const float TargetFov = (bIsSprinting ? 93.f : 85.f) + PhaseSnapFovKick * 11.f;
			FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(FirstPersonCamera->FieldOfView, TargetFov, DeltaSeconds, 14.f));
		}
	}
	else
	{
		bIsSprinting = false;
	}

	// Active [Q] Dual-Reality Phase Shift
	if (!IsGrabbed() && PC->WasInputKeyJustPressed(EKeys::Q))
	{
		if (GM->TryPlayerPhaseShift())
		{
			PhaseSnapFovKick = 1.f;
		}
	}

	const bool bInteractDown = PC->IsInputKeyDown(EKeys::E) || PC->IsInputKeyDown(EKeys::LeftMouseButton);
	const bool bInteractJust = bInteractDown && !bPrevInteractDown;
	bPrevInteractDown = bInteractDown;

	const bool bThrowDown = PC->IsInputKeyDown(EKeys::F) || PC->IsInputKeyDown(EKeys::RightMouseButton);
	const bool bThrowJust = bThrowDown && !bPrevThrowDown;
	bPrevThrowDown = bThrowDown;

	if (PC->WasInputKeyJustPressed(EKeys::G))
	{
		DropHeldItem();
	}

	// Struggle mechanics when grabbed
	if (AShiftCustomer* Monster = GrabbingMonster.Get())
	{
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

	const bool bRefuseJust = PC->WasInputKeyJustPressed(EKeys::R);
	UpdateFocusAndInteraction(DeltaSeconds, bInteractJust, bInteractDown, bThrowJust, bRefuseJust);
}

void AShiftPlayerCharacter::UpdateFocusAndInteraction(float DeltaSeconds, bool bInteractJustPressed, bool bInteractDown, bool bThrowJustPressed, bool bRefuseJustPressed)
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
			ToThrow->SetActorLocation(ReleaseLocation(*ToThrow));
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

	// 0a. Ringing back-office desk telephone (Night 2+ story event)
	if (GM->bOfficePhoneRinging && (!GM->Director || !GM->Director->IsEffectiveShifted()))
	{
		const FVector PhonePos = GM->Director ? GM->Director->GetOfficePhoneLocation() : FVector(1135.f, 1490.f, 84.f);
		const FVector ToPhone = PhonePos - Start;
		const float DistToPhone = ToPhone.Size();
		if (DistToPhone < 220.f && FVector::DotProduct(FirstPersonCamera->GetForwardVector(), ToPhone.GetSafeNormal()) > 0.65f)
		{
			CrosshairPrompt = ShiftLoc::Text(TEXT("[E] Answer Ringing Telephone"));
			if (bInteractJustPressed)
			{
				GM->AnswerOfficePhone();
			}
			return;
		}
	}

	// 0b. Collectible Story Archives & Tarnished Brass Key (hit or close gaze fallback)
	AShiftClueActor* FocusedClue = Cast<AShiftClueActor>(HitActor);
	if (!FocusedClue)
	{
		const bool bShifted = GM->Director && GM->Director->IsEffectiveShifted();
		float BestDot = 0.84f;
		for (const TWeakObjectPtr<AShiftClueActor>& CluePtr : GM->StoryClues)
		{
			AShiftClueActor* Candidate = CluePtr.Get();
			if (!Candidate || !Candidate->IsCurrentlyVisible(GM->NightIndex, bShifted, GM->CollectedCluesMask, GM->bHasBrassKey, GM->bOfficePhoneAnswered)) { continue; }
			const FVector ToClue = Candidate->GetActorLocation() - Start;
			if (ToClue.Size() > 210.f) { continue; }
			const float Dot = FVector::DotProduct(FirstPersonCamera->GetForwardVector(), ToClue.GetSafeNormal());
			if (Dot > BestDot)
			{
				BestDot = Dot;
				FocusedClue = Candidate;
			}
		}
	}
	if (FocusedClue)
	{
		if (FocusedClue->bIsBrassKey)
		{
			CrosshairPrompt = ShiftLoc::Text(TEXT("[E] Take Tarnished Brass Key"));
		}
		else if (FocusedClue->bRequiresBrassKey)
		{
			CrosshairPrompt = GM->bHasBrassKey
				? ShiftLoc::Text(TEXT("[E] Unlock Iron Casket with Brass Key"))
				: ShiftLoc::Text(TEXT("[E] Sealed Iron Casket (Requires Brass Key)"));
		}
		else
		{
			CrosshairPrompt = ShiftLoc::Text(TEXT("[E] Inspect Archive"));
		}
		if (bInteractJustPressed)
		{
			GM->InteractWithClue(FocusedClue);
		}
		return;
	}

	// 1. Task target (chore / spill / warehouse stock source)
	if (AShiftTaskTarget* Task = Cast<AShiftTaskTarget>(HitActor))
	{
		FocusedTask = Task;
		if (Task->Type == EShiftTaskType::BreakerBox)
		{
			if (!GM->Director || !GM->Director->IsShifted())
			{
				CrosshairPrompt = ShiftLoc::Text(TEXT("Emergency Breaker (Standby — Use during Store Shift)"));
			}
			else if (GM->Director->IsBreakerOverridden())
			{
				CrosshairPrompt = ShiftLoc::Format(TEXT("Emergency Breaker active (%ds left)"), {LexToString(FMath::CeilToInt(GM->Director->BreakerOverrideRemaining()))});
			}
			else if (GM->Director->BreakerCooldownRemaining() > 0.f)
			{
				CrosshairPrompt = ShiftLoc::Format(TEXT("Emergency Breaker recharging (%ds)"), {LexToString(FMath::CeilToInt(GM->Director->BreakerCooldownRemaining()))});
			}
			else
			{
				CrosshairPrompt = ShiftLoc::Text(TEXT("[E] Flip Emergency Breaker (Restore lights 10s - Attracts Entity!)"));
				if (bInteractJustPressed)
				{
					GM->ActivateBreakerBox();
				}
			}
			return;
		}
		if (Task->Type == EShiftTaskType::CartonSource)
		{
			if (HeldItem.IsValid())
			{
				CrosshairPrompt = ShiftLoc::Text(TEXT("Hands full. Put it back on a shelf, or press [G] to drop it."));
				return;
			}
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
			if (bRefuseJustPressed)
			{
				GM->OnCustomerRefused(Cust);
				return;
			}
			const FString WantedLabel = ShiftData::ProductLabel(Cust->Spec.WantedSku);
			if (GM->IsAutomatedSession() && Cust->Spec.bMonster && HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product && HeldItem->Sku == Cust->Spec.MonsterSku)
			{
				const FString TrueLabel = ShiftData::ProductLabel(Cust->Spec.MonsterSku);
				CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Hand %s to Customer  |  [R] Refuse"), {FString(TrueLabel)});
				if (bInteractJustPressed)
				{
					ConsumeHeldItem();
					GM->OnMonsterWardedOff(Cust);
				}
			}
			else if (HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product && HeldItem->Sku == Cust->Spec.WantedSku)
			{
				CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Hand %s to Customer  |  [R] Refuse"), {FString(WantedLabel)});
				if (bInteractJustPressed)
				{
					if (Cust->Spec.bMonster)
					{
						DropHeldItem();
						Cust->RevealMonster(this);
					}
					else
					{
						ConsumeHeldItem();
						GM->OnCustomerHelped(Cust);
					}
				}
			}
			else
			{
				// Never spoil the monster's true item on the crosshair; the player must observe visual tells and check the Shift Note!
				CrosshairPrompt = ShiftLoc::Format(TEXT("Customer: \"%s\" (Bring %s)  |  [R] Refuse"), {FString(Cust->Spec.Line.IsEmpty() ? TEXT("Can you find this for me?") : *Cust->Spec.Line), FString(WantedLabel)});
			}
			return;
		}
		if (Cust->State == ECustomerState::WaitingAtCheckout)
		{
			if (bRefuseJustPressed)
			{
				GM->OnCustomerRefused(Cust);
				return;
			}
			CrosshairPrompt = TEXT("[E] Ring Up Customer at Checkout  |  [R] Refuse");
			if (bInteractJustPressed)
			{
				GM->OnCheckoutCompleted(Cust);
			}
			return;
		}
		if (!Cust->IsMonsterActive() && Cust->State != ECustomerState::Leaving)
		{
			if (bRefuseJustPressed)
			{
				GM->OnCustomerRefused(Cust);
				return;
			}
			CrosshairPrompt = TEXT("[R] Refuse Service");
			return;
		}
		if (Cust->IsMonsterActive())
		{
			CrosshairPrompt = ShiftLoc::Format(TEXT("ENTITY DEMANDS: %s (Deliver to Checkout Register!)"), {FString(ShiftData::ProductLabel(Cust->Spec.MonsterSku))});
			return;
		}
	}

	// 3. Checkout Register / Scanner / Counter (or Ancestral Hall Shrine in Otherworld)
	if (HitActor)
	{
		const bool bInAncestralHall = GM->Director && GM->Director->IsEffectiveShifted();
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
					const bool bValidLostDelivery = HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product
						&& HeldItem->Sku == C->Spec.MonsterSku && (GM->IsAutomatedSession() || HeldItem->bLostItem);
					if (bValidLostDelivery)
					{
						CrosshairPrompt = ShiftLoc::Format(bInAncestralHall
							? TEXT("[E] Offer %s at the shrine to appease the spirit!")
							: TEXT("[E] Deliver %s to appease the entity!"), {FString(Demanded)});
						if (bInteractJustPressed)
						{
							ConsumeHeldItem();
							GM->OnMonsterDemandDelivered(C);
						}
						return;
					}
					if (HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product && HeldItem->Sku == C->Spec.MonsterSku && !HeldItem->bLostItem)
					{
						CrosshairPrompt = TEXT("Not its lost item! Find the red-glowing lost item spawned in the store!");
						return;
					}
					CrosshairPrompt = ShiftLoc::Format(bInAncestralHall
						? TEXT("Ancestral Shrine: Follow the cyan beacon and offer the Cursed Relic [%s] here")
						: TEXT("Checkout: Bring the cyan-glowing [%s] here to banish the entity"), {FString(Demanded)});
					return;
				}
				if (C->State == ECustomerState::WaitingAtCheckout && !bInAncestralHall)
				{
					CrosshairPrompt = TEXT("[E] Ring Up Customer");
					if (bInteractJustPressed)
					{
						GM->OnCheckoutCompleted(C);
					}
					return;
				}
			}
			if (bInAncestralHall && !GM->IsAutomatedSession())
			{
				CrosshairPrompt = GM->NightIndex == 0
					? TEXT("Three incense sticks still smolder on the shrine... yet the ash is soaking wet.")
					: (GM->NightIndex == 1
						? TEXT("Only two incense sticks remain lit. Someone was just kneeling here.")
						: TEXT("One last incense stick flickers before the tablet. The name on it looks familiar..."));
				return;
			}
		}

		// 4. Oden station — hidden inside the Otherworld ancestral hall
		if (HitActor->ActorHasTag(TEXT("Interactive.Oden")) && (!bInAncestralHall || GM->IsAutomatedSession()))
		{
			if (HeldItem.IsValid())
			{
				if (HeldItem->bHotCup)
				{
					CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Put back %s"), {FString(HeldItem->DisplayName())});
					if (bInteractJustPressed) { ConsumeHeldItem(); }
				}
				else
				{
					CrosshairPrompt = ShiftLoc::Text(TEXT("Hands full. Put it back on a shelf, or press [G] to drop it."));
				}
				return;
			}
			CrosshairPrompt = TEXT("[E] Take Hot Oden Cup (Strong Stun Throwable)");
			if (bInteractJustPressed)
			{
				AShiftItem* Oden = GetWorld()->SpawnActor<AShiftItem>(GetActorLocation(), FRotator::ZeroRotator);
				Oden->InitProduct(TEXT("Noodles"));
				Oden->bHotCup = true;
				PickupItem(Oden);
			}
			return;
		}

		// Put merchandise back: only available in Normal convenience store state (hidden shelves in Otherworld must not prompt)
		if ((!bInAncestralHall || GM->IsAutomatedSession()) && HeldItem.IsValid() && HeldItem->Kind == EShiftItemKind::Product && !HeldItem->bLostItem && !HeldItem->bHotCup)
		{
			const FString HitName = HitActor->GetName();
			const bool bShelfLike = HitActor->ActorHasTag(TEXT("Fixture.Shelf")) || HitActor->ActorHasTag(TEXT("Interactive.FridgeDoor"))
				|| HitName.Contains(TEXT("Fridge")) || HitName.Contains(TEXT("WallShelf")) || HitName.Contains(TEXT("Drinks"))
				|| HitName.Contains(TEXT("Gondola")) || HitName.Contains(TEXT("Shelf"))
				|| !ExtractSkuTag(HitActor).IsNone();
			if (bShelfLike)
			{
				CrosshairPrompt = ShiftLoc::Format(TEXT("[E] Put back %s"), {FString(HeldItem->DisplayName())});
				if (bInteractJustPressed) { ConsumeHeldItem(); }
				return;
			}
		}

		// 5. Already-spawned AShiftItem
		if (AShiftItem* ShiftItem = Cast<AShiftItem>(HitActor))
		{
			if (HeldItem.IsValid())
			{
				CrosshairPrompt = ShiftLoc::Text(TEXT("Hands full. Put it back on a shelf, or press [G] to drop it."));
				return;
			}
			CrosshairPrompt = ShiftItem->bLostItem
				? ShiftLoc::Format(TEXT("[E] Take Cursed Relic (%s) -> Offer at Shrine!"), {FString(ShiftItem->DisplayName())})
				: ShiftLoc::Format(TEXT("[E] Pick up %s"), {FString(ShiftItem->DisplayName())});
			if (bInteractJustPressed)
			{
				PickupItem(ShiftItem);
			}
			return;
		}

		// 6. Mop / Broom props in the level
		if (HitActor->ActorHasTag(TEXT("Tool.Mop")) || HitActor->ActorHasTag(TEXT("Tool.Broom")))
		{
			if (HeldItem.IsValid())
			{
				CrosshairPrompt = ShiftLoc::Text(TEXT("Hands full. Put it back on a shelf, or press [G] to drop it."));
				return;
			}
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

		// 7. In the Otherworld ancestral hall, never grab invisible modern shelves — show atmospheric environmental whispers instead!
		if (bInAncestralHall && !GM->IsAutomatedSession())
		{
			if (FVector::Dist2D(Hit.ImpactPoint, FVector(800.f, 450.f, 0.f)) < 145.f)
			{
				CrosshairPrompt = GM->NightIndex == 0
					? TEXT("A sealed black wooden coffin... deep scratch marks line the inner seam.")
					: (GM->NightIndex == 1
						? TEXT("The coffin lid has slid open a crack... freezing damp air seeps out.")
						: TEXT("The coffin lid lies overturned. Inside is completely empty."));
			}
			else if (FVector::Dist2D(Hit.ImpactPoint, FVector(800.f, 972.f, 0.f)) < 135.f)
			{
				CrosshairPrompt = TEXT("A faded memorial table... two pale green flames refuse to go out.");
			}
			else if (GM->Director && GM->Director->IsMonsterShifted() && (HitActor->ActorHasTag(TEXT("Fixture.Shelf")) || HitActor->GetName().Contains(TEXT("Gondola"))))
			{
				CrosshairPrompt = TEXT("Press [Q] to snap back to the store for 2.5s — or pull the corridor breaker.");
			}
			return;
		}

		// 7b. Hero product props or shelf display stock -> pick up a carryable product
		const FName Sku = ExtractSkuTag(HitActor);
		const bool bTakeTarget = !Sku.IsNone() || HitActor->ActorHasTag(TEXT("Fixture.Shelf")) || HitActor->ActorHasTag(TEXT("Interactive.FridgeDoor"))
			|| HitActor->GetName().Contains(TEXT("Fridge")) || HitActor->GetName().Contains(TEXT("WallShelf")) || HitActor->GetName().Contains(TEXT("Drinks"));
		if (bTakeTarget && HeldItem.IsValid())
		{
			CrosshairPrompt = ShiftLoc::Text(TEXT("Hands full. Put it back on a shelf, or press [G] to drop it."));
			return;
		}
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
				if (GM->Director && GM->Director->IsEffectiveShifted())
				{
					BestSku = ShiftData::ShiftedSku(BestSku);
				}
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
