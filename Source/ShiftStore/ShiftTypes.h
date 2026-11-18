#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ShiftTypes.generated.h"

/** What a carried object is used for. */
UENUM()
enum class EShiftItemKind : uint8
{
	Product,
	Carton,
	Mop
};

UENUM()
enum class EShiftTaskType : uint8
{
	Restock,      // bring a carton from the warehouse to a shelf
	Count,        // hold E to count stock
	Tidy,         // hold E to face / tidy a shelf
	Trash,        // hold E to empty a bin
	Spill,        // hold E with the mop to clean
	CartonSource, // permanent: press E to take a carton
	CookOden,     // hold E at the oden station to prepare hot broth
	FixAnomaly,   // hold E to reset a localized micro-shift anomaly
	BreakerBox    // permanent: press E during Shift to temporarily restore normal power/layout
};

UENUM()
enum class EShiftPhase : uint8
{
	LanguageSelection,
	Title,
	NightIntro,
	Working,
	Cleanup,
	Dawn,
	GameOver,
	Ending
};

struct FShiftProduct
{
	FName Sku;
	const TCHAR* Label;   // shown to the player
	float Cost;           // loss when destroyed
	float Price;          // income when sold
	float StunSeconds;    // how long a hit stuns the monster
};

struct FShiftCustomerSpec
{
	FName WantedSku;
	bool bAsksHelp = false;
	bool bMonster = false;
	FName MonsterSku;          // the "lost" item a revealed monster demands
	FString Line;              // what the customer says when asking for help
	FString AnomalyHint;       // subtle contradictory tell hinting at MonsterSku before reveal
};

struct FShiftTaskSpot
{
	EShiftTaskType Type;
	FVector Location;
	const TCHAR* Label;
};

struct FShiftNightSpec
{
	FString Title;
	FString Intro;
	TArray<FShiftCustomerSpec> Customers;
	TArray<int32> OpeningTasks;   // indices into ShiftData::TaskSpots()
	int32 Spills = 2;
	float TaskTimeLimit = 120.f;
	bool bEerieEvents = false;
};

enum class EShiftGuideAction : uint8 { Work, CollectCarton, DeliverCarton, CollectMop, MopFloor, CookOden, FixAnomaly, FlipBreaker };

struct FShiftTaskGuidance
{
	TWeakObjectPtr<class AShiftTaskTarget> Task;
	TWeakObjectPtr<AActor> Target;
	FVector Location = FVector::ZeroVector;
	FString TaskLabel;
	EShiftGuideAction Action = EShiftGuideAction::Work;
};

UENUM()
enum class EShiftDialogueEvent : uint8 { WorkStart, WitnessReveal, Grabbed, Escaped, LowSanity };

namespace ShiftData
{
	inline constexpr ECollisionChannel CustomerCollisionChannel = ECC_GameTraceChannel2;
	const TArray<FShiftProduct>& Products();
	const FShiftProduct* FindProduct(FName Sku);
	FString ProductLabel(FName Sku);
	/** Returns the shifted SKU when shelves/coolers swap stock during an active Shift. */
	FName ShiftedSku(FName NormalSku);

	const TArray<FShiftTaskSpot>& TaskSpots();
	const TArray<FShiftTaskSpot>& AnomalySpots();
	const TArray<FVector>& BrowsePoints();
	const TArray<FVector>& LostItemSpots();
	const TArray<FVector>& RestockSpots();
	const TArray<FShiftNightSpec>& Nights();

	// Store landmarks (cm, X east, Y into the store, Z up).
	inline const FVector OutsideDoor(800.f, -260.f, 100.f);
	inline const FVector InsideDoor(800.f, 140.f, 100.f);
	inline const FVector CheckoutCustomerSpot(1250.f, 300.f, 100.f);
	inline const FVector RegisterLocation(1270.f, 180.f, 110.f);
	inline const FVector BreakerLocation(900.f, 1125.f, 130.f);

	constexpr float MaxSanity = 100.f;
	constexpr float LossLimit = 40.f;        // store losses beyond this end the run
	constexpr float OverdueDrainPerSec = 1.2f;
	constexpr float SeenDrainPerSec = 2.0f;
	constexpr float GrabbedDrainPerSec = 11.f;
	constexpr float AmbushHit = 30.f;
	constexpr float OverdueRecover = 6.f;
	constexpr float OnTimeRecover = 2.f;
}
