#include "ShiftDialogueWidget.h"

#include "../ShiftLocalization.h"
#include "../ShiftStoreGameMode.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UShiftDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ClickCatcher)
	{
		ClickCatcher->OnClicked.AddDynamic(this, &UShiftDialogueWidget::OnClick);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UShiftDialogueWidget::StartPrologue()
{
	if (bActive)
	{
		return;
	}

	Speakers = {
		ShiftLoc::Text(TEXT("STORE MANAGER")),
		ShiftLoc::Text(TEXT("STORE MANAGER")),
		ShiftLoc::Text(TEXT("STORE MANAGER")),
		ShiftLoc::Text(TEXT("STORE MANAGER")),
		ShiftLoc::Text(TEXT("YOU")),
		ShiftLoc::Text(TEXT("STORE MANAGER"))
	};

	Lines = {
		ShiftLoc::Text(TEXT("First night shift. The store is yours until 06:00.")),
		ShiftLoc::Text(TEXT("Keep the shelves full, serve every customer, and clean up before dawn.")),
		ShiftLoc::Text(TEXT("WASD to move, mouse to look, E to interact or hold for chores.")),
		ShiftLoc::Text(TEXT("Tab shows your task, R opens the rules, Esc pauses.")),
		ShiftLoc::Text(TEXT("...What if the lights go out?")),
		ShiftLoc::Text(TEXT("Then don't argue with what walks in. Find what it wants and bring it to the register."))
	};

	LineIndex = 0;
	VisibleChars = 0;
	TypeTimer = 0.f;
	bActive = true;
	bTyping = false;

	SetVisibility(ESlateVisibility::Visible);
	ShowLine();
}

void UShiftDialogueWidget::ShowLine()
{
	if (!Lines.IsValidIndex(LineIndex))
	{
		return;
	}

	if (SpeakerText)
	{
		SpeakerText->SetText(FText::FromString(Speakers[LineIndex]));
	}
	if (DialogueText)
	{
		DialogueText->SetText(FText::GetEmpty());
	}
	if (ContinueText)
	{
		ContinueText->SetText(FText::GetEmpty());
	}

	VisibleChars = 0;
	TypeTimer = 0.f;
	bTyping = true;
}

void UShiftDialogueWidget::NativeTick(
	const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bActive || !bTyping || !Lines.IsValidIndex(LineIndex))
	{
		return;
	}

	TypeTimer += InDeltaTime;

	const FString& Full = Lines[LineIndex];
	const int32 Target = FMath::Min(
		FMath::FloorToInt(TypeTimer * 35.f),
		Full.Len());

	if (DialogueText)
	{
		DialogueText->SetText(FText::FromString(Full.Left(Target)));
	}

	if (Target >= Full.Len())
	{
		bTyping = false;
		if (ContinueText)
		{
			ContinueText->SetText(FText::FromString(
				ShiftLoc::Text(TEXT("▼ Click to continue"))));
		}
	}
}

void UShiftDialogueWidget::OnClick()
{
	Advance();
}

void UShiftDialogueWidget::Advance()
{
	if (!bActive || !Lines.IsValidIndex(LineIndex))
	{
		return;
	}

	if (bTyping)
	{
		bTyping = false;
		if (DialogueText)
		{
			DialogueText->SetText(FText::FromString(Lines[LineIndex]));
		}
		if (ContinueText)
		{
			ContinueText->SetText(FText::FromString(
				ShiftLoc::Text(TEXT("▼ Click to continue"))));
		}
		return;
	}

	++LineIndex;

	if (Lines.IsValidIndex(LineIndex))
	{
		ShowLine();
		return;
	}

	HideDialogue();

	if (AShiftStoreGameMode* GM = GetWorld()
		? GetWorld()->GetAuthGameMode<AShiftStoreGameMode>()
		: nullptr)
	{
		GM->OnAdvanceScreenKey();
	}
}

void UShiftDialogueWidget::HideDialogue()
{
	bActive = false;
	bTyping = false;
	SetVisibility(ESlateVisibility::Collapsed);
}