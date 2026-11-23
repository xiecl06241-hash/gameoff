#include "ShiftCustomer.h"
#include "ShiftLocalization.h"
#include "ShiftGuidance.h"
#include "ShiftStoreGameMode.h"
#include "ShiftDirector.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftItem.h"
#include "ShiftTaskTarget.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/StaticMesh.h"
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

	MonsterPose = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("MonsterPose"));
	MonsterPose->SetupAttachment(GetMesh());
	MonsterPose->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	MonsterPose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MonsterPose->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EyeSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EyeMaterial(TEXT("/Game/ShiftStore/Materials/M_ColdLight.M_ColdLight"));
	AnomalyMaskLeftEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnomalyMaskLeftEye"));
	AnomalyMaskRightEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnomalyMaskRightEye"));
	for (UStaticMeshComponent* Eye : { AnomalyMaskLeftEye.Get(), AnomalyMaskRightEye.Get() })
	{
		Eye->SetupAttachment(GetMesh(), TEXT("head"));
		Eye->SetStaticMesh(EyeSphere.Object);
		Eye->SetMaterial(0, EyeMaterial.Object);
		Eye->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Eye->SetVisibility(false);
		Eye->SetRelativeScale3D(FVector(0.06f, 0.04f, 0.09f));
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
	TellTimer = 3.0f;
	StareTriggerTime = 0.f;
	OtherworldPeekTime = 0.f;
	bOtherworldPreviewActive = false;
	if (Spec.bMonster && Spec.Tell == EDisguiseTell::None)
	{
		if (Spec.MonsterSku == TEXT("Milk"))       { Spec.Tell = EDisguiseTell::RedGlare; }
		else if (Spec.MonsterSku == TEXT("Bread")) { Spec.Tell = EDisguiseTell::Floating; }
		else                                       { Spec.Tell = EDisguiseTell::HeadTilt; }
	}
	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		Skel->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
		Skel->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		Skel->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
	}
	AnomalyMaskLeftEye->SetVisibility(false);
	AnomalyMaskRightEye->SetVisibility(false);
	MonsterPose->SetVisibility(false);
	GetMesh()->SetVisibility(true);
	EyeLight->SetIntensity(0.f);
	bAutomatedTest = FParse::Param(FCommandLine::Get(), TEXT("ShiftSelfTest"))
		|| FParse::Param(FCommandLine::Get(), TEXT("ShiftPlaythroughTest"))
		|| FParse::Param(FCommandLine::Get(), TEXT("ShiftFeedbackTest"))
		|| FParse::Param(FCommandLine::Get(), TEXT("ShiftLocalizationTest"))
		|| FParse::Param(FCommandLine::Get(), TEXT("ShiftPauseTest"))
		|| FParse::Param(FCommandLine::Get(), TEXT("ShiftSensitivityTest"));
	RevealThreshold = bAutomatedTest ? 4.2f : FMath::FRandRange(13.0f, 17.0f);
	const FLinearColor Colors[] = {
		{ 0.18f, 0.25f, 0.38f },
		{ 0.16f, 0.32f, 0.24f },
		{ 0.38f, 0.26f, 0.18f },
		{ 0.34f, 0.16f, 0.22f },
		{ 0.28f, 0.30f, 0.32f }
	};
	const FLinearColor Tint = Colors[bAutomatedTest ? 0 : FMath::RandRange(0, 4)];
	for (int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot)
	{
		if (UMaterialInstanceDynamic* Material = GetMesh()->CreateDynamicMaterialInstance(Slot))
		{
			for (const FName Param : { FName(TEXT("Tint")), FName(TEXT("BaseColor")), FName(TEXT("Color")), FName(TEXT("BodyColor")), FName(TEXT("Paint Tint")) })
			{
				Material->SetVectorParameterValue(Param, Tint);
			}
		}
	}
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
	if (FVector::DistSquared2D(Current, MonsterLastPosition) >= 4.f)
	{
		MonsterLastPosition = Current;
		MonsterStuckTime = 0.f;
	}
	MonsterStuckTime += DeltaSeconds;
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
	const bool bNeedRoute = !bHasStoreRouteGoal || !StoreRouteGoal.Equals(Goal, 120.f) || !StoreRoute.IsValidIndex(StoreRouteIndex);
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
		Skel->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
		Skel->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
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
	MonsterPose->SetSkeletalMesh(GetMesh()->GetSkeletalMeshAsset());
	MonsterPose->CopyPoseFromSkeletalComponent(GetMesh());
	for (int32 Slot = 0; Slot < GetMesh()->GetNumMaterials(); ++Slot)
	{
		MonsterPose->SetMaterial(Slot, GetMesh()->GetMaterial(Slot));
	}
	MonsterPose->SetVisibility(true);
	GetMesh()->SetVisibility(false);
	for (UStaticMeshComponent* Eye : { AnomalyMaskLeftEye.Get(), AnomalyMaskRightEye.Get() })
	{
		const bool bLeft = Eye == AnomalyMaskLeftEye.Get();
		const bool bHead = GetMesh()->DoesSocketExist(TEXT("head"));
		Eye->AttachToComponent(MonsterPose, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bHead ? FName(TEXT("head")) : NAME_None);
		Eye->SetRelativeLocation(bHead ? FVector(bLeft ? 4.f : 7.f, 12.f, bLeft ? -5.f : 5.f) : FVector(14.f, bLeft ? -6.f : 7.f, bLeft ? 162.f : 165.f));
		Eye->SetRelativeScale3D(FVector(0.06f, 0.04f, 0.09f));
		Eye->SetVisibility(true);
	}
	EyeLight->SetIntensity(95.f);
}

void AShiftCustomer::RevealMonster(APawn* Player)
{
	if (IsMonsterActive() || State == ECustomerState::Leaving) { return; }
	ChaseElapsedSeconds = 0.f;
	ChaseFlickerTime = 0.f;
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

void AShiftCustomer::AttractToNoise(const FVector& NoiseLocation)
{
	if (!IsMonsterActive()) { return; }
	Stun(1.4f);
	TargetPos = NoiseLocation;
	RepathTimer = 0.f;
	bHasStoreRouteGoal = false;
	StoreRoute.Empty();
}

void AShiftCustomer::SetOtherworldPreview(bool bShowTrueForm)
{
	if (!Spec.bMonster || IsMonsterActive() || State == ECustomerState::Leaving) { return; }
	if (bOtherworldPreviewActive == bShowTrueForm) { return; }
	bOtherworldPreviewActive = bShowTrueForm;
	if (USkeletalMeshComponent* Skel = GetMesh())
	{
		Skel->SetRelativeScale3D(bShowTrueForm ? FVector(0.92f, 0.92f, 1.18f) : FVector(1.f, 1.f, 1.f));
		for (UStaticMeshComponent* Eye : { AnomalyMaskLeftEye.Get(), AnomalyMaskRightEye.Get() })
		{
			if (!Eye) { continue; }
			if (bShowTrueForm)
			{
				const bool bLeft = Eye == AnomalyMaskLeftEye.Get();
				const bool bHead = Skel->DoesSocketExist(TEXT("head"));
				Eye->AttachToComponent(Skel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bHead ? FName(TEXT("head")) : NAME_None);
				Eye->SetRelativeLocation(bHead ? FVector(bLeft ? 4.f : 7.f, 12.f, bLeft ? -5.f : 5.f) : FVector(14.f, bLeft ? -6.f : 7.f, bLeft ? 162.f : 165.f));
				Eye->SetRelativeScale3D(FVector(0.06f, 0.04f, 0.09f));
			}
			Eye->SetVisibility(bShowTrueForm);
		}
	}
	EyeLight->SetIntensity(bShowTrueForm ? 80.f : 0.f);
}

void AShiftCustomer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (MonsterPose->IsVisible())
	{
		MonsterPose->CopyPoseFromSkeletalComponent(GetMesh());
		const FQuat HeadRotation = MonsterPose->GetBoneRotationByName(TEXT("head"), EBoneSpaces::ComponentSpace).Quaternion();
		const FQuat TwistedHead = FQuat(FVector::UpVector, PI) * HeadRotation;
		MonsterPose->SetBoneRotationByName(TEXT("head"), TwistedHead.Rotator(), EBoneSpaces::ComponentSpace);
	}
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

	// 3D spatial footsteps for walking customers & stalking/hunting monsters (floating ghosts remain eerily silent!)
	const bool bFloatingSilent = Spec.bMonster && Spec.Tell == EDisguiseTell::Floating && !IsMonsterActive();
	if (GM && !bFloatingSilent && GetVelocity().Size2D() > 45.f)
	{
		FootstepTimer -= DeltaSeconds;
		if (FootstepTimer <= 0.f)
		{
			FootstepTimer = IsMonsterActive() ? 0.32f : 0.48f;
			const FVector FootLoc = GetActorLocation() - FVector(0.f, 0.f, 75.f);
			GM->PlayFootstepCue(&FootLoc, IsMonsterActive() ? 0.80f : 0.34f);
		}
	}
	else
	{
		FootstepTimer = FMath::Min(FootstepTimer, 0.12f);
	}

	// Active [Q] Otherworld Flash-Peek: disguised entities reveal their terrifying true form in the Otherworld,
	// and attack immediately if the player stares at their true form for more than 2.5 seconds!
	if (Spec.bMonster && !IsMonsterActive() && State != ECustomerState::Leaving && GM && GM->Director)
	{
		const bool bPeekActive = GM->Director->IsPlayerPhaseShifted();
		SetOtherworldPreview(bPeekActive);
		if (bPeekActive && Player && PlayerSeesMe(Player))
		{
			OtherworldPeekTime += DeltaSeconds;
			if (OtherworldPeekTime > 2.5f)
			{
				RevealMonster(Player);
				return;
			}
		}
		else
		{
			OtherworldPeekTime = FMath::Max(0.f, OtherworldPeekTime - DeltaSeconds * 0.6f);
		}
	}

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
			const bool bLiveTell = Spec.bMonster && RevealThreshold > 5.f;
			const bool bWallStare = bLiveTell && Spec.Tell == EDisguiseTell::HeadTilt && FMath::Fmod(StateTimer, 3.0f) > 1.2f;
			const float TargetYaw = bWallStare ? (ToP.Rotation().Yaw + 150.f) : ToP.Rotation().Yaw;
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(0, TargetYaw, 0), DeltaSeconds, bWallStare ? 10.f : 5.f));

			if (bLiveTell)
			{
				if (USkeletalMeshComponent* Skel = GetMesh())
				{
					if (Spec.Tell == EDisguiseTell::HeadTilt)
					{
						Skel->SetRelativeRotation(FRotator(16.f, -90.f, 26.f));
					}
					else if (Spec.Tell == EDisguiseTell::Floating)
					{
						Skel->SetRelativeLocation(FVector(0.f, 0.f, -64.f + 7.f * FMath::Sin(StateTimer * 3.4f)));
					}
				}
				if (Spec.Tell == EDisguiseTell::RedGlare)
				{
					EyeLight->SetIntensity(32.f + 22.f * FMath::Sin(StateTimer * 6.f));
					TellTimer -= DeltaSeconds;
					if (TellTimer <= 0.f)
					{
						TellTimer = 2.6f;
						if (GM && GM->Director && !GM->Director->IsShifted()) { GM->Director->Flicker(0.24f); }
					}
				}
				if (ToP.Size2D() < 155.f && PlayerSeesMe(Player))
				{
					StareTriggerTime += DeltaSeconds;
				}
				else
				{
					StareTriggerTime = FMath::Max(0.f, StareTriggerTime - DeltaSeconds * 0.5f);
				}
			}
		}
		if (Spec.bMonster && (StateTimer > RevealThreshold || StareTriggerTime > 2.2f))
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
		ChaseElapsedSeconds += DeltaSeconds;
		const bool bHearing = !bAutomatedTest && Spec.Tell == EDisguiseTell::HeadTilt;
		const bool bHeardSprint = bHearing && Player->bIsSprinting;
		const float SightRange = bHearing && !Player->bIsSprinting ? 700.f : (State == ECustomerState::MonsterStalking ? 1300.f : 1100.f);
		const bool bSees = CanSeePlayer(Player, SightRange, 0.2f);
		bSeeingPlayer = bSees;
		if (bSees || bHeardSprint)
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
		float HuntSpeed = bSees ? 248.f : 195.f;
		if (!bAutomatedTest)
		{
			if (Spec.Tell == EDisguiseTell::Floating) { HuntSpeed = PlayerSeesMe(Player) ? 55.f : 335.f; }
			else if (Spec.Tell == EDisguiseTell::HeadTilt)
			{
				if (bHeardSprint) { HuntSpeed = 310.f; }
				else if (FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) > 380.f) { HuntSpeed = 185.f; }
			}
			else if (Spec.Tell == EDisguiseTell::RedGlare)
			{
				HuntSpeed = 245.f + FMath::Clamp(ChaseElapsedSeconds * 2.2f, 0.f, 45.f);
				ChaseFlickerTime += DeltaSeconds;
				if (ChaseFlickerTime >= 5.f)
				{
					ChaseFlickerTime -= 5.f;
					if (GM && GM->Director) { GM->Director->Flicker(0.45f); }
				}
			}
		}
		MoveToward(TargetPos, HuntSpeed, DeltaSeconds);

		if (bSees && FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) < 92.f)
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
