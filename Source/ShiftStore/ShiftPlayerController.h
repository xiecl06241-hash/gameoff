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
	bool IsMouseSettingsOpen() const { return bMouseSettingsOpen; }
	int32 MouseSettingsSelection() const { return SelectedMouseSetting; }
	float MouseSensitivity() const { return Sensitivity; }
	static constexpr float MinSensitivity = 0.10f;
	static constexpr float MaxSensitivity = 3.f;
	void SetMouseSensitivity(float Value, bool bPersist = true);

	// --- UMG menu API ---
	void UIResume();
	void UIReturnToTitle();
	void UIQuit();

	/** Applies GameAndUI for menus/pause and GameOnly for gameplay. */
	void UpdateUIInputMode();
	bool UsesUMG() const;
	void SetRulesOpen(bool bOpen);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void SetupInputComponent() override;

private:
	void DecreaseSensitivity();
	void IncreaseSensitivity();
	void ReleaseMenuMouse();
	void UpdateSensitivityFromMouse(float X, float Width);
	void SaveMouseSensitivity();
	void CloseMouseSettings();
	bool bMouseSettingsOpen = false;
	bool bDraggingSensitivity = false;
	bool bSensitivityDirty = false;
	int32 SelectedMouseSetting = 0;
	float Sensitivity = 1.f;
	void TickSensitivityTest();
	void FinishSensitivityTest();
	int32 SensitivityTestStep = 0;
	double SensitivityTestNextTime = 0.;
	TMap<FString, bool> SensitivityTestChecks;
	FVector2D SensitivityTestBaseline = FVector2D::ZeroVector;
	float SensitivityTestTimer = 0.f;
	float SensitivityTestSanity = 0.f;
	void SetPauseMenuOpen(bool bOpen);
	// bool UsesUMG() const;
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
	bool bLastMenuInput = false;

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

	// rules button
	void ToggleRules();
	bool bRulesOpen = false;
};
