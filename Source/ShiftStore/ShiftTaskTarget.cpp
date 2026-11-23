#include "ShiftTaskTarget.h"
#include "ShiftStoreGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"

AShiftTaskTarget::AShiftTaskTarget()
{
	PrimaryActorTick.bCanEverTick = true;

	Hitbox = CreateDefaultSubobject<USphereComponent>(TEXT("Hitbox"));
	Hitbox->InitSphereRadius(42.f);
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Hitbox->SetCanEverAffectNavigation(false);
	RootComponent = Hitbox;

	Marker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Hitbox);
	Marker->SetHorizontalAlignment(EHTA_Center);
	Marker->SetVerticalAlignment(EVRTA_TextCenter);
	Marker->SetWorldSize(9.f);
	Marker->SetRelativeLocation(FVector(0, 0, 45));
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SpillMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Spill"));
	SpillMesh->SetupAttachment(Hitbox);
	SpillMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpillMesh->SetCanEverAffectNavigation(false);
	SpillMesh->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded()) { SpillMesh->SetStaticMesh(Cylinder.Object); }

	SpillDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SpillDecal"));
	SpillDecal->SetupAttachment(Hitbox);
	SpillDecal->SetVisibility(false);
	SpillDecal->DecalSize = FVector(28.f, 78.f, 78.f);
	SpillDecal->FadeScreenSize = 0.001f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorSpillMat(TEXT("/Game/ShiftStore/Materials/M_FloorSpill.M_FloorSpill"));
	if (FloorSpillMat.Succeeded()) { SpillBaseMaterial = FloorSpillMat.Object; }

	static ConstructorHelpers::FObjectFinder<UTexture2D> CoffeeD(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Coffee_D.T_Spill_Coffee_D"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CoffeeN(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Coffee_N.T_Spill_Coffee_N"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CoffeeM(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Coffee_M.T_Spill_Coffee_M"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> IchorD(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Ichor_D.T_Spill_Ichor_D"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> IchorN(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Ichor_N.T_Spill_Ichor_N"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> IchorM(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Ichor_M.T_Spill_Ichor_M"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MudD(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Mud_D.T_Spill_Mud_D"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MudN(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Mud_N.T_Spill_Mud_N"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MudM(TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Mud_M.T_Spill_Mud_M"));

	if (CoffeeD.Succeeded() && CoffeeN.Succeeded() && CoffeeM.Succeeded())
	{
		SpillAlbedoTextures.Add(CoffeeD.Object);
		SpillNormalTextures.Add(CoffeeN.Object);
		SpillMaskTextures.Add(CoffeeM.Object);
	}
	if (IchorD.Succeeded() && IchorN.Succeeded() && IchorM.Succeeded())
	{
		SpillAlbedoTextures.Add(IchorD.Object);
		SpillNormalTextures.Add(IchorN.Object);
		SpillMaskTextures.Add(IchorM.Object);
	}
	if (MudD.Succeeded() && MudN.Succeeded() && MudM.Succeeded())
	{
		SpillAlbedoTextures.Add(MudD.Object);
		SpillNormalTextures.Add(MudN.Object);
		SpillMaskTextures.Add(MudM.Object);
	}
}

void AShiftTaskTarget::Setup(EShiftTaskType InType, const FString& InLabel, float TimeLimit)
{
	Type = InType;
	Label = InLabel;
	RestartTimer(TimeLimit);
	if (Type == EShiftTaskType::Spill)
	{
		if (SpillDecal && SpillBaseMaterial)
		{
			SpillMesh->SetVisibility(false);

			static uint32 NextSpillVariant = 0;
			const int32 VariantCount = FMath::Min3(SpillAlbedoTextures.Num(), SpillNormalTextures.Num(), SpillMaskTextures.Num());
			const int32 VariantIndex = VariantCount > 0 ? static_cast<int32>((NextSpillVariant++) % static_cast<uint32>(VariantCount)) : INDEX_NONE;

			const float HalfX = FMath::FRandRange(68.f, 94.f);
			const float HalfY = HalfX * FMath::FRandRange(0.84f, 1.14f);
			SpillDecal->DecalSize = FVector(28.f, HalfY, HalfX);
			SpillDecal->SetRelativeLocation(FVector(0.f, 0.f, -GetActorLocation().Z));
			SpillDecal->SetRelativeRotation(FRotator(-90.f, FMath::FRandRange(0.f, 360.f), 0.f));

			SpillMID = UMaterialInstanceDynamic::Create(SpillBaseMaterial, this);
			if (SpillMID)
			{
				if (VariantIndex != INDEX_NONE)
				{
					SpillMID->SetTextureParameterValue(TEXT("AlbedoTex"), SpillAlbedoTextures[VariantIndex]);
					SpillMID->SetTextureParameterValue(TEXT("NormalTex"), SpillNormalTextures[VariantIndex]);
					SpillMID->SetTextureParameterValue(TEXT("MaskTex"), SpillMaskTextures[VariantIndex]);
				}
				const float ToneJitter = FMath::FRandRange(0.88f, 1.12f);
				SpillMID->SetVectorParameterValue(TEXT("TintColor"), FLinearColor(ToneJitter, ToneJitter * FMath::FRandRange(0.94f, 1.04f), ToneJitter * FMath::FRandRange(0.92f, 1.04f), 1.f));
				SpillMID->SetScalarParameterValue(TEXT("WetnessScale"), FMath::FRandRange(0.92f, 1.15f));
				SpillMID->SetScalarParameterValue(TEXT("CleanProgress"), 0.f);
				SpillDecal->SetDecalMaterial(SpillMID);
			}
			SpillDecal->SetVisibility(true);
		}
		else
		{
			// Fallback if decal material is unavailable
			SpillMesh->SetVisibility(true);
			SpillMesh->SetRelativeScale3D(FVector(FMath::FRandRange(0.8f, 1.3f), FMath::FRandRange(0.6f, 1.1f), 0.01f));
			SpillMesh->SetRelativeLocation(FVector(0, 0, -GetActorLocation().Z + 1.f));
			if (UMaterialInstanceDynamic* MID = SpillMesh->CreateDynamicMaterialInstance(0))
			{
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.06f, 0.02f));
			}
		}
	}
	if (Type == EShiftTaskType::BreakerBox)
	{
		Marker->SetText(FText::FromString(TEXT("[ BREAKER ]")));
		Marker->SetTextRenderColor(FColor(120, 225, 255));
	}
	else if (Type == EShiftTaskType::FixAnomaly)
	{
		Marker->SetText(FText::FromString(TEXT("[ SHIFT! ]")));
		Marker->SetTextRenderColor(FColor(255, 110, 80));
	}
	else
	{
		Marker->SetText(FText::FromString(Type == EShiftTaskType::CartonSource ? TEXT("[ E ]") : TEXT("[ ! ]")));
		Marker->SetTextRenderColor(Type == EShiftTaskType::CartonSource ? FColor(180, 200, 190) : FColor(255, 210, 90));
	}
}

void AShiftTaskTarget::RestartTimer(float TimeLimit)
{
	Deadline = TimeLimit;
	TimeLeft = TimeLimit;
	Progress = 0.f;
}

float AShiftTaskTarget::HoldDuration() const
{
	const bool bSelfTest = FParse::Param(FCommandLine::Get(), TEXT("ShiftSelfTest"));
	switch (Type)
	{
	case EShiftTaskType::Restock:    return bSelfTest ? 1.5f : 0.8f;
	case EShiftTaskType::Count:      return bSelfTest ? 3.0f : 1.2f;
	case EShiftTaskType::Tidy:       return bSelfTest ? 2.5f : 1.0f;
	case EShiftTaskType::Trash:      return bSelfTest ? 2.0f : 1.0f;
	case EShiftTaskType::Spill:      return bSelfTest ? 2.5f : 1.0f;
	case EShiftTaskType::CookOden:   return bSelfTest ? 6.0f : 2.2f;
	case EShiftTaskType::FixAnomaly: return bSelfTest ? 1.5f : 1.0f;
	default: return 0.f;
	}
}

bool AShiftTaskTarget::NeedsItem(EShiftItemKind& OutKind) const
{
	if (Type == EShiftTaskType::Restock) { OutKind = EShiftItemKind::Carton; return true; }
	if (Type == EShiftTaskType::Spill) { OutKind = EShiftItemKind::Mop; return true; }
	return false;
}

void AShiftTaskTarget::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	if (GM && (GM->Phase == EShiftPhase::Working || GM->Phase == EShiftPhase::Cleanup)
		&& GM->ActiveTasks.Contains(this) && Deadline > 0.f && TimeLeft > 0.f)
	{
		TimeLeft = FMath::Max(0.f, TimeLeft - DeltaSeconds);
	}

	// Billboard the marker toward the camera and bob it a little.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Marker->GetComponentLocation();
		Marker->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
	const float T = GetWorld()->GetTimeSeconds();
	Marker->SetRelativeLocation(FVector(0, 0, 45.f + 3.f * FMath::Sin(T * 2.5f)));
	if (IsOverdue()) { Marker->SetTextRenderColor(FMath::Fmod(T, 0.6f) < 0.3f ? FColor(255, 60, 40) : FColor(120, 20, 10)); }
	if (Type == EShiftTaskType::Spill && SpillMID)
	{
		SpillMID->SetScalarParameterValue(TEXT("CleanProgress"), FMath::Clamp(Progress, 0.f, 1.f));
	}
}
