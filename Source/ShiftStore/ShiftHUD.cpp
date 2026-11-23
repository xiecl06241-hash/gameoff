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

namespace MinimalHorror
{
	static const FLinearColor Ivory(0.91f, 0.89f, 0.84f, 0.94f);
	static const FLinearColor SoftIvory(0.84f, 0.82f, 0.77f, 0.82f);
	static const FLinearColor Muted(0.62f, 0.60f, 0.56f, 0.78f);
	static const FLinearColor Crimson(0.86f, 0.19f, 0.16f, 0.96f);
	static const FLinearColor DriedBlood(0.68f, 0.13f, 0.11f, 0.88f);
	static const FLinearColor WarmAmber(0.88f, 0.73f, 0.44f, 0.95f);
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
	// Frameless whisper-soft gradient wash — never a solid box
	if (BgAlpha > 0.01f)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, BgAlpha), X, Y, W, H);
	}
	if (bLeftStripe)
	{
		DrawRect(FLinearColor(AccentLine.R, AccentLine.G, AccentLine.B, 0.55f), X, Y + 4.f, 1.5f, H - 8.f);
	}
}

void AShiftHUD::DrawMemoCard(float X, float Y, float W, float H, bool bCorrupted)
{
	// Frameless contextual note strip — blends seamlessly into the dark store floor
	const FLinearColor Accent = bCorrupted ? MinimalHorror::Crimson : MinimalHorror::DriedBlood;
	DrawRect(FLinearColor(0.f, 0.f, 0.f, bCorrupted ? 0.42f : 0.28f), X, Y, W, H);
	DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.75f), X, Y + 4.f, 2.f, H - 8.f);
}

void AShiftHUD::DrawThinBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back)
{
	const float Clamped = FMath::Clamp(Pct, 0.f, 1.f);
	DrawRect(Back, X, Y, W, H);
	if (Clamped > 0.005f)
	{
		DrawRect(Fill, X, Y, W * Clamped, H);
	}
}

void AShiftHUD::DrawHorrorBackdrop(float W, float H, bool bBloodTint)
{
	// Translucent dark veil so the live 3D dimly-lit convenience store remains atmospherically visible behind menus
	const float Alpha = bBloodTint ? 0.82f : 0.72f;
	DrawRect(bBloodTint ? FLinearColor(0.03f, 0.005f, 0.005f, Alpha) : FLinearColor(0.008f, 0.008f, 0.008f, Alpha), 0.f, 0.f, W, H);
	// Soft cinematic top/bottom letterbox shading
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.42f), 0.f, 0.f, W, H * 0.08f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.42f), 0.f, H * 0.92f, W, H * 0.08f);
	// Single whisper-thin crimson hairline below title
	DrawRect(FLinearColor(0.72f, 0.14f, 0.12f, 0.45f), W * 0.28f, H * 0.242f, W * 0.44f, 1.f);
}

void AShiftHUD::DrawHorrorMenuItem(const FBox2D& Box, bool bSelected, bool bDanger)
{
	if (!bSelected) { return; }
	const float X = Box.Min.X, Y = Box.Min.Y;
	const float W = Box.GetSize().X, H = Box.GetSize().Y;
	const FLinearColor Bar = bDanger ? MinimalHorror::Crimson : MinimalHorror::DriedBlood;
	// Clean, borderless warm-blood underline & subtle highlight wash
	DrawRect(FLinearColor(Bar.R, Bar.G, Bar.B, 0.16f), X, Y, W, H);
	DrawRect(Bar, X, Y + 6.f, 2.5f, H - 12.f);
	DrawRect(FLinearColor(Bar.R, Bar.G, Bar.B, 0.45f), X, Y + H - 1.f, W, 1.f);
}

void AShiftHUD::DrawMouseSettings(const AShiftPlayerController* PC)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	DrawCenteredText(TEXT("Mouse Sensitivity"), H * 0.14f, MinimalHorror::Ivory, 1.75f);
	DrawCenteredText(TEXT("Lower is slower. Changes are saved automatically."), H * 0.29f, MinimalHorror::Muted, 0.92f);
	DrawCenteredText(ShiftLoc::Format(TEXT("Mouse sensitivity: %sx"), {FString::Printf(TEXT("%.2f"), PC->MouseSensitivity())}),
		H * 0.38f, PC->MouseSettingsSelection() == 0 ? MinimalHorror::WarmAmber : MinimalHorror::Ivory, 1.25f);

	const float Fraction = (PC->MouseSensitivity() - AShiftPlayerController::MinSensitivity) /
		(AShiftPlayerController::MaxSensitivity - AShiftPlayerController::MinSensitivity);
	const float TrackX = W * 0.30f;
	const float TrackY = H * 0.48f;
	const float TrackW = W * 0.40f;

	DrawThinBar(TrackX, TrackY, TrackW, 3.f, Fraction, MinimalHorror::Crimson, FLinearColor(0.35f, 0.33f, 0.30f, 0.45f));
	const float ThumbX = TrackX + TrackW * Fraction;
	DrawRect(MinimalHorror::Ivory, ThumbX - 1.5f, TrackY - 8.f, 3.f, 19.f);

	DrawText(TEXT("0.10x"), MinimalHorror::Muted, TrackX, H * 0.535f, InterfaceFont(), 0.86f);
	DrawText(TEXT("3.00x"), MinimalHorror::Muted, W * 0.70f - 56.f, H * 0.535f, InterfaceFont(), 0.86f);

	const TCHAR* Labels[] = {TEXT("Restore Default"), TEXT("Back")};
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FBox2D Box = SensitivityButton(Index, W, H);
		const bool bSelected = PC->MouseSettingsSelection() == Index + 1;
		DrawHorrorMenuItem(Box, bSelected);
		DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f,
			bSelected ? MinimalHorror::Ivory : MinimalHorror::Muted, 1.18f);
	}
	DrawCenteredText(TEXT("[Left / Right] Adjust    [Up / Down] Select    [Esc] Back"), H * 0.87f, MinimalHorror::Muted, 0.88f);
}

void AShiftHUD::DrawLocalizedText(const FString& Str, FLinearColor Color, float X, float Y, UFont* Font, float Scale, float MaxWidth)
{
	const FString Text = ShiftLoc::Text(Str);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	if (MaxWidth <= 0.f) { MaxWidth = X < Canvas->ClipX * 0.5f ? 392.f : Canvas->ClipX - X - 38.f; }
	if (TW > MaxWidth) { Scale *= MaxWidth / TW; }
	// Double soft drop-shadow so frameless text reads crisply over any 3D lighting
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.85f), X + 1.5f, Y + 1.5f, Font, Scale);
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.45f), X - 0.5f, Y + 1.f, Font, Scale);
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
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.90f), X + 2.f, Y + 2.f, Font, Scale);
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

		DrawCenteredText(TEXT("PAUSED"), H * 0.135f, MinimalHorror::Ivory, 1.95f);
		const TCHAR* Labels[] = {TEXT("Resume Game"), TEXT("Mouse Sensitivity"), TEXT("Return to Main Menu"), TEXT("Quit Game")};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FBox2D Box = PauseButton(Index, W, H);
			const bool bSelected = PC->PauseSelection() == Index;
			DrawHorrorMenuItem(Box, bSelected, Index >= 2);
			DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f,
				bSelected ? MinimalHorror::Ivory : MinimalHorror::Muted, 1.24f);
		}
		DrawCenteredText(TEXT("Only dawn checkpoints are saved. Returning to the menu discards this night's progress."),
			H * 0.77f, MinimalHorror::Muted, 0.88f);
		DrawCenteredText(TEXT("[Esc] Resume    [Up / Down] Select    [Enter / Click] Confirm"),
			H * 0.87f, MinimalHorror::Muted, 0.88f);
		return;
	}

	// --- Full-Screen Overlay Phases ---
	if (GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup)
	{
		if (GM->Phase == EShiftPhase::Title)
		{
			// Let the live dimly-lit 3D horror store shine directly behind the Title Screen!
			DrawRect(FLinearColor(0.005f, 0.005f, 0.005f, 0.46f), 0.f, 0.f, W, H);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), 0.f, 0.f, W, H * 0.10f);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f), 0.f, H * 0.86f, W, H * 0.14f);
			DrawRect(FLinearColor(0.76f, 0.15f, 0.12f, 0.55f), W * 0.30f, H * 0.285f, W * 0.40f, 1.f);

			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.165f, MinimalHorror::Ivory, 2.75f);
			DrawCenteredText(TEXT("Convenience Store Night-Shift Horror — GitHub Game Off 2026 (Theme: SHIFT)"), H * 0.31f, MinimalHorror::SoftIvory, 0.96f);
			DrawCenteredText(TEXT("When an entity reveals itself, the store SHIFTS into an abnormal state."), H * 0.44f, MinimalHorror::Crimson, 1.12f);

			const FBox2D StartBox(FVector2D(W * 0.32f, H * 0.65f), FVector2D(W * 0.68f, H * 0.72f));
			DrawHorrorMenuItem(StartBox, true);
			DrawCenteredText(TEXT("[SPACE / E] New Game — Night 1"), H * 0.668f, MinimalHorror::Ivory, 1.32f);

			if (GM->bHasSavedProgress)
			{
				DrawCenteredText(ShiftLoc::Format(TEXT("[C] Continue Saved Game — %s"), {GM->CurrentNightSpec().Title}), H * 0.755f, MinimalHorror::WarmAmber, 1.05f);
			}
			DrawCenteredText(TEXT("Controls: [WASD] Move   [Mouse] Look   [E / LMB] Interact / Hold Chore   [F / RMB] Throw Item   [G] Drop"), H * 0.885f, MinimalHorror::Muted, 0.86f);
			DrawCenteredText(TEXT("[L] Change language"), H * 0.93f, MinimalHorror::Muted, 0.84f);
			return;
		}

		DrawHorrorBackdrop(W, H, GM->Phase == EShiftPhase::GameOver);

		if (GM->Phase == EShiftPhase::LanguageSelection)
		{
			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.135f, MinimalHorror::Ivory, 2.1f);
			DrawCenteredText(TEXT("Language / 言語 / 語言 / 语言"), H * 0.26f, MinimalHorror::Muted, 1.2f);
			for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index)
			{
				const FBox2D Box = LanguageButton(Index, W, H);
				const bool bSelected = GM->SelectedLanguage == Index;
				DrawHorrorMenuItem(Box, bSelected);
				UFont* NativeFont = InterfaceFont(Index);
				const FString Name = FString::Printf(TEXT("%d     %s"), Index + 1, *ShiftLoc::NativeName(Index));
				float TW = 0.f, TH = 0.f;
				GetTextSize(Name, TW, TH, NativeFont, 1.30f);
				DrawText(Name, bSelected ? MinimalHorror::Ivory : MinimalHorror::Muted,
					Box.Min.X + 26.f, Box.Min.Y + (Box.GetSize().Y - TH) * 0.5f, NativeFont, 1.30f);
			}
			DrawCenteredText(TEXT("Confirm [Enter]"), H * 0.74f, MinimalHorror::WarmAmber, 1.22f);
			DrawCenteredText(TEXT("[Up / Down] Select    [1 / 2 / 3 / 4] Language    [Enter] Confirm"), H * 0.84f, MinimalHorror::Muted, 0.88f);
		}
		else if (GM->Phase == EShiftPhase::NightIntro)
		{
			const FShiftNightSpec& N = GM->CurrentNightSpec();
			DrawCenteredText(N.Title, H * 0.145f, MinimalHorror::Ivory, 2.25f);
			TArray<FString> Lines;
			ShiftLoc::Text(N.Intro).ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				DrawCenteredText(Lines[i], H * 0.32f + i * 38.f, MinimalHorror::Ivory, 1.08f);
			}
			DrawCenteredText(ShiftLoc::Format(TEXT("Customers Expected Tonight: %d   |   Active Rules: %d"), {LexToString(N.Customers.Num()), LexToString(GM->ActiveRuleCount())}), H * 0.54f, MinimalHorror::WarmAmber, 0.98f);
			DrawCenteredText(TEXT("[WASD] Move   [E] Interact / Hand Item   [Tab] Shift Note   [R] Refuse   [Q] Glimpse Old Scene"), H * 0.61f, MinimalHorror::SoftIvory, 0.90f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Chore time limit: %ds. Timers are paused on this screen."), {LexToString(FMath::RoundToInt(N.TaskTimeLimit))}), H * 0.665f, MinimalHorror::Muted, 0.86f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] to Begin Night"), H * 0.76f, MinimalHorror::WarmAmber, 1.28f);
		}
		else if (GM->Phase == EShiftPhase::Dawn)
		{
			DrawCenteredText(TEXT("06:00 AM — DAWN"), H * 0.145f, MinimalHorror::WarmAmber, 2.2f);
			DrawCenteredText(ShiftLoc::Format(TEXT("%s COMPLETE — Store Handed Over to Day Shift"), {FString(GM->CurrentNightSpec().Title)}), H * 0.35f, MinimalHorror::Ivory, 1.25f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Sales Revenue: $%.2f   |   Merchandise Losses: -$%.2f   |   Progress Saved"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses)}), H * 0.47f, MinimalHorror::Muted, 1.12f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] for Next Night"), H * 0.72f, MinimalHorror::WarmAmber, 1.28f);
		}
		else if (GM->Phase == EShiftPhase::GameOver)
		{
			DrawCenteredText(TEXT("SHIFT TERMINATED"), H * 0.145f, MinimalHorror::Crimson, 2.3f);
			DrawCenteredText(GM->GameOverReason, H * 0.41f, MinimalHorror::Ivory, 1.25f);
			DrawCenteredText(TEXT("Press [SPACE], [E], or [R] to Rewind to Start of Current Night"), H * 0.71f, MinimalHorror::WarmAmber, 1.25f);
		}
		else if (GM->Phase == EShiftPhase::Ending)
		{
			DrawCenteredText(GM->IsTrueEnding() ? TEXT("TRUE ENDING — MASTER OF THE NIGHT SHIFT") : TEXT("SURVIVOR ENDING — 06:00 AM HANDOVER"),
				H * 0.145f, GM->IsTrueEnding() ? MinimalHorror::WarmAmber : MinimalHorror::Ivory, 2.0f);
			const FString EndingBody = GM->IsTrueEnding()
				? ShiftLoc::Text(TEXT("You saw through every disguise before the store could fully shift.\nThe day-shift clerk left a note on the register:\n\"You understand the rules now. The store is yours.\""))
				: ShiftLoc::Text(TEXT("Three nights of shifting aisles and blackouts are finally over.\nSunlight hits the front glass. You drop your apron on the counter\nand walk out before the morning door chime rings."));
			TArray<FString> EndingLines;
			EndingBody.ParseIntoArrayLines(EndingLines);
			for (int32 Line = 0; Line < EndingLines.Num(); ++Line)
			{
				DrawCenteredText(EndingLines[Line], H * (0.31f + Line * 0.052f), MinimalHorror::Ivory, 1.12f);
			}
			DrawCenteredText(ShiftLoc::Format(TEXT("Final Revenue: $%.2f   |   Total Losses: -$%.2f   |   Net: $%.2f"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses), FString::Printf(TEXT("%.2f"), GM->Income - GM->Losses)}), H * 0.51f, MinimalHorror::WarmAmber, 1.2f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Disguises Warded Off: %d   |   Store Shifts Survived: %d"), {LexToString(GM->TotalMonstersWarded), LexToString(GM->TotalShiftsTriggered)}), H * 0.585f, MinimalHorror::Muted, 1.02f);
			DrawCenteredText(TEXT("Thank you for playing SHIFT / 24! Press [SPACE] to Play Again."), H * 0.73f, MinimalHorror::Ivory, 1.24f);
		}
		return;
	}

	// ============================================================================================
	// IN-GAME PLAY HUD: Frameless, Diegetic, Contextual-Hide (90% Pure 3D Store!)
	// ============================================================================================
	const bool bShifted = GM->Director && GM->Director->IsShifted();

	// --- 1. Top-Left: Ultra-Clean Frameless Status Typography (NO Boxes!) ---
	DrawLocalizedText(ShiftLoc::Format(TEXT("%s  |  Customers: %d / %d  |  %s"), {FString(GM->CurrentNightSpec().Title), LexToString(GM->CustomersServed), LexToString(GM->CurrentNightSpec().Customers.Num()), FString(bShifted ? TEXT("STATE: SHIFTED [ABNORMAL]") : TEXT("STATE: NORMAL"))}),
		bShifted ? MinimalHorror::Crimson : MinimalHorror::SoftIvory, 28.f, 24.f, Font, 0.86f, 360.f);

	const float SanityPct = GM->Sanity / ShiftData::MaxSanity;
	const FLinearColor SanityCol = (SanityPct < 0.35f || bShifted) ? MinimalHorror::Crimson : FLinearColor(0.84f, 0.81f, 0.75f, 0.80f);
	DrawLocalizedText(ShiftLoc::Format(TEXT("SANITY: %d%%"), {LexToString(FMath::RoundToInt(GM->Sanity))}),
		SanityPct < 0.35f ? MinimalHorror::Crimson : MinimalHorror::Muted, 28.f, 52.f, Font, 0.80f, 120.f);
	DrawThinBar(150.f, 61.f, 150.f, 3.f, SanityPct, SanityCol, FLinearColor(0.25f, 0.24f, 0.22f, 0.35f));

	const float LossPct = GM->Losses / ShiftData::LossLimit;
	DrawLocalizedText(ShiftLoc::Format(TEXT("SALES: +$%.1f   LOSS: -$%.1f / -$%.0f"), {FString::Printf(TEXT("%.1f"), GM->Income), FString::Printf(TEXT("%.1f"), GM->Losses), FString::Printf(TEXT("%.0f"), ShiftData::LossLimit)}),
		LossPct > 0.7f ? MinimalHorror::Crimson : MinimalHorror::Muted, 28.f, 76.f, Font, 0.78f, 340.f);
	if (LossPct > 0.01f)
	{
		DrawThinBar(28.f, 100.f, 272.f, 2.f, LossPct, MinimalHorror::DriedBlood, FLinearColor(0.20f, 0.18f, 0.17f, 0.30f));
	}

	// Minimal [Q] Old Scene Glimpse & [Tab] Shift Note indicators
	if (GM->Director)
	{
		if (GM->Director->IsPlayerPhaseShifted())
		{
			DrawLocalizedText(TEXT("[Q] OLD SCENE ACTIVE (-Sanity)"), MinimalHorror::Crimson, 28.f, 104.f, Font, 0.76f, 320.f);
		}
		else if (GM->Director->PlayerPhaseCooldownRemaining() > 0.5f)
		{
			DrawLocalizedText(ShiftLoc::Format(TEXT("[Q] Glimpse Old Scene (%ds)"), {LexToString(FMath::CeilToInt(GM->Director->PlayerPhaseCooldownRemaining()))}), MinimalHorror::Muted, 28.f, 104.f, Font, 0.74f, 280.f);
		}
		else
		{
			DrawLocalizedText(TEXT("[Q] Glimpse Old Scene"), FLinearColor(0.74f, 0.72f, 0.66f, 0.68f), 28.f, 104.f, Font, 0.74f, 260.f);
		}
	}
	DrawLocalizedText(ShiftLoc::Format(TEXT("[Tab] Shift Note (%d)"), {LexToString(GM->ActiveRuleCount())}),
		GM->bRuleNoteOpen ? MinimalHorror::WarmAmber : FLinearColor(0.76f, 0.73f, 0.66f, 0.72f), 28.f, 126.f, Font, 0.74f, 260.f);

	// Auto-Hide Stamina: ONLY visible while actively sprinting or recovering (< 99%)
	if (Player && (Player->bIsSprinting || Player->Stamina < 99.f))
	{
		const float StamPct = FMath::Clamp(Player->Stamina / 100.f, 0.f, 1.f);
		DrawLocalizedText(TEXT("SPRINT [Shift]"), MinimalHorror::Muted, 28.f, 150.f, Font, 0.74f, 115.f);
		DrawThinBar(150.f, 158.f, 150.f, 2.5f, StamPct,
			StamPct < 0.25f ? MinimalHorror::Crimson : MinimalHorror::SoftIvory,
			FLinearColor(0.20f, 0.19f, 0.18f, 0.32f));
	}

	// Auto-Hide Held Item: ONLY visible when carrying something (clean bottom-center whisper above subtitle)
	if (Player && Player->HeldItem.IsValid())
	{
		const FString HeldStr = ShiftLoc::Format(TEXT("CARRYING: %s  ([F/RMB] Throw  [G] Drop)"), {FString(Player->HeldItem->DisplayName())});
		DrawCenteredText(HeldStr, H - 134.f, MinimalHorror::WarmAmber, 0.86f);
	}

	// First-night single-step onboarding whisper (non-intrusive, disappears after Night 1)
	if (const FString Hint = GM->GetOnboardingHint(Player); !Hint.IsEmpty() && GM->SubtitleTime <= 0.f && GM->RuleNoteRaiseAlpha < 0.25f)
	{
		DrawRect(FLinearColor(0.02f, 0.015f, 0.01f, 0.56f), W * 0.14f, H - 72.f, W * 0.72f, 34.f);
		DrawCenteredText(Hint, H - 66.f, MinimalHorror::WarmAmber, 0.88f);
	}

	// --- 3. Proximity Heartbeat / Jumpscare Organic Dark-Blood Vignette ---
	if (GM->HeartbeatPulseAlpha > 0.02f)
	{
		const float A = FMath::Clamp(GM->HeartbeatPulseAlpha * 0.34f, 0.f, 0.38f);
		for (int32 Layer = 0; Layer < 4; ++Layer)
		{
			const float Band = 14.f + Layer * 16.f;
			const FLinearColor PulseCol(0.50f, 0.02f, 0.02f, A * (1.f - Layer * 0.24f));
			DrawRect(PulseCol, 0.f, 0.f, W, Band);
			DrawRect(PulseCol, 0.f, H - Band, W, Band);
			DrawRect(PulseCol, 0.f, Band, Band, H - Band * 2.f);
			DrawRect(PulseCol, W - Band, Band, Band, H - Band * 2.f);
		}
	}

	// --- 4. Top-Right Frameless Objective Typography ---
	const float ChecklistHeight = DrawObjectivePanel(GM, Player);

	// --- 5. Center-Top Clean Alert Banner ---
	if (GM->BannerTimer > 0.f && !GM->BannerText.IsEmpty())
	{
		const float BannerY = FMath::Max(160.f, FMath::Max(32.f + ChecklistHeight, H * 0.21f));
		DrawCenteredText(GM->BannerText, BannerY, bShifted ? MinimalHorror::Crimson : MinimalHorror::WarmAmber, 1.04f);
	}

	// --- 6. 3D World Customer Speech Callout (Frameless Cinema Subtitle) ---
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
				const float X = FMath::Clamp(Screen.X - TW * 0.5f, 16.f, W - TW - 16.f);
				float Y = FMath::Max(Screen.Y, H * 0.35f);
				if (X < 360.f) { Y = FMath::Max(Y, 150.f); }
				if (X + TW > W - 410.f) { Y = FMath::Max(Y, ChecklistHeight + 36.f); }
				Y = FMath::Min(Y, H * 0.72f);

				DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.42f), X - 10.f, Y - 4.f, TW + 20.f, Lines.Num() * 28.f + 8.f);
				for (int32 Index = 0; Index < Lines.Num(); ++Index)
				{
					DrawText(Lines[Index], FLinearColor(0.f, 0.f, 0.f, 0.88f), X + 1.5f, Y + Index * 28.f + 1.5f, Font, 0.94f);
					DrawText(Lines[Index], FLinearColor(Customer->Speech->TextRenderColor), X, Y + Index * 28.f, Font, 0.94f);
				}
			}
		}
	}

	// --- 7. Whisper-Quiet Horror Reticle & Frameless Interaction Prompt ---
	const float CX = W * 0.5f;
	const float CY = H * 0.5f;
	const bool bNoteBlockingView = GM->RuleNoteRaiseAlpha > 0.45f;
	const bool bHasPrompt = !bNoteBlockingView && Player && !Player->CrosshairPrompt.IsEmpty();
	if (bHasPrompt)
	{
		const FLinearColor DotCol = Player->IsGrabbed() ? MinimalHorror::Crimson : MinimalHorror::Ivory;
		DrawRect(DotCol, CX - 2.f, CY - 2.f, 4.f, 4.f);
		DrawCenteredText(Player->CrosshairPrompt, CY + 24.f, Player->IsGrabbed() ? MinimalHorror::Crimson : MinimalHorror::Ivory, 1.02f);
		const float BarProg = Player->IsGrabbed() ? Player->StruggleProgress : Player->HoldProgress;
		if (BarProg > 0.01f)
		{
			DrawThinBar(CX - 85.f, CY + 56.f, 170.f, 3.f, BarProg,
				Player->IsGrabbed() ? MinimalHorror::Crimson : MinimalHorror::WarmAmber,
				FLinearColor(0.18f, 0.17f, 0.16f, 0.55f));
		}
	}
	else if (!bNoteBlockingView)
	{
		DrawRect(FLinearColor(0.90f, 0.88f, 0.82f, 0.42f), CX - 1.5f, CY - 1.5f, 3.f, 3.f);
	}

	DrawTaskGuidance(GM, Player);
	DrawSubtitle(GM);
	if (GM->RuleNoteRaiseAlpha > 0.01f)
	{
		DrawRuleNoteOverlay(GM);
	}
	DrawLocalizedText(TEXT("[Esc] Pause"), FLinearColor(0.65f, 0.63f, 0.58f, 0.52f), W - 118.f, H - 32.f, Font, 0.78f, 108.f);
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
	const float Width = FMath::Min(370.f, Canvas->ClipX * 0.31f);
	const float X = Canvas->ClipX - Width - 26.f;
	const FLinearColor Accent = MinimalHorror::WarmAmber;
	const FLinearColor Normal = MinimalHorror::SoftIvory;
	const FLinearColor Dim = MinimalHorror::Muted;
	const auto Add = [this, &Rows, Width](const FString& Text, FLinearColor Color, float Scale = 0.86f) {
		Rows.Add({WrapText(Text, Width - 18.f, Scale), Color, Scale});
	};

	bool bEntityAlert = false;
	Add(TEXT("CURRENT OBJECTIVE"), MinimalHorror::Muted, 0.80f);
	if (const AShiftCustomer* C = GM->ActiveCustomer.Get())
	{
		if (C->IsMonsterActive())
		{
			bEntityAlert = true;
			Add(TEXT("!! ABNORMAL ENTITY IN STORE !!"), MinimalHorror::Crimson, 0.92f);
			Add(ShiftLoc::Format(TEXT("Demand: Find [%s] (Red Glow)"), {ShiftData::ProductLabel(C->Spec.MonsterSku)}), Accent);
			Add(TEXT("-> Deliver it to the Checkout Register!"), Normal);
			Add(TEXT("Shelves swapped stock! Corridor Breaker [E] restores light 10s."), Dim, 0.80f);
		}
		else if (C->State == ECustomerState::WaitingForHelp)
		{
			Add(ShiftLoc::Format(TEXT("* Customer Help: Bring [%s]"), {ShiftData::ProductLabel(C->Spec.WantedSku)}), Accent);
			Add(TEXT("Take the product from a shelf, then press [E] on the customer."), Normal);
			Add(TEXT("Press [Tab] to check Shift Note — press [R] to refuse if abnormal!"), FLinearColor(0.88f, 0.58f, 0.34f), 0.80f);
		}
		else if (C->State == ECustomerState::WaitingAtCheckout) { Add(TEXT("* Customer waiting at Checkout Register [E]"), Accent); }
		else if (C->State == ECustomerState::Leaving) { Add(TEXT("Customer is leaving. Keep the doorway clear."), Normal); }
		else { Add(TEXT("* Customer in store (Watch them closely)"), Normal); }
		for (const TWeakObjectPtr<AShiftTaskTarget>& T : GM->ActiveTasks)
		{
			if (T.IsValid() && T->Type == EShiftTaskType::FixAnomaly)
			{
				Add(ShiftLoc::Format(TEXT("! ANOMALY: %s (%ds)"), {ShiftLoc::Text(T->Label), LexToString(FMath::CeilToInt(T->TimeLeft))}), MinimalHorror::Crimson, 0.84f);
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
			Add(Guide.TaskLabel, Accent, 0.96f);
			++ObjectiveTaskRows;
			Add(ShiftLoc::Format(TEXT("Time left: %s"), {Time(Guide.Task.Get())}), Guide.Task->IsOverdue() ? MinimalHorror::Crimson : Normal);
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
					Add(ShiftLoc::Format(TEXT("%s · %s"), {T->Label, Time(T.Get())}), T->IsOverdue() ? MinimalHorror::Crimson : Dim, 0.82f);
				}
			}
			if (Overdue > 0)
			{
				bEntityAlert = true;
				Add(ShiftLoc::Format(TEXT("%d overdue chores are draining sanity!"), {LexToString(Overdue)}), MinimalHorror::Crimson);
			}
			if (GM->ActiveTasks.Num() > 1) { Add(TEXT("[V] Next chore"), Dim, 0.80f); }
		}
		else if (const int32 Loose = GM->Phase == EShiftPhase::Cleanup ? GM->CountLooseMerchandise() : 0; Loose > 0)
		{
			Add(ShiftLoc::Format(TEXT("Tidy up: %d items left on the floor"), {LexToString(Loose)}), Accent, 0.96f);
			Add(TEXT("Pick up loose merchandise and press [E] at any shelf to put it back."), Normal);
		}
		else { Add(TEXT("Waiting for the next customer."), Dim); }
	}

	float Height = 16.f;
	for (const FRow& Row : Rows) { Height += Row.Lines.Num() * 23.f * Row.Scale + 6.f; }
	bObjectiveFitsViewport = Height + 24.f < Canvas->ClipY - 100.f;

	// Frameless: only a whisper-thin vertical left hairline (no solid box!)
	DrawHorrorScrim(X, 24.f, Width, Height, bEntityAlert ? MinimalHorror::Crimson : FLinearColor(0.80f, 0.77f, 0.70f), bEntityAlert ? 0.34f : 0.f, true);
	float Y = 26.f;
	for (const FRow& Row : Rows)
	{
		for (const FString& Line : Row.Lines)
		{
			DrawText(Line, FLinearColor(0.f, 0.f, 0.f, 0.90f), X + 13.5f, Y + 1.5f, InterfaceFont(), Row.Scale);
			DrawText(Line, Row.Color, X + 12.f, Y, InterfaceFont(), Row.Scale);
			Y += 23.f * Row.Scale;
		}
		Y += 6.f;
	}
	return Height;
}

void AShiftHUD::DrawSubtitle(AShiftStoreGameMode* GM)
{
	SubtitleLines = 0;
	if (GM->SubtitleTime <= 0.f || GM->SubtitleText.IsEmpty()) { return; }
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float SafeLeft = GM->ActiveCustomer.IsValid() ? 456.f : 120.f;
	const float SafeRight = W - 140.f;
	const float AvailW = FMath::Max(320.f, SafeRight - SafeLeft);
	const TArray<FString> Lines = WrapText(GM->SubtitleText, AvailW - 24.f, 1.02f, 2);
	SubtitleLines = Lines.Num();

	UFont* Font = InterfaceFont();
	float MaxLW = 0.f;
	for (const FString& L : Lines)
	{
		float LW = 0.f, LH = 0.f;
		GetTextSize(L, LW, LH, Font, 1.02f);
		MaxLW = FMath::Max(MaxLW, LW);
	}
	const float CenterX = (W * 0.5f - MaxLW * 0.5f >= SafeLeft && W * 0.5f + MaxLW * 0.5f <= SafeRight)
		? W * 0.5f
		: (SafeLeft + SafeRight) * 0.5f;
	const float Y = H - 48.f - Lines.Num() * 28.f;
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		float LW = 0.f, LH = 0.f;
		GetTextSize(Lines[Index], LW, LH, Font, 1.02f);
		const float LX = CenterX - LW * 0.5f;
		DrawText(Lines[Index], FLinearColor(0.f, 0.f, 0.f, 0.92f), LX + 1.8f, Y + Index * 28.f + 1.8f, Font, 1.02f);
		DrawText(Lines[Index], MinimalHorror::Ivory, LX, Y + Index * 28.f, Font, 1.02f);
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
	const bool bOnScreen = Screen.Z > 0.f && Position.X > 70.f && Position.X < W - 70.f && Position.Y > H * 0.30f && Position.Y < H - 140.f;
	FVector2D Direction(0.f, -1.f);
	if (!bOnScreen)
	{
		const FRotator CameraRotation = PlayerOwner->PlayerCameraManager->GetCameraRotation();
		const FVector Delta = Target - PlayerOwner->PlayerCameraManager->GetCameraLocation();
		const float Bearing = FMath::Atan2(FVector::DotProduct(Delta, CameraRotation.RotateVector(FVector::RightVector)),
			FVector::DotProduct(Delta, CameraRotation.Vector()));
		Direction = FVector2D(FMath::Sin(Bearing), -FMath::Cos(Bearing));
		const FVector2D Center(W * 0.5f, H * 0.52f), Extents(W * 0.5f - 86.f, H * 0.5f - 150.f);
		const float Distance = FMath::Min(Extents.X / FMath::Max(FMath::Abs(Direction.X), 0.001f), Extents.Y / FMath::Max(FMath::Abs(Direction.Y), 0.001f));
		Position = Center + Direction * Distance;
		const float MaxY = (GM->ActiveCustomer.IsValid() && Position.X < 460.f) ? H - 190.f : H - 140.f;
		Position.Y = FMath::Clamp(Position.Y, 150.f, MaxY);
	}
	else
	{
		FHitResult Occlusion;
		FCollisionQueryParams Params(TEXT("GuideMarkerView"), false, Player);
		Params.AddIgnoredActor(Guide->Current.Target.Get());
		if (GetWorld()->LineTraceSingleByChannel(Occlusion, Player->FirstPersonCamera->GetComponentLocation(), Target, ECC_Visibility, Params)) { return; }
	}

	const FVector2D Side(-Direction.Y, Direction.X);
	const FVector2D Tip = Position + Direction * 11.f;
	const FLinearColor Accent = MinimalHorror::WarmAmber;
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 7.f + Side.X * 8.f, Position.Y - Direction.Y * 7.f + Side.Y * 8.f, Accent, 2.f);
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 7.f - Side.X * 8.f, Position.Y - Direction.Y * 7.f - Side.Y * 8.f, Accent, 2.f);

	const FShiftTaskGuidance& G = Guide->Current;
	const FString Name = Guide->Status != EShiftRouteStatus::Arrived ? TEXT("FOLLOW AISLE") :
		(G.Action == EShiftGuideAction::CollectCarton ? TEXT("WAREHOUSE CARTON") :
		(G.Action == EShiftGuideAction::CollectMop ? TEXT("MOP") : G.TaskLabel));
	const int32 Meters = FMath::Max(1, FMath::RoundToInt(FVector::Dist2D(Player->GetActorLocation(), G.Location) / 100.f));
	const FString Tag = FString::Printf(TEXT("%s · %dm"), *ShiftLoc::Text(Name), Meters);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Tag, TW, TH, InterfaceFont(), 0.78f);
	const float X = FMath::Clamp(Position.X - TW * 0.5f, 18.f, W - TW - 18.f);
	const float Y = FMath::Clamp(Position.Y + 14.f, 18.f, H - TH - 46.f);
	DrawText(Tag, FLinearColor(0.f, 0.f, 0.f, 0.88f), X + 1.5f, Y + 1.5f, InterfaceFont(), 0.78f);
	DrawText(Tag, Accent, X, Y, InterfaceFont(), 0.78f);
}

void AShiftHUD::DrawRuleNoteOverlay(AShiftStoreGameMode* GM)
{
	if (!Canvas || !GM) { return; }
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	UFont* Font = InterfaceFont();
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Ease = FMath::SmoothStep(0.f, 1.f, GM->RuleNoteRaiseAlpha);
	const int32 RuleCount = GM->ActiveRuleCount();
	const bool bCorrupted = GM->NightIndex >= 2 && (GM->HasActiveAnomaly() || (GM->Director && GM->Director->IsShifted()));

	// Slight peripheral darkening while focusing eyes up close on the held paper note
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.34f * Ease), 0.f, 0.f, W, H);

	// Held paper note pulled up from pocket — intentionally blocks ~70% of the viewport so checking rules feels tense!
	const float SwayX = FMath::Sin(Now * 1.9f) * 3.5f * Ease;
	const float SwayY = FMath::Cos(Now * 2.4f) * 2.5f * Ease;
	const float NoteW = W * 0.68f;
	const float NoteH = H * 0.86f;
	const float X = (W - NoteW) * 0.5f + SwayX;
	const float Y = FMath::Lerp(H + 36.f, H * 0.18f, Ease) + SwayY;

	// Soft drop shadow behind held paper
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f * Ease), X - 12.f, Y + 10.f, NoteW + 24.f, NoteH);

	// Aged dim yellowed pocket paper (muted for 2:30 AM store lighting — never blinding white)
	const FLinearColor PaperBase = bCorrupted
		? FLinearColor(0.64f, 0.52f, 0.44f, 0.98f)
		: FLinearColor(0.73f, 0.67f, 0.54f, 0.98f);
	DrawRect(PaperBase, X, Y, NoteW, NoteH);

	// Worn edges & inner paper shadow
	const FLinearColor EdgeShade(0.32f, 0.26f, 0.18f, 0.38f);
	DrawRect(EdgeShade, X, Y, NoteW, 10.f);
	DrawRect(EdgeShade, X, Y, 12.f, NoteH);
	DrawRect(EdgeShade, X + NoteW - 12.f, Y, 12.f, NoteH);

	// Pocket fold creases (horizontal & vertical center fold lines)
	DrawRect(FLinearColor(0.26f, 0.21f, 0.14f, 0.28f), X + 8.f, Y + NoteH * 0.44f, NoteW - 16.f, 2.5f);
	DrawRect(FLinearColor(0.86f, 0.80f, 0.68f, 0.22f), X + 8.f, Y + NoteH * 0.44f + 2.5f, NoteW - 16.f, 2.f);
	DrawRect(FLinearColor(0.26f, 0.21f, 0.14f, 0.22f), X + NoteW * 0.5f, Y + 8.f, 2.5f, NoteH - 16.f);

	// Faint dried crimson stain near lower-right corner
	DrawRect(FLinearColor(0.38f, 0.06f, 0.05f, 0.26f), X + NoteW * 0.72f, Y + NoteH * 0.58f, NoteW * 0.18f, NoteH * 0.14f);
	DrawRect(FLinearColor(0.30f, 0.04f, 0.04f, 0.20f), X + NoteW * 0.76f, Y + NoteH * 0.62f, NoteW * 0.12f, NoteH * 0.10f);

	// Dark ink typography on paper
	const FLinearColor InkDark(0.11f, 0.09f, 0.07f, 0.95f);
	const FLinearColor InkRed(0.56f, 0.08f, 0.07f, 0.96f);
	const FLinearColor InkMuted(0.28f, 0.23f, 0.18f, 0.88f);

	// Top pocket hint
	{
		const FString TopHint = ShiftLoc::Text(TEXT("[Tab] Put Back in Pocket"));
		float TW = 0.f, TH = 0.f;
		GetTextSize(TopHint, TW, TH, Font, 0.86f);
		DrawText(TopHint, InkMuted, X + NoteW - TW - 28.f, Y + 22.f, Font, 0.86f);
	}

	// Title + vermilion underline
	{
		const FString Title = ShiftLoc::Text(TEXT("NIGHT SHIFT NOTE"));
		float TW = 0.f, TH = 0.f;
		GetTextSize(Title, TW, TH, Font, 1.55f);
		DrawText(Title, bCorrupted ? InkRed : InkDark, X + (NoteW - TW) * 0.5f, Y + 52.f, Font, 1.55f);
		DrawRect(InkRed, X + 52.f, Y + 102.f, NoteW - 104.f, 2.f);
	}

	const TCHAR* Rules[] = {
		TEXT("1. Tilted head / staring blankly -> Not human. Press [R] to refuse service immediately."),
		TEXT("2. Red eye glare & flickering light -> Not human. Press [R] to refuse service immediately."),
		TEXT("3. Feet floating silently off the floor -> Not human. Press [R] to refuse service immediately.")
	};
	float CurY = Y + 132.f;
	for (int32 i = 0; i < RuleCount && i < 3; ++i)
	{
		const TArray<FString> Wrapped = WrapText(Rules[i], NoteW - 104.f, 1.08f, 3);
		for (const FString& Line : Wrapped)
		{
			DrawText(Line, i == 0 ? InkRed : InkDark, X + 52.f, CurY, Font, 1.08f);
			CurY += 34.f;
		}
		CurY += 20.f;
	}

	// Bottom warning scribble on the note
	const TArray<FString> Foot = WrapText(TEXT("[R] Refuse abnormal visitors — never refuse normal guests.  [Q] Glimpse the old scene if the air feels cold."), NoteW - 104.f, 0.90f, 2);
	float FootY = Y + NoteH * 0.66f;
	for (const FString& Line : Foot)
	{
		DrawText(Line, InkMuted, X + 52.f, FootY, Font, 0.90f);
		FootY += 28.f;
	}
}
