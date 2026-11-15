#include "ShiftItem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "ShiftCustomer.h"
#include "ShiftStoreGameMode.h"

AShiftItem::AShiftItem()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // never hit the thrower's pawn
	Mesh->SetCollisionResponseToChannel(ShiftData::CustomerCollisionChannel, ECR_Ignore);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->SetLinearDamping(0.3f);
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Mesh->SetMassOverrideInKg(NAME_None, 1.f, true);
	}
	Tags.Add(TEXT("Shift.Item"));
}

static UStaticMesh* LoadStoreMesh(const FString& Name)
{
	const FString Path = FString::Printf(TEXT("/Game/ShiftStore/Meshes/SM_%s.SM_%s"), *Name, *Name);
	return LoadObject<UStaticMesh>(nullptr, *Path);
}

void AShiftItem::InitProduct(FName InSku)
{
	Kind = EShiftItemKind::Product;
	Sku = InSku;
	Mesh->SetStaticMesh(LoadStoreMesh(FString(TEXT("Product_")) + Sku.ToString()));
}

void AShiftItem::InitKind(EShiftItemKind InKind)
{
	Kind = InKind;
	if (Kind == EShiftItemKind::Carton) { Mesh->SetStaticMesh(LoadStoreMesh(TEXT("Carton"))); Mesh->SetMassOverrideInKg(NAME_None, 4.f, true); }
	if (Kind == EShiftItemKind::Mop)    { Mesh->SetStaticMesh(LoadStoreMesh(TEXT("Mop"))); Mesh->SetMassOverrideInKg(NAME_None, 2.f, true); }
}

FString AShiftItem::DisplayName() const
{
	switch (Kind)
	{
	case EShiftItemKind::Carton: return TEXT("Stock carton");
	case EShiftItemKind::Mop: return TEXT("Mop");
	default: return ShiftData::ProductLabel(Sku);
	}
}

bool AShiftItem::CanSimulate() const
{
	const UStaticMesh* SM = Mesh->GetStaticMesh();
	const UBodySetup* Body = SM ? SM->GetBodySetup() : nullptr;
	return Body && Body->CollisionTraceFlag != CTF_UseComplexAsSimple && Body->AggGeom.GetElementCount() > 0;
}

void AShiftItem::SetHeld(bool bHeld)
{
	bInFlight = false;
	Mesh->SetCollisionResponseToChannel(ShiftData::CustomerCollisionChannel, ECR_Ignore);
	const bool bPhysics = !bHeld && CanSimulate();
	Mesh->SetSimulatePhysics(bPhysics);
	Mesh->SetCollisionEnabled(bHeld ? ECollisionEnabled::NoCollision : (bPhysics ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::QueryOnly));
	if (!bHeld && !bPhysics)
	{
		// Complex-only meshes (the carton) are set down on the floor instead of simulated.
		FHitResult Floor;
		const FVector Start = GetActorLocation();
		FCollisionQueryParams Params(TEXT("ItemDrop"), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Floor, Start, Start - FVector(0, 0, 400), ECC_Visibility, Params))
		{
			SetActorLocation(Floor.ImpactPoint);
		}
		SetActorRotation(FRotator(0, GetActorRotation().Yaw, 0));
	}
	if (bHeld)
	{
		bInFlight = false;
		if (Glow) { Glow->SetVisibility(false); }
	}
}

void AShiftItem::Throw(const FVector& Velocity, AActor* Thrower)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetHeld(false);
	if (!Mesh->IsSimulatingPhysics()) { return; }
	Mesh->SetCollisionResponseToChannel(ShiftData::CustomerCollisionChannel, ECR_Block);
	Mesh->SetPhysicsLinearVelocity(Velocity);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector(FMath::FRandRange(-360.f, 360.f), 540.f, 0.f));
	SetOwner(Thrower);
	bInFlight = true;
	FlightTime = 0.f;
	if (!Mesh->OnComponentHit.IsAlreadyBound(this, &AShiftItem::OnHit))
	{
		Mesh->OnComponentHit.AddDynamic(this, &AShiftItem::OnHit);
	}
}

void AShiftItem::OnHit(UPrimitiveComponent* HitComp, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit)
{
	if (!bInFlight) { return; }
	bInFlight = false;
	Mesh->SetCollisionResponseToChannel(ShiftData::CustomerCollisionChannel, ECR_Ignore);
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	if (!GM) { return; }
	if (AShiftCustomer* Customer = Cast<AShiftCustomer>(Other))
	{
		GM->OnItemHitCustomer(this, Customer);
	}
	else if (Kind == EShiftItemKind::Product && !bLostItem)
	{
		GM->OnItemBroken(this, false);
	}
}

void AShiftItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bInFlight)
	{
		FlightTime += DeltaSeconds;
		if (FlightTime > 3.f)
		{
			bInFlight = false;
			Mesh->SetCollisionResponseToChannel(ShiftData::CustomerCollisionChannel, ECR_Ignore);
		}
	}
	if (bLostItem && !Glow)
	{
		// A faint pulsing glow is the only hint where the monster's item ended up.
		Glow = NewObject<UPointLightComponent>(this);
		Glow->SetupAttachment(Mesh);
		Glow->RegisterComponent();
		Glow->SetIntensityUnits(ELightUnits::Lumens);
		Glow->SetLightColor(FLinearColor(0.9f, 0.15f, 0.1f));
		Glow->SetAttenuationRadius(180.f);
		Glow->SetCastShadows(false);
	}
	if (Glow && Glow->IsVisible())
	{
		GlowTime += DeltaSeconds;
		Glow->SetIntensity(30.f + 25.f * FMath::Sin(GlowTime * 3.f));
		Glow->SetWorldLocation(GetActorLocation() + FVector(0, 0, 25));
	}
}
