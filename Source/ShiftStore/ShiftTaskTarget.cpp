#include "ShiftTaskTarget.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
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
}

void AShiftTaskTarget::Setup(EShiftTaskType InType, const FString& InLabel, float TimeLimit)
{
	Type = InType;
	Label = InLabel;
	RestartTimer(TimeLimit);
	if (Type == EShiftTaskType::Spill)
	{
		SpillMesh->SetVisibility(true);
		SpillMesh->SetRelativeScale3D(FVector(FMath::FRandRange(0.8f, 1.3f), FMath::FRandRange(0.6f, 1.1f), 0.01f));
		SpillMesh->SetRelativeLocation(FVector(0, 0, -GetActorLocation().Z + 1.f));
		if (UMaterialInstanceDynamic* MID = SpillMesh->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.06f, 0.02f));
		}
	}
	Marker->SetText(FText::FromString(Type == EShiftTaskType::CartonSource ? TEXT("[ STOCK ]") : TEXT("[ ! ]")));
	Marker->SetTextRenderColor(Type == EShiftTaskType::CartonSource ? FColor(180, 200, 190) : FColor(255, 210, 90));
}

void AShiftTaskTarget::RestartTimer(float TimeLimit)
{
	Deadline = TimeLimit;
	TimeLeft = TimeLimit;
	Progress = 0.f;
}

float AShiftTaskTarget::HoldDuration() const
{
	switch (Type)
	{
	case EShiftTaskType::Restock: return 1.5f;
	case EShiftTaskType::Count: return 3.0f;
	case EShiftTaskType::Tidy: return 2.5f;
	case EShiftTaskType::Trash: return 2.0f;
	case EShiftTaskType::Spill: return 2.5f;
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
	if (Deadline > 0.f && TimeLeft > 0.f) { TimeLeft = FMath::Max(0.f, TimeLeft - DeltaSeconds); }

	// Billboard the marker toward the camera and bob it a little.
	if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector ToCam = Cam->GetCameraLocation() - Marker->GetComponentLocation();
		Marker->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
	}
	const float T = GetWorld()->GetTimeSeconds();
	Marker->SetRelativeLocation(FVector(0, 0, 45.f + 3.f * FMath::Sin(T * 2.5f)));
	if (IsOverdue()) { Marker->SetTextRenderColor(FMath::Fmod(T, 0.6f) < 0.3f ? FColor(255, 60, 40) : FColor(120, 20, 10)); }
}
