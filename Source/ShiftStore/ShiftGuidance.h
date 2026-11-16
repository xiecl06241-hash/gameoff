#pragma once

#include "CoreMinimal.h"

class UWorld;
class APawn;
class AActor;

namespace ShiftGuidance
{
	// Store aisle/door anchors, joined only when a player-sized capsule can walk between them.
	TArray<FVector> FindRoute(UWorld* World, APawn* Player, AActor* TargetActor, const FVector& Target, bool bRequireExactGoal = false);
	bool CanWalkSegment(UWorld* World, APawn* Player, AActor* TargetActor, const FVector& Start, const FVector& End);
}
