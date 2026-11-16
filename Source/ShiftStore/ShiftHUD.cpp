#include "ShiftHUD.h"
#include "ShiftLocalization.h"
#include "ShiftGuidance.h"
#include "ShiftStoreGameMode.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftPlayerController.h"
#include "ShiftTaskTarget.h"
#include "ShiftCustomer.h"
#include "ShiftDirector.h"
#include "ShiftItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/ConstructorHelpers.h"

AShiftHUD::AShiftHUD()
{
	static ConstructorHelpers::FObjectFinder<UFont> SC(TEXT("/Game/ShiftStore/Fonts/NotoSansCJKsc-Regular_Font.NotoSansCJKsc-Regular_Font"));
	static ConstructorHelpers::FObjectFinder<UFont> JP(TEXT("/Game/ShiftStore/Fonts/NotoSansCJKjp-Regular_Font.NotoSansCJKjp-Regular_Font"));
	static ConstructorHelpers::FObjectFinder<UFont> TC(TEXT("/Game/ShiftStore/Fonts/NotoSansCJKtc-Regular_Font.NotoSansCJKtc-Regular_Font"));
	ChineseFont = SC.Object;
	JapaneseFont = JP.Object;
	TraditionalChineseFont = TC.Object;
}

UFont* AShiftHUD::InterfaceFont(int32 Language) const
{
	if (Language < 0) { Language = ShiftLoc::CurrentLanguage(); }
	UFont* Font = Language == 1 ? JapaneseFont.Get() : (Language == 2 ? TraditionalChineseFont.Get() : ChineseFont.Get());
	return Font ? Font : (GEngine ? GEngine->GetLargeFont() : nullptr);
}

bool AShiftHUD::HasRuntimeFonts() const
{
	return ChineseFont && JapaneseFont && TraditionalChineseFont
		&& ChineseFont->FontCacheType == EFontCacheType::Runtime
		&& JapaneseFont->FontCacheType == EFontCacheType::Runtime
		&& TraditionalChineseFont->FontCacheType == EFontCacheType::Runtime;
}

FBox2D AShiftHUD::LanguageButton(int32 Index, float Width, float Height)
{
	return FBox2D(FVector2D(Width * 0.30f, Height * (0.32f + Index * 0.095f)),
		FVector2D(Width * 0.70f, Height * (0.395f + Index * 0.095f)));
}

FBox2D AShiftHUD::PauseButton(int32 Index, float Width, float Height)
{
	return FBox2D(FVector2D(Width * 0.30f, Height * (0.36f + Index * 0.11f)),
		FVector2D(Width * 0.70f, Height * (0.445f + Index * 0.11f)));
}

void AShiftHUD::DrawLocalizedText(const FString& Str, FLinearColor Color, float X, float Y, UFont* Font, float Scale, float MaxWidth)
{
	const FString Text = ShiftLoc::Text(Str);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	if (MaxWidth <= 0.f) { MaxWidth = X < Canvas->ClipX * 0.5f ? 392.f : Canvas->ClipX - X - 38.f; }
	if (TW > MaxWidth) { Scale *= MaxWidth / TW; }
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AShiftHUD::DrawBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back)
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Fill, X + 2.f, Y + 2.f, FMath::Clamp(Pct, 0.f, 1.f) * (W - 4.f), H - 4.f);
}

void AShiftHUD::DrawCenteredText(const FString& Str, float Y, FLinearColor Color, float Scale)
{
	if (!Canvas) { return; }
	UFont* Font = InterfaceFont();
	const FString Text = ShiftLoc::Text(Str);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	const float MaxWidth = Canvas->ClipX * 0.88f;
	if (TW > MaxWidth) { Scale *= MaxWidth / TW; TW = MaxWidth; }
	DrawText(Text, Color, (Canvas->ClipX - TW) * 0.5f, Y, Font, Scale);
}

void AShiftHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) { return; }

	AShiftStoreGameMode* GM = GetWorld()->GetAuthGameMode<AShiftStoreGameMode>();
	AShiftPlayerCharacter* Player = Cast<AShiftPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!GM) { return; }

	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	UFont* Font = InterfaceFont();

	if (const AShiftPlayerController* PC = Cast<AShiftPlayerController>(PlayerOwner); PC && PC->IsPauseMenuOpen())
	{
		DrawRect(FLinearColor(0.015f, 0.025f, 0.025f, 0.94f), 0.f, 0.f, W, H);
		DrawRect(FLinearColor(0.18f, 0.55f, 0.48f), W * 0.25f, H * 0.29f, W * 0.50f, 3.f);
		DrawCenteredText(TEXT("PAUSED"), H * 0.20f, FLinearColor(0.82f, 0.95f, 0.90f), 2.f);
		const TCHAR* Labels[] = {TEXT("Resume Game"), TEXT("Return to Main Menu"), TEXT("Quit Game")};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FBox2D Box = PauseButton(Index, W, H);
			const bool bSelected = PC->PauseSelection() == Index;
			DrawRect(bSelected ? FLinearColor(0.13f, 0.34f, 0.29f) : FLinearColor(0.05f, 0.10f, 0.10f),
				Box.Min.X, Box.Min.Y, Box.GetSize().X, Box.GetSize().Y);
			if (bSelected) { DrawRect(FLinearColor(0.45f, 0.90f, 0.72f), Box.Min.X, Box.Min.Y, 4.f, Box.GetSize().Y); }
			DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f,
				bSelected ? FLinearColor::White : FLinearColor(0.65f, 0.74f, 0.70f), 1.3f);
		}
		DrawCenteredText(TEXT("Only dawn checkpoints are saved. Returning to the menu discards this night's progress."),
			H * 0.77f, FLinearColor(0.85f, 0.78f, 0.60f), 0.95f);
		DrawCenteredText(TEXT("[Esc] Resume    [Up / Down] Select    [Enter / Click] Confirm"),
			H * 0.85f, FLinearColor(0.65f, 0.82f, 0.78f), 0.95f);
		return;
	}

	// Full-screen overlay phases (Title, NightIntro, Dawn, GameOver, Ending)
	if (GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup)
	{
		DrawRect(FLinearColor(0.02f, 0.03f, 0.03f, 0.90f), 0.f, 0.f, W, H);
		DrawRect(FLinearColor(0.18f, 0.55f, 0.48f, 1.f), W * 0.15f, H * 0.22f, W * 0.70f, 3.f);

		if (GM->Phase == EShiftPhase::LanguageSelection)
		{
			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.13f, FLinearColor(0.82f, 0.95f, 0.90f), 1.9f);
			// Native language names remain readable before a selection is made.
			DrawCenteredText(TEXT("Language / 言語 / 語言 / 语言"), H * 0.25f, FLinearColor::White, 1.4f);
			for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index)
			{
				const FBox2D Box = LanguageButton(Index, W, H);
				const bool bSelected = GM->SelectedLanguage == Index;
				DrawRect(bSelected ? FLinearColor(0.13f, 0.34f, 0.29f, 1.f) : FLinearColor(0.05f, 0.10f, 0.10f, 1.f),
					Box.Min.X, Box.Min.Y, Box.GetSize().X, Box.GetSize().Y);
				if (bSelected) { DrawRect(FLinearColor(0.45f, 0.90f, 0.72f), Box.Min.X, Box.Min.Y, 4.f, Box.GetSize().Y); }
				UFont* NativeFont = InterfaceFont(Index);
				const FString Name = FString::Printf(TEXT("%d     %s"), Index + 1, *ShiftLoc::NativeName(Index));
				float TW = 0.f, TH = 0.f;
				GetTextSize(Name, TW, TH, NativeFont, 1.35f);
				DrawText(Name, bSelected ? FLinearColor::White : FLinearColor(0.65f, 0.74f, 0.70f),
					Box.Min.X + 24.f, Box.Min.Y + (Box.GetSize().Y - TH) * 0.5f, NativeFont, 1.35f);
			}
			DrawCenteredText(TEXT("Confirm [Enter]"), H * 0.74f, FLinearColor(1.f, 0.90f, 0.45f), 1.3f);
			DrawCenteredText(TEXT("[Up / Down] Select    [1 / 2 / 3 / 4] Language    [Enter] Confirm"), H * 0.82f, FLinearColor(0.65f, 0.82f, 0.78f), 0.95f);
		}
		else if (GM->Phase == EShiftPhase::Title)
		{
			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.27f, FLinearColor(0.82f, 0.95f, 0.90f), 2.4f);
			DrawCenteredText(TEXT("Convenience Store Night-Shift Horror — GitHub Game Off 2026 (Theme: SHIFT)"), H * 0.36f, FLinearColor(0.60f, 0.75f, 0.70f), 1.1f);
			DrawCenteredText(TEXT("Complete clerk chores before timers expire. Serve every customer."), H * 0.45f, FLinearColor(0.85f, 0.88f, 0.86f), 1.15f);
			DrawCenteredText(TEXT("When an entity reveals itself, the store SHIFTS into an abnormal state."), H * 0.50f, FLinearColor(0.95f, 0.45f, 0.40f), 1.15f);
			DrawCenteredText(TEXT("Find the item it demands and deliver it to the Checkout Register to restore the store."), H * 0.55f, FLinearColor(0.85f, 0.88f, 0.86f), 1.15f);
			DrawCenteredText(TEXT("Controls: [WASD] Move   [Mouse] Look   [E / LMB] Interact / Hold Chore   [F / RMB] Throw Item   [G] Drop"), H * 0.65f, FLinearColor(0.65f, 0.82f, 0.78f), 1.05f);
			DrawCenteredText(TEXT("[SPACE / E] New Game — Night 1"), H * 0.73f, FLinearColor(1.f, 0.90f, 0.45f), 1.4f);
			if (GM->bHasSavedProgress)
			{
				DrawCenteredText(ShiftLoc::Format(TEXT("[C] Continue Saved Game — %s"), {GM->CurrentNightSpec().Title}), H * 0.81f, FLinearColor(0.75f, 0.90f, 0.85f), 1.1f);
			}
			DrawCenteredText(TEXT("[L] Change language"), H * 0.89f, FLinearColor(0.65f, 0.82f, 0.78f), 0.95f);
		}
		else if (GM->Phase == EShiftPhase::NightIntro)
		{
			const FShiftNightSpec& N = GM->CurrentNightSpec();
			DrawCenteredText(N.Title, H * 0.28f, FLinearColor(0.82f, 0.95f, 0.90f), 2.2f);
			TArray<FString> Lines;
			ShiftLoc::Text(N.Intro).ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				DrawCenteredText(Lines[i], H * 0.40f + i * 34.f, FLinearColor(0.88f, 0.90f, 0.88f), 1.2f);
			}
			DrawCenteredText(ShiftLoc::Format(TEXT("Customers Expected Tonight: %d   |   Auto-Save Active"), {LexToString(N.Customers.Num())}), H * 0.58f, FLinearColor(0.65f, 0.82f, 0.78f), 1.1f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Chore time limit: %ds. Timers are paused on this screen."), {LexToString(FMath::RoundToInt(N.TaskTimeLimit))}), H * 0.65f, FLinearColor(0.75f, 0.90f, 0.85f), 1.f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] to Begin Night"), H * 0.74f, FLinearColor(1.f, 0.90f, 0.45f), 1.4f);
		}
		else if (GM->Phase == EShiftPhase::Dawn)
		{
			DrawCenteredText(TEXT("06:00 AM — DAWN"), H * 0.28f, FLinearColor(0.95f, 0.88f, 0.55f), 2.2f);
			DrawCenteredText(ShiftLoc::Format(TEXT("%s COMPLETE — Store Handed Over to Day Shift"), {FString(GM->CurrentNightSpec().Title)}), H * 0.38f, FLinearColor(0.82f, 0.95f, 0.90f), 1.3f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Sales Revenue: $%.2f   |   Merchandise Losses: -$%.2f   |   Progress Saved"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses)}), H * 0.48f, FLinearColor(0.85f, 0.88f, 0.86f), 1.2f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] for Next Night"), H * 0.72f, FLinearColor(1.f, 0.90f, 0.45f), 1.4f);
		}
		else if (GM->Phase == EShiftPhase::GameOver)
		{
			DrawCenteredText(TEXT("SHIFT TERMINATED"), H * 0.28f, FLinearColor(0.95f, 0.25f, 0.20f), 2.3f);
			DrawCenteredText(GM->GameOverReason, H * 0.42f, FLinearColor(0.92f, 0.85f, 0.85f), 1.35f);
			DrawCenteredText(TEXT("Press [SPACE], [E], or [R] to Rewind to Start of Current Night"), H * 0.70f, FLinearColor(1.f, 0.90f, 0.45f), 1.35f);
		}
		else if (GM->Phase == EShiftPhase::Ending)
		{
			DrawCenteredText(TEXT("ALL NIGHT SHIFTS SURVIVED"), H * 0.27f, FLinearColor(0.82f, 0.95f, 0.90f), 2.2f);
			DrawCenteredText(TEXT("The morning clerk unlocks the front door. The aisles are back where they belong."), H * 0.40f, FLinearColor(0.88f, 0.90f, 0.88f), 1.2f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Final Revenue: $%.2f   |   Total Losses: -$%.2f   |   Net: $%.2f"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses), FString::Printf(TEXT("%.2f"), GM->Income - GM->Losses)}), H * 0.50f, FLinearColor(0.95f, 0.88f, 0.55f), 1.3f);
			DrawCenteredText(TEXT("Thank you for playing SHIFT / 24! Press [SPACE] to Play Again."), H * 0.72f, FLinearColor(1.f, 0.90f, 0.45f), 1.35f);
		}
		return;
	}

	// --- Top-Left Status Panel: Sanity, Ledger, Shift State ---
	const bool bShifted = GM->Director && GM->Director->IsShifted();
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.78f), 24.f, 24.f, 420.f, 156.f);
	DrawLocalizedText(ShiftLoc::Format(TEXT("%s  |  Customers: %d / %d  |  %s"), {FString(GM->CurrentNightSpec().Title), LexToString(GM->CustomersServed), LexToString(GM->CurrentNightSpec().Customers.Num()), FString(bShifted ? TEXT("STATE: SHIFTED [ABNORMAL]") : TEXT("STATE: NORMAL"))}),
		bShifted ? FLinearColor(1.f, 0.32f, 0.28f) : FLinearColor(0.75f, 0.92f, 0.86f), 36.f, 32.f, Font, 1.0f);

	// Sanity bar
	const float SanityPct = GM->Sanity / ShiftData::MaxSanity;
	DrawLocalizedText(ShiftLoc::Format(TEXT("SANITY: %d%%"), {LexToString(FMath::RoundToInt(GM->Sanity))}),
		SanityPct < 0.35f ? FLinearColor(1.f, 0.3f, 0.25f) : FLinearColor(0.85f, 0.92f, 0.90f), 36.f, 70.f, Font, 0.95f, 158.f);
	DrawBar(205.f, 75.f, 225.f, 16.f, SanityPct,
		SanityPct < 0.35f ? FLinearColor(0.92f, 0.22f, 0.18f, 1.f) : FLinearColor(0.25f, 0.80f, 0.68f, 1.f),
		FLinearColor(0.12f, 0.16f, 0.16f, 0.9f));

	// Store ledger / loss limit
	const float LossPct = GM->Losses / ShiftData::LossLimit;
	DrawLocalizedText(ShiftLoc::Format(TEXT("SALES: +$%.1f   LOSS: -$%.1f / -$%.0f"), {FString::Printf(TEXT("%.1f"), GM->Income), FString::Printf(TEXT("%.1f"), GM->Losses), FString::Printf(TEXT("%.0f"), ShiftData::LossLimit)}),
		LossPct > 0.7f ? FLinearColor(1.f, 0.4f, 0.3f) : FLinearColor(0.85f, 0.90f, 0.85f), 36.f, 111.f, Font, 0.92f);
	DrawBar(36.f, 146.f, 394.f, 12.f, LossPct, FLinearColor(0.90f, 0.42f, 0.18f, 1.f), FLinearColor(0.12f, 0.15f, 0.15f, 0.9f));

	// Held item indicator
	if (Player)
	{
		const FString HeldStr = Player->HeldItem.IsValid()
			? ShiftLoc::Format(TEXT("CARRYING: %s  ([F/RMB] Throw  [G] Drop)"), {FString(Player->HeldItem->DisplayName())})
			: TEXT("HANDS: Empty  ([E] Pick up / Interact)");
		DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.72f), 24.f, 188.f, 420.f, 44.f);
		DrawLocalizedText(HeldStr, FLinearColor(0.92f, 0.90f, 0.70f), 36.f, 197.f, Font, 0.92f);
	}

	// --- Top-Right Panel: Priority Objective, Customer / Entity Demand, Active Chores ---
	const float RX = W - 430.f;
	float RY = 24.f;
	const float ChecklistHeight = FMath::Max(220.f, 70.f + GM->ActiveTasks.Num() * 32.f + (GM->ActiveCustomer.IsValid() ? 100.f : 0.f) + (GM->Phase == EShiftPhase::Cleanup ? 36.f : 0.f));
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.78f), RX, RY, 406.f, ChecklistHeight);
	DrawLocalizedText(TEXT("SHIFT CHECKLIST & ALERTS"), FLinearColor(0.75f, 0.92f, 0.86f), RX + 14.f, RY + 10.f, Font, 1.0f);
	RY += 48.f;

	if (GM->ActiveCustomer.IsValid())
	{
		AShiftCustomer* C = GM->ActiveCustomer.Get();
		if (C->IsMonsterActive())
		{
			DrawLocalizedText(TEXT("!! ABNORMAL ENTITY IN STORE !!"), FLinearColor(1.f, 0.25f, 0.22f), RX + 14.f, RY, Font, 1.05f);
			RY += 32.f;
			DrawLocalizedText(ShiftLoc::Format(TEXT("Demand: Find [%s] (Red Glow)"), {FString(ShiftData::ProductLabel(C->Spec.MonsterSku))}),
				FLinearColor(1.f, 0.82f, 0.45f), RX + 14.f, RY, Font, 0.95f);
			RY += 32.f;
			DrawLocalizedText(TEXT("-> Deliver it to the Checkout Register!"), FLinearColor(0.95f, 0.90f, 0.85f), RX + 14.f, RY, Font, 0.95f);
			RY += 32.f;
		}
		else if (C->State == ECustomerState::WaitingForHelp)
		{
			DrawLocalizedText(ShiftLoc::Format(TEXT("* Customer Help: Bring [%s]"), {FString(ShiftData::ProductLabel(C->Spec.WantedSku))}),
				FLinearColor(1.f, 0.88f, 0.40f), RX + 14.f, RY, Font, 0.98f);
			RY += 32.f;
		}
		else if (C->State == ECustomerState::WaitingAtCheckout)
		{
			DrawLocalizedText(TEXT("* Customer waiting at Checkout Register [E]"),
				FLinearColor(0.45f, 1.f, 0.65f), RX + 14.f, RY, Font, 0.98f);
			RY += 32.f;
		}
		else
		{
			DrawLocalizedText(TEXT("* Customer in store (Watch them closely)"),
				FLinearColor(0.80f, 0.92f, 0.88f), RX + 14.f, RY, Font, 0.95f);
			RY += 32.f;
		}
	}

	if (GM->Phase == EShiftPhase::Cleanup)
	{
		DrawLocalizedText(TEXT("DAWN CLEANUP (Equip Mop in Warehouse):"), FLinearColor(0.95f, 0.88f, 0.45f), RX + 14.f, RY, Font, 0.95f);
		RY += 32.f;
	}

	for (const TWeakObjectPtr<AShiftTaskTarget>& TP : GM->ActiveTasks)
	{
		if (const AShiftTaskTarget* T = TP.Get())
		{
			const bool bOver = T->IsOverdue();
			const FString TimeStr = (T->Deadline > 0.f)
				? (bOver ? TEXT("OVERDUE! (-Sanity)") : ShiftLoc::Format(TEXT("%ds"), {LexToString(FMath::CeilToInt(T->TimeLeft))}))
				: TEXT("Required");
			DrawLocalizedText(ShiftLoc::Format(TEXT("- %s  [%s]"), {FString(T->Label), FString(TimeStr)}),
				bOver ? FLinearColor(1.f, 0.30f, 0.25f) : FLinearColor(0.86f, 0.90f, 0.88f),
				RX + 14.f, RY, Font, 0.92f);
			RY += 32.f;
		}
	}

	// Status banner (center top)
	if (GM->BannerTimer > 0.f && !GM->BannerText.IsEmpty())
	{
		const float BannerY = FMath::Max(40.f + ChecklistHeight, H * 0.25f);
		DrawRect(FLinearColor(0.05f, 0.02f, 0.02f, 0.82f), W * 0.05f, BannerY, W * 0.90f, 42.f);
		DrawCenteredText(GM->BannerText, BannerY + 8.f, FLinearColor(1.f, 0.85f, 0.45f), 1.15f);
	}

	// Canvas runtime fonts support CJK. The old offline TextRender font did not.
	if (AShiftCustomer* Customer = GM->ActiveCustomer.Get(); Customer && Customer->Speech && !Customer->Speech->Text.IsEmpty())
	{
		const FVector Head = Customer->Speech->GetComponentLocation();
		if (Player && FVector::DistSquared(Player->GetActorLocation(), Head) < FMath::Square(1200.f) && PlayerOwner->LineOfSightTo(Customer))
		{
			const FVector Screen = Project(Head);
			if (Screen.Z > 0.f && Screen.X > 0.f && Screen.X < W && Screen.Y > H * 0.30f && Screen.Y < H * 0.90f)
			{
				const FString Speech = ShiftLoc::Text(Customer->Speech->Text.ToString());
				float TW = 0.f, TH = 0.f;
				GetTextSize(Speech, TW, TH, Font, 1.f);
				const float Scale = TW > W * 0.7f ? W * 0.7f / TW : 1.f;
				TW *= Scale;
				const float X = FMath::Clamp(Screen.X - TW * 0.5f, 12.f, W - TW - 12.f);
				DrawRect(FLinearColor(0.02f, 0.03f, 0.03f, 0.8f), X - 8.f, Screen.Y - 4.f, TW + 16.f, TH * Scale + 8.f);
				DrawText(Speech, FLinearColor(Customer->Speech->TextRenderColor), X, Screen.Y, Font, Scale);
			}
		}
	}

	// Center Crosshair & Interaction Prompt
	const float CX = W * 0.5f;
	const float CY = H * 0.5f;
	DrawRect(FLinearColor(0.9f, 0.95f, 0.92f, 0.85f), CX - 5.f, CY - 1.f, 10.f, 2.f);
	DrawRect(FLinearColor(0.9f, 0.95f, 0.92f, 0.85f), CX - 1.f, CY - 5.f, 2.f, 10.f);

	if (Player && !Player->CrosshairPrompt.IsEmpty())
	{
		DrawCenteredText(Player->CrosshairPrompt, CY + 22.f, FLinearColor(1.f, 0.95f, 0.72f), 1.1f);
		const float BarProg = Player->IsGrabbed() ? Player->StruggleProgress : Player->HoldProgress;
		if (BarProg > 0.01f)
		{
			DrawBar(CX - 90.f, CY + 52.f, 180.f, 12.f, BarProg,
				FLinearColor(0.30f, 0.92f, 0.70f, 1.f), FLinearColor(0.1f, 0.12f, 0.12f, 0.9f));
		}
	}
	DrawTaskGuidance(GM, Player);
	DrawLocalizedText(TEXT("[Esc] Pause"), FLinearColor(0.65f, 0.82f, 0.78f), W - 170.f, H - 36.f, Font, 0.85f, 150.f);
}

void AShiftHUD::DrawTaskGuidance(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player)
{
	FShiftTaskGuidance Guide;
	if (!GM->GetTaskGuidance(Player, Guide))
	{
		GuideActor.Reset();
		GuideRoute.Empty();
		return;
	}
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bChanged = GuideActor != Guide.Target || !GuideDestination.Equals(Guide.Location, 1.f);
	bool bRebuild = bChanged;
	if (!bChanged && Now - GuideRouteCheckedAt >= 2.f)
	{
		GuideRouteCheckedAt = Now;
		bRebuild = !GuideRoute.IsEmpty() && !ShiftGuidance::CanWalkSegment(GetWorld(), Player, Guide.Target.Get(), Player->GetActorLocation(), GuideRoute[0]);
	}
	if (bRebuild)
	{
		GuideActor = Guide.Target;
		GuideDestination = Guide.Location;
		GuideRoute = ShiftGuidance::FindRoute(GetWorld(), Player, Guide.Target.Get(), Guide.Location);
		GuideRouteCheckedAt = Now;
	}
	while (!GuideRoute.IsEmpty() && FVector::Dist2D(Player->GetActorLocation(), GuideRoute[0]) < 65.f) { GuideRoute.RemoveAt(0); }
	const FVector Waypoint = GuideRoute.IsEmpty() ? Guide.Location : GuideRoute[0];
	const bool bViaAisle = GuideRoute.Num() > 1;
	const FString Marker = bViaAisle ? TEXT("FOLLOW AISLE") :
		(Guide.Action == EShiftGuideAction::CollectCarton ? TEXT("WAREHOUSE CARTON") :
		(Guide.Action == EShiftGuideAction::CollectMop ? TEXT("MOP") : TEXT("WORK HERE")));
	const FVector Screen = Project(Waypoint + FVector(0,0,70));
	FVector2D Position(Screen.X, Screen.Y);
	if (Screen.Z <= 0.f)
	{
		const FVector Right = PlayerOwner->PlayerCameraManager->GetCameraRotation().RotateVector(FVector::RightVector);
		Position = FVector2D(FVector::DotProduct(Waypoint - Player->GetActorLocation(), Right) >= 0.f ? W - 86.f : 86.f, H * 0.57f);
	}
	Position.X = FMath::Clamp(Position.X, 86.f, W - 86.f);
	Position.Y = FMath::Clamp(Position.Y, H * 0.39f, H - 215.f);
	const FLinearColor Accent(1.f, 0.85f, 0.25f);
	DrawLine(Position.X, Position.Y - 12.f, Position.X + 12.f, Position.Y, Accent, 3.f);
	DrawLine(Position.X + 12.f, Position.Y, Position.X, Position.Y + 12.f, Accent, 3.f);
	DrawLine(Position.X, Position.Y + 12.f, Position.X - 12.f, Position.Y, Accent, 3.f);
	DrawLine(Position.X - 12.f, Position.Y, Position.X, Position.Y - 12.f, Accent, 3.f);
	const FString Label = ShiftLoc::Format(TEXT("%s · %dm"), {Marker, LexToString(FMath::CeilToInt(FVector::Dist2D(Player->GetActorLocation(), Waypoint) / 100.f))});
	UFont* Font = InterfaceFont();
	float TW = 0.f, TH = 0.f;
	GetTextSize(Label, TW, TH, Font, 0.9f);
	const float LabelX = FMath::Clamp(Position.X - TW * 0.5f, 12.f, W - TW - 12.f);
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.9f), LabelX - 6.f, Position.Y + 17.f, TW + 12.f, TH + 6.f);
	DrawText(Label, Accent, LabelX, Position.Y + 18.f, Font, 0.9f);

	const float PanelWidth = FMath::Min(840.f, W - 48.f), Y = H - 158.f;
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.92f), 24.f, Y, PanelWidth, 140.f);
	DrawRect(Accent, 24.f, Y, 4.f, 140.f);
	DrawLocalizedText(ShiftLoc::Format(TEXT("NEXT CHORE: %s"), {Guide.TaskLabel}), Accent, 38.f, Y + 8.f, Font, 1.f, PanelWidth - 28.f);
	const bool bCarton = Guide.Action == EShiftGuideAction::CollectCarton || Guide.Action == EShiftGuideAction::DeliverCarton;
	const bool bMop = Guide.Action == EShiftGuideAction::CollectMop || Guide.Action == EShiftGuideAction::MopFloor;
	const bool bFirstStep = Guide.Action == EShiftGuideAction::CollectCarton || Guide.Action == EShiftGuideAction::CollectMop;
	const FLinearColor Dim(0.48f, 0.61f, 0.57f), Bright(0.92f, 0.97f, 0.94f);
	if (bCarton || bMop)
	{
		DrawLocalizedText(bCarton ? TEXT("1/2  Warehouse: press [E] to take a stock carton.") : TEXT("1/2  Follow the marker; press [E] to take the mop."),
			bFirstStep ? Bright : Dim, 38.f, Y + 43.f, Font, 0.95f, PanelWidth - 28.f);
		DrawLocalizedText(bCarton ? TEXT("2/2  Return to the marked shelf; HOLD [E] to restock.") : TEXT("2/2  Return to the marked spill; HOLD [E] to mop."),
			bFirstStep ? Dim : Bright, 38.f, Y + 75.f, Font, 0.95f, PanelWidth - 28.f);
	}
	else
	{
		DrawLocalizedText(TEXT("Follow the yellow aisle markers to the work location."), Bright, 38.f, Y + 43.f, Font, 0.95f, PanelWidth - 28.f);
		DrawLocalizedText(Guide.Action == EShiftGuideAction::CookOden
			? TEXT("At the oden station, HOLD [E] to prepare the broth.")
			: TEXT("Look at the task marker and HOLD [E] to work."), Bright, 38.f, Y + 75.f, Font, 0.95f, PanelWidth - 28.f);
	}
	DrawLocalizedText(TEXT("[TAB] Next chore   |   Finish all chores to admit customers."), Dim, 38.f, Y + 108.f, Font, 0.85f, PanelWidth - 28.f);
}
