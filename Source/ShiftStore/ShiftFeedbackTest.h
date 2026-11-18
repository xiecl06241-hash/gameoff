#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftCustomer.h"
#include "ShiftFeedbackTest.generated.h"

class AShiftPlayerCharacter;
class AShiftStoreGameMode;

/** Opt-in, isolated runtime probe. Never spawned during normal play. */
UCLASS()
class SHIFTSTORE_API AShiftFeedbackTest : public AActor
{
	GENERATED_BODY()
public:
	AShiftFeedbackTest();
	virtual void Tick(float DeltaSeconds) override;
private:
	void Check(const FString& Name, bool bPassed);
	void Screenshot(const FString& Name);
	void Finish();
	void SetupBlockedCustomer();
	void TickPlaythrough(float DeltaSeconds);
	bool RunBenchmarkRoute(AShiftStoreGameMode& Game, AShiftPlayerCharacter& Clerk, float DeltaSeconds);
	TMap<FString, bool> Checks;
	int32 Step = 0;
	int32 BlockCase = 0;
	double NextStepTime = 0.;
	double StartedAt = 0.;
	TWeakObjectPtr<AShiftCustomer> Customer;
	TWeakObjectPtr<class AShiftItem> Item;
	FVector SavedLocation = FVector::ZeroVector;
	FVector SavedCustomerGoal = FVector::ZeroVector;
	float SavedTimer = 0.f;
	float SavedSanity = 0.f;
	float SavedFOV = 0.f;
	int32 SavedPlanCount = 0;
	int32 SavedCourtesyCount = 0;
	float SavedLosses = 0.f;
	ECustomerState SavedState = ECustomerState::Entering;
	TArray<FVector> BenchmarkRoute;
	TArray<double> FrameTimes;
	double LastFrameTime = 0.;
	double LastProgressAt = 0.;
	double RevealFrameMs = 0.;
	int32 PlaythroughBeat = INDEX_NONE;
	int32 BenchmarkPoint = 0;
	bool bBenchmarkStarted = false;
	bool bBenchmarkFinished = false;
	bool bSampledShift = false;
};
