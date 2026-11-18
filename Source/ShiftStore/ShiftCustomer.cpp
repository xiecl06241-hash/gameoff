#include "ShiftCustomer.h"
#include "ShiftLocalization.h"
#include "ShiftGuidance.h"
#include "ShiftStoreGameMode.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftItem.h"
#include "ShiftTaskTarget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"

AShiftCustomer::AShiftCustomer()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCapsuleComponent()->SetCollisionObjectType(ShiftData::CustomerCollisionChannel);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 190.f;

	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		// CDO references make the mannequin, animation BP and their dependencies cookable.
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> CustomerMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
		static ConstructorHelpers::FClassFinder<UAnimInstance> CustomerAnimation(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
		if (CustomerMesh.Succeeded()) { Skel->SetSkeletalMesh(CustomerMesh.Object); }
		if (CustomerAnimation.Succeeded()) { Skel->SetAnimInstanceClass(CustomerAnimation.Class); }
		Skel->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
		Skel->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		Skel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Skel->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Skel->SetCollisionResponseToAllChannels(ECR_Ignore);
		Skel->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Skel->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		Skel->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	}

	Speech = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Speech"));
	Speech->SetupAttachment(RootComponent);
	Speech->SetRelativeLocation(FVector(0.f, 0.f, 108.f));
	Speech->SetHorizontalAlignment(EHTA_Center);
	Speech->SetVerticalAlignment(EVRTA_TextCenter);
	Speech->SetWorldSize(7.5f);
	Speech->SetTextRenderColor(FColor(235, 240, 230));
	Speech->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Speech->SetVisibility(false); // Drawn by ShiftHUD with the selected runtime CJK font.

	EyeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeLight"));
	EyeLight->SetupAttachment(RootComponent);
	EyeLight->SetRelativeLocation(FVector(24.f, 0.f, 68.f));
	EyeLight->SetIntensityUnits(ELightUnits::Lumens);
	EyeLight->SetIntensity(0.f);
	EyeLight->SetLightColor(FLinearColor(0.95f, 0.12f, 0.08f));
	EyeLight->SetAttenuationRadius(240.f);
	EyeLight->SetCastShadows(false);
}

void AShiftCustomer::BeginPlay()
{
	Super::BeginPlay();
}

void AShiftCustomer::Init(const FShiftCustomerSpec& InSpec)
{
	Spec = InSpec;
	State = ECustomerState::Entering;
	TargetPos = ShiftData::InsideDoor;
	StateTimer = 0.f;
	bHasStoreRouteGoal = false;
	StoreRoute.Empty();
	Speech->SetText(FText::GetEmpty());
	bBlockedByPlayer = false;
	BlockedSeconds = 0.f;
	ClearSeconds = 0.f;
	CourtesyTime = 0.f;
	CourtesyCount = 0;
	LastCourtesyTime = -100.;
}

bool AShiftCustomer::CanSeePlayer(APawn* Player, float MaxDist, float CosHalfFOV) const
{
	if (!Player) { return false; }
	const FVector Eye = GetActorLocation() + FVector(0, 0, 64.f);
	const FVector Target = Player->GetActorLocation() + FVector(0, 0, 50.f);
	const FVector Diff = Target - Eye;
	const float DistSq = Diff.SizeSquared();
	if (DistSq > MaxDist * MaxDist) { return false; }
	const FVector Dir = Diff.GetSafeNormal();
	if (FVector::DotProduct(GetActorForwardVector(), Dir) < CosHalfFOV && DistSq > 220.f * 220.f)
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("MonsterSight"), false, this);
	Params.AddIgnoredActor(Player);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Target, ECC_Visibility, Params);
}

bool AShiftCustomer::PlayerSeesMe(APawn* Player) const
{
	if (!Player) { return false; }
	APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const FVector CamPos = Cam ? Cam->GetCameraLocation() : Player->GetActorLocation() + FVector(0, 0, 64.f);
	const FVector CamFwd = Cam ? Cam->GetCameraRotation().Vector() : Player->GetActorForwardVector();
	const FVector ToMe = (GetActorLocation() + FVector(0, 0, 50.f)) - CamPos;
	if (ToMe.SizeSquared() > 1400.f * 1400.f) { return false; }
	if (FVector::DotProduct(CamFwd, ToMe.GetSafeNormal()) < 0.45f) { return false; }
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("WitnessCheck"), false, Player);
	Params.AddIgnoredActor(this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, CamPos, GetActorLocation() + FVector(0, 0, 50.f), ECC_Visibility, Params);
}

void AShiftCustomer::MoveToward(const FVector& Goal, float Speed, float DeltaSeconds)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	if (!IsMonsterActive())
	{
		FollowStoreRoute(Goal, DeltaSeconds);
		return;
	}
	MonsterMoveToward(Goal, DeltaSeconds);
}

bool AShiftCustomer::MonsterPathClear(const FVector& From, const FVector& Dir, float Distance) const
{
	// Full-height capsule sweep so low props (bins, extinguishers, carts) block just like they do the body.
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(TEXT("MonsterPath"), false, this);
	const FVector Start = From + FVector(0.f, 0.f, 6.f);
	GetWorld()->SweepMultiByChannel(Hits, Start, Start + Dir * Distance, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeCapsule(34.f, 82.f), Params);
	for (const FHitResult& Hit : Hits)
	{
		const AActor* Other = Hit.GetActor();
		if (Hit.bBlockingHit && !Hit.bStartPenetrating && Other && !Other->IsA<APawn>() && !Other->IsA<AShiftItem>() && !Other->IsA<AShiftTaskTarget>())
		{
			return false;
		}
	}
	return true;
}

void AShiftCustomer::MonsterMoveToward(const FVector& Goal, float DeltaSeconds)
{
	const FVector Current = GetActorLocation();
	FVector Flat = Goal - Current;
	Flat.Z = 0.f;
	const float Dist = Flat.Size();
	if (Dist < 15.f) { return; }
	const FVector Direct = Flat / Dist;
	MonsterReplanCooldown -= DeltaSeconds;

	// Stuck watchdog: if input is applied but we are not progressing, swap sides and force a fresh route.
	MonsterStuckTime = FVector::DistSquared2D(Current, MonsterLastPosition) < 4.f ? MonsterStuckTime + DeltaSeconds : 0.f;
	MonsterLastPosition = Current;
	if (MonsterStuckTime > 0.45f)
	{
		MonsterStuckTime = 0.f;
		bMonsterSteerRight = !bMonsterSteerRight;
		bHasStoreRouteGoal = false;
		StoreRoute.Empty();
		MonsterReplanCooldown = 0.f;
	}

	// 1. Straight line is clear: chase directly.
	if (MonsterPathClear(Current, Direct, FMath::Min(Dist, 110.f)))
	{
		StoreRoute.Empty();
		bHasStoreRouteGoal = false;
		AddMovementInput(Direct, 1.f);
		return;
	}

	// 2. Blocked: follow a route through the store's aisle graph, replanning at most twice a second.
	const bool bNeedRoute = !bHasStoreRouteGoal || !StoreRouteGoal.Equals(Goal, 120.f) || StoreRoute.IsEmpty();
	if (bNeedRoute && MonsterReplanCooldown <= 0.f)
	{
		MonsterReplanCooldown = 0.5f;
		StoreRoute = ShiftGuidance::FindRoute(GetWorld(), this, nullptr, Goal, false).Waypoints;
		StoreRouteGoal = Goal;
		bHasStoreRouteGoal = true;
		StoreRouteIndex = 0;
	}
	while (StoreRoute.IsValidIndex(StoreRouteIndex) && FVector::Dist2D(Current, StoreRoute[StoreRouteIndex]) < 40.f) { ++StoreRouteIndex; }
	if (StoreRoute.IsValidIndex(StoreRouteIndex))
	{
		const FVector WaypointDir = (StoreRoute[StoreRouteIndex] - Current).GetSafeNormal2D();
		if (MonsterPathClear(Current, WaypointDir, 80.f))
		{
			AddMovementInput(WaypointDir, 1.f);
			return;
		}
	}

	// 3. No usable route: slide around the obstacle, trying progressively wider angles on the preferred side.
	const float Side = bMonsterSteerRight ? -1.f : 1.f;
	for (const float Angle : { 35.f, 70.f, 105.f, 140.f })
	{
		for (const float Sign : { Side, -Side })
		{
			const FVector Candidate = Direct.RotateAngleAxis(Angle * Sign, FVector::UpVector);
			if (MonsterPathClear(Current, Candidate, 90.f))
			{
				AddMovementInput(Candidate, 1.f);
				return;
			}
		}
	}
	AddMovementInput(Direct, 1.f);
}

void AShiftCustomer::FollowStoreRoute(const FVector& Goal, float DeltaSeconds)
{
	StoreRouteRetry -= DeltaSeconds;
	const FVector Current = GetActorLocation();
	if (!bHasStoreRouteGoal || !StoreRouteGoal.Equals(Goal, 1.f) || (StoreRoute.IsEmpty() && StoreRouteRetry <= 0.f))
	{
		StoreRoute = ShiftGuidance::FindRoute(GetWorld(), this, nullptr, Goal, true).Waypoints;
		StoreRouteGoal = Goal;
		bHasStoreRouteGoal = true;
		StoreRouteIndex = 0;
		StoreRouteRetry = 1.f;
		RouteStalledTime = 0.f;
		PreviousRouteLocation = Current;
	}
	while (StoreRoute.IsValidIndex(StoreRouteIndex) && FVector::Dist2D(Current, StoreRoute[StoreRouteIndex]) < 30.f) { ++StoreRouteIndex; }
	if (!StoreRoute.IsValidIndex(StoreRouteIndex))
	{
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}
	RouteStalledTime = FVector::DistSquared2D(Current, PreviousRouteLocation) < 0.25f
		? RouteStalledTime + DeltaSeconds : 0.f;
	PreviousRouteLocation = Current;
	if (RouteStalledTime > 0.8f)
	{
		// A temporarily blocked doorway should produce a wait/retry, not alternating sidesteps.
		StoreRoute.Empty();
		StoreRouteRetry = 0.35f;
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}
	const FVector Direction = (StoreRoute[StoreRouteIndex] - Current).GetSafeNormal2D();
	if (!UpdatePlayerBlock(DeltaSeconds, Direction)) { AddMovementInput(Direction, 1.f); }
}

bool AShiftCustomer::UpdatePlayerBlock(float DeltaSeconds, const FVector& Direction)
{
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params(TEXT("CustomerPlayerBlock"), false, this);
	FHitResult Hit;
	const FVector Start = GetActorLocation();
	const bool bHitPlayer = GetWorld()->SweepSingleByObjectType(Hit, Start, Start + Direction * 100.f,
		FQuat::Identity, Objects, FCollisionShape::MakeCapsule(34.f, 85.f), Params)
		&& Hit.GetActor() == UGameplayStatics::GetPlayerPawn(this, 0);
	if (bHitPlayer) { bBlockedByPlayer = true; ClearSeconds = 0.f; }
	else if (bBlockedByPlayer)
	{
		ClearSeconds += DeltaSeconds;
		if (ClearSeconds >= 0.2f) { bBlockedByPlayer = false; BlockedSeconds = 0.f; }
	}
	if (!bBlockedByPlayer) { return false; }
	BlockedSeconds += DeltaSeconds;
	RouteStalledTime = 0.f;
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
	const double Now = GetWorld()->GetTimeSeconds();
	if (BlockedSeconds >= 0.8f && Now - LastCourtesyTime >= 8.)
	{
		if (CourtesyTime <= 0.f) { SavedSpeech = Speech->Text; }
		Speech->SetText(FText::FromString(TEXT("Excuse me, could I get through?")));
		CourtesyTime = 2.5f;
		LastCourtesyTime = Now;
		++CourtesyCount;
	}
	if (BlockedSeconds >= 0.8f)
	{
		if (const APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			const FRotator Facing(0.f, (P->GetActorLocation() - Start).Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Facing, DeltaSeconds, 4.f));
		}
	}
	return true;
}

FVector AShiftCustomer::GrabFocusLocation() const
{
	if (const USkeletalMeshComponent* Skel = GetMesh(); Skel && Skel->GetBoneIndex(TEXT("head")) != INDEX_NONE)
	{
		return Skel->GetSocketLocation(TEXT("head")) + GetActorForwardVector() * 8.f;
	}
	return GetActorLocation() + FVector(0.f, 0.f, 80.f);
}

void AShiftCustomer::BeginLeaving()
{
	State = ECustomerState::Leaving;
	bBlockedByPlayer = false;
	BlockedSeconds = 0.f;
	ClearSeconds = 0.f;
	StateTimer = 0.f;
	TargetPos = ShiftData::OutsideDoor;
	bHasStoreRouteGoal = false;
	StoreRoute.Empty();
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
}

void AShiftCustomer::ApplyMonsterVisuals()
{
	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		Skel->SetRelativeScale3D(FVector(0.92f, 0.92f, 1.18f)); // elongated silhouette
		UMaterialInterface* DarkMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShiftStore/Materials/M_Charcoal.M_Charcoal"));
		if (DarkMat)
		{
			for (int32 i = 0; i < Skel->GetNumMaterials(); ++i)
			{
				Skel->SetMaterial(i, DarkMat);
			}
		}
	}
	EyeLight->SetIntensity(95.f);
}

void AShiftCustomer::RevealMonster(APawn* Player)
{
	bWitnessedReveal = PlayerSeesMe(Player);
	ApplyMonsterVisuals();
	CourtesyTime = 0.f;
	bBlockedByPlayer = false;
	State = bWitnessedReveal ? ECustomerState::MonsterHunting : ECustomerState::MonsterStalking;
	StateTimer = 0.f;
	Speech->SetTextRenderColor(FColor(255, 80, 70));
	Speech->SetText(FText::FromString(ShiftLoc::Format(TEXT("WHERE IS MY %s"), {FString(ShiftData::ProductLabel(Spec.MonsterSku))})));
	PickSearchPoint();

	if (AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>())
	{
		GM->OnMonsterRevealed(this, bWitnessedReveal);
	}
}

void AShiftCustomer::PickSearchPoint()
{
	const TArray<FVector>& Pts = ShiftData::BrowsePoints();
	if (Pts.Num() > 0)
	{
		PatrolIdx = (PatrolIdx + FMath::RandRange(1, Pts.Num() - 1)) % Pts.Num();
		TargetPos = Pts[PatrolIdx];
	}
}

void AShiftCustomer::Stun(float Seconds)
{
	if (!IsMonsterActive()) { return; }
	if (State == ECustomerState::MonsterGrabbing)
	{
		if (AShiftPlayerCharacter* P = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			P->ReleaseFromGrab();
		}
	}
	State = ECustomerState::MonsterStunned;
	StunTimer = FMath::Max(StunTimer, Seconds);
	Speech->SetText(FText::FromString(TEXT("[ STUNNED ]")));
}

void AShiftCustomer::OnReleasedGrab()
{
	State = ECustomerState::MonsterStunned;
	StunTimer = 1.6f;
	Speech->SetText(FText::FromString(ShiftLoc::Format(TEXT("BRING ME %s -> REGISTER"), {FString(ShiftData::ProductLabel(Spec.MonsterSku))})));
}

void AShiftCustomer::SatisfyAndLeave()
{
	if (State == ECustomerState::MonsterGrabbing)
	{
		if (AShiftPlayerCharacter* P = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			P->ReleaseFromGrab();
		}
	}
	EyeLight->SetIntensity(0.f);
	Speech->SetTextRenderColor(FColor(200, 220, 210));
	Speech->SetText(FText::FromString(TEXT("...Thank you.")));
	BeginLeaving();
}

void AShiftCustomer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bSeeingPlayer = false;

	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Speech->GetComponentLocation();
		Speech->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}

	AShiftPlayerCharacter* Player = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	if (GM && GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup)
	{
		ConsumeMovementInputVector();
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}
	if (CourtesyTime > 0.f)
	{
		CourtesyTime = FMath::Max(0.f, CourtesyTime - DeltaSeconds);
		if (CourtesyTime == 0.f && !IsMonsterActive() && Speech->Text.ToString() == TEXT("Excuse me, could I get through?")) { Speech->SetText(SavedSpeech); }
	}
	// Moving states advance their timers only after the obstruction check below.
	const bool bMoving = State == ECustomerState::Entering || State == ECustomerState::Browsing
		|| State == ECustomerState::WalkingToCheckout || State == ECustomerState::Leaving;
	if (!bMoving) { StateTimer += DeltaSeconds; }

	switch (State)
	{
	case ECustomerState::Entering:
		MoveToward(TargetPos, 185.f, DeltaSeconds);
		if (!bBlockedByPlayer) { StateTimer += DeltaSeconds; }
		if (!bBlockedByPlayer && FVector::Dist2D(GetActorLocation(), TargetPos) < 45.f)
		{
			State = ECustomerState::Browsing;
			StateTimer = 0.f;
			const TArray<FVector>& Pts = ShiftData::BrowsePoints();
			TargetPos = Pts[FMath::RandRange(0, Pts.Num() - 1)];
		}
		break;

	case ECustomerState::Browsing:
		MoveToward(TargetPos, 175.f, DeltaSeconds);
		if (!bBlockedByPlayer) { StateTimer += DeltaSeconds; }
		if (!bBlockedByPlayer && FVector::Dist2D(GetActorLocation(), TargetPos) < 55.f)
		{
			StateTimer = 0.f;
			if (Spec.bAsksHelp)
			{
				State = ECustomerState::WaitingForHelp;
				ConsumeMovementInputVector();
				GetCharacterMovement()->StopMovementImmediately();
				Speech->SetText(FText::FromString(ShiftLoc::Format(TEXT("[Need: %s]"), {FString(ShiftData::ProductLabel(Spec.WantedSku))})));
				if (GM) { GM->OnCustomerNeedsHelp(this); }
			}
			else
			{
				State = ECustomerState::WalkingToCheckout;
				TargetPos = ShiftData::CheckoutCustomerSpot;
			}
		}
		break;

	case ECustomerState::WaitingForHelp:
		if (Player)
		{
			const FVector ToP = Player->GetActorLocation() - GetActorLocation();
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, ToP.Rotation().Yaw, 0), DeltaSeconds, 5.f));
		}
		// Monster customers reveal while the clerk goes to fetch their product
		if (Spec.bMonster && StateTimer > 4.2f)
		{
			RevealMonster(Player);
		}
		break;

	case ECustomerState::WalkingToCheckout:
		MoveToward(TargetPos, 185.f, DeltaSeconds);
		if (!bBlockedByPlayer) { StateTimer += DeltaSeconds; }
		if (!bBlockedByPlayer && FVector::Dist2D(GetActorLocation(), TargetPos) < 55.f)
		{
			State = ECustomerState::WaitingAtCheckout;
			ConsumeMovementInputVector();
			GetCharacterMovement()->StopMovementImmediately();
			StateTimer = 0.f;
			Speech->SetText(FText::FromString(TEXT("[ Waiting at Register - Press E ]")));
			if (GM) { GM->OnCustomerAtCheckout(this); }
		}
		break;

	case ECustomerState::WaitingAtCheckout:
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, -90.f, 0), DeltaSeconds, 5.f));
		break;

	case ECustomerState::MonsterStalking:
	case ECustomerState::MonsterHunting:
	{
		if (!Player) { break; }
		const bool bSees = CanSeePlayer(Player, State == ECustomerState::MonsterStalking ? 1300.f : 1100.f, 0.2f);
		bSeeingPlayer = bSees;
		if (bSees)
		{
			TargetPos = Player->GetActorLocation();
			RepathTimer = 0.f;
		}
		else
		{
			RepathTimer += DeltaSeconds;
			if (FVector::Dist2D(GetActorLocation(), TargetPos) < 65.f || RepathTimer > 4.5f)
			{
				RepathTimer = 0.f;
				PickSearchPoint();
			}
		}
		const float HuntSpeed = bSees ? 248.f : 195.f; // Slightly slower than player (260) so looping shelves works
		MoveToward(TargetPos, HuntSpeed, DeltaSeconds);

		if (FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) < 92.f)
		{
			const bool bUnseenAmbush = (State == ECustomerState::MonsterStalking);
			State = ECustomerState::MonsterGrabbing;
			StateTimer = 0.f;
			Speech->SetText(FText::FromString(TEXT("[ GRABBED! Mash E or throw item! ]")));
			Player->OnGrabbedByMonster(this, bUnseenAmbush);
		}
		break;
	}

	case ECustomerState::MonsterGrabbing:
		if (Player)
		{
			const FVector ToP = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			SetActorRotation(ToP.Rotation());
		}
		break;

	case ECustomerState::MonsterStunned:
		StunTimer -= DeltaSeconds;
		if (StunTimer <= 0.f)
		{
			State = ECustomerState::MonsterHunting;
			Speech->SetText(FText::FromString(ShiftLoc::Format(TEXT("WHERE IS MY %s"), {FString(ShiftData::ProductLabel(Spec.MonsterSku))})));
		}
		break;

	case ECustomerState::Leaving:
		MoveToward(TargetPos, 210.f, DeltaSeconds);
		if (!bBlockedByPlayer) { StateTimer += DeltaSeconds; }
		if (!bBlockedByPlayer && FVector::Dist2D(GetActorLocation(), TargetPos) < 60.f)
		{
			if (GM) { GM->OnCustomerFinished(this); }
			Destroy();
		}
		break;
	}
}
