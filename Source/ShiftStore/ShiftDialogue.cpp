#include "ShiftStoreGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void AShiftStoreGameMode::ResetDialogue()
{
	SubtitleText.Empty();
	SubtitleTime = 0.f;
	SubtitlePriority = -1;
	SpokenThisNight.Empty();
	DialogueLastSpoken.Empty();
}

void AShiftStoreGameMode::TickDialogue(float DeltaSeconds)
{
	SubtitleTime = FMath::Max(0.f, SubtitleTime - DeltaSeconds);
	if (SubtitleTime <= 0.f || (Phase != EShiftPhase::Working && Phase != EShiftPhase::Cleanup))
	{
		SubtitleText.Empty();
		SubtitleTime = 0.f;
		SubtitlePriority = -1;
	}
}

bool AShiftStoreGameMode::RequestDialogue(EShiftDialogueEvent Event)
{
	if (Phase != EShiftPhase::Working && Phase != EShiftPhase::Cleanup) { return false; }
	const bool bOnce = Event != EShiftDialogueEvent::Grabbed && Event != EShiftDialogueEvent::Escaped;
	const int32 Priority = Event == EShiftDialogueEvent::Grabbed || Event == EShiftDialogueEvent::Escaped ? 3 :
		(Event == EShiftDialogueEvent::WitnessReveal ? 2 : (Event == EShiftDialogueEvent::LowSanity ? 1 : 0));
	const double Now = GetWorld()->GetTimeSeconds();
	const double* Last = DialogueLastSpoken.Find(Event);
	if ((bOnce && SpokenThisNight.Contains(Event)) || (Last && Now - *Last < 8.)
		|| (SubtitleTime > 0.f && Priority < SubtitlePriority)) { return false; }
	switch (Event)
	{
	case EShiftDialogueEvent::WorkStart: SubtitleText = TEXT("Let's get the store ready."); break;
	case EShiftDialogueEvent::WitnessReveal: SubtitleText = TEXT("What... was that?"); break;
	case EShiftDialogueEvent::Grabbed: SubtitleText = TEXT("Let go of me!"); break;
	case EShiftDialogueEvent::Escaped: SubtitleText = TEXT("I need some distance..."); break;
	case EShiftDialogueEvent::LowSanity: SubtitleText = TEXT("Stay calm. Don't panic."); break;
	}
	SubtitleTime = 3.f;
	SubtitlePriority = Priority;
	SpokenThisNight.Add(Event);
	DialogueLastSpoken.Add(Event, Now);
	if (const TObjectPtr<USoundBase>* Voice = DialogueVoices.Find(Event); Voice && *Voice)
	{
		UGameplayStatics::PlaySound2D(this, Voice->Get());
	}
	return true;
}
