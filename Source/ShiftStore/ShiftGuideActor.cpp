#include "ShiftGuideActor.h"
#include "ShiftStoreGameMode.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftDirector.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AShiftGuideActor::AShiftGuideActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Arrows = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorArrows"));
	Highlight = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TargetBrackets"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(TEXT("/Game/ShiftStore/Materials/M_Guide.M_Guide"));
	for (UInstancedStaticMeshComponent* Component : {Arrows.Get(), Highlight.Get()})
	{
		Component->SetupAttachment(RootComponent);
		Component->SetStaticMesh(Cube.Object);
		Component->SetMaterial(0, Glow.Object);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(false);
	}
}

void AShiftGuideActor::BeginPlay()
{
	Super::BeginPlay();
	HighlightMaterial = Highlight->CreateDynamicMaterialInstance(0);
}

FVector AShiftGuideActor::MarkerLocation() const
{
	return Status == EShiftRouteStatus::Arrived ? Current.Location : (Route.IsEmpty() ? FVector::ZeroVector : Route[0]);
}

void AShiftGuideActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	AShiftPlayerCharacter* Clerk = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	FShiftTaskGuidance Next;
	if (!GM || !GM->GetTaskGuidance(Clerk, Next))
	{
		bActive = false;
		Current = FShiftTaskGuidance();
		Status = EShiftRouteStatus::Unreachable;
		Route.Empty();
		Arrows->ClearInstances();
		Highlight->ClearInstances();
		FloorRouteLength = 0.f;
		bNeedsReplan = true;
		return;
	}
	const bool bChanged = Current.Target != Next.Target || Current.Task != Next.Task
		|| Current.Action != Next.Action || !Current.Location.Equals(Next.Location, 1.f);
	if (bChanged)
	{
		Route.Empty();
		Status = EShiftRouteStatus::Unreachable;
		Arrows->ClearInstances();
		Highlight->ClearInstances();
		bNeedsReplan = true;
	}
	Current = Next;
	bActive = true;
	const double Now = GetWorld()->GetTimeSeconds();
	const float ShiftAlpha = GM->Director ? GM->Director->Alpha() : 0.f;
	while (!Route.IsEmpty() && FVector::Dist2D(Clerk->GetActorLocation(), Route[0]) < 55.f) { Route.RemoveAt(0); }
	if (Now - LastPlanTime >= 0.5)
	{
		const bool bBlocked = !Route.IsEmpty() && !ShiftGuidance::CanWalkSegment(GetWorld(), Clerk, Current.Target.Get(), Clerk->GetActorLocation(), Route[0]);
		const bool bMoved = FVector::Dist2D(Clerk->GetActorLocation(), LastPlanPosition) > 120.f;
		if (bNeedsReplan || bBlocked || bMoved || Route.IsEmpty() || FMath::Abs(ShiftAlpha - LastShiftAlpha) > 0.01f)
		{
			const FShiftRouteResult Result = ShiftGuidance::FindRoute(GetWorld(), Clerk, Current.Target.Get(), Current.Location);
			Status = Result.Status;
			Route = Result.Waypoints;
			LastPlanPosition = Clerk->GetActorLocation();
			LastShiftAlpha = ShiftAlpha;
			bNeedsReplan = false;
			++ReplanCount;
		}
		LastPlanTime = Now;
	}
	if (Now - LastGeometryTime >= 0.1 || bChanged)
	{
		BuildGeometry(Clerk);
		LastGeometryTime = Now;
	}
	if (HighlightMaterial) { HighlightMaterial->SetScalarParameterValue(TEXT("Brightness"), 2.5f + 0.5f * FMath::Sin(Now * 2. * PI * 1.2)); }
}

void AShiftGuideActor::AddStroke(UInstancedStaticMeshComponent* Component, const FVector& A, const FVector& B, float Width)
{
	const FVector Delta = B - A;
	Component->AddInstance(FTransform(Delta.Rotation(), (A + B) * 0.5f, FVector(Delta.Size() / 100.f, Width / 100.f, 0.012f)), true);
}

void AShiftGuideActor::BuildGeometry(AShiftPlayerCharacter* Clerk)
{
	Arrows->ClearInstances();
	Highlight->ClearInstances();
	FloorRouteLength = 0.f;
	if (Status == EShiftRouteStatus::Arrived)
	{
		const float Extent = 34.f + 3.f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f * PI * 1.2f);
		const FVector View = (Current.Location - Clerk->FirstPersonCamera->GetComponentLocation()).GetSafeNormal();
		const FVector Center = Current.Location - View * 60.f;
		const FVector Right = Clerk->FirstPersonCamera->GetRightVector();
		const FVector Up = Clerk->FirstPersonCamera->GetUpVector();
		for (float X : {-1.f, 1.f}) for (float Y : {-1.f, 1.f})
		{
			const FVector Corner = Center + (Right * X + Up * Y) * Extent;
			AddStroke(Highlight, Corner, Corner - Right * X * 12.f, 2.f);
			AddStroke(Highlight, Corner, Corner - Up * Y * 12.f, 2.f);
		}
		return;
	}
	if (Status != EShiftRouteStatus::Reachable || Route.IsEmpty()) { return; }
	FVector Start = Clerk->GetActorLocation();
	float UntilArrow = 65.f;
	for (const FVector& End : Route)
	{
		const FVector Direction = (End - Start).GetSafeNormal2D();
		const float Length = FVector::Dist2D(Start, End);
		const FVector Side(-Direction.Y, Direction.X, 0.f);
		for (float Distance = UntilArrow; Distance < Length && FloorRouteLength + Distance <= 500.f; Distance += 70.f)
		{
			FVector Point = Start + Direction * Distance;
			// Find the actual walkable floor; depth-tested meshes cannot shine through shelves.
			FHitResult Floor;
			FCollisionQueryParams Params(TEXT("GuideFloor"), false, Clerk);
			Params.AddIgnoredActor(this);
			if (GetWorld()->LineTraceSingleByChannel(Floor, FVector(Point.X, Point.Y, 30.f), FVector(Point.X, Point.Y, -40.f), ECC_Visibility, Params))
			{
				Point.Z = Floor.ImpactPoint.Z + 2.f;
				AddStroke(Arrows, Point + Direction * 12.f, Point - Direction * 10.f + Side * 12.f, 3.f);
				AddStroke(Arrows, Point + Direction * 12.f, Point - Direction * 10.f - Side * 12.f, 3.f);
			}
		}
		UntilArrow = 35.f;
		FloorRouteLength = FMath::Min(500.f, FloorRouteLength + Length);
		if (FloorRouteLength >= 500.f) { break; }
		Start = End;
	}
}
