#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftCustomer.h"
#include "ShiftFeedbackTest.generated.h"

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
	TMap<FString, bool> Checks;
	int32 Step = 0;
	int32 BlockCase = 0;
	double NextStepTime = 0.;
	double StartedAt = 0.;
	TWeakObjectPtr<AShiftCustomer> Customer;
	TWeakObjectPtr<class AShiftItem> Item;
	FVector SavedLocation = FVector::ZeroVector;
	float SavedTimer = 0.f;
	float SavedSanity = 0.f;
	float SavedFOV = 0.f;
	int32 SavedPlanCount = 0;
	int32 SavedCourtesyCount = 0;
	float SavedLosses = 0.f;
	ECustomerState SavedState = ECustomerState::Entering;
};
