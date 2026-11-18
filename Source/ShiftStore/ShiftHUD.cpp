#include "ShiftHUD.h"
#include "ShiftLocalization.h"
#include "ShiftGuidance.h"
#include "ShiftGuideActor.h"
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
#include "Camera/CameraComponent.h"
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
	return FBox2D(FVector2D(Width * 0.30f, Height * (0.32f + Index * 0.10f)),
		FVector2D(Width * 0.70f, Height * (0.395f + Index * 0.10f)));
}

FBox2D AShiftHUD::SensitivitySlider(float Width, float Height)
{
	return FBox2D(FVector2D(Width * 0.30f - 12.f, Height * 0.44f), FVector2D(Width * 0.70f + 12.f, Height * 0.53f));
}

FBox2D AShiftHUD::SensitivityButton(int32 Index, float Width, float Height)
{
	return FBox2D(FVector2D(Width * 0.30f, Height * (0.63f + Index * 0.11f)),
		FVector2D(Width * 0.70f, Height * (0.705f + Index * 0.11f)));
}

void AShiftHUD::DrawMouseSettings(const AShiftPlayerController* PC)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FLinearColor Accent(0.45f, 0.90f, 0.72f);
	DrawCenteredText(TEXT("Mouse Sensitivity"), H * 0.20f, FLinearColor(0.82f, 0.95f, 0.90f), 1.8f);
	DrawCenteredText(TEXT("Lower is slower. Changes are saved automatically."), H * 0.32f, FLinearColor(0.72f, 0.84f, 0.80f), 0.95f);
	DrawCenteredText(ShiftLoc::Format(TEXT("Mouse sensitivity: %sx"), {FString::Printf(TEXT("%.2f"), PC->MouseSensitivity())}),
		H * 0.39f, PC->MouseSettingsSelection() == 0 ? Accent : FLinearColor::White, 1.25f);
	const float Fraction = (PC->MouseSensitivity() - AShiftPlayerController::MinSensitivity) /
		(AShiftPlayerController::MaxSensitivity - AShiftPlayerController::MinSensitivity);
	DrawRect(FLinearColor(0.12f, 0.20f, 0.18f), W * 0.30f, H * 0.48f, W * 0.40f, 8.f);
	DrawRect(Accent, W * 0.30f, H * 0.48f, W * 0.40f * Fraction, 8.f);
	DrawRect(FLinearColor::White, W * (0.30f + 0.40f * Fraction) - 4.f, H * 0.48f - 10.f, 8.f, 28.f);
	DrawText(TEXT("0.10x"), FLinearColor(0.65f, 0.78f, 0.72f), W * 0.30f, H * 0.54f, InterfaceFont(), 0.9f);
	DrawText(TEXT("3.00x"), FLinearColor(0.65f, 0.78f, 0.72f), W * 0.70f - 65.f, H * 0.54f, InterfaceFont(), 0.9f);
	const TCHAR* Labels[] = {TEXT("Restore Default"), TEXT("Back")};
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FBox2D Box = SensitivityButton(Index, W, H);
		const bool bSelected = PC->MouseSettingsSelection() == Index + 1;
		DrawRect(bSelected ? FLinearColor(0.13f, 0.34f, 0.29f) : FLinearColor(0.05f, 0.10f, 0.10f), Box.Min.X, Box.Min.Y, Box.GetSize().X, Box.GetSize().Y);
		DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f, bSelected ? FLinearColor::White : FLinearColor(0.65f, 0.74f, 0.70f), 1.2f);
	}
	DrawCenteredText(TEXT("[Left / Right] Adjust    [Up / Down] Select    [Esc] Back"), H * 0.89f, FLinearColor(0.65f, 0.82f, 0.78f), 0.95f);
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
		if (PC->IsMouseSettingsOpen()) { DrawMouseSettings(PC); return; }
		DrawCenteredText(TEXT("PAUSED"), H * 0.20f, FLinearColor(0.82f, 0.95f, 0.90f), 2.f);
		const TCHAR* Labels[] = {TEXT("Resume Game"), TEXT("Mouse Sensitivity"), TEXT("Return to Main Menu"), TEXT("Quit Game")};
		for (int32 Index = 0; Index < 4; ++Index)
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
			H * 0.78f, FLinearColor(0.85f, 0.78f, 0.60f), 0.95f);
		DrawCenteredText(TEXT("[Esc] Resume    [Up / Down] Select    [Enter / Click] Confirm"),
			H * 0.88f, FLinearColor(0.65f, 0.82f, 0.78f), 0.95f);
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

	const float ChecklistHeight = DrawObjectivePanel(GM, Player);

	// Status banner (center top)
	if (GM->BannerTimer > 0.f && !GM->BannerText.IsEmpty())
	{
		const float BannerY = FMath::Max(40.f + ChecklistHeight, H * 0.25f);
		DrawRect(FLinearColor(0.05f, 0.02f, 0.02f, 0.82f), W * 0.05f, BannerY, W * 0.90f, 42.f);
		DrawCenteredText(GM->BannerText, BannerY + 8.f, FLinearColor(1.f, 0.85f, 0.45f), 1.15f);
	}

	// Canvas runtime fonts support CJK. The old offline TextRender font did not.
	if (AShiftCustomer* Customer = GM->ActiveCustomer.Get(); Customer && Customer->State != ECustomerState::MonsterGrabbing && Customer->Speech && !Customer->Speech->Text.IsEmpty())
	{
		const FVector Head = Customer->Speech->GetComponentLocation();
		if (Player && FVector::DistSquared(Player->GetActorLocation(), Head) < FMath::Square(1200.f) && PlayerOwner->LineOfSightTo(Customer))
		{
			const FVector Screen = Project(Head);
			if (Screen.Z > 0.f && Screen.X > 0.f && Screen.X < W && Screen.Y < H * 0.90f)
			{
				const FString Speech = ShiftLoc::Text(Customer->Speech->Text.ToString());
				const TArray<FString> Lines = WrapText(Speech, W * 0.65f, 0.95f, 2);
				float TW = 0.f;
				for (const FString& Line : Lines)
				{
					float LW = 0.f, LH = 0.f;
					GetTextSize(Line, LW, LH, Font, 0.95f);
					TW = FMath::Max(TW, LW);
				}
				const float X = FMath::Clamp(Screen.X - TW * 0.5f, 12.f, W - TW - 12.f);
				float Y = FMath::Max(Screen.Y, H * 0.35f);
				if (X < 444.f) { Y = FMath::Max(Y, 248.f); }
				if (X + TW > W - 455.f) { Y = FMath::Max(Y, ChecklistHeight + 42.f); }
				Y = FMath::Min(Y, H * 0.72f);
				DrawRect(FLinearColor(0.02f, 0.03f, 0.03f, 0.85f), X - 8.f, Y - 4.f, TW + 16.f, Lines.Num() * 30.f + 8.f);
				for (int32 Index = 0; Index < Lines.Num(); ++Index) { DrawText(Lines[Index], FLinearColor(Customer->Speech->TextRenderColor), X, Y + Index * 30.f, Font, 0.95f); }
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
	DrawSubtitle(GM);
	DrawLocalizedText(TEXT("[Esc] Pause"), FLinearColor(0.65f, 0.82f, 0.78f), W - 170.f, H - 36.f, Font, 0.85f, 150.f);
}

TArray<FString> AShiftHUD::WrapText(const FString& Str, float Width, float Scale, int32 MaxLines) const
{
	const FString Text = ShiftLoc::Text(Str);
	TArray<FString> Lines;
	FString Line;
	const auto Measure = [this, Scale](const FString& Value) {
		float W = 0.f, H = 0.f;
		GetTextSize(Value, W, H, InterfaceFont(), Scale);
		return W;
	};
	for (TCHAR Character : Text)
	{
		if (Character == TEXT('\n')) { Lines.Add(Line); Line.Empty(); continue; }
		Line.AppendChar(Character);
		if (Measure(Line) > Width && Line.Len() > 1)
		{
			int32 Space = INDEX_NONE;
			Line.FindLastChar(TEXT(' '), Space);
			const int32 Split = Space > 0 ? Space : Line.Len() - 1;
			Lines.Add(Line.Left(Split).TrimEnd());
			Line = Line.Mid(Split).TrimStart();
		}
	}
	if (!Line.IsEmpty()) { Lines.Add(Line); }
	if (MaxLines > 0 && Lines.Num() > MaxLines)
	{
		Lines.SetNum(MaxLines);
		FString& Last = Lines.Last();
		while (!Last.IsEmpty() && Measure(Last + TEXT("...")) > Width) { Last.LeftChopInline(1); }
		Last += TEXT("...");
	}
	return Lines;
}

float AShiftHUD::DrawObjectivePanel(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player)
{
	ObjectiveTaskRows = 0;
	struct FRow { TArray<FString> Lines; FLinearColor Color; float Scale; };
	TArray<FRow> Rows;
	const float Width = FMath::Min(420.f, Canvas->ClipX * 0.34f);
	const float X = Canvas->ClipX - Width - 24.f;
	const FLinearColor Accent(1.f, 0.85f, 0.35f), Normal(0.86f, 0.93f, 0.90f), Dim(0.62f, 0.76f, 0.71f);
	const auto Add = [this, &Rows, Width](const FString& Text, FLinearColor Color, float Scale = 0.95f) {
		Rows.Add({WrapText(Text, Width - 28.f, Scale), Color, Scale});
	};
	Add(TEXT("CURRENT OBJECTIVE"), FLinearColor(0.75f, 0.92f, 0.86f), 1.f);
	if (const AShiftCustomer* C = GM->ActiveCustomer.Get())
	{
		if (C->IsMonsterActive())
		{
			Add(TEXT("!! ABNORMAL ENTITY IN STORE !!"), FLinearColor(1.f, 0.30f, 0.26f));
			Add(ShiftLoc::Format(TEXT("Demand: Find [%s] (Red Glow)"), {ShiftData::ProductLabel(C->Spec.MonsterSku)}), Accent);
			Add(TEXT("-> Deliver it to the Checkout Register!"), Normal);
			Add(TEXT("Shelves swapped stock! Corridor Breaker [E] restores light 10s."), Dim, 0.88f);
		}
		else if (C->State == ECustomerState::WaitingForHelp)
		{
			Add(ShiftLoc::Format(TEXT("* Customer Help: Bring [%s]"), {ShiftData::ProductLabel(C->Spec.WantedSku)}), Accent);
			Add(TEXT("Take the product from a shelf, then press [E] on the customer."), Normal);
			if (C->Spec.bMonster && !C->Spec.AnomalyHint.IsEmpty())
			{
				Add(TEXT("Suspect disguise? Hand their whispered true item to ward off!"), FLinearColor(1.f, 0.62f, 0.35f), 0.88f);
			}
		}
		else if (C->State == ECustomerState::WaitingAtCheckout) { Add(TEXT("* Customer waiting at Checkout Register [E]"), Accent); }
		else if (C->State == ECustomerState::Leaving) { Add(TEXT("Customer is leaving. Keep the doorway clear."), Normal); }
		else { Add(TEXT("* Customer in store (Watch them closely)"), Normal); }
		for (const TWeakObjectPtr<AShiftTaskTarget>& T : GM->ActiveTasks)
		{
			if (T.IsValid() && T->Type == EShiftTaskType::FixAnomaly)
			{
				Add(ShiftLoc::Format(TEXT("! ANOMALY: %s (%ds)"), {ShiftLoc::Text(T->Label), LexToString(FMath::CeilToInt(T->TimeLeft))}), FLinearColor(1.f, 0.45f, 0.30f), 0.9f);
			}
		}
	}
	else
	{
		FShiftTaskGuidance Guide;
		if (GM->GetTaskGuidance(Player, Guide) && Guide.Task.IsValid())
		{
			const auto Time = [](const AShiftTaskTarget* Task) {
				return Task->IsOverdue() ? ShiftLoc::Text(TEXT("OVERDUE! (-Sanity)")) :
					ShiftLoc::Format(TEXT("%ds"), {LexToString(FMath::CeilToInt(Task->TimeLeft))});
			};
			Add(Guide.TaskLabel, Accent, 1.05f);
			++ObjectiveTaskRows;
			Add(ShiftLoc::Format(TEXT("Time left: %s"), {Time(Guide.Task.Get())}), Guide.Task->IsOverdue() ? FLinearColor(1.f, 0.35f, 0.30f) : Normal);
			const TCHAR* Step = TEXT("Look at the task marker and HOLD [E] to work.");
			switch (Guide.Action)
			{
			case EShiftGuideAction::CollectCarton: Step = TEXT("1/2  Warehouse: press [E] to take a stock carton."); break;
			case EShiftGuideAction::DeliverCarton: Step = TEXT("2/2  Return to the marked shelf; HOLD [E] to restock."); break;
			case EShiftGuideAction::CollectMop: Step = TEXT("1/2  Follow the marker; press [E] to take the mop."); break;
			case EShiftGuideAction::MopFloor: Step = TEXT("2/2  Return to the marked spill; HOLD [E] to mop."); break;
			case EShiftGuideAction::CookOden: Step = TEXT("At the oden station, HOLD [E] to prepare the broth."); break;
			case EShiftGuideAction::FixAnomaly: Step = TEXT("Follow the yellow marker; HOLD [E] to reset the store anomaly."); break;
			default: break;
			}
			Add(Step, Normal);
			if (GM->GuideRenderer && GM->GuideRenderer->bActive && !GM->GuideRenderer->IsReplanning() && GM->GuideRenderer->Status == EShiftRouteStatus::Unreachable)
			{
				Add(TEXT("Route blocked. Return to an aisle."), Accent);
			}
			int32 Overdue = 0;
			for (const TWeakObjectPtr<AShiftTaskTarget>& T : GM->ActiveTasks)
			{
				if (!T.IsValid()) { continue; }
				if (T->IsOverdue()) { ++Overdue; }
				if (T != Guide.Task)
				{
					++ObjectiveTaskRows;
					Add(ShiftLoc::Format(TEXT("%s · %s"), {T->Label, Time(T.Get())}), T->IsOverdue() ? FLinearColor(1.f, 0.35f, 0.30f) : Dim, 0.9f);
				}
			}
			if (Overdue > 0) { Add(ShiftLoc::Format(TEXT("%d overdue chores are draining sanity!"), {LexToString(Overdue)}), FLinearColor(1.f, 0.30f, 0.25f)); }
			if (GM->ActiveTasks.Num() > 1) { Add(TEXT("[TAB] Next chore"), Dim, 0.9f); }
		}
		else if (const int32 Loose = GM->Phase == EShiftPhase::Cleanup ? GM->CountLooseMerchandise() : 0; Loose > 0)
		{
			Add(ShiftLoc::Format(TEXT("Tidy up: %d items left on the floor"), {LexToString(Loose)}), Accent, 1.05f);
			Add(TEXT("Pick up loose merchandise and press [E] at any shelf to put it back."), Normal);
		}
		else { Add(TEXT("Waiting for the next customer."), Dim); }
	}
	float Height = 24.f;
	for (const FRow& Row : Rows) { Height += Row.Lines.Num() * 27.f * Row.Scale + 10.f; }
	bObjectiveFitsViewport = Height + 24.f < Canvas->ClipY - 100.f;
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.86f), X, 24.f, Width, Height);
	float Y = 36.f;
	for (const FRow& Row : Rows)
	{
		for (const FString& Line : Row.Lines)
		{
			DrawText(Line, Row.Color, X + 14.f, Y, InterfaceFont(), Row.Scale);
			Y += 27.f * Row.Scale;
		}
		Y += 10.f;
	}
	return Height;
}

void AShiftHUD::DrawSubtitle(AShiftStoreGameMode* GM)
{
	SubtitleLines = 0;
	if (GM->SubtitleTime <= 0.f || GM->SubtitleText.IsEmpty()) { return; }
	const TArray<FString> Lines = WrapText(GM->SubtitleText, Canvas->ClipX * 0.72f, 1.1f, 2);
	SubtitleLines = Lines.Num();
	const float Y = Canvas->ClipY - 58.f - Lines.Num() * 32.f;
	DrawRect(FLinearColor(0.015f, 0.025f, 0.025f, 0.85f), Canvas->ClipX * 0.12f, Y - 8.f, Canvas->ClipX * 0.76f, Lines.Num() * 32.f + 16.f);
	for (int32 Index = 0; Index < Lines.Num(); ++Index) { DrawCenteredText(Lines[Index], Y + Index * 32.f, FLinearColor(0.95f, 0.95f, 0.88f), 1.1f); }
}

void AShiftHUD::DrawTaskGuidance(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player)
{
	const AShiftGuideActor* Guide = GM->GuideRenderer;
	if (!Guide || !Player || !Guide->bActive || Guide->Status == EShiftRouteStatus::Unreachable) { return; }
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FVector Target = Guide->MarkerLocation() + FVector(0.f, 0.f, 50.f);
	const FVector Screen = Project(Target);
	FVector2D Position(Screen.X, Screen.Y);
	const bool bOnScreen = Screen.Z > 0.f && Position.X > 70.f && Position.X < W - 70.f && Position.Y > H * 0.32f && Position.Y < H - 135.f;
	FVector2D Direction(0.f, -1.f);
	if (!bOnScreen)
	{
		const FRotator CameraRotation = PlayerOwner->PlayerCameraManager->GetCameraRotation();
		const FVector Delta = Target - PlayerOwner->PlayerCameraManager->GetCameraLocation();
		const float Bearing = FMath::Atan2(FVector::DotProduct(Delta, CameraRotation.RotateVector(FVector::RightVector)),
			FVector::DotProduct(Delta, CameraRotation.Vector()));
		Direction = FVector2D(FMath::Sin(Bearing), -FMath::Cos(Bearing));
		const FVector2D Center(W * 0.5f, H * 0.58f), Extents(W * 0.5f - 86.f, H * 0.5f - 155.f);
		const float Distance = FMath::Min(Extents.X / FMath::Max(FMath::Abs(Direction.X), 0.001f), Extents.Y / FMath::Max(FMath::Abs(Direction.Y), 0.001f));
		Position = Center + Direction * Distance;
		// Keep the navigation marker above the subtitle strip.
		Position.Y = FMath::Min(Position.Y, H - 195.f);
	}
	else
	{
		FHitResult Occlusion;
		FCollisionQueryParams Params(TEXT("GuideMarkerView"), false, Player);
		Params.AddIgnoredActor(Guide->Current.Target.Get());
		if (GetWorld()->LineTraceSingleByChannel(Occlusion, Player->FirstPersonCamera->GetComponentLocation(), Target, ECC_Visibility, Params)) { return; }
	}
	const FVector2D Side(-Direction.Y, Direction.X);
	const FVector2D Tip = Position + Direction * 14.f;
	const FLinearColor Accent(1.f, 0.86f, 0.30f);
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 9.f + Side.X * 11.f, Position.Y - Direction.Y * 9.f + Side.Y * 11.f, Accent, 4.f);
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 9.f - Side.X * 11.f, Position.Y - Direction.Y * 9.f - Side.Y * 11.f, Accent, 4.f);
	const FShiftTaskGuidance& G = Guide->Current;
	const FString Name = Guide->Status != EShiftRouteStatus::Arrived ? TEXT("FOLLOW AISLE") :
		(G.Action == EShiftGuideAction::CollectCarton ? TEXT("WAREHOUSE CARTON") :
		(G.Action == EShiftGuideAction::CollectMop ? TEXT("MOP") : G.TaskLabel));
	const FString Label = ShiftLoc::Format(TEXT("%s · %dm"), {Name, LexToString(FMath::CeilToInt(FVector::Dist2D(Player->GetActorLocation(), Guide->MarkerLocation()) / 100.f))});
	float TW = 0.f, TH = 0.f;
	GetTextSize(Label, TW, TH, InterfaceFont(), 0.9f);
	const float X = FMath::Clamp(Position.X - TW * 0.5f, 12.f, W - TW - 12.f);
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.9f), X - 6.f, Position.Y + 18.f, TW + 12.f, TH + 6.f);
	DrawText(Label, Accent, X, Position.Y + 19.f, InterfaceFont(), 0.9f);
}
