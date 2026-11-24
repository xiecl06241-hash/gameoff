#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShiftTypes.h"
#include "ShiftStoreGameMode.generated.h"

class AShiftDirector;
class AShiftTaskTarget;
class AShiftCustomer;
class AShiftItem;
class AShiftClueActor;
class UAudioComponent;
class AShiftPlayerCharacter;

UCLASS()
class SHIFTSTORE_API AShiftStoreGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShiftStoreGameMode();

	const FShiftNightSpec& CurrentNightSpec() const;

	void StartNight(int32 InNightIndex);
	void RestartCurrentNight();
	void OnAdvanceScreenKey();
	void ContinueSavedGame();
	void ReturnToTitle();
	void CycleGuidedTask();
	bool GetTaskGuidance(const AShiftPlayerCharacter* Player, FShiftTaskGuidance& Out) const;
	/** Products that are on the floor or in the clerk's hands and still have to go back on a shelf. */
	int32 CountLooseMerchandise() const;
	bool bHasSavedProgress = false;
	void OpenLanguageSelection();
	void ConfirmLanguage();
	int32 SelectedLanguage = 0;

	void OnTaskCompleted(AShiftTaskTarget* Task);
	void OnCustomerNeedsHelp(AShiftCustomer* Customer);
	void OnCustomerHelped(AShiftCustomer* Customer);
	void OnCustomerAtCheckout(AShiftCustomer* Customer);
	void OnCheckoutCompleted(AShiftCustomer* Customer);
	void OnCustomerRefused(AShiftCustomer* Customer);
	void OnMonsterRevealed(AShiftCustomer* Monster, bool bWitnessed);
	void OnMonsterDemandDelivered(AShiftCustomer* Monster);
	void OnMonsterWardedOff(AShiftCustomer* Customer);
	void OnCustomerFinished(AShiftCustomer* Customer);
	void ActivateBreakerBox();
	bool TryPlayerPhaseShift();
	void TriggerMicroShift();
	bool HasActiveAnomaly() const;
	bool HasBlockingChore() const;
	bool IsAutomatedSession() const { return bAutomatedSession; }
	int32 ActiveRuleCount() const { return FMath::Clamp(NightIndex + 1, 1, 3); }
	void ToggleRuleNote();
	FString GetOnboardingHint(const AShiftPlayerCharacter* Player) const;
	bool bRuleNoteOpen = false;
	float RuleNoteAutoHideTimer = 0.f;
	float RuleNoteRaiseAlpha = 0.f;
	bool bHasUsedPhaseShift = false;
	bool bShowPhaseShiftPrompt = false;
	float PhaseShiftPromptTimer = 0.f;
	void PlayPositionalAnomalyPulse(const FVector& Location);
	void PlayFootstepCue(const FVector* OptionalWorldLocation = nullptr, float Volume = 0.35f);
	void PlayHeartbeatCue(float Tension, bool bReverb = false);
	bool PlayExternalOrSynthesizedCue(const FString& WavFileName, float FallbackFreqHz, float FallbackDurationSec, float Volume = 0.28f, bool bFallbackNoise = false, const FVector* OptionalWorldLocation = nullptr);
	int32 TotalMonstersWarded = 0;
	int32 TotalShiftsTriggered = 0;
	bool bEndingStatsComplete = true;
	static constexpr int32 TotalStoryClues = 6;
	int32 CollectedCluesMask = 0;
	bool bHasBrassKey = false;
	bool bOfficePhoneRinging = false;
	bool bOfficePhoneAnswered = false;
	bool bArchiveReaderOpen = false;
	FString ActiveArchiveTitleKey;
	FString ActiveArchiveBodyKey;
	int32 GetCollectedClueCount() const;
	bool InteractWithClue(AShiftClueActor* Clue);
	bool AnswerOfficePhone();
	void CloseArchiveReader();
	FString GetInvestigationLead() const;
	bool IsTrueEnding() const { return GetCollectedClueCount() >= TotalStoryClues || (bEndingStatsComplete && TotalShiftsTriggered == 0 && TotalMonstersWarded > 0); }
	void BuildNightCustomerRoster();

	void OnItemHitCustomer(AShiftItem* Item, AShiftCustomer* Customer);
	void OnItemBroken(AShiftItem* Item, bool bUsedOnMonster);

	void ModifySanity(float Delta, const FString& Reason = TEXT(""));
	void SetStatusBanner(const FString& InText, float Seconds = 3.f);
	bool RequestDialogue(EShiftDialogueEvent Event);
	FString SubtitleText;
	float SubtitleTime = 0.f;
	UPROPERTY(EditDefaultsOnly, Category="Dialogue") TMap<EShiftDialogueEvent, TObjectPtr<class USoundBase>> DialogueVoices;
	UPROPERTY() TObjectPtr<class AShiftGuideActor> GuideRenderer;

	UFUNCTION(Exec) void ShiftForceState(int32 bShifted);
	UFUNCTION(Exec) void ShiftSkipNight();
	UFUNCTION(Exec) void ShiftRunSelfTest();

	UPROPERTY() TObjectPtr<AShiftDirector> Director;
	UPROPERTY() TWeakObjectPtr<AShiftTaskTarget> CartonSource;
	UPROPERTY() TWeakObjectPtr<AShiftTaskTarget> BreakerBoxTarget;
	TArray<TWeakObjectPtr<AShiftTaskTarget>> ActiveTasks;
	TArray<TWeakObjectPtr<AShiftClueActor>> StoryClues;
	TWeakObjectPtr<AShiftCustomer> ActiveCustomer;
	TWeakObjectPtr<AShiftItem> ActiveLostItem;

	EShiftPhase Phase = EShiftPhase::LanguageSelection;
	int32 NightIndex = 0;
	int32 NextCustomerIdx = 0;
	int32 CustomersServed = 0;
	TArray<FShiftCustomerSpec> ActiveNightCustomers;
	float HeartbeatPulseAlpha = 0.f;
	float HeartbeatTimer = 0.f;

	float Sanity = ShiftData::MaxSanity;
	float Income = 0.f;
	float Losses = 0.f;

	FString BannerText;
	float BannerTimer = 0.f;
	FString GameOverReason;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ResetDialogue();
	void TickDialogue(float DeltaSeconds);
	TSet<EShiftDialogueEvent> SpokenThisNight;
	TMap<EShiftDialogueEvent, double> DialogueLastSpoken;
	int32 SubtitlePriority = -1;
	void TickLocalizationTest(float DeltaSeconds);
	void FinishLocalizationTest();
	void CheckLocalization(const FString& Name, bool bPassed);
	TMap<FString, bool> LocalizationChecks;
	int32 LocalizationStep = 0;
	float LocalizationTime = 0.f;
	void SpawnTaskFromSpot(int32 SpotIdx);
	void SpawnRestockAfterService();
	void SpawnPostServiceTask();
	void SpawnCleanupSpills(int32 Count);
	void SpawnStoryClues();
	void RefreshStoryClues();
	void ClearNightActors();
	bool TryFinishCleanup();
	void TrySpawnNextCustomer();
	void TriggerGameOver(const FString& Reason);
	bool SaveProgress();
	bool LoadProgress();
	const FString& ProgressSlotName() const;
	static const FString PlayerSaveSlotName;
	int32 CheckpointNight = 0;
	int32 CheckpointWarded = 0;
	int32 CheckpointShifts = 0;
	int32 CheckpointCluesMask = 0;
	bool bCheckpointBrassKey = false;
	bool bCheckpointPhoneAnswered = false;
	float CheckpointIncome = 0.f;
	float CheckpointLosses = 0.f;
	bool bHasNightCheckpoint = false;
	int32 GuidedTaskIndex = 0;
	int32 SalesSinceRestock = 0;
	int32 ServiceTaskRotation = 0;

	enum class ESelfTestStage : uint8 { Idle, Setup, IntroPause, ActiveTimer, RestockPickup, RestockWork, ServiceSetup, CustomerExit, CookService, ReturnStock, Regression, Projectile, Reveal, Shift, Restore, Flicker, Finished };
	void TickSelfTest(float DeltaSeconds);
	bool CheckSelfTest(const FString& Name, bool bSuccess, const FString& Detail = TEXT(""));
	void AdvanceSelfTest(ESelfTestStage Stage);
	void FinishSelfTest(bool bSuccess, const FString& Failure = TEXT(""));
	ESelfTestStage SelfTestStage = ESelfTestStage::Idle;
	bool bSelfTestRunning = false;
	float SelfTestStageTime = 0.f;
	float SelfTestElapsed = 0.f;
	int32 SelfTestInitialTasks = 0;
	FString SelfTestSaveSlotName;
	TMap<FString, bool> SelfTestChecks;
	TWeakObjectPtr<AShiftItem> SelfTestProjectile;
	TArray<uint8> FormalSaveBeforeSelfTest;
	bool bFormalSaveExistedBeforeSelfTest = false;
	bool bSelfTestInteractPressed = false;
	TWeakObjectPtr<AShiftTaskTarget> SelfTestRestockTarget;
	TWeakObjectPtr<AShiftCustomer> SelfTestDepartingCustomer;
	TWeakObjectPtr<AShiftTaskTarget> SelfTestCookTarget;
	FVector SelfTestExitLastLocation = FVector::ZeroVector;
	float SelfTestExitLastYaw = 0.f;
	float SelfTestExitMaxYawStep = 0.f;
	float SelfTestExitSeconds = 0.f;

	void SetupProceduralAudio();
	void PlaySynthesizedCue(float FreqHz, float DurationSec, float Volume = 0.25f, bool bNoise = false);

	UPROPERTY() TObjectPtr<UAudioComponent> FridgeHumComp;
	UPROPERTY() TObjectPtr<UAudioComponent> OtherworldAmbienceComp;
	UPROPERTY() TObjectPtr<UAudioComponent> CueAudioComp;
	float OtherworldChimeTimer = 3.2f;
	float OtherworldStaySeconds = 0.f;
	float OtherworldSanityDebt = 0.f;
	float OfficePhoneTriggerTimer = 0.f;
	float OfficePhoneRingInterval = 0.f;
	float CustomerSpawnCooldown = 2.0f;
	float EerieFlickerTimer = 14.f;
	float MicroShiftTimer = 28.f;
	int32 MicroShiftRotation = 0;
	bool bAutomatedSession = false;
	float AudioFeedTimer = 0.f;
};
