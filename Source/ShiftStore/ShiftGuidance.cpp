#include "ShiftGuidance.h"
#include "ShiftTaskTarget.h"
#include "ShiftCustomer.h"
#include "ShiftItem.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	FVector FloorPoint(const FVector& Point) { return FVector(Point.X, Point.Y, 100.f); }

	FCollisionQueryParams GuideQuery(UWorld* World, APawn* Player, AActor* Target)
	{
		FCollisionQueryParams Params(TEXT("ClerkGuide"), false, Player);
		Params.AddIgnoredActor(Target);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->IsA<APawn>() || It->IsA<AShiftTaskTarget>() || It->IsA<AShiftItem>()) { Params.AddIgnoredActor(*It); }
		}
		return Params;
	}

	bool Clear(UWorld* World, const FVector& Start, const FVector& End, const FCollisionQueryParams& Params)
	{
		FHitResult Hit;
		return !World->SweepSingleByChannel(Hit, FloorPoint(Start), FloorPoint(End) + FVector(0.f, 0.f, 0.1f),
			FQuat::Identity, ECC_Visibility, FCollisionShape::MakeCapsule(34.f, 96.f), Params);
	}
}

bool ShiftGuidance::CanWalkSegment(UWorld* World, APawn* Player, AActor* TargetActor, const FVector& Start, const FVector& End)
{
	return World && Player && Clear(World, Start, End, GuideQuery(World, Player, TargetActor));
}

FShiftRouteResult ShiftGuidance::FindRoute(UWorld* World, APawn* Player, AActor* TargetActor, const FVector& Target, bool bRequireExactGoal)
{
	FShiftRouteResult Result;
	if (!World || !Player) { return Result; }
	const FCollisionQueryParams Params = GuideQuery(World, Player, TargetActor);
	FHitResult GoalHit;
	const float Reach = bRequireExactGoal ? 30.f : 185.f;
	if (FVector::Dist2D(Player->GetActorLocation(), Target) <= Reach
		&& Clear(World, Player->GetActorLocation(), Player->GetActorLocation(), Params)
		&& !World->LineTraceSingleByChannel(GoalHit, Player->GetPawnViewLocation(), Target, ECC_Visibility, Params))
	{
		Result.Status = EShiftRouteStatus::Arrived;
		return Result;
	}
	TArray<FVector> Nodes = { FloorPoint(Player->GetActorLocation()) };
	for (float Y : { 280.f, 600.f, 900.f })
	{
		for (float X : { 200.f, 600.f, 1000.f, 1455.f }) { Nodes.Emplace(X, Y, 100.f); }
	}
	Nodes.Append({ FVector(1455,1125,100), FVector(1035,1125,100), FVector(395,1125,100),
		FVector(1435,1125,100), FVector(395,1250,100), FVector(395,1380,100),
		FVector(100,1360,100), FVector(1035,1330,100), FVector(1170,1370,100), FVector(1435,1360,100) });
	// The entrance and the open strip in front of the shelves keep departures clear of the counter.
	Nodes.Append({ FVector(1000,300,100), FVector(800,300,100), FVector(800,140,100), FVector(800,-120,100),
		FVector(1000,50,100), FVector(800,50,100), FVector(1455,50,100) });
	const int32 FirstGoal = Nodes.Num();
	// Work targets sit against shelves. End within interaction reach, with a clear view of the target.
	for (const FVector2D Offset : { FVector2D(0,0), FVector2D(0,100), FVector2D(0,-100),
		FVector2D(100,0), FVector2D(-100,0), FVector2D(100,100), FVector2D(-100,100),
		FVector2D(100,-100), FVector2D(-100,-100) })
	{
		if (bRequireExactGoal && !Offset.IsNearlyZero()) { continue; }
		const FVector Point = FloorPoint(Target) + FVector(Offset.X, Offset.Y, 0.f);
		FHitResult Hit;
		if (Clear(World, Point, Point, Params) && !World->LineTraceSingleByChannel(Hit,
			Point + FVector(0,0,66), Target, ECC_Visibility, Params)) { Nodes.Add(Point); }
	}
	if (Nodes.Num() == FirstGoal) { return Result; }
	const int32 Count = Nodes.Num();
	TArray<float> Cost;
	TArray<int32> Previous;
	TArray<bool> Visited;
	TArray<int8> Edges;
	Cost.Init(TNumericLimits<float>::Max(), Count);
	Previous.Init(INDEX_NONE, Count);
	Visited.Init(false, Count);
	Edges.Init(-1, Count * Count);
	Cost[0] = 0.f;
	int32 EndNode = INDEX_NONE;
	for (int32 Iteration = 0; Iteration < Count; ++Iteration)
	{
		int32 Current = INDEX_NONE;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (!Visited[Index] && Cost[Index] < TNumericLimits<float>::Max()
				&& (Current == INDEX_NONE || Cost[Index] < Cost[Current])) { Current = Index; }
		}
		if (Current == INDEX_NONE) { break; }
		if (Current >= FirstGoal) { EndNode = Current; break; }
		Visited[Current] = true;
		for (int32 Next = 1; Next < Count; ++Next)
		{
			const float NewCost = Cost[Current] + FVector::Dist2D(Nodes[Current], Nodes[Next]);
			if (Visited[Next] || NewCost >= Cost[Next]) { continue; }
			int8& Edge = Edges[Current * Count + Next];
			if (Edge < 0)
			{
				Edge = Clear(World, Nodes[Current], Nodes[Next], Params) ? 1 : 0;
				Edges[Next * Count + Current] = Edge;
			}
			if (Edge) { Cost[Next] = NewCost; Previous[Next] = Current; }
		}
	}
	for (int32 Index = EndNode; Index > 0; Index = Previous[Index])
	{
		Result.Waypoints.Insert(Nodes[Index], 0);
	}
	if (EndNode != INDEX_NONE) { Result.Status = EShiftRouteStatus::Reachable; }
	return Result;
}
