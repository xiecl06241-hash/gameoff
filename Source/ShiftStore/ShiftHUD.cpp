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

namespace HorrorPalette
{
	static const FLinearColor Bone(0.89f, 0.86f, 0.79f, 1.f);
	static const FLinearColor Ash(0.58f, 0.55f, 0.50f, 1.f);
	static const FLinearColor Blood(0.76f, 0.14f, 0.12f, 1.f);
	static const FLinearColor BloodBright(0.92f, 0.22f, 0.18f, 1.f);
	static const FLinearColor Candle(0.85f, 0.68f, 0.36f, 1.f);
	static const FLinearColor Hairline(0.85f, 0.82f, 0.75f, 0.20f);
}

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

void AShiftHUD::DrawHorrorScrim(float X, float Y, float W, float H, FLinearColor AccentLine, float BgAlpha, bool bLeftStripe)
{
	// Soft dark ink wash that lets the 3D convenience store breathe behind the text
	DrawRect(FLinearColor(0.015f, 0.013f, 0.012f, BgAlpha), X, Y, W, H);
	DrawRect(FLinearColor(0.01f, 0.008f, 0.008f, BgAlpha * 0.45f), X - 4.f, Y - 3.f, W + 8.f, H + 6.f);
	if (bLeftStripe)
	{
		DrawRect(FLinearColor(AccentLine.R, AccentLine.G, AccentLine.B, 0.85f), X, Y + 3.f, 2.f, H - 6.f);
	}
	DrawRect(FLinearColor(AccentLine.R, AccentLine.G, AccentLine.B, 0.16f), X, Y, W, 1.f);
	DrawRect(FLinearColor(AccentLine.R, AccentLine.G, AccentLine.B, 0.16f), X, Y + H - 1.f, W, 1.f);
}

void AShiftHUD::DrawMemoCard(float X, float Y, float W, float H, bool bCorrupted)
{
	// Aged, stained dark-sepia employee memo taped to the bottom-left corner
	const FLinearColor PaperBg = bCorrupted
		? FLinearColor(0.08f, 0.018f, 0.016f, 0.86f)
		: FLinearColor(0.068f, 0.058f, 0.046f, 0.80f);
	const FLinearColor BorderCol = bCorrupted
		? FLinearColor(0.76f, 0.14f, 0.12f, 0.65f)
		: FLinearColor(0.72f, 0.66f, 0.54f, 0.28f);

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), X + 4.f, Y + 4.f, W, H);
	DrawRect(PaperBg, X, Y, W, H);
	// Subtle weathered border
	DrawRect(BorderCol, X, Y, W, 1.f);
	DrawRect(BorderCol, X, Y + H - 1.f, W, 1.f);
	DrawRect(BorderCol, X, Y, 1.f, H);
	DrawRect(BorderCol, X + W - 1.f, Y, 1.f, H);
	// Two translucent masking-tape strips holding the note at the top corners
	const FLinearColor Tape(0.76f, 0.71f, 0.58f, 0.22f);
	DrawRect(Tape, X + 14.f, Y - 5.f, 38.f, 10.f);
	DrawRect(Tape, X + W - 52.f, Y - 5.f, 38.f, 10.f);
	// Thin dried-blood rule under the memo heading
	DrawRect(FLinearColor(0.68f, 0.16f, 0.14f, 0.55f), X + 12.f, Y + 32.f, W - 24.f, 1.f);
}

void AShiftHUD::DrawThinBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back)
{
	const float Clamped = FMath::Clamp(Pct, 0.f, 1.f);
	DrawRect(Back, X, Y, W, H);
	if (Clamped > 0.005f)
	{
		DrawRect(Fill, X, Y, W * Clamped, H);
		// Subtle bright tip
		DrawRect(FLinearColor(1.f, 0.96f, 0.88f, 0.65f), FMath::Max(X, X + W * Clamped - 2.f), Y, 2.f, H);
	}
}

void AShiftHUD::DrawHorrorBackdrop(float W, float H, bool bBloodTint)
{
	// Stark psychological-horror dark cinema fade with subtle top/bottom letterbox vignette
	DrawRect(bBloodTint ? FLinearColor(0.032f, 0.008f, 0.008f, 0.94f) : FLinearColor(0.014f, 0.013f, 0.012f, 0.93f), 0.f, 0.f, W, H);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, W, H * 0.08f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, H * 0.92f, W, H * 0.08f);

	// Single eerie hair-line divider across the top section
	const FLinearColor LineCol = bBloodTint ? FLinearColor(0.76f, 0.14f, 0.12f, 0.55f) : FLinearColor(0.65f, 0.14f, 0.12f, 0.45f);
	DrawRect(LineCol, W * 0.20f, H * 0.242f, W * 0.60f, 1.f);
}

void AShiftHUD::DrawHorrorMenuItem(const FBox2D& Box, bool bSelected, bool bDanger)
{
	const float X = Box.Min.X, Y = Box.Min.Y;
	const float W = Box.GetSize().X, H = Box.GetSize().Y;
	if (bSelected)
	{
		const FLinearColor Wash = bDanger ? FLinearColor(0.32f, 0.04f, 0.04f, 0.55f) : FLinearColor(0.24f, 0.05f, 0.045f, 0.48f);
		const FLinearColor Bar = bDanger ? HorrorPalette::BloodBright : HorrorPalette::Blood;
		DrawRect(Wash, X, Y, W, H);
		DrawRect(Bar, X, Y + 4.f, 3.f, H - 8.f);
		DrawRect(FLinearColor(Bar.R, Bar.G, Bar.B, 0.35f), X, Y + H - 1.f, W, 1.f);
	}
	else
	{
		DrawRect(FLinearColor(0.04f, 0.036f, 0.034f, 0.32f), X, Y, W, H);
		DrawRect(FLinearColor(0.85f, 0.82f, 0.75f, 0.08f), X, Y + H - 1.f, W, 1.f);
	}
}

void AShiftHUD::DrawMouseSettings(const AShiftPlayerController* PC)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawCenteredText(TEXT("Mouse Sensitivity"), H * 0.16f, HorrorPalette::Bone, 1.75f);
	DrawCenteredText(TEXT("Lower is slower. Changes are saved automatically."), H * 0.29f, HorrorPalette::Ash, 0.95f);
	DrawCenteredText(ShiftLoc::Format(TEXT("Mouse sensitivity: %sx"), {FString::Printf(TEXT("%.2f"), PC->MouseSensitivity())}),
		H * 0.38f, PC->MouseSettingsSelection() == 0 ? HorrorPalette::Candle : HorrorPalette::Bone, 1.25f);

	const float Fraction = (PC->MouseSensitivity() - AShiftPlayerController::MinSensitivity) /
		(AShiftPlayerController::MaxSensitivity - AShiftPlayerController::MinSensitivity);
	const float TrackX = W * 0.30f;
	const float TrackY = H * 0.48f;
	const float TrackW = W * 0.40f;

	DrawThinBar(TrackX, TrackY, TrackW, 4.f, Fraction, HorrorPalette::BloodBright, FLinearColor(0.25f, 0.22f, 0.20f, 0.65f));
	const float ThumbX = TrackX + TrackW * Fraction;
	DrawRect(HorrorPalette::Bone, ThumbX - 2.f, TrackY - 9.f, 4.f, 22.f);

	DrawText(TEXT("0.10x"), HorrorPalette::Ash, TrackX, H * 0.535f, InterfaceFont(), 0.88f);
	DrawText(TEXT("3.00x"), HorrorPalette::Ash, W * 0.70f - 58.f, H * 0.535f, InterfaceFont(), 0.88f);

	const TCHAR* Labels[] = {TEXT("Restore Default"), TEXT("Back")};
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FBox2D Box = SensitivityButton(Index, W, H);
		const bool bSelected = PC->MouseSettingsSelection() == Index + 1;
		DrawHorrorMenuItem(Box, bSelected);
		DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f,
			bSelected ? HorrorPalette::Bone : HorrorPalette::Ash, 1.2f);
	}
	DrawCenteredText(TEXT("[Left / Right] Adjust    [Up / Down] Select    [Esc] Back"), H * 0.87f, HorrorPalette::Ash, 0.92f);
}

void AShiftHUD::DrawLocalizedText(const FString& Str, FLinearColor Color, float X, float Y, UFont* Font, float Scale, float MaxWidth)
{
	const FString Text = ShiftLoc::Text(Str);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	if (MaxWidth <= 0.f) { MaxWidth = X < Canvas->ClipX * 0.5f ? 392.f : Canvas->ClipX - X - 38.f; }
	if (TW > MaxWidth) { Scale *= MaxWidth / TW; }
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.85f), X + 1.5f, Y + 1.5f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AShiftHUD::DrawBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back)
{
	DrawThinBar(X, Y, W, H, Pct, Fill, Back);
}

void AShiftHUD::DrawCenteredText(const FString& Str, float Y, FLinearColor Color, float Scale)
{
	if (!Canvas) { return; }
	UFont* Font = InterfaceFont();
	const FString Text = ShiftLoc::Text(Str);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	const float MaxWidth = Canvas->ClipX * 0.86f;
	if (TW > MaxWidth) { Scale *= MaxWidth / TW; TW = MaxWidth; }
	const float X = (Canvas->ClipX - TW) * 0.5f;
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.88f), X + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
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
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// --- Pause Menu Overlay ---
	if (const AShiftPlayerController* PC = Cast<AShiftPlayerController>(PlayerOwner); PC && PC->IsPauseMenuOpen())
	{
		DrawHorrorBackdrop(W, H, false);
		if (PC->IsMouseSettingsOpen()) { DrawMouseSettings(PC); return; }

		DrawCenteredText(TEXT("PAUSED"), H * 0.135f, HorrorPalette::Bone, 1.95f);
		const TCHAR* Labels[] = {TEXT("Resume Game"), TEXT("Mouse Sensitivity"), TEXT("Return to Main Menu"), TEXT("Quit Game")};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FBox2D Box = PauseButton(Index, W, H);
			const bool bSelected = PC->PauseSelection() == Index;
			DrawHorrorMenuItem(Box, bSelected, Index >= 2);
			DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f,
				bSelected ? HorrorPalette::Bone : HorrorPalette::Ash, 1.26f);
		}
		DrawCenteredText(TEXT("Only dawn checkpoints are saved. Returning to the menu discards this night's progress."),
			H * 0.77f, FLinearColor(0.74f, 0.62f, 0.44f), 0.92f);
		DrawCenteredText(TEXT("[Esc] Resume    [Up / Down] Select    [Enter / Click] Confirm"),
			H * 0.87f, HorrorPalette::Ash, 0.92f);
		return;
	}

	// --- Full-Screen Overlay Phases ---
	if (GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup)
	{
		DrawHorrorBackdrop(W, H, GM->Phase == EShiftPhase::GameOver);

		if (GM->Phase == EShiftPhase::LanguageSelection)
		{
			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.13f, HorrorPalette::Bone, 2.0f);
			DrawCenteredText(TEXT("Language / 言語 / 語言 / 语言"), H * 0.255f, HorrorPalette::Ash, 1.25f);
			for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index)
			{
				const FBox2D Box = LanguageButton(Index, W, H);
				const bool bSelected = GM->SelectedLanguage == Index;
				DrawHorrorMenuItem(Box, bSelected);
				UFont* NativeFont = InterfaceFont(Index);
				const FString Name = FString::Printf(TEXT("%d     %s"), Index + 1, *ShiftLoc::NativeName(Index));
				float TW = 0.f, TH = 0.f;
				GetTextSize(Name, TW, TH, NativeFont, 1.32f);
				DrawText(Name, bSelected ? HorrorPalette::Bone : HorrorPalette::Ash,
					Box.Min.X + 26.f, Box.Min.Y + (Box.GetSize().Y - TH) * 0.5f, NativeFont, 1.32f);
			}
			DrawCenteredText(TEXT("Confirm [Enter]"), H * 0.74f, HorrorPalette::Candle, 1.25f);
			DrawCenteredText(TEXT("[Up / Down] Select    [1 / 2 / 3 / 4] Language    [Enter] Confirm"), H * 0.84f, HorrorPalette::Ash, 0.92f);
		}
		else if (GM->Phase == EShiftPhase::Title)
		{
			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.135f, HorrorPalette::Bone, 2.55f);
			DrawCenteredText(TEXT("Convenience Store Night-Shift Horror — GitHub Game Off 2026 (Theme: SHIFT)"), H * 0.265f, HorrorPalette::Ash, 1.02f);

			DrawCenteredText(TEXT("Complete clerk chores before timers expire. Serve every customer."), H * 0.39f, FLinearColor(0.80f, 0.78f, 0.72f), 1.12f);
			DrawCenteredText(TEXT("When an entity reveals itself, the store SHIFTS into an abnormal state."), H * 0.45f, HorrorPalette::BloodBright, 1.15f);
			DrawCenteredText(TEXT("Find the item it demands and deliver it to the Checkout Register to restore the store."), H * 0.51f, FLinearColor(0.80f, 0.78f, 0.72f), 1.12f);
			DrawCenteredText(TEXT("Controls: [WASD] Move   [Mouse] Look   [E / LMB] Interact / Hold Chore   [F / RMB] Throw Item   [G] Drop"), H * 0.61f, HorrorPalette::Ash, 0.98f);

			const FBox2D StartBox(FVector2D(W * 0.30f, H * 0.695f), FVector2D(W * 0.70f, H * 0.76f));
			DrawHorrorMenuItem(StartBox, true);
			DrawCenteredText(TEXT("[SPACE / E] New Game — Night 1"), H * 0.712f, HorrorPalette::Bone, 1.32f);

			if (GM->bHasSavedProgress)
			{
				DrawCenteredText(ShiftLoc::Format(TEXT("[C] Continue Saved Game — %s"), {GM->CurrentNightSpec().Title}), H * 0.795f, HorrorPalette::Candle, 1.08f);
			}
			DrawCenteredText(TEXT("[L] Change language"), H * 0.88f, HorrorPalette::Ash, 0.92f);
		}
		else if (GM->Phase == EShiftPhase::NightIntro)
		{
			const FShiftNightSpec& N = GM->CurrentNightSpec();
			DrawCenteredText(N.Title, H * 0.155f, HorrorPalette::Bone, 2.2f);
			TArray<FString> Lines;
			ShiftLoc::Text(N.Intro).ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				DrawCenteredText(Lines[i], H * 0.36f + i * 36.f, HorrorPalette::Bone, 1.18f);
			}
			DrawCenteredText(ShiftLoc::Format(TEXT("Customers Expected Tonight: %d   |   Auto-Save Active"), {LexToString(N.Customers.Num())}), H * 0.55f, HorrorPalette::Ash, 1.05f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Chore time limit: %ds. Timers are paused on this screen."), {LexToString(FMath::RoundToInt(N.TaskTimeLimit))}), H * 0.615f, HorrorPalette::Ash, 0.98f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] to Begin Night"), H * 0.73f, HorrorPalette::Candle, 1.32f);
		}
		else if (GM->Phase == EShiftPhase::Dawn)
		{
			DrawCenteredText(TEXT("06:00 AM — DAWN"), H * 0.16f, HorrorPalette::Candle, 2.2f);
			DrawCenteredText(ShiftLoc::Format(TEXT("%s COMPLETE — Store Handed Over to Day Shift"), {FString(GM->CurrentNightSpec().Title)}), H * 0.35f, HorrorPalette::Bone, 1.28f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Sales Revenue: $%.2f   |   Merchandise Losses: -$%.2f   |   Progress Saved"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses)}), H * 0.47f, HorrorPalette::Ash, 1.15f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] for Next Night"), H * 0.72f, HorrorPalette::Candle, 1.32f);
		}
		else if (GM->Phase == EShiftPhase::GameOver)
		{
			DrawCenteredText(TEXT("SHIFT TERMINATED"), H * 0.16f, HorrorPalette::BloodBright, 2.3f);
			DrawCenteredText(GM->GameOverReason, H * 0.41f, HorrorPalette::Bone, 1.28f);
			DrawCenteredText(TEXT("Press [SPACE], [E], or [R] to Rewind to Start of Current Night"), H * 0.71f, HorrorPalette::Candle, 1.28f);
		}
		else if (GM->Phase == EShiftPhase::Ending)
		{
			DrawCenteredText(GM->IsTrueEnding() ? TEXT("TRUE ENDING — MASTER OF THE NIGHT SHIFT") : TEXT("SURVIVOR ENDING — 06:00 AM HANDOVER"),
				H * 0.155f, GM->IsTrueEnding() ? HorrorPalette::Candle : HorrorPalette::Bone, 2.0f);
			const FString EndingBody = GM->IsTrueEnding()
				? ShiftLoc::Text(TEXT("You saw through every disguise before the store could fully shift.\nThe day-shift clerk left a note on the register:\n\"You understand the rules now. The store is yours.\""))
				: ShiftLoc::Text(TEXT("Three nights of shifting aisles and blackouts are finally over.\nSunlight hits the front glass. You drop your apron on the counter\nand walk out before the morning door chime rings."));
			TArray<FString> EndingLines;
			EndingBody.ParseIntoArrayLines(EndingLines);
			for (int32 Line = 0; Line < EndingLines.Num(); ++Line)
			{
				DrawCenteredText(EndingLines[Line], H * (0.31f + Line * 0.052f), HorrorPalette::Bone, 1.14f);
			}
			DrawCenteredText(ShiftLoc::Format(TEXT("Final Revenue: $%.2f   |   Total Losses: -$%.2f   |   Net: $%.2f"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses), FString::Printf(TEXT("%.2f"), GM->Income - GM->Losses)}), H * 0.51f, HorrorPalette::Candle, 1.22f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Disguises Warded Off: %d   |   Store Shifts Survived: %d"), {LexToString(GM->TotalMonstersWarded), LexToString(GM->TotalShiftsTriggered)}), H * 0.585f, HorrorPalette::Ash, 1.05f);
			DrawCenteredText(TEXT("Thank you for playing SHIFT / 24! Press [SPACE] to Play Again."), H * 0.73f, HorrorPalette::Bone, 1.28f);
		}
		return;
	}

	// ============================================================================================
	// IN-GAME PLAY HUD: Minimalist Psychological Horror (Zero Sci-Fi Clutter)
	// ============================================================================================
	const bool bShifted = GM->Director && GM->Director->IsShifted();
	const FLinearColor Accent = bShifted ? HorrorPalette::BloodBright : HorrorPalette::Bone;

	// --- 1. Top-Left Minimalist Bio & Ledger Strip ---
	DrawHorrorScrim(24.f, 24.f, 380.f, 128.f, bShifted ? HorrorPalette::Blood : FLinearColor(0.72f, 0.68f, 0.60f), bShifted ? 0.68f : 0.48f, true);
	DrawLocalizedText(ShiftLoc::Format(TEXT("%s  |  Customers: %d / %d  |  %s"), {FString(GM->CurrentNightSpec().Title), LexToString(GM->CustomersServed), LexToString(GM->CurrentNightSpec().Customers.Num()), FString(bShifted ? TEXT("STATE: SHIFTED [ABNORMAL]") : TEXT("STATE: NORMAL"))}),
		bShifted ? HorrorPalette::BloodBright : HorrorPalette::Bone, 36.f, 32.f, Font, 0.92f, 356.f);

	// Razor-thin Sanity bar
	const float SanityPct = GM->Sanity / ShiftData::MaxSanity;
	const FLinearColor SanityCol = (SanityPct < 0.35f || bShifted) ? HorrorPalette::BloodBright : FLinearColor(0.84f, 0.80f, 0.72f, 0.92f);
	DrawLocalizedText(ShiftLoc::Format(TEXT("SANITY: %d%%"), {LexToString(FMath::RoundToInt(GM->Sanity))}),
		SanityPct < 0.35f ? HorrorPalette::BloodBright : HorrorPalette::Ash, 36.f, 66.f, Font, 0.88f, 130.f);
	DrawThinBar(174.f, 77.f, 214.f, 5.f, SanityPct, SanityCol, FLinearColor(0.20f, 0.18f, 0.16f, 0.55f));

	// Store ledger & razor-thin loss indicator
	const float LossPct = GM->Losses / ShiftData::LossLimit;
	DrawLocalizedText(ShiftLoc::Format(TEXT("SALES: +$%.1f   LOSS: -$%.1f / -$%.0f"), {FString::Printf(TEXT("%.1f"), GM->Income), FString::Printf(TEXT("%.1f"), GM->Losses), FString::Printf(TEXT("%.0f"), ShiftData::LossLimit)}),
		LossPct > 0.7f ? HorrorPalette::BloodBright : HorrorPalette::Ash, 36.f, 98.f, Font, 0.85f, 356.f);
	DrawThinBar(36.f, 130.f, 352.f, 3.f, LossPct, HorrorPalette::Blood, FLinearColor(0.18f, 0.16f, 0.15f, 0.45f));

	// Held item & razor-thin Sprint Stamina strip
	if (Player)
	{
		DrawHorrorScrim(24.f, 160.f, 380.f, 64.f, Player->HeldItem.IsValid() ? HorrorPalette::Candle : FLinearColor(0.55f, 0.52f, 0.48f), 0.44f, true);
		const FString HeldStr = Player->HeldItem.IsValid()
			? ShiftLoc::Format(TEXT("CARRYING: %s  ([F/RMB] Throw  [G] Drop)"), {FString(Player->HeldItem->DisplayName())})
			: TEXT("HANDS: Empty  ([E] Pick up / Interact)");
		DrawLocalizedText(HeldStr, Player->HeldItem.IsValid() ? HorrorPalette::Candle : HorrorPalette::Ash, 36.f, 166.f, Font, 0.84f, 356.f);

		const float StamPct = FMath::Clamp(Player->Stamina / 100.f, 0.f, 1.f);
		DrawLocalizedText(TEXT("SPRINT [Shift]"), HorrorPalette::Ash, 36.f, 196.f, Font, 0.78f, 128.f);
		DrawThinBar(174.f, 205.f, 214.f, 3.f, StamPct,
			StamPct < 0.25f ? HorrorPalette::Blood : FLinearColor(0.82f, 0.79f, 0.72f, 0.82f),
			FLinearColor(0.18f, 0.16f, 0.15f, 0.45f));
	}

	// --- 2. Bottom-Left Aged Paper Employee Memo (《夜班异常守则》) ---
	{
		const float RuleY = H - 146.f;
		const float RuleW = 436.f;
		const float RuleH = 122.f;
		const bool bCorrupted = GM->NightIndex >= 2 && (GM->HasActiveAnomaly() || bShifted);

		DrawMemoCard(24.f, RuleY, RuleW, RuleH, bCorrupted);
		if (bCorrupted)
		{
			const FLinearColor WarnCol(0.92f, 0.20f + 0.12f * FMath::Sin(Now * 14.f), 0.18f, 0.95f);
			DrawLocalizedText(TEXT("[! SIGNAL CORRUPTED — FIX ANOMALY !]"), WarnCol, 34.f, RuleY - 24.f, Font, 0.78f, RuleW - 20.f);
		}
		DrawLocalizedText(TEXT("SHIFT RULES — Observe Customer Before Handing:"), bCorrupted ? HorrorPalette::BloodBright : HorrorPalette::Candle, 36.f, RuleY + 8.f, Font, 0.82f, RuleW - 24.f);
		DrawLocalizedText(TEXT("1. Tilted head / stares wall -> Hand [PANTRY Tomato Soup]"), HorrorPalette::Bone, 36.f, RuleY + 38.f, Font, 0.79f, RuleW - 24.f);
		DrawLocalizedText(TEXT("2. Red eye glare & light flicker -> Hand [DAILY Whole Milk]"), HorrorPalette::Bone, 36.f, RuleY + 65.f, Font, 0.79f, RuleW - 24.f);
		DrawLocalizedText(TEXT("3. Feet floating off floor -> Hand [MORNING Bread]"), HorrorPalette::Bone, 36.f, RuleY + 92.f, Font, 0.79f, RuleW - 24.f);
	}

	// --- 3. Proximity Heartbeat / Jumpscare Organic Dark-Blood Vignette ---
	if (GM->HeartbeatPulseAlpha > 0.02f)
	{
		const float A = FMath::Clamp(GM->HeartbeatPulseAlpha * 0.36f, 0.f, 0.40f);
		for (int32 Layer = 0; Layer < 4; ++Layer)
		{
			const float Band = 14.f + Layer * 16.f;
			const FLinearColor PulseCol(0.55f, 0.02f, 0.02f, A * (1.f - Layer * 0.24f));
			DrawRect(PulseCol, 0.f, 0.f, W, Band);
			DrawRect(PulseCol, 0.f, H - Band, W, Band);
			DrawRect(PulseCol, 0.f, Band, Band, H - Band * 2.f);
			DrawRect(PulseCol, W - Band, Band, Band, H - Band * 2.f);
		}
	}

	// --- 4. Top-Right Minimalist Checklist ---
	const float ChecklistHeight = DrawObjectivePanel(GM, Player);

	// --- 5. Center-Top Unsettling Alert Banner ---
	if (GM->BannerTimer > 0.f && !GM->BannerText.IsEmpty())
	{
		const float BannerY = FMath::Max(242.f, FMath::Max(36.f + ChecklistHeight, H * 0.24f));
		const FString BannerStr = ShiftLoc::Text(GM->BannerText);
		float BTW = 0.f, BTH = 0.f;
		GetTextSize(BannerStr, BTW, BTH, Font, 1.08f);
		const float BannerW = FMath::Clamp(BTW + 56.f, 320.f, W * 0.76f);
		const float BannerX = (W - BannerW) * 0.5f;
		DrawHorrorScrim(BannerX, BannerY, BannerW, 40.f, bShifted ? HorrorPalette::BloodBright : HorrorPalette::Candle, 0.72f, true);
		DrawCenteredText(GM->BannerText, BannerY + 8.f, bShifted ? HorrorPalette::BloodBright : HorrorPalette::Bone, 1.08f);
	}

	// --- 6. 3D World Customer Speech Callout ---
	if (AShiftCustomer* Customer = GM->ActiveCustomer.Get(); Customer && Customer->State != ECustomerState::MonsterGrabbing && Customer->Speech && !Customer->Speech->Text.IsEmpty())
	{
		const FVector Head = Customer->Speech->GetComponentLocation();
		if (Player && FVector::DistSquared(Player->GetActorLocation(), Head) < FMath::Square(1200.f) && PlayerOwner->LineOfSightTo(Customer))
		{
			const FVector Screen = Project(Head);
			if (Screen.Z > 0.f && Screen.X > 0.f && Screen.X < W && Screen.Y < H * 0.90f)
			{
				const FString Speech = ShiftLoc::Text(Customer->Speech->Text.ToString());
				const TArray<FString> Lines = WrapText(Speech, W * 0.65f, 0.94f, 2);
				float TW = 0.f;
				for (const FString& Line : Lines)
				{
					float LW = 0.f, LH = 0.f;
					GetTextSize(Line, LW, LH, Font, 0.94f);
					TW = FMath::Max(TW, LW);
				}
				const float X = FMath::Clamp(Screen.X - TW * 0.5f, 12.f, W - TW - 12.f);
				float Y = FMath::Max(Screen.Y, H * 0.35f);
				if (X < 420.f) { Y = FMath::Max(Y, 236.f); }
				if (X + TW > W - 455.f) { Y = FMath::Max(Y, ChecklistHeight + 42.f); }
				Y = FMath::Min(Y, H * 0.72f);

				DrawHorrorScrim(X - 10.f, Y - 5.f, TW + 20.f, Lines.Num() * 28.f + 10.f,
					Customer->IsMonsterActive() ? HorrorPalette::BloodBright : FLinearColor(0.75f, 0.72f, 0.65f), 0.70f, true);
				for (int32 Index = 0; Index < Lines.Num(); ++Index)
				{
					DrawText(Lines[Index], FLinearColor(Customer->Speech->TextRenderColor), X, Y + Index * 28.f, Font, 0.94f);
				}
			}
		}
	}

	// --- 7. Minimalist Horror Reticle & Clean Floating Interaction Prompt ---
	const float CX = W * 0.5f;
	const float CY = H * 0.5f;
	const bool bHasPrompt = Player && !Player->CrosshairPrompt.IsEmpty();
	if (bHasPrompt)
	{
		const FLinearColor DotCol = Player->IsGrabbed() ? HorrorPalette::BloodBright : HorrorPalette::Bone;
		DrawRect(DotCol, CX - 2.f, CY - 2.f, 4.f, 4.f);
		DrawCenteredText(Player->CrosshairPrompt, CY + 24.f, Player->IsGrabbed() ? HorrorPalette::BloodBright : HorrorPalette::Bone, 1.05f);
		const float BarProg = Player->IsGrabbed() ? Player->StruggleProgress : Player->HoldProgress;
		if (BarProg > 0.01f)
		{
			DrawThinBar(CX - 90.f, CY + 58.f, 180.f, 4.f, BarProg,
				Player->IsGrabbed() ? HorrorPalette::BloodBright : HorrorPalette::Candle,
				FLinearColor(0.15f, 0.14f, 0.13f, 0.75f));
		}
	}
	else
	{
		// Quiet, unobtrusive 3px bone-white dot so the 3D horror store takes center stage
		DrawRect(FLinearColor(0.88f, 0.85f, 0.78f, 0.55f), CX - 1.5f, CY - 1.5f, 3.f, 3.f);
	}

	DrawTaskGuidance(GM, Player);
	DrawSubtitle(GM);
	DrawLocalizedText(TEXT("[Esc] Pause"), FLinearColor(0.68f, 0.65f, 0.58f, 0.75f), W - 135.f, H - 36.f, Font, 0.82f, 120.f);
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
	const float Width = FMath::Min(396.f, Canvas->ClipX * 0.33f);
	const float X = Canvas->ClipX - Width - 24.f;
	const FLinearColor Accent = HorrorPalette::Candle;
	const FLinearColor Normal = HorrorPalette::Bone;
	const FLinearColor Dim = HorrorPalette::Ash;
	const auto Add = [this, &Rows, Width](const FString& Text, FLinearColor Color, float Scale = 0.92f) {
		Rows.Add({WrapText(Text, Width - 26.f, Scale), Color, Scale});
	};

	bool bEntityAlert = false;
	Add(TEXT("CURRENT OBJECTIVE"), HorrorPalette::Ash, 0.88f);
	if (const AShiftCustomer* C = GM->ActiveCustomer.Get())
	{
		if (C->IsMonsterActive())
		{
			bEntityAlert = true;
			Add(TEXT("!! ABNORMAL ENTITY IN STORE !!"), HorrorPalette::BloodBright, 0.96f);
			Add(ShiftLoc::Format(TEXT("Demand: Find [%s] (Red Glow)"), {ShiftData::ProductLabel(C->Spec.MonsterSku)}), Accent);
			Add(TEXT("-> Deliver it to the Checkout Register!"), Normal);
			Add(TEXT("Shelves swapped stock! Corridor Breaker [E] restores light 10s."), Dim, 0.84f);
		}
		else if (C->State == ECustomerState::WaitingForHelp)
		{
			Add(ShiftLoc::Format(TEXT("* Customer Help: Bring [%s]"), {ShiftData::ProductLabel(C->Spec.WantedSku)}), Accent);
			Add(TEXT("Take the product from a shelf, then press [E] on the customer."), Normal);
			Add(TEXT("Check SHIFT RULES (Bottom-Left): Observe posture before handing!"), FLinearColor(0.86f, 0.56f, 0.34f), 0.84f);
		}
		else if (C->State == ECustomerState::WaitingAtCheckout) { Add(TEXT("* Customer waiting at Checkout Register [E]"), Accent); }
		else if (C->State == ECustomerState::Leaving) { Add(TEXT("Customer is leaving. Keep the doorway clear."), Normal); }
		else { Add(TEXT("* Customer in store (Watch them closely)"), Normal); }
		for (const TWeakObjectPtr<AShiftTaskTarget>& T : GM->ActiveTasks)
		{
			if (T.IsValid() && T->Type == EShiftTaskType::FixAnomaly)
			{
				Add(ShiftLoc::Format(TEXT("! ANOMALY: %s (%ds)"), {ShiftLoc::Text(T->Label), LexToString(FMath::CeilToInt(T->TimeLeft))}), HorrorPalette::BloodBright, 0.88f);
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
			Add(Guide.TaskLabel, Accent, 1.0f);
			++ObjectiveTaskRows;
			Add(ShiftLoc::Format(TEXT("Time left: %s"), {Time(Guide.Task.Get())}), Guide.Task->IsOverdue() ? HorrorPalette::BloodBright : Normal);
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
					Add(ShiftLoc::Format(TEXT("%s · %s"), {T->Label, Time(T.Get())}), T->IsOverdue() ? HorrorPalette::BloodBright : Dim, 0.88f);
				}
			}
			if (Overdue > 0)
			{
				bEntityAlert = true;
				Add(ShiftLoc::Format(TEXT("%d overdue chores are draining sanity!"), {LexToString(Overdue)}), HorrorPalette::BloodBright);
			}
			if (GM->ActiveTasks.Num() > 1) { Add(TEXT("[TAB] Next chore"), Dim, 0.85f); }
		}
		else if (const int32 Loose = GM->Phase == EShiftPhase::Cleanup ? GM->CountLooseMerchandise() : 0; Loose > 0)
		{
			Add(ShiftLoc::Format(TEXT("Tidy up: %d items left on the floor"), {LexToString(Loose)}), Accent, 1.0f);
			Add(TEXT("Pick up loose merchandise and press [E] at any shelf to put it back."), Normal);
		}
		else { Add(TEXT("Waiting for the next customer."), Dim); }
	}

	float Height = 22.f;
	for (const FRow& Row : Rows) { Height += Row.Lines.Num() * 25.f * Row.Scale + 8.f; }
	bObjectiveFitsViewport = Height + 24.f < Canvas->ClipY - 100.f;

	DrawHorrorScrim(X, 24.f, Width, Height, bEntityAlert ? HorrorPalette::BloodBright : FLinearColor(0.72f, 0.68f, 0.60f), bEntityAlert ? 0.68f : 0.48f, true);
	float Y = 34.f;
	for (const FRow& Row : Rows)
	{
		for (const FString& Line : Row.Lines)
		{
			DrawText(Line, FLinearColor(0.f, 0.f, 0.f, 0.82f), X + 15.5f, Y + 1.5f, InterfaceFont(), Row.Scale);
			DrawText(Line, Row.Color, X + 14.f, Y, InterfaceFont(), Row.Scale);
			Y += 25.f * Row.Scale;
		}
		Y += 8.f;
	}
	return Height;
}

void AShiftHUD::DrawSubtitle(AShiftStoreGameMode* GM)
{
	SubtitleLines = 0;
	if (GM->SubtitleTime <= 0.f || GM->SubtitleText.IsEmpty()) { return; }
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float SafeLeft = 478.f;
	const float SafeRight = W - 155.f;
	const float AvailW = FMath::Max(320.f, SafeRight - SafeLeft);
	const TArray<FString> Lines = WrapText(GM->SubtitleText, AvailW - 24.f, 1.04f, 2);
	SubtitleLines = Lines.Num();

	UFont* Font = InterfaceFont();
	float MaxLW = 0.f;
	for (const FString& L : Lines)
	{
		float LW = 0.f, LH = 0.f;
		GetTextSize(L, LW, LH, Font, 1.04f);
		MaxLW = FMath::Max(MaxLW, LW);
	}
	const float CenterX = (W * 0.5f - MaxLW * 0.5f >= SafeLeft && W * 0.5f + MaxLW * 0.5f <= SafeRight)
		? W * 0.5f
		: (SafeLeft + SafeRight) * 0.5f;
	const float Y = H - 52.f - Lines.Num() * 30.f;
	// Clean cinema subtitles: soft translucent shadow strip without clunky boxes
	DrawRect(FLinearColor(0.01f, 0.01f, 0.01f, 0.55f), CenterX - MaxLW * 0.5f - 18.f, Y - 6.f, MaxLW + 36.f, Lines.Num() * 30.f + 12.f);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		float LW = 0.f, LH = 0.f;
		GetTextSize(Lines[Index], LW, LH, Font, 1.04f);
		const float LX = CenterX - LW * 0.5f;
		DrawText(Lines[Index], FLinearColor(0.f, 0.f, 0.f, 0.90f), LX + 1.8f, Y + Index * 30.f + 1.8f, Font, 1.04f);
		DrawText(Lines[Index], HorrorPalette::Bone, LX, Y + Index * 30.f, Font, 1.04f);
	}
}

void AShiftHUD::DrawTaskGuidance(AShiftStoreGameMode* GM, AShiftPlayerCharacter* Player)
{
	const AShiftGuideActor* Guide = GM->GuideRenderer;
	if (!Guide || !Player || !Guide->bActive || Guide->Status == EShiftRouteStatus::Unreachable) { return; }
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FVector Target = Guide->MarkerLocation() + FVector(0.f, 0.f, 50.f);
	const FVector Screen = Project(Target);
	FVector2D Position(Screen.X, Screen.Y);
	const bool bOnScreen = Screen.Z > 0.f && Position.X > 70.f && Position.X < W - 70.f && Position.Y > H * 0.32f && Position.Y < H - 170.f;
	FVector2D Direction(0.f, -1.f);
	if (!bOnScreen)
	{
		const FRotator CameraRotation = PlayerOwner->PlayerCameraManager->GetCameraRotation();
		const FVector Delta = Target - PlayerOwner->PlayerCameraManager->GetCameraLocation();
		const float Bearing = FMath::Atan2(FVector::DotProduct(Delta, CameraRotation.RotateVector(FVector::RightVector)),
			FVector::DotProduct(Delta, CameraRotation.Vector()));
		Direction = FVector2D(FMath::Sin(Bearing), -FMath::Cos(Bearing));
		const FVector2D Center(W * 0.5f, H * 0.54f), Extents(W * 0.5f - 86.f, H * 0.5f - 175.f);
		const float Distance = FMath::Min(Extents.X / FMath::Max(FMath::Abs(Direction.X), 0.001f), Extents.Y / FMath::Max(FMath::Abs(Direction.Y), 0.001f));
		Position = Center + Direction * Distance;
		const float MaxY = Position.X < 490.f ? H - 204.f : H - 185.f;
		Position.Y = FMath::Clamp(Position.Y, 250.f, MaxY);
	}
	else
	{
		FHitResult Occlusion;
		FCollisionQueryParams Params(TEXT("GuideMarkerView"), false, Player);
		Params.AddIgnoredActor(Guide->Current.Target.Get());
		if (GetWorld()->LineTraceSingleByChannel(Occlusion, Player->FirstPersonCamera->GetComponentLocation(), Target, ECC_Visibility, Params)) { return; }
	}

	const FVector2D Side(-Direction.Y, Direction.X);
	const FVector2D Tip = Position + Direction * 12.f;
	const FLinearColor Accent = HorrorPalette::Candle;
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 8.f + Side.X * 9.f, Position.Y - Direction.Y * 8.f + Side.Y * 9.f, Accent, 2.5f);
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 8.f - Side.X * 9.f, Position.Y - Direction.Y * 8.f - Side.Y * 9.f, Accent, 2.5f);

	const FShiftTaskGuidance& G = Guide->Current;
	const FString Name = Guide->Status != EShiftRouteStatus::Arrived ? TEXT("FOLLOW AISLE") :
		(G.Action == EShiftGuideAction::CollectCarton ? TEXT("WAREHOUSE CARTON") :
		(G.Action == EShiftGuideAction::CollectMop ? TEXT("MOP") : G.TaskLabel));
	const int32 Meters = FMath::Max(1, FMath::RoundToInt(FVector::Dist2D(Player->GetActorLocation(), G.Location) / 100.f));
	const FString Tag = FString::Printf(TEXT("%s · %dm"), *ShiftLoc::Text(Name), Meters);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Tag, TW, TH, InterfaceFont(), 0.82f);
	const float X = FMath::Clamp(Position.X - TW * 0.5f, 18.f, W - TW - 18.f);
	const float Y = FMath::Clamp(Position.Y + 16.f, 18.f, H - TH - 52.f);
	DrawRect(FLinearColor(0.01f, 0.01f, 0.01f, 0.55f), X - 6.f, Y - 2.f, TW + 12.f, TH + 4.f);
	DrawText(Tag, Accent, X, Y, InterfaceFont(), 0.82f);
}
