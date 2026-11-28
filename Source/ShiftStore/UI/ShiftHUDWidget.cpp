#include "ShiftHUDWidget.h"

#include "../ShiftCustomer.h"
#include "../ShiftDirector.h"
#include "../ShiftGuideActor.h"
#include "../ShiftItem.h"
#include "../ShiftLocalization.h"
#include "../ShiftPlayerCharacter.h"
#include "../ShiftStoreGameMode.h"
#include "../ShiftTaskTarget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"
#include "../ShiftPlayerController.h"
#include "Components/Button.h"
#include "Components/Widget.h"


namespace
{
	const FLinearColor Bone(0.89f, 0.86f, 0.79f, 1.f);
	const FLinearColor Ash(0.58f, 0.55f, 0.50f, 1.f);
	const FLinearColor Blood(0.76f, 0.14f, 0.12f, 1.f);
	const FLinearColor BloodBright(0.92f, 0.22f, 0.18f, 1.f);
	const FLinearColor Candle(0.85f, 0.68f, 0.36f, 1.f);
	const FLinearColor Warn(0.86f, 0.56f, 0.34f, 1.f);
}

UShiftHUDWidget::UShiftHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UFont> SC(TEXT("/Game/ShiftStore/Fonts/NotoSansCJKsc-Regular_Font.NotoSansCJKsc-Regular_Font"));
	static ConstructorHelpers::FObjectFinder<UFont> JP(TEXT("/Game/ShiftStore/Fonts/NotoSansCJKjp-Regular_Font.NotoSansCJKjp-Regular_Font"));
	static ConstructorHelpers::FObjectFinder<UFont> TC(TEXT("/Game/ShiftStore/Fonts/NotoSansCJKtc-Regular_Font.NotoSansCJKtc-Regular_Font"));
	ChineseFont = SC.Object;
	JapaneseFont = JP.Object;
	TraditionalChineseFont = TC.Object;
}

void UShiftHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildObjectiveRows();
	ApplyFonts();

	if (RulesButton)
	{
		RulesButton->OnClicked.AddDynamic(this, &UShiftHUDWidget::ToggleRules);
	}
	if (RulesCard)
	{
		RulesCard->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RulesButtonLabel)
	{
		RulesButtonLabel->SetText(FText::FromString(
			ShiftLoc::Text(TEXT("SHOW RULES"))));
	}
}

void UShiftHUDWidget::BuildObjectiveRows()
{
	if (!ObjectiveRows)
	{
		return;
	}

	ObjectiveRows->ClearChildren();
	ObjectiveRowPool.Reset();

	// Twelve rows cover the largest objective panel in the current three nights,
	// including the guidance rows plus other active chores.
	for (int32 Index = 0; Index < 12; ++Index)
	{
		UTextBlock* Row = NewObject<UTextBlock>(this);
		Row->SetAutoWrapText(true);
		Row->SetVisibility(ESlateVisibility::Collapsed);
		if (NightText)
		{
			Row->SetFont(NightText->GetFont());
		}
		ObjectiveRows->AddChildToVerticalBox(Row);
		ObjectiveRowPool.Add(Row);
	}
}

void UShiftHUDWidget::ApplyFonts()
{
	const int32 Language = ShiftLoc::CurrentLanguage();
	if (Language == LastLanguage)
	{
		return;
	}
	LastLanguage = Language;

	UFont* Font = Language == 1 ? JapaneseFont.Get()
		: (Language == 2 ? TraditionalChineseFont.Get() : ChineseFont.Get());
	if (!Font)
	{
		return;
	}

	TArray<UTextBlock*> Texts;
	Texts.Add(NightText.Get());
	Texts.Add(SanityText.Get());
	Texts.Add(LedgerText.Get());
	Texts.Add(CarryText.Get());
	Texts.Add(RulesTitle.Get());
	Texts.Add(RuleText1.Get());
	Texts.Add(RuleText2.Get());
	Texts.Add(RuleText3.Get());
	Texts.Add(CorruptionText.Get());
	Texts.Add(ObjectiveTitle.Get());
	Texts.Add(PromptText.Get());
	Texts.Add(SubtitleText.Get());
	Texts.Add(BannerText.Get());

	for (UTextBlock* Text : Texts)
	{
		ApplyFontTo(Text, Font);
	}
	for (const TObjectPtr<UTextBlock>& Row : ObjectiveRowPool)
	{
		ApplyFontTo(Row.Get(), Font);
	}
}

void UShiftHUDWidget::ApplyFontTo(UTextBlock* Text, UFont* Font) const
{
	if (!Text || !Font)
	{
		return;
	}
	FSlateFontInfo Info = Text->GetFont();
	Info.FontObject = Font;
	Text->SetFont(Info);
}

void UShiftHUDWidget::SetTextIfChanged(UTextBlock* Block, const FString& Value)
{
	if (Block && Block->GetText().ToString() != Value)
	{
		Block->SetText(FText::FromString(Value));
	}
}

void UShiftHUDWidget::Refresh(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player)
{
	if (!GM)
	{
		return;
	}

	ApplyFonts();

	const bool bShifted = GM->Director && GM->Director->IsShifted();

	// --- Top-left: night, customers, state ---
	const FString Night = ShiftLoc::Format(
		TEXT("%s  |  Customers: %d / %d  |  %s"),
		{
			GM->CurrentNightSpec().Title,
			LexToString(GM->CustomersServed),
			LexToString(GM->CurrentNightSpec().Customers.Num()),
			FString(bShifted ? TEXT("STATE: SHIFTED [ABNORMAL]") : TEXT("STATE: NORMAL"))
		});
	SetTextIfChanged(NightText, Night);
	if (NightText)
	{
		NightText->SetColorAndOpacity(FSlateColor(bShifted ? BloodBright : Bone));
	}

	// --- Sanity ---
	const float SanityPct = FMath::Clamp(GM->Sanity / ShiftData::MaxSanity, 0.f, 1.f);
	const FLinearColor SanityCol = (SanityPct < 0.35f || bShifted)
		? BloodBright
		: FLinearColor(0.84f, 0.80f, 0.72f, 0.92f);
	SetTextIfChanged(SanityText, ShiftLoc::Format(
		TEXT("SANITY: %d%%"), { LexToString(FMath::RoundToInt(GM->Sanity)) }));
	if (SanityText)
	{
		SanityText->SetColorAndOpacity(FSlateColor(SanityPct < 0.35f ? BloodBright : Ash));
	}
	if (SanityBar)
	{
		SanityBar->SetPercent(SanityPct);
		SanityBar->SetFillColorAndOpacity(SanityCol);
	}

	// --- Sales / losses ---
	const float LossPct = FMath::Clamp(GM->Losses / ShiftData::LossLimit, 0.f, 1.f);
	SetTextIfChanged(LedgerText, ShiftLoc::Format(
		TEXT("SALES: +$%.1f   LOSS: -$%.1f / -$%.0f"),
		{
			FString::Printf(TEXT("%.1f"), GM->Income),
			FString::Printf(TEXT("%.1f"), GM->Losses),
			FString::Printf(TEXT("%.0f"), ShiftData::LossLimit)
		}));
	if (LedgerText)
	{
		LedgerText->SetColorAndOpacity(FSlateColor(LossPct > 0.7f ? BloodBright : Ash));
	}
	if (LossBar)
	{
		LossBar->SetPercent(LossPct);
		LossBar->SetFillColorAndOpacity(Blood);
	}

	// --- Carried item / stamina ---
	if (Player)
	{
		// const bool bHasItem = Player->HeldItem.IsValid();
		const AShiftItem* Held = Player->HeldItem.Get();
		const bool bHasItem = Held != nullptr;
		const FString HeldLabel = Held ? Held->DisplayName() : FString();
		SetTextIfChanged(CarryText, bHasItem
		? ShiftLoc::Format(TEXT("CARRYING: %s  ([F/RMB] Throw  [G] Drop)"), {HeldLabel})
		: ShiftLoc::Text(TEXT("HANDS: Empty  ([E] Pick up / Interact)")));
		if (CarryText)
		{
			CarryText->SetVisibility(ESlateVisibility::HitTestInvisible);
			CarryText->SetColorAndOpacity(FSlateColor(bHasItem ? Candle : Ash));
		}

		const float StamPct = FMath::Clamp(Player->Stamina / 100.f, 0.f, 1.f);
		if (StaminaBar)
		{
			StaminaBar->SetPercent(StamPct);
			StaminaBar->SetFillColorAndOpacity(StamPct < 0.25f ? Blood : FLinearColor(0.82f, 0.79f, 0.72f, 0.82f));
		}

		// --- Crosshair / interaction prompt ---
		const bool bHasPrompt = !Player->CrosshairPrompt.IsEmpty();
		const bool bGrabbed = Player->IsGrabbed();
		SetTextIfChanged(PromptText, bHasPrompt
			? ShiftLoc::Text(Player->CrosshairPrompt)
			: FString());
		if (PromptText)
		{
			PromptText->SetVisibility(bHasPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			PromptText->SetColorAndOpacity(FSlateColor(bGrabbed ? BloodBright : Bone));
		}
		if (CrosshairDot)
		{
			CrosshairDot->SetVisibility(bHasPrompt ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}

		const float Hold = bGrabbed ? Player->StruggleProgress : Player->HoldProgress;
		if (HoldBar)
		{
			HoldBar->SetPercent(FMath::Clamp(Hold, 0.f, 1.f));
			HoldBar->SetVisibility(Hold > 0.01f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			HoldBar->SetFillColorAndOpacity(bGrabbed ? BloodBright : Candle);
		}
	}
	else
	{
		SetTextIfChanged(CarryText, FString());
		if (CarryText) { CarryText->SetVisibility(ESlateVisibility::Collapsed); }
		if (StaminaBar) { StaminaBar->SetPercent(0.f); }
		if (PromptText) { PromptText->SetVisibility(ESlateVisibility::Collapsed); }
		if (HoldBar) { HoldBar->SetVisibility(ESlateVisibility::Collapsed); }
	}

	// --- Subtitle / banner ---
	const bool bSubtitle = GM->SubtitleTime > 0.f && !GM->SubtitleText.IsEmpty();
	SetTextIfChanged(SubtitleText, bSubtitle ? ShiftLoc::Text(GM->SubtitleText) : FString());
	if (SubtitleText)
	{
		SubtitleText->SetVisibility(bSubtitle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	const bool bBanner = GM->BannerTimer > 0.f && !GM->BannerText.IsEmpty();
	SetTextIfChanged(BannerText, bBanner ? ShiftLoc::Text(GM->BannerText) : FString());
	if (BannerText)
	{
		BannerText->SetVisibility(bBanner ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		BannerText->SetColorAndOpacity(FSlateColor(bShifted ? BloodBright : Bone));
	}

	RefreshObjective(GM, Player);
	RefreshRules(GM, bShifted);

	// --- Heartbeat vignette ---
	if (HeartbeatOverlay)
	{
		const bool bPulse = GM->HeartbeatPulseAlpha > 0.02f;
		HeartbeatOverlay->SetVisibility(bPulse ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		HeartbeatOverlay->SetRenderOpacity(FMath::Clamp(GM->HeartbeatPulseAlpha * 0.36f, 0.f, 0.40f));
	}
}

void UShiftHUDWidget::RefreshRules(AShiftStoreGameMode* GM, bool bShifted)
{
	const bool bCorrupted = GM->NightIndex >= 2 && (GM->HasActiveAnomaly() || bShifted);

	SetTextIfChanged(RulesTitle, ShiftLoc::Text(TEXT("SHIFT RULES — Observe Customer Before Handing:")));
	SetTextIfChanged(RuleText1, ShiftLoc::Text(TEXT("1. Tilted head / stares wall -> Hand [PANTRY Tomato Soup]")));
	SetTextIfChanged(RuleText2, ShiftLoc::Text(TEXT("2. Red eye glare & light flicker -> Hand [DAILY Whole Milk]")));
	SetTextIfChanged(RuleText3, ShiftLoc::Text(TEXT("3. Feet floating off floor -> Hand [MORNING Bread]")));
	SetTextIfChanged(CorruptionText, ShiftLoc::Text(TEXT("[! SIGNAL CORRUPTED — FIX ANOMALY !]")));

	if (RulesTitle)
	{
		RulesTitle->SetColorAndOpacity(FSlateColor(bCorrupted ? BloodBright : Candle));
	}
	if (CorruptionText)
	{
		CorruptionText->SetVisibility(bCorrupted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UShiftHUDWidget::RefreshObjective(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player)
{
	TArray<FString> Lines;
	TArray<FLinearColor> Colors;

	const auto Add = [&Lines, &Colors](const FString& Line, const FLinearColor& Color)
	{
		Lines.Add(Line);
		Colors.Add(Color);
	};

	bool bEntityAlert = false;

	if (const AShiftCustomer* Customer = GM->ActiveCustomer.Get())
	{
		if (Customer->IsMonsterActive())
		{
			bEntityAlert = true;
			Add(ShiftLoc::Text(TEXT("!! ABNORMAL ENTITY IN STORE !!")), BloodBright);
			Add(ShiftLoc::Format(TEXT("Demand: Find [%s] (Red Glow)"),
				{ FString(ShiftData::ProductLabel(Customer->Spec.MonsterSku)) }), Candle);
			Add(ShiftLoc::Text(TEXT("-> Deliver it to the Checkout Register!")), Bone);
			Add(ShiftLoc::Text(TEXT("Shelves swapped stock! Corridor Breaker [E] restores light 10s.")), Ash);
		}
		else if (Customer->State == ECustomerState::WaitingForHelp)
		{
			Add(ShiftLoc::Format(TEXT("* Customer Help: Bring [%s]"),
				{ FString(ShiftData::ProductLabel(Customer->Spec.WantedSku)) }), Candle);
			Add(ShiftLoc::Text(TEXT("Take the product from a shelf, then press [E] on the customer.")), Bone);
			Add(ShiftLoc::Text(TEXT("Check SHIFT RULES (Bottom-Left): Observe posture before handing!")), Warn);
		}
		else if (Customer->State == ECustomerState::WaitingAtCheckout)
		{
			Add(ShiftLoc::Text(TEXT("* Customer waiting at Checkout Register [E]")), Candle);
		}
		else if (Customer->State == ECustomerState::Leaving)
		{
			Add(ShiftLoc::Text(TEXT("Customer is leaving. Keep the doorway clear.")), Bone);
		}
		else
		{
			Add(ShiftLoc::Text(TEXT("* Customer in store (Watch them closely)")), Bone);
		}

		for (const TWeakObjectPtr<AShiftTaskTarget>& TaskPtr : GM->ActiveTasks)
		{
			const AShiftTaskTarget* Task = TaskPtr.Get();
			if (Task && Task->Type == EShiftTaskType::FixAnomaly)
			{
				Add(ShiftLoc::Format(TEXT("! ANOMALY: %s (%ds)"),
					{
						ShiftLoc::Text(Task->Label),
						LexToString(FMath::CeilToInt(Task->TimeLeft))
					}), BloodBright);
			}
		}
	}
	else
	{
		FShiftTaskGuidance Guide;
		if (GM->GetTaskGuidance(Player, Guide) && Guide.Task.IsValid())
		{
			const auto TimeText = [](const AShiftTaskTarget* Task) -> FString
			{
				if (!Task)
				{
					return FString();
				}
				return Task->IsOverdue()
					? ShiftLoc::Text(TEXT("OVERDUE! (-Sanity)"))
					: ShiftLoc::Format(TEXT("%ds"), { LexToString(FMath::CeilToInt(Task->TimeLeft)) });
			};

			Add(ShiftLoc::Text(Guide.TaskLabel), Candle);
			Add(ShiftLoc::Format(TEXT("Time left: %s"), { TimeText(Guide.Task.Get()) }),
				Guide.Task->IsOverdue() ? BloodBright : Bone);

			const TCHAR* Step = TEXT("Look at the task marker and HOLD [E] to work.");
			switch (Guide.Action)
			{
			case EShiftGuideAction::CollectCarton: Step = TEXT("1/2  Warehouse: press [E] to take a stock carton."); break;
			case EShiftGuideAction::DeliverCarton: Step = TEXT("2/2  Return to the marked shelf; HOLD [E] to restock."); break;
			case EShiftGuideAction::CollectMop:    Step = TEXT("1/2  Follow the marker; press [E] to take the mop."); break;
			case EShiftGuideAction::MopFloor:      Step = TEXT("2/2  Return to the marked spill; HOLD [E] to mop."); break;
			case EShiftGuideAction::CookOden:      Step = TEXT("At the oden station, HOLD [E] to prepare the broth."); break;
			case EShiftGuideAction::FixAnomaly:    Step = TEXT("Follow the yellow marker; HOLD [E] to reset the store anomaly."); break;
			default: break;
			}
			Add(ShiftLoc::Text(Step), Bone);

			if (GM->GuideRenderer && GM->GuideRenderer->bActive
				&& !GM->GuideRenderer->IsReplanning()
				&& GM->GuideRenderer->Status == EShiftRouteStatus::Unreachable)
			{
				Add(ShiftLoc::Text(TEXT("Route blocked. Return to an aisle.")), Candle);
			}

			int32 Overdue = 0;
			for (const TWeakObjectPtr<AShiftTaskTarget>& TaskPtr : GM->ActiveTasks)
			{
				const AShiftTaskTarget* Task = TaskPtr.Get();
				if (!Task)
				{
					continue;
				}
				if (Task->IsOverdue())
				{
					++Overdue;
				}
				if (Task != Guide.Task.Get())
				{
					Add(ShiftLoc::Format(TEXT("%s · %s"), { Task->Label, TimeText(Task) }),
						Task->IsOverdue() ? BloodBright : Ash);
				}
			}

			if (Overdue > 0)
			{
				bEntityAlert = true;
				Add(ShiftLoc::Format(TEXT("%d overdue chores are draining sanity!"),
					{ LexToString(Overdue) }), BloodBright);
			}
			if (GM->ActiveTasks.Num() > 1)
			{
				Add(ShiftLoc::Text(TEXT("[TAB] Next chore")), Ash);
			}
		}
		else if (const int32 Loose = GM->Phase == EShiftPhase::Cleanup ? GM->CountLooseMerchandise() : 0; Loose > 0)
		{
			Add(ShiftLoc::Format(TEXT("Tidy up: %d items left on the floor"), { LexToString(Loose) }), Candle);
			Add(ShiftLoc::Text(TEXT("Pick up loose merchandise and press [E] at any shelf to put it back.")), Bone);
		}
		else
		{
			Add(ShiftLoc::Text(TEXT("Waiting for the next customer.")), Ash);
		}
	}

	if (ObjectiveTitle)
	{
		SetTextIfChanged(ObjectiveTitle, bEntityAlert
			? ShiftLoc::Text(TEXT("!! ABNORMAL ENTITY IN STORE !!"))
			: ShiftLoc::Text(TEXT("CURRENT OBJECTIVE")));
		ObjectiveTitle->SetColorAndOpacity(FSlateColor(bEntityAlert ? BloodBright : Ash));
	}

	SetObjectiveRows(Lines, Colors);
}

void UShiftHUDWidget::SetObjectiveRows(const TArray<FString>& Lines, const TArray<FLinearColor>& Colors)
{
	for (int32 Index = 0; Index < ObjectiveRowPool.Num(); ++Index)
	{
		UTextBlock* Row = ObjectiveRowPool[Index].Get();
		if (!Row)
		{
			continue;
		}

		if (Lines.IsValidIndex(Index))
		{
			SetTextIfChanged(Row, Lines[Index]);
			Row->SetColorAndOpacity(FSlateColor(Colors.IsValidIndex(Index) ? Colors[Index] : Ash));
			Row->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Row->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// rules button
void UShiftHUDWidget::ToggleRules()
{
    bRulesExpanded = !bRulesExpanded;

    if (RulesCard)
    {
        RulesCard->SetVisibility(bRulesExpanded
            ? ESlateVisibility::HitTestInvisible
            : ESlateVisibility::Collapsed);
    }

    if (RulesButtonLabel)
    {
        RulesButtonLabel->SetText(FText::FromString(ShiftLoc::Text(
            bRulesExpanded ? TEXT("HIDE RULES") : TEXT("SHOW RULES"))));
    }

    // 不再调用 PC->SetRulesOpen(...)
}