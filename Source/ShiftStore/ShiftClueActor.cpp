#include "ShiftClueActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AShiftClueActor::AShiftClueActor()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BodyMesh->SetCastShadow(false);

	AccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccentMesh"));
	AccentMesh->SetupAttachment(Root);
	AccentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AccentMesh->SetCastShadow(false);

	BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
	BeaconLight->SetupAttachment(Root);
	BeaconLight->SetRelativeLocation(FVector(0.f, 0.f, 22.f));
	BeaconLight->SetIntensityUnits(ELightUnits::Lumens);
	BeaconLight->SetIntensity(0.f);
	BeaconLight->SetAttenuationRadius(240.f);
	BeaconLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CubeFinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeFinder.Object);
		AccentMesh->SetStaticMesh(CylFinder.Succeeded() ? CylFinder.Object : CubeFinder.Object);
	}
}

void AShiftClueActor::Configure(int32 InClueId, int32 InMinNightIndex, bool bInOtherworldOnly, bool bInNormalOnly, bool bInIsBrassKey, bool bInRequiresBrassKey, bool bInRequiresOfficePhone, const FString& InTitleKey, const FString& InBodyKey)
{
	ClueId = InClueId;
	MinNightIndex = InMinNightIndex;
	bOtherworldOnly = bInOtherworldOnly;
	bNormalOnly = bInNormalOnly;
	bIsBrassKey = bInIsBrassKey;
	bRequiresBrassKey = bInRequiresBrassKey;
	bRequiresOfficePhone = bInRequiresOfficePhone;
	TitleKey = InTitleKey;
	BodyKey = InBodyKey;

	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (bIsBrassKey)
	{
		// Gleaming tarnished brass key
		BodyMesh->SetRelativeScale3D(FVector(0.18f, 0.045f, 0.025f));
		AccentMesh->SetRelativeLocation(FVector(-9.f, 0.f, 0.f));
		AccentMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.025f));
		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.45f, 1.05f, 0.28f, 1.f));
				BodyMesh->SetMaterial(0, MID);
				AccentMesh->SetMaterial(0, MID);
			}
		}
		BeaconLight->SetLightColor(FLinearColor(1.0f, 0.78f, 0.24f, 1.f));
	}
	else if (bRequiresBrassKey)
	{
		// Sealed dark iron-wood casket under the rear ancestral altar
		BodyMesh->SetRelativeScale3D(FVector(0.36f, 0.24f, 0.16f));
		AccentMesh->SetRelativeLocation(FVector(0.f, -12.f, 2.f));
		AccentMesh->SetRelativeScale3D(FVector(0.07f, 0.04f, 0.08f));
		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* BodyMID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				BodyMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.18f, 0.06f, 0.05f, 1.f));
				BodyMesh->SetMaterial(0, BodyMID);
			}
			if (UMaterialInstanceDynamic* LockMID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				LockMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.25f, 0.88f, 0.24f, 1.f));
				AccentMesh->SetMaterial(0, LockMID);
			}
		}
		BeaconLight->SetLightColor(FLinearColor(0.18f, 0.96f, 0.88f, 1.f));
	}
	else
	{
		// Aged-paper archive folder / handover letter with vermilion wax seal
		BodyMesh->SetRelativeScale3D(FVector(0.28f, 0.21f, 0.022f));
		AccentMesh->SetRelativeLocation(FVector(6.f, 4.f, 1.5f));
		AccentMesh->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.012f));
		if (BaseMat)
		{
			if (UMaterialInstanceDynamic* PaperMID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				const FLinearColor PaperCol = bOtherworldOnly ? FLinearColor(0.85f, 0.92f, 0.88f, 1.f) : FLinearColor(0.95f, 0.88f, 0.70f, 1.f);
				PaperMID->SetVectorParameterValue(TEXT("Color"), PaperCol);
				BodyMesh->SetMaterial(0, PaperMID);
			}
			if (UMaterialInstanceDynamic* SealMID = UMaterialInstanceDynamic::Create(BaseMat, this))
			{
				SealMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.82f, 0.10f, 0.08f, 1.f));
				AccentMesh->SetMaterial(0, SealMID);
			}
		}
		BeaconLight->SetLightColor(bOtherworldOnly ? FLinearColor(0.18f, 0.96f, 0.86f, 1.f) : FLinearColor(1.0f, 0.76f, 0.32f, 1.f));
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

bool AShiftClueActor::IsCurrentlyVisible(int32 NightIndex, bool bEffectiveShifted, int32 CollectedMask, bool bHasBrassKey, bool bPhoneAnswered) const
{
	if (NightIndex < MinNightIndex) { return false; }
	if (bIsBrassKey)
	{
		if (bHasBrassKey) { return false; }
	}
	else if (ClueId >= 0 && (CollectedMask & (1 << ClueId)) != 0)
	{
		return false;
	}
	if (bRequiresOfficePhone && !bPhoneAnswered) { return false; }
	if (bOtherworldOnly && !bEffectiveShifted) { return false; }
	if (bNormalOnly && bEffectiveShifted) { return false; }
	return true;
}

void AShiftClueActor::RefreshVisualState(int32 NightIndex, bool bEffectiveShifted, int32 CollectedMask, bool bHasBrassKey, bool bPhoneAnswered)
{
	const bool bShouldShow = IsCurrentlyVisible(NightIndex, bEffectiveShifted, CollectedMask, bHasBrassKey, bPhoneAnswered);
	if (bVisibleNow != bShouldShow)
	{
		bVisibleNow = bShouldShow;
		SetActorHiddenInGame(!bVisibleNow);
		SetActorEnableCollision(bVisibleNow);
		if (!bVisibleNow && BeaconLight)
		{
			BeaconLight->SetIntensity(0.f);
		}
	}
}

void AShiftClueActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bVisibleNow || !BeaconLight) { return; }
	PulseTime += DeltaSeconds;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(PulseTime * 3.2f);
	const float BaseIntensity = bOtherworldOnly ? 140.f : 95.f;
	BeaconLight->SetIntensity(BaseIntensity + Pulse * 85.f);
}
