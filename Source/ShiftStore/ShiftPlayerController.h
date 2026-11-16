#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShiftPlayerController.generated.h"

UCLASS()
class SHIFTSTORE_API AShiftPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;
	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }
	int32 PauseSelection() const { return SelectedPauseAction; }
	bool BlocksGameplayInput();
	void TogglePauseMenu();

protected:
	virtual void SetupInputComponent() override;

private:
	void SetPauseMenuOpen(bool bOpen);
	void PreviousPauseAction();
	void NextPauseAction();
	void ConfirmPauseAction();
	void ClickPauseAction();
	void TickPauseTest();
	void FinishPauseTest();
	bool bPauseMenuOpen = false;
	bool bWaitForGameplayRelease = false;
	int32 SelectedPauseAction = 0;
	uint64 MenuTransitionFrame = MAX_uint64;
	FVector2D PreviousMenuMouse = FVector2D(-1.f, -1.f);

	int32 PauseTestStep = 0;
	double PauseTestNextTime = 0.;
	TMap<FString, bool> PauseTestChecks;
	float PauseTestTimeLeft = 0.f;
	float PauseTestSanity = 0.f;
	double PauseTestWorldTime = 0.;
	float PauseTestMonsterTimer = 0.f;
	FVector PauseTestMonsterLocation = FVector::ZeroVector;
	FVector PauseTestPlayerLocation = FVector::ZeroVector;
	float PauseTestShiftAlpha = 0.f;
};
