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

void AShiftHUD::DrawCornerBrackets(float X, float Y, float W, float H, float Len, float Thick, FLinearColor Color)
{
	// Top-Left
	DrawRect(Color, X, Y, Len, Thick);
	DrawRect(Color, X, Y, Thick, Len);
	// Top-Right
	DrawRect(Color, X + W - Len, Y, Len, Thick);
	DrawRect(Color, X + W - Thick, Y, Thick, Len);
	// Bottom-Left
	DrawRect(Color, X, Y + H - Thick, Len, Thick);
	DrawRect(Color, X, Y + H - Len, Thick, Len);
	// Bottom-Right
	DrawRect(Color, X + W - Len, Y + H - Thick, Len, Thick);
	DrawRect(Color, X + W - Thick, Y + H - Len, Thick, Len);
}

void AShiftHUD::DrawHazardStripe(float X, float Y, float W, float H, FLinearColor ColA, FLinearColor ColB, int32 Blocks)
{
	if (Blocks <= 0) { return; }
	const float Step = W / static_cast<float>(Blocks);
	for (int32 i = 0; i < Blocks; ++i)
	{
		DrawRect((i % 2 == 0) ? ColA : ColB, X + i * Step, Y, Step + 0.5f, H);
	}
}

void AShiftHUD::DrawStyledPanel(float X, float Y, float W, float H, FLinearColor Accent, FLinearColor Bg, bool bGlitch, float HeaderH)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float JitterX = bGlitch ? FMath::Sin(Now * 37.f) * 1.8f : 0.f;

	// Outer drop shadow & chromatic ghost offset during Shift
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X + 6.f, Y + 6.f, W, H);
	if (bGlitch)
	{
		DrawRect(FLinearColor(0.85f, 0.05f, 0.12f, 0.22f), X - 3.f + JitterX, Y - 2.f, W + 6.f, H + 4.f);
		DrawRect(FLinearColor(0.05f, 0.80f, 0.95f, 0.14f), X + 3.f - JitterX, Y + 2.f, W, H);
	}

	// Main dark-glass body
	DrawRect(Bg, X, Y, W, H);

	// Optional header ribbon tint
	if (HeaderH > 0.f)
	{
		DrawRect(FLinearColor(Accent.R * 0.22f, Accent.G * 0.22f, Accent.B * 0.22f, 0.55f), X + 2.f, Y + 2.f, W - 4.f, HeaderH);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.35f), X + 10.f, Y + HeaderH + 1.f, W - 20.f, 1.f);
	}

	// Subtle horizontal CRT scanlines
	const FLinearColor ScanCol(Accent.R, Accent.G, Accent.B, bGlitch ? 0.055f : 0.028f);
	for (float SY = Y + 6.f; SY < Y + H - 4.f; SY += 8.f)
	{
		DrawRect(ScanCol, X + 4.f, SY, W - 8.f, 1.f);
	}

	// Animated subtle scan beam sweeping down the panel
	const float SweepY = Y + FMath::Fmod(Now * (bGlitch ? 95.f : 38.f), FMath::Max(12.f, H));
	if (SweepY >= Y + 2.f && SweepY <= Y + H - 4.f)
	{
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, bGlitch ? 0.16f : 0.07f), X + 4.f, SweepY, W - 8.f, 2.f);
	}

	// Thin hair-line border
	const FLinearColor FrameCol(Accent.R, Accent.G, Accent.B, bGlitch ? 0.48f : 0.26f);
	DrawRect(FrameCol, X, Y, W, 1.f);
	DrawRect(FrameCol, X, Y + H - 1.f, W, 1.f);
	DrawRect(FrameCol, X, Y, 1.f, H);
	DrawRect(FrameCol, X + W - 1.f, Y, 1.f, H);

	// Left vertical neon accent Spine + right micro-tick
	DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.92f), X, Y + 4.f, 4.f, H - 8.f);
	DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.65f), X + W - 3.f, Y + 10.f, 2.f, 18.f);

	// L-shaped viewfinder corner brackets
	DrawCornerBrackets(X - 1.f, Y - 1.f, W + 2.f, H + 2.f, 13.f, 2.f, FLinearColor(Accent.R, Accent.G, Accent.B, 0.95f));
}

void AShiftHUD::DrawSegmentedBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back, int32 Segments)
{
	const float Clamped = FMath::Clamp(Pct, 0.f, 1.f);
	// Outer recessed frame
	DrawRect(FLinearColor(0.01f, 0.02f, 0.02f, 0.92f), X - 1.f, Y - 1.f, W + 2.f, H + 2.f);
	DrawRect(Back, X, Y, W, H);
	DrawCornerBrackets(X - 1.f, Y - 1.f, W + 2.f, H + 2.f, 5.f, 1.f, FLinearColor(Fill.R, Fill.G, Fill.B, 0.65f));

	Segments = FMath::Max(4, Segments);
	const float Gap = 2.f;
	const float SegW = (W - 4.f - Gap * (Segments - 1)) / static_cast<float>(Segments);
	const float ExactSegments = Clamped * Segments;
	const int32 FullCount = FMath::FloorToInt(ExactSegments);
	const float Partial = ExactSegments - FullCount;

	for (int32 i = 0; i < Segments; ++i)
	{
		const float SX = X + 2.f + i * (SegW + Gap);
		// Faint unlit LED cell
		DrawRect(FLinearColor(Fill.R * 0.18f, Fill.G * 0.18f, Fill.B * 0.18f, 0.35f), SX, Y + 2.f, SegW, H - 4.f);
		if (i < FullCount)
		{
			const float Boost = (i == FullCount - 1) ? 1.2f : 1.f;
			DrawRect(FLinearColor(FMath::Min(1.f, Fill.R * Boost), FMath::Min(1.f, Fill.G * Boost), FMath::Min(1.f, Fill.B * Boost), Fill.A),
				SX, Y + 2.f, SegW, H - 4.f);
			// Top specular highlight strip on each lit cell
			DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.25f), SX, Y + 2.f, SegW, FMath::Max(1.f, (H - 4.f) * 0.25f));
		}
		else if (i == FullCount && Partial > 0.05f)
		{
			DrawRect(Fill, SX, Y + 2.f, SegW * Partial, H - 4.f);
		}
	}

	// Leading-edge bright spark marker
	if (Clamped > 0.01f)
	{
		const float EdgeX = FMath::Clamp(X + 2.f + Clamped * (W - 4.f) - 1.5f, X + 2.f, X + W - 4.f);
		DrawRect(FLinearColor(1.f, 1.f, 0.95f, 0.95f), EdgeX, Y, 3.f, H);
	}
}

void AShiftHUD::DrawWaveform(float X, float Y, float W, float H, float Time, float Agitation, FLinearColor Color)
{
	const int32 Steps = 24;
	const float StepX = W / static_cast<float>(Steps);
	const float MidY = Y + H * 0.5f;
	float PrevX = X;
	float PrevY = MidY;
	for (int32 i = 1; i <= Steps; ++i)
	{
		const float CurX = X + i * StepX;
		const float Phase = Time * (4.5f + Agitation * 9.f) + i * 0.55f;
		// ECG spike waveform
		const float Spike = FMath::Pow(FMath::Max(0.f, FMath::Sin(Phase)), 7.f) * (H * 0.48f);
		const float Wave = FMath::Sin(Phase * 2.f) * (H * (0.12f + 0.22f * Agitation));
		const float CurY = FMath::Clamp(MidY - Spike + Wave, Y + 1.f, Y + H - 1.f);
		DrawLine(PrevX, PrevY, CurX, CurY, Color, 1.6f);
		PrevX = CurX;
		PrevY = CurY;
	}
}

void AShiftHUD::DrawMenuBackdrop(float W, float H, bool bAlert)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const FLinearColor Primary = bAlert ? FLinearColor(0.95f, 0.22f, 0.18f, 1.f) : FLinearColor(0.22f, 0.88f, 0.74f, 1.f);
	const FLinearColor Secondary = bAlert ? FLinearColor(1.f, 0.55f, 0.20f, 1.f) : FLinearColor(1.f, 0.82f, 0.35f, 1.f);

	// Deep layered terminal backdrop
	DrawRect(FLinearColor(0.012f, 0.020f, 0.022f, 0.94f), 0.f, 0.f, W, H);
	DrawRect(FLinearColor(Primary.R * 0.08f, Primary.G * 0.08f, Primary.B * 0.08f, 0.35f), W * 0.08f, H * 0.08f, W * 0.84f, H * 0.84f);

	// Full-screen CRT subtle scanline grid
	for (float SY = 0.f; SY < H; SY += 6.f)
	{
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.22f), 0.f, SY, W, 2.f);
	}
	const float ScanBeamY = FMath::Fmod(Now * 110.f, H);
	DrawRect(FLinearColor(Primary.R, Primary.G, Primary.B, 0.05f), 0.f, ScanBeamY, W, 18.f);

	// Outer CCTV Viewfinder frame & corner brackets
	const float MarginX = W * 0.05f;
	const float MarginY = H * 0.06f;
	DrawCornerBrackets(MarginX, MarginY, W - MarginX * 2.f, H - MarginY * 2.f, 36.f, 3.f, Primary);
	DrawRect(FLinearColor(Primary.R, Primary.G, Primary.B, 0.22f), MarginX + 42.f, MarginY + 1.f, W - MarginX * 2.f - 84.f, 1.f);
	DrawRect(FLinearColor(Primary.R, Primary.G, Primary.B, 0.22f), MarginX + 42.f, H - MarginY - 2.f, W - MarginX * 2.f - 84.f, 1.f);

	// Top & bottom technical telemetry decorations (ASCII only, not scanned by check_localization)
	UFont* Font = InterfaceFont();
	const bool bBlink = FMath::Fmod(Now, 1.2f) < 0.75f;
	if (bBlink)
	{
		DrawRect(FLinearColor(0.95f, 0.18f, 0.15f, 0.95f), MarginX + 18.f, MarginY + 18.f, 10.f, 10.f);
	}
	DrawText(TEXT("REC // CAM-01 [24H-STOREFRONT]"), FLinearColor(Primary.R, Primary.G, Primary.B, 0.80f), MarginX + 36.f, MarginY + 13.f, Font, 0.82f);
	DrawText(TEXT("SYS.SHIFT-OS v2.4 // POS-TERMINAL"), FLinearColor(Secondary.R, Secondary.G, Secondary.B, 0.75f), W - MarginX - 310.f, MarginY + 13.f, Font, 0.82f);

	// Center divider line with glowing nodes and live ECG waveform
	const float DivY = H * 0.235f;
	DrawRect(FLinearColor(Primary.R, Primary.G, Primary.B, 0.35f), W * 0.14f, DivY, W * 0.72f, 2.f);
	DrawRect(Secondary, W * 0.14f, DivY - 2.f, 16.f, 6.f);
	DrawRect(Secondary, W * 0.86f - 16.f, DivY - 2.f, 16.f, 6.f);
	DrawWaveform(W * 0.44f, DivY - 12.f, W * 0.12f, 24.f, Now, bAlert ? 0.9f : 0.25f, Secondary);
}

void AShiftHUD::DrawStyledButton(const FBox2D& Box, bool bSelected, FLinearColor Accent)
{
	const float X = Box.Min.X, Y = Box.Min.Y;
	const float W = Box.GetSize().X, H = Box.GetSize().Y;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float Pulse = 0.85f + 0.15f * FMath::Sin(Now * 6.f);

	// Drop shadow
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.55f), X + 4.f, Y + 4.f, W, H);

	if (bSelected)
	{
		// Glowing active button card
		DrawRect(FLinearColor(Accent.R * 0.28f * Pulse, Accent.G * 0.28f * Pulse, Accent.B * 0.28f * Pulse, 0.95f), X, Y, W, H);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.18f), X + 6.f, Y + 4.f, W - 12.f, H * 0.38f);
		// Left & Right dual neon lock bars
		DrawRect(Accent, X, Y, 6.f, H);
		DrawRect(FLinearColor(1.f, 0.85f, 0.35f, 0.95f), X + W - 5.f, Y, 5.f, H);
		// Top & bottom crisp border
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.85f), X, Y, W, 1.5f);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.85f), X, Y + H - 1.5f, W, 1.5f);
		DrawCornerBrackets(X - 2.f, Y - 2.f, W + 4.f, H + 4.f, 10.f, 2.f, FLinearColor(1.f, 0.88f, 0.42f, 1.f));
	}
	else
	{
		// Sleek recessed dark button
		DrawRect(FLinearColor(0.035f, 0.065f, 0.068f, 0.88f), X, Y, W, H);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.22f), X, Y, W, 1.f);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.22f), X, Y + H - 1.f, W, 1.f);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.35f), X, Y + 4.f, 2.f, H - 8.f);
		DrawCornerBrackets(X, Y, W, H, 7.f, 1.f, FLinearColor(Accent.R, Accent.G, Accent.B, 0.45f));
	}
}

void AShiftHUD::DrawMouseSettings(const AShiftPlayerController* PC)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const FLinearColor Accent(0.28f, 0.94f, 0.78f);
	const FLinearColor Amber(1.f, 0.84f, 0.36f);

	DrawStyledPanel(W * 0.22f, H * 0.16f, W * 0.56f, H * 0.70f, Accent, FLinearColor(0.018f, 0.032f, 0.035f, 0.92f), false, H * 0.11f);
	DrawCenteredText(TEXT("Mouse Sensitivity"), H * 0.185f, FLinearColor(0.88f, 0.98f, 0.94f), 1.75f);
	DrawCenteredText(TEXT("Lower is slower. Changes are saved automatically."), H * 0.31f, FLinearColor(0.72f, 0.86f, 0.82f), 0.95f);
	DrawCenteredText(ShiftLoc::Format(TEXT("Mouse sensitivity: %sx"), {FString::Printf(TEXT("%.2f"), PC->MouseSensitivity())}),
		H * 0.38f, PC->MouseSettingsSelection() == 0 ? Amber : FLinearColor::White, 1.3f);

	const float Fraction = (PC->MouseSensitivity() - AShiftPlayerController::MinSensitivity) /
		(AShiftPlayerController::MaxSensitivity - AShiftPlayerController::MinSensitivity);
	const float TrackX = W * 0.30f;
	const float TrackY = H * 0.475f;
	const float TrackW = W * 0.40f;

	// Calibration scale ticks & segmented slider bar
	DrawSegmentedBar(TrackX, TrackY, TrackW, 14.f, Fraction, Accent, FLinearColor(0.06f, 0.11f, 0.11f, 0.95f), 24);
	for (int32 Tick = 0; Tick <= 10; ++Tick)
	{
		const float TX = TrackX + TrackW * (Tick / 10.f);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, Tick % 5 == 0 ? 0.85f : 0.38f), TX, TrackY - 7.f, 1.5f, 5.f);
	}

	// Bracketed thumb handle
	const float ThumbX = TrackX + TrackW * Fraction;
	DrawRect(Amber, ThumbX - 6.f, TrackY - 10.f, 12.f, 34.f);
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 1.f), ThumbX - 2.f, TrackY - 5.f, 4.f, 24.f);

	DrawText(TEXT("0.10x"), FLinearColor(0.68f, 0.84f, 0.78f), TrackX, H * 0.54f, InterfaceFont(), 0.9f);
	DrawText(TEXT("1.00x [DEFAULT]"), FLinearColor(0.55f, 0.72f, 0.68f), TrackX + TrackW * 0.31f - 48.f, H * 0.54f, InterfaceFont(), 0.78f);
	DrawText(TEXT("3.00x"), FLinearColor(0.68f, 0.84f, 0.78f), W * 0.70f - 65.f, H * 0.54f, InterfaceFont(), 0.9f);

	const TCHAR* Labels[] = {TEXT("Restore Default"), TEXT("Back")};
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FBox2D Box = SensitivityButton(Index, W, H);
		const bool bSelected = PC->MouseSettingsSelection() == Index + 1;
		DrawStyledButton(Box, bSelected, Accent);
		DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f,
			bSelected ? FLinearColor::White : FLinearColor(0.68f, 0.78f, 0.74f), 1.2f);
	}
	DrawCenteredText(TEXT("[Left / Right] Adjust    [Up / Down] Select    [Esc] Back"), H * 0.89f, FLinearColor(0.72f, 0.90f, 0.84f), 0.95f);
}

void AShiftHUD::DrawLocalizedText(const FString& Str, FLinearColor Color, float X, float Y, UFont* Font, float Scale, float MaxWidth)
{
	const FString Text = ShiftLoc::Text(Str);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Text, TW, TH, Font, Scale);
	if (MaxWidth <= 0.f) { MaxWidth = X < Canvas->ClipX * 0.5f ? 392.f : Canvas->ClipX - X - 38.f; }
	if (TW > MaxWidth) { Scale *= MaxWidth / TW; }
	// Subtle drop shadow for crisp readability against any background
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.75f), X + 1.5f, Y + 1.5f, Font, Scale);
	DrawText(Text, Color, X, Y, Font, Scale);
}

void AShiftHUD::DrawBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back)
{
	DrawSegmentedBar(X, Y, W, H, Pct, Fill, Back, 18);
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
	const float X = (Canvas->ClipX - TW) * 0.5f;
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, Color.A * 0.80f), X + 2.f, Y + 2.f, Font, Scale);
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
		DrawMenuBackdrop(W, H, false);
		if (PC->IsMouseSettingsOpen()) { DrawMouseSettings(PC); return; }

		DrawStyledPanel(W * 0.24f, H * 0.15f, W * 0.52f, H * 0.69f, FLinearColor(0.25f, 0.90f, 0.75f),
			FLinearColor(0.016f, 0.030f, 0.032f, 0.92f), false, H * 0.11f);
		DrawCenteredText(TEXT("PAUSED"), H * 0.175f, FLinearColor(0.88f, 0.98f, 0.94f), 2.0f);

		const TCHAR* Labels[] = {TEXT("Resume Game"), TEXT("Mouse Sensitivity"), TEXT("Return to Main Menu"), TEXT("Quit Game")};
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FBox2D Box = PauseButton(Index, W, H);
			const bool bSelected = PC->PauseSelection() == Index;
			DrawStyledButton(Box, bSelected, FLinearColor(0.28f, 0.92f, 0.76f));
			DrawCenteredText(Labels[Index], Box.Min.Y + Box.GetSize().Y * 0.24f,
				bSelected ? FLinearColor::White : FLinearColor(0.68f, 0.78f, 0.74f), 1.3f);
		}
		DrawCenteredText(TEXT("Only dawn checkpoints are saved. Returning to the menu discards this night's progress."),
			H * 0.77f, FLinearColor(1.f, 0.82f, 0.45f), 0.92f);
		DrawCenteredText(TEXT("[Esc] Resume    [Up / Down] Select    [Enter / Click] Confirm"),
			H * 0.88f, FLinearColor(0.72f, 0.90f, 0.84f), 0.95f);
		return;
	}

	// --- Full-Screen Overlay Phases (LanguageSelection, Title, NightIntro, Dawn, GameOver, Ending) ---
	if (GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup)
	{
		const bool bDangerPhase = (GM->Phase == EShiftPhase::GameOver);
		DrawMenuBackdrop(W, H, bDangerPhase);

		if (GM->Phase == EShiftPhase::LanguageSelection)
		{
			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.125f, FLinearColor(0.88f, 0.98f, 0.94f), 2.0f);
			DrawStyledPanel(W * 0.24f, H * 0.24f, W * 0.52f, H * 0.56f, FLinearColor(0.25f, 0.90f, 0.75f),
				FLinearColor(0.018f, 0.032f, 0.035f, 0.90f), false, H * 0.065f);
			DrawCenteredText(TEXT("Language / 言語 / 語言 / 语言"), H * 0.252f, FLinearColor(1.f, 0.86f, 0.40f), 1.32f);

			for (int32 Index = 0; Index < ShiftLoc::LanguageCount; ++Index)
			{
				const FBox2D Box = LanguageButton(Index, W, H);
				const bool bSelected = GM->SelectedLanguage == Index;
				DrawStyledButton(Box, bSelected, FLinearColor(0.28f, 0.92f, 0.76f));
				UFont* NativeFont = InterfaceFont(Index);
				const FString Name = FString::Printf(TEXT("[0%d]    %s"), Index + 1, *ShiftLoc::NativeName(Index));
				float TW = 0.f, TH = 0.f;
				GetTextSize(Name, TW, TH, NativeFont, 1.35f);
				DrawText(Name, bSelected ? FLinearColor::White : FLinearColor(0.68f, 0.78f, 0.74f),
					Box.Min.X + 28.f, Box.Min.Y + (Box.GetSize().Y - TH) * 0.5f, NativeFont, 1.35f);
				if (bSelected)
				{
					DrawText(TEXT("<< ACTIVE"), FLinearColor(1.f, 0.86f, 0.40f),
						Box.Max.X - 135.f, Box.Min.Y + (Box.GetSize().Y - TH) * 0.5f + 4.f, NativeFont, 0.92f);
				}
			}
			DrawCenteredText(TEXT("Confirm [Enter]"), H * 0.74f, FLinearColor(1.f, 0.90f, 0.45f), 1.3f);
			DrawCenteredText(TEXT("[Up / Down] Select    [1 / 2 / 3 / 4] Language    [Enter] Confirm"), H * 0.84f, FLinearColor(0.72f, 0.90f, 0.84f), 0.95f);
		}
		else if (GM->Phase == EShiftPhase::Title)
		{
			// Decorative title marquee card
			DrawStyledPanel(W * 0.14f, H * 0.11f, W * 0.72f, H * 0.76f, FLinearColor(0.25f, 0.90f, 0.75f),
				FLinearColor(0.015f, 0.028f, 0.030f, 0.90f), false, H * 0.13f);
			DrawHazardStripe(W * 0.14f + 6.f, H * 0.11f + 4.f, W * 0.72f - 12.f, 6.f,
				FLinearColor(0.25f, 0.90f, 0.75f, 0.85f), FLinearColor(0.03f, 0.08f, 0.08f, 0.85f), 28);

			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.145f, FLinearColor(0.90f, 1.f, 0.96f), 2.45f);
			DrawCenteredText(TEXT("Convenience Store Night-Shift Horror — GitHub Game Off 2026 (Theme: SHIFT)"), H * 0.27f, FLinearColor(1.f, 0.84f, 0.38f), 1.08f);

			// Mission briefing sub-card
			DrawStyledPanel(W * 0.20f, H * 0.34f, W * 0.60f, H * 0.25f, FLinearColor(0.95f, 0.38f, 0.32f),
				FLinearColor(0.025f, 0.035f, 0.038f, 0.88f), false, 0.f);
			DrawCenteredText(TEXT("Complete clerk chores before timers expire. Serve every customer."), H * 0.37f, FLinearColor(0.88f, 0.94f, 0.92f), 1.12f);
			DrawCenteredText(TEXT("When an entity reveals itself, the store SHIFTS into an abnormal state."), H * 0.435f, FLinearColor(1.f, 0.42f, 0.36f), 1.15f);
			DrawCenteredText(TEXT("Find the item it demands and deliver it to the Checkout Register to restore the store."), H * 0.50f, FLinearColor(0.88f, 0.94f, 0.92f), 1.12f);

			DrawCenteredText(TEXT("Controls: [WASD] Move   [Mouse] Look   [E / LMB] Interact / Hold Chore   [F / RMB] Throw Item   [G] Drop"), H * 0.625f, FLinearColor(0.68f, 0.88f, 0.82f), 1.0f);

			// Primary CTA button frame
			const FBox2D StartBox(FVector2D(W * 0.28f, H * 0.695f), FVector2D(W * 0.72f, H * 0.765f));
			DrawStyledButton(StartBox, true, FLinearColor(0.28f, 0.94f, 0.78f));
			DrawCenteredText(TEXT("[SPACE / E] New Game — Night 1"), H * 0.712f, FLinearColor(1.f, 0.94f, 0.55f), 1.38f);

			if (GM->bHasSavedProgress)
			{
				const FBox2D ContBox(FVector2D(W * 0.32f, H * 0.78f), FVector2D(W * 0.68f, H * 0.835f));
				DrawStyledButton(ContBox, false, FLinearColor(1.f, 0.82f, 0.35f));
				DrawCenteredText(ShiftLoc::Format(TEXT("[C] Continue Saved Game — %s"), {GM->CurrentNightSpec().Title}), H * 0.793f, FLinearColor(0.85f, 0.96f, 0.90f), 1.08f);
			}
			DrawCenteredText(TEXT("[L] Change language"), H * 0.89f, FLinearColor(0.70f, 0.88f, 0.82f), 0.95f);
		}
		else if (GM->Phase == EShiftPhase::NightIntro)
		{
			const FShiftNightSpec& N = GM->CurrentNightSpec();
			DrawStyledPanel(W * 0.18f, H * 0.14f, W * 0.64f, H * 0.68f, FLinearColor(0.28f, 0.92f, 0.76f),
				FLinearColor(0.016f, 0.030f, 0.032f, 0.92f), false, H * 0.11f);
			DrawHazardStripe(W * 0.18f + 6.f, H * 0.14f + 4.f, W * 0.64f - 12.f, 5.f,
				FLinearColor(1.f, 0.82f, 0.35f, 0.9f), FLinearColor(0.04f, 0.07f, 0.07f, 0.9f), 24);

			DrawCenteredText(N.Title, H * 0.17f, FLinearColor(0.92f, 1.f, 0.96f), 2.15f);

			// Shift Handover Memo Sub-box
			DrawStyledPanel(W * 0.23f, H * 0.34f, W * 0.54f, H * 0.18f, FLinearColor(1.f, 0.82f, 0.35f),
				FLinearColor(0.028f, 0.042f, 0.042f, 0.90f), false, 0.f);
			TArray<FString> Lines;
			ShiftLoc::Text(N.Intro).ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				DrawCenteredText(Lines[i], H * 0.38f + i * 36.f, FLinearColor(0.94f, 0.96f, 0.92f), 1.18f);
			}

			DrawCenteredText(ShiftLoc::Format(TEXT("Customers Expected Tonight: %d   |   Auto-Save Active"), {LexToString(N.Customers.Num())}), H * 0.565f, FLinearColor(0.68f, 0.90f, 0.84f), 1.08f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Chore time limit: %ds. Timers are paused on this screen."), {LexToString(FMath::RoundToInt(N.TaskTimeLimit))}), H * 0.63f, FLinearColor(0.80f, 0.92f, 0.88f), 1.0f);

			const FBox2D BeginBox(FVector2D(W * 0.28f, H * 0.705f), FVector2D(W * 0.72f, H * 0.78f));
			DrawStyledButton(BeginBox, true, FLinearColor(0.28f, 0.94f, 0.78f));
			DrawCenteredText(TEXT("Press [SPACE] or [E] to Begin Night"), H * 0.725f, FLinearColor(1.f, 0.92f, 0.50f), 1.35f);
		}
		else if (GM->Phase == EShiftPhase::Dawn)
		{
			DrawStyledPanel(W * 0.18f, H * 0.15f, W * 0.64f, H * 0.66f, FLinearColor(1.f, 0.86f, 0.38f),
				FLinearColor(0.020f, 0.032f, 0.030f, 0.92f), false, H * 0.12f);
			DrawCenteredText(TEXT("06:00 AM — DAWN"), H * 0.18f, FLinearColor(1.f, 0.90f, 0.48f), 2.2f);
			DrawCenteredText(ShiftLoc::Format(TEXT("%s COMPLETE — Store Handed Over to Day Shift"), {FString(GM->CurrentNightSpec().Title)}), H * 0.36f, FLinearColor(0.84f, 0.98f, 0.92f), 1.3f);

			DrawStyledPanel(W * 0.23f, H * 0.44f, W * 0.54f, H * 0.14f, FLinearColor(0.28f, 0.92f, 0.76f),
				FLinearColor(0.015f, 0.028f, 0.028f, 0.90f), false, 0.f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Sales Revenue: $%.2f   |   Merchandise Losses: -$%.2f   |   Progress Saved"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses)}), H * 0.49f, FLinearColor(0.92f, 0.96f, 0.92f), 1.18f);

			const FBox2D NextBox(FVector2D(W * 0.28f, H * 0.685f), FVector2D(W * 0.72f, H * 0.765f));
			DrawStyledButton(NextBox, true, FLinearColor(1.f, 0.84f, 0.36f));
			DrawCenteredText(TEXT("Press [SPACE] or [E] for Next Night"), H * 0.708f, FLinearColor(1.f, 0.94f, 0.55f), 1.38f);
		}
		else if (GM->Phase == EShiftPhase::GameOver)
		{
			DrawStyledPanel(W * 0.18f, H * 0.15f, W * 0.64f, H * 0.66f, FLinearColor(0.96f, 0.22f, 0.18f),
				FLinearColor(0.038f, 0.014f, 0.016f, 0.94f), true, H * 0.12f);
			DrawHazardStripe(W * 0.18f + 6.f, H * 0.15f + 4.f, W * 0.64f - 12.f, 6.f,
				FLinearColor(0.95f, 0.20f, 0.16f, 0.95f), FLinearColor(0.08f, 0.02f, 0.02f, 0.95f), 24);

			DrawCenteredText(TEXT("SHIFT TERMINATED"), H * 0.185f, FLinearColor(1.f, 0.28f, 0.22f), 2.3f);
			DrawCenteredText(GM->GameOverReason, H * 0.42f, FLinearColor(0.96f, 0.88f, 0.88f), 1.32f);

			const FBox2D RetryBox(FVector2D(W * 0.24f, H * 0.675f), FVector2D(W * 0.76f, H * 0.755f));
			DrawStyledButton(RetryBox, true, FLinearColor(0.95f, 0.26f, 0.20f));
			DrawCenteredText(TEXT("Press [SPACE], [E], or [R] to Rewind to Start of Current Night"), H * 0.698f, FLinearColor(1.f, 0.92f, 0.50f), 1.3f);
		}
		else if (GM->Phase == EShiftPhase::Ending)
		{
			const FLinearColor EndAccent = GM->IsTrueEnding() ? FLinearColor(1.f, 0.86f, 0.36f) : FLinearColor(0.28f, 0.92f, 0.76f);
			DrawStyledPanel(W * 0.15f, H * 0.12f, W * 0.70f, H * 0.74f, EndAccent,
				FLinearColor(0.016f, 0.028f, 0.030f, 0.93f), false, H * 0.12f);
			DrawCenteredText(GM->IsTrueEnding() ? TEXT("TRUE ENDING — MASTER OF THE NIGHT SHIFT") : TEXT("SURVIVOR ENDING — 06:00 AM HANDOVER"), H * 0.155f, EndAccent, 2.0f);

			const FString EndingBody = GM->IsTrueEnding()
				? ShiftLoc::Text(TEXT("You saw through every disguise before the store could fully shift.\nThe day-shift clerk left a note on the register:\n\"You understand the rules now. The store is yours.\""))
				: ShiftLoc::Text(TEXT("Three nights of shifting aisles and blackouts are finally over.\nSunlight hits the front glass. You drop your apron on the counter\nand walk out before the morning door chime rings."));
			TArray<FString> EndingLines;
			EndingBody.ParseIntoArrayLines(EndingLines);
			for (int32 Line = 0; Line < EndingLines.Num(); ++Line)
			{
				DrawCenteredText(EndingLines[Line], H * (0.31f + Line * 0.052f), FLinearColor(0.90f, 0.95f, 0.92f), 1.15f);
			}

			DrawStyledPanel(W * 0.21f, H * 0.49f, W * 0.58f, H * 0.16f, EndAccent,
				FLinearColor(0.022f, 0.038f, 0.040f, 0.90f), false, 0.f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Final Revenue: $%.2f   |   Total Losses: -$%.2f   |   Net: $%.2f"), {FString::Printf(TEXT("%.2f"), GM->Income), FString::Printf(TEXT("%.2f"), GM->Losses), FString::Printf(TEXT("%.2f"), GM->Income - GM->Losses)}), H * 0.515f, FLinearColor(1.f, 0.90f, 0.52f), 1.25f);
			DrawCenteredText(ShiftLoc::Format(TEXT("Disguises Warded Off: %d   |   Store Shifts Survived: %d"), {LexToString(GM->TotalMonstersWarded), LexToString(GM->TotalShiftsTriggered)}), H * 0.585f, FLinearColor(0.75f, 0.94f, 0.88f), 1.08f);

			const FBox2D ReplayBox(FVector2D(W * 0.25f, H * 0.70f), FVector2D(W * 0.75f, H * 0.78f));
			DrawStyledButton(ReplayBox, true, EndAccent);
			DrawCenteredText(TEXT("Thank you for playing SHIFT / 24! Press [SPACE] to Play Again."), H * 0.722f, FLinearColor(1.f, 0.94f, 0.55f), 1.3f);
		}
		return;
	}

	// ============================================================================================
	// IN-GAME PLAY HUD (Working & Cleanup Phases): Cyber-POS Terminal & CCTV Tactical Monitor
	// ============================================================================================
	const bool bShifted = GM->Director && GM->Director->IsShifted();
	const FLinearColor ThemeAccent = bShifted ? FLinearColor(0.98f, 0.24f, 0.20f, 1.f) : FLinearColor(0.24f, 0.90f, 0.76f, 1.f);
	const FLinearColor ThemeBg = bShifted ? FLinearColor(0.045f, 0.012f, 0.015f, 0.86f) : FLinearColor(0.015f, 0.032f, 0.035f, 0.84f);

	// Subtle corner CCTV screen framing ticks
	DrawCornerBrackets(10.f, 10.f, W - 20.f, H - 20.f, 22.f, 1.5f, FLinearColor(ThemeAccent.R, ThemeAccent.G, ThemeAccent.B, 0.35f));

	// --- 1. Top-Left Status Panel: POS Clerk Monitor, Bio-Sanity ECG, Ledger & Equipment ---
	DrawStyledPanel(24.f, 24.f, 424.f, 156.f, ThemeAccent, ThemeBg, bShifted, 36.f);
	if (bShifted)
	{
		DrawHazardStripe(28.f, 25.f, 416.f, 4.f, FLinearColor(0.98f, 0.20f, 0.16f, 0.9f), FLinearColor(0.10f, 0.02f, 0.02f, 0.9f), 20);
	}
	// Blinking live status indicator LED
	const bool bLedOn = FMath::Fmod(Now, bShifted ? 0.45f : 1.4f) < (bShifted ? 0.25f : 0.9f);
	DrawRect(bLedOn ? ThemeAccent : FLinearColor(0.15f, 0.22f, 0.22f, 0.8f), 34.f, 37.f, 8.f, 8.f);

	DrawLocalizedText(ShiftLoc::Format(TEXT("%s  |  Customers: %d / %d  |  %s"), {FString(GM->CurrentNightSpec().Title), LexToString(GM->CustomersServed), LexToString(GM->CurrentNightSpec().Customers.Num()), FString(bShifted ? TEXT("STATE: SHIFTED [ABNORMAL]") : TEXT("STATE: NORMAL"))}),
		bShifted ? FLinearColor(1.f, 0.40f, 0.34f) : FLinearColor(0.84f, 0.98f, 0.92f), 48.f, 31.f, Font, 0.96f, 388.f);

	// Sanity bar + Live animated ECG Brainwave Monitor
	const float SanityPct = GM->Sanity / ShiftData::MaxSanity;
	const FLinearColor SanityCol = SanityPct < 0.35f ? FLinearColor(0.98f, 0.22f, 0.18f, 1.f) : (bShifted ? FLinearColor(1.f, 0.65f, 0.24f, 1.f) : FLinearColor(0.24f, 0.92f, 0.76f, 1.f));
	DrawLocalizedText(ShiftLoc::Format(TEXT("SANITY: %d%%"), {LexToString(FMath::RoundToInt(GM->Sanity))}),
		SanityPct < 0.35f ? FLinearColor(1.f, 0.36f, 0.30f) : FLinearColor(0.88f, 0.96f, 0.94f), 36.f, 70.f, Font, 0.92f, 122.f);
	DrawWaveform(158.f, 71.f, 44.f, 20.f, Now, (1.f - SanityPct) + (bShifted ? 0.5f : 0.f), SanityCol);
	DrawSegmentedBar(208.f, 74.f, 224.f, 16.f, SanityPct, SanityCol, FLinearColor(0.05f, 0.09f, 0.09f, 0.95f), 16);

	// Store ledger / loss limit gauge
	const float LossPct = GM->Losses / ShiftData::LossLimit;
	const FLinearColor LedgerCol = LossPct > 0.7f ? FLinearColor(1.f, 0.26f, 0.20f, 1.f) : FLinearColor(0.98f, 0.58f, 0.20f, 1.f);
	DrawLocalizedText(ShiftLoc::Format(TEXT("SALES: +$%.1f   LOSS: -$%.1f / -$%.0f"), {FString::Printf(TEXT("%.1f"), GM->Income), FString::Printf(TEXT("%.1f"), GM->Losses), FString::Printf(TEXT("%.0f"), ShiftData::LossLimit)}),
		LossPct > 0.7f ? FLinearColor(1.f, 0.45f, 0.35f) : FLinearColor(0.90f, 0.94f, 0.88f), 36.f, 110.f, Font, 0.90f, 396.f);
	DrawSegmentedBar(36.f, 146.f, 396.f, 12.f, LossPct, LedgerCol, FLinearColor(0.05f, 0.08f, 0.08f, 0.95f), 22);

	// Held item & [Shift] Sprint Stamina Dock
	if (Player)
	{
		const bool bHolding = Player->HeldItem.IsValid();
		const FLinearColor DockAccent = bHolding ? FLinearColor(1.f, 0.84f, 0.36f, 1.f) : FLinearColor(0.32f, 0.82f, 0.96f, 1.f);
		DrawStyledPanel(24.f, 188.f, 424.f, 76.f, DockAccent, ThemeBg, false, 34.f);

		const FString HeldStr = bHolding
			? ShiftLoc::Format(TEXT("CARRYING: %s  ([F/RMB] Throw  [G] Drop)"), {FString(Player->HeldItem->DisplayName())})
			: TEXT("HANDS: Empty  ([E] Pick up / Interact)");
		DrawLocalizedText(HeldStr, bHolding ? FLinearColor(1.f, 0.92f, 0.58f) : FLinearColor(0.82f, 0.92f, 0.90f), 36.f, 194.f, Font, 0.88f, 398.f);

		const float StamPct = FMath::Clamp(Player->Stamina / 100.f, 0.f, 1.f);
		const FLinearColor StamCol = Player->bIsSprinting ? FLinearColor(0.38f, 0.95f, 1.f, 1.f) : (StamPct < 0.25f ? FLinearColor(1.f, 0.45f, 0.28f, 1.f) : FLinearColor(0.24f, 0.76f, 0.92f, 0.95f));
		DrawLocalizedText(TEXT("SPRINT [Shift]"), StamCol, 36.f, 232.f, Font, 0.84f, 155.f);
		DrawSegmentedBar(208.f, 236.f, 224.f, 12.f, StamPct, StamCol, FLinearColor(0.05f, 0.09f, 0.10f, 0.95f), 16);
	}

	// --- 2. Bottom-Left Pinned Shift Anomaly Rulebook (Tactical Protocol Clipboard) ---
	{
		const float RuleY = H - 150.f;
		const float RuleW = 452.f;
		const float RuleH = 126.f;
		const bool bCorrupted = GM->NightIndex >= 2 && (GM->HasActiveAnomaly() || bShifted);
		const FLinearColor RuleAccent = bCorrupted
			? FLinearColor(0.95f, 0.20f, 0.18f, 1.f)
			: FLinearColor(1.f, 0.80f, 0.28f, 1.f);

		DrawStyledPanel(24.f, RuleY, RuleW, RuleH, RuleAccent,
			bCorrupted ? FLinearColor(0.05f, 0.012f, 0.015f, 0.90f) : FLinearColor(0.020f, 0.032f, 0.034f, 0.88f),
			bCorrupted, 32.f);
		DrawHazardStripe(28.f, RuleY + 1.f, RuleW - 8.f, 3.5f,
			RuleAccent, FLinearColor(0.04f, 0.05f, 0.05f, 0.9f), 22);

		if (bCorrupted)
		{
			const FLinearColor WarnCol(1.f, 0.30f + 0.20f * FMath::Sin(Now * 14.f), 0.25f, 0.98f);
			DrawStyledPanel(24.f, RuleY - 28.f, RuleW, 24.f, WarnCol, FLinearColor(0.08f, 0.01f, 0.01f, 0.92f), true, 0.f);
			DrawLocalizedText(TEXT("[! SIGNAL CORRUPTED — FIX ANOMALY !]"), WarnCol, 36.f, RuleY - 25.f, Font, 0.78f, RuleW - 24.f);
		}

		DrawLocalizedText(TEXT("SHIFT RULES — Observe Customer Before Handing:"), FLinearColor(1.f, 0.88f, 0.42f), 36.f, RuleY + 7.f, Font, 0.85f, RuleW - 24.f);

		// Three color-coded rule badges + row dividers
		const FLinearColor BadgeCols[3] = {
			FLinearColor(1.f, 0.52f, 0.24f, 0.95f),
			FLinearColor(0.98f, 0.26f, 0.28f, 0.95f),
			FLinearColor(0.38f, 0.86f, 1.f, 0.95f)
		};
		const TCHAR* RuleTexts[3] = {
			TEXT("1. Tilted head / stares wall -> Hand [PANTRY Tomato Soup]"),
			TEXT("2. Red eye glare & light flicker -> Hand [DAILY Whole Milk]"),
			TEXT("3. Feet floating off floor -> Hand [MORNING Bread]")
		};
		for (int32 RIdx = 0; RIdx < 3; ++RIdx)
		{
			const float RY = RuleY + 38.f + RIdx * 28.f;
			DrawRect(FLinearColor(BadgeCols[RIdx].R * 0.22f, BadgeCols[RIdx].G * 0.22f, BadgeCols[RIdx].B * 0.22f, 0.55f), 32.f, RY + 1.f, RuleW - 18.f, 23.f);
			DrawRect(BadgeCols[RIdx], 32.f, RY + 1.f, 4.f, 23.f);
			DrawLocalizedText(RuleTexts[RIdx], FLinearColor(0.92f, 0.97f, 0.95f), 42.f, RY + 2.f, Font, 0.81f, RuleW - 32.f);
		}
	}

	// --- 3. Proximity Heartbeat / Jumpscare Layered Crimson Vignette ---
	if (GM->HeartbeatPulseAlpha > 0.02f)
	{
		const float A = FMath::Clamp(GM->HeartbeatPulseAlpha * 0.34f, 0.f, 0.38f);
		for (int32 Layer = 0; Layer < 3; ++Layer)
		{
			const float Band = 10.f + Layer * 12.f;
			const FLinearColor PulseCol(0.82f, 0.03f, 0.03f, A * (1.f - Layer * 0.28f));
			DrawRect(PulseCol, 0.f, 0.f, W, Band);
			DrawRect(PulseCol, 0.f, H - Band, W, Band);
			DrawRect(PulseCol, 0.f, Band, Band, H - Band * 2.f);
			DrawRect(PulseCol, W - Band, Band, Band, H - Band * 2.f);
		}
	}

	// --- 4. Top-Right Objective & Dispatch Terminal ---
	const float ChecklistHeight = DrawObjectivePanel(GM, Player);

	// --- 5. Center-Top Alert Banner ---
	if (GM->BannerTimer > 0.f && !GM->BannerText.IsEmpty())
	{
		const float BannerY = FMath::Max(276.f, FMath::Max(36.f + ChecklistHeight, H * 0.25f));
		const FString BannerStr = ShiftLoc::Text(GM->BannerText);
		float BTW = 0.f, BTH = 0.f;
		GetTextSize(BannerStr, BTW, BTH, Font, 1.12f);
		const float BannerW = FMath::Clamp(BTW + 64.f, 360.f, W * 0.76f);
		const float BannerX = (W - BannerW) * 0.5f;
		DrawStyledPanel(BannerX, BannerY, BannerW, 46.f, FLinearColor(1.f, 0.82f, 0.32f),
			FLinearColor(0.035f, 0.022f, 0.015f, 0.90f), bShifted, 0.f);
		DrawHazardStripe(BannerX + 6.f, BannerY + 2.f, BannerW - 12.f, 3.f,
			FLinearColor(1.f, 0.82f, 0.32f, 0.85f), FLinearColor(0.05f, 0.03f, 0.02f, 0.85f), 24);
		DrawCenteredText(GM->BannerText, BannerY + 10.f, FLinearColor(1.f, 0.90f, 0.50f), 1.12f);
	}

	// --- 6. 3D World Customer Speech Callout Card ---
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
				if (X < 444.f) { Y = FMath::Max(Y, 272.f); }
				if (X + TW > W - 455.f) { Y = FMath::Max(Y, ChecklistHeight + 42.f); }
				Y = FMath::Min(Y, H * 0.72f);

				const FLinearColor BubbleAccent = Customer->IsMonsterActive()
					? FLinearColor(1.f, 0.25f, 0.20f, 1.f)
					: FLinearColor(0.32f, 0.92f, 0.78f, 1.f);
				DrawStyledPanel(X - 12.f, Y - 6.f, TW + 24.f, Lines.Num() * 30.f + 12.f, BubbleAccent,
					FLinearColor(0.015f, 0.028f, 0.030f, 0.90f), Customer->IsMonsterActive(), 0.f);
				for (int32 Index = 0; Index < Lines.Num(); ++Index)
				{
					DrawText(Lines[Index], FLinearColor(Customer->Speech->TextRenderColor), X, Y + Index * 30.f, Font, 0.95f);
				}
			}
		}
	}

	// --- 7. Center Tactical Reticle & Interactive Callout Pill ---
	const float CX = W * 0.5f;
	const float CY = H * 0.5f;
	const bool bHasPrompt = Player && !Player->CrosshairPrompt.IsEmpty();
	const FLinearColor ReticleCol = bHasPrompt
		? FLinearColor(1.f, 0.90f, 0.42f, 0.95f)
		: FLinearColor(0.82f, 0.96f, 0.90f, 0.78f);

	// Center precision dot
	DrawRect(ReticleCol, CX - 1.5f, CY - 1.5f, 3.f, 3.f);
	// Dynamic expanding reticle brackets when focusing an interactable
	const float Spread = bHasPrompt ? (18.f + 2.f * FMath::Sin(Now * 8.f)) : 10.f;
	DrawCornerBrackets(CX - Spread, CY - Spread, Spread * 2.f, Spread * 2.f, bHasPrompt ? 6.f : 4.f, 1.5f, ReticleCol);

	if (bHasPrompt)
	{
		const FString PromptText = ShiftLoc::Text(Player->CrosshairPrompt);
		float PW = 0.f, PH = 0.f;
		GetTextSize(PromptText, PW, PH, Font, 1.05f);
		PW = FMath::Min(PW, W * 0.80f);
		const float PillW = PW + 36.f;
		const float PillX = (W - PillW) * 0.5f;
		const float PillY = CY + 24.f;

		const FLinearColor PromptAccent = Player->IsGrabbed()
			? FLinearColor(1.f, 0.24f, 0.20f, 1.f)
			: FLinearColor(1.f, 0.86f, 0.36f, 1.f);
		DrawStyledPanel(PillX, PillY, PillW, 36.f, PromptAccent,
			FLinearColor(0.015f, 0.028f, 0.030f, 0.88f), Player->IsGrabbed(), 0.f);
		DrawCenteredText(Player->CrosshairPrompt, PillY + 5.f, FLinearColor(1.f, 0.96f, 0.78f), 1.05f);

		const float BarProg = Player->IsGrabbed() ? Player->StruggleProgress : Player->HoldProgress;
		if (BarProg > 0.01f)
		{
			const FLinearColor ProgCol = Player->IsGrabbed()
				? FLinearColor(1.f, 0.32f, 0.24f, 1.f)
				: FLinearColor(0.28f, 0.95f, 0.76f, 1.f);
			DrawSegmentedBar(CX - 110.f, PillY + 42.f, 220.f, 14.f, BarProg, ProgCol,
				FLinearColor(0.04f, 0.08f, 0.08f, 0.95f), 16);
		}
	}

	DrawTaskGuidance(GM, Player);
	DrawSubtitle(GM);

	// Bottom-Right Pause Keybadge
	DrawStyledPanel(W - 178.f, H - 46.f, 154.f, 30.f, FLinearColor(ThemeAccent.R, ThemeAccent.G, ThemeAccent.B, 0.65f),
		FLinearColor(0.015f, 0.028f, 0.030f, 0.80f), false, 0.f);
	DrawLocalizedText(TEXT("[Esc] Pause"), FLinearColor(0.78f, 0.94f, 0.88f), W - 162.f, H - 42.f, Font, 0.84f, 132.f);
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
	const FLinearColor Accent(1.f, 0.86f, 0.36f), Normal(0.88f, 0.95f, 0.92f), Dim(0.64f, 0.80f, 0.75f);
	const auto Add = [this, &Rows, Width](const FString& Text, FLinearColor Color, float Scale = 0.95f) {
		Rows.Add({WrapText(Text, Width - 28.f, Scale), Color, Scale});
	};

	bool bEntityAlert = false;
	Add(TEXT("CURRENT OBJECTIVE"), FLinearColor(0.80f, 0.96f, 0.90f), 1.f);
	if (const AShiftCustomer* C = GM->ActiveCustomer.Get())
	{
		if (C->IsMonsterActive())
		{
			bEntityAlert = true;
			Add(TEXT("!! ABNORMAL ENTITY IN STORE !!"), FLinearColor(1.f, 0.32f, 0.28f));
			Add(ShiftLoc::Format(TEXT("Demand: Find [%s] (Red Glow)"), {ShiftData::ProductLabel(C->Spec.MonsterSku)}), Accent);
			Add(TEXT("-> Deliver it to the Checkout Register!"), Normal);
			Add(TEXT("Shelves swapped stock! Corridor Breaker [E] restores light 10s."), Dim, 0.88f);
		}
		else if (C->State == ECustomerState::WaitingForHelp)
		{
			Add(ShiftLoc::Format(TEXT("* Customer Help: Bring [%s]"), {ShiftData::ProductLabel(C->Spec.WantedSku)}), Accent);
			Add(TEXT("Take the product from a shelf, then press [E] on the customer."), Normal);
			Add(TEXT("Check SHIFT RULES (Bottom-Left): Observe posture before handing!"), FLinearColor(1.f, 0.74f, 0.38f), 0.88f);
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
			if (Overdue > 0)
			{
				bEntityAlert = true;
				Add(ShiftLoc::Format(TEXT("%d overdue chores are draining sanity!"), {LexToString(Overdue)}), FLinearColor(1.f, 0.30f, 0.25f));
			}
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

	const FLinearColor PanelAccent = bEntityAlert ? FLinearColor(0.98f, 0.24f, 0.20f, 1.f) : FLinearColor(0.26f, 0.90f, 0.76f, 1.f);
	const FLinearColor PanelBg = bEntityAlert ? FLinearColor(0.045f, 0.012f, 0.015f, 0.88f) : FLinearColor(0.015f, 0.032f, 0.035f, 0.86f);
	DrawStyledPanel(X, 24.f, Width, Height, PanelAccent, PanelBg, bEntityAlert, 36.f);
	if (bEntityAlert)
	{
		DrawHazardStripe(X + 6.f, 25.f, Width - 12.f, 4.f, PanelAccent, FLinearColor(0.08f, 0.02f, 0.02f, 0.9f), 18);
	}

	float Y = 36.f;
	for (int32 RIdx = 0; RIdx < Rows.Num(); ++RIdx)
	{
		const FRow& Row = Rows[RIdx];
		if (RIdx > 1)
		{
			DrawRect(FLinearColor(PanelAccent.R, PanelAccent.G, PanelAccent.B, 0.14f), X + 14.f, Y - 4.f, Width - 28.f, 1.f);
		}
		for (const FString& Line : Row.Lines)
		{
			DrawText(Line, FLinearColor(0.f, 0.f, 0.f, 0.75f), X + 15.5f, Y + 1.5f, InterfaceFont(), Row.Scale);
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
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float SafeLeft = 492.f;
	const float SafeRight = W - 192.f;
	const float AvailW = FMath::Max(320.f, SafeRight - SafeLeft);
	const TArray<FString> Lines = WrapText(GM->SubtitleText, AvailW - 36.f, 1.05f, 2);
	SubtitleLines = Lines.Num();

	UFont* Font = InterfaceFont();
	float MaxLW = 0.f;
	for (const FString& L : Lines)
	{
		float LW = 0.f, LH = 0.f;
		GetTextSize(L, LW, LH, Font, 1.05f);
		MaxLW = FMath::Max(MaxLW, LW);
	}
	const float BoxW = FMath::Clamp(MaxLW + 44.f, 240.f, AvailW);
	const float CenterX = (W * 0.5f - BoxW * 0.5f >= SafeLeft && W * 0.5f + BoxW * 0.5f <= SafeRight)
		? W * 0.5f
		: (SafeLeft + SafeRight) * 0.5f;
	const float BoxX = CenterX - BoxW * 0.5f;
	const float Y = H - 52.f - Lines.Num() * 30.f;
	const float BoxH = Lines.Num() * 30.f + 16.f;

	DrawStyledPanel(BoxX, Y - 8.f, BoxW, BoxH, FLinearColor(0.32f, 0.90f, 0.78f),
		FLinearColor(0.012f, 0.024f, 0.026f, 0.90f), false, 0.f);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		float LW = 0.f, LH = 0.f;
		GetTextSize(Lines[Index], LW, LH, Font, 1.05f);
		const float LX = CenterX - LW * 0.5f;
		DrawText(Lines[Index], FLinearColor(0.f, 0.f, 0.f, 0.80f), LX + 1.5f, Y + Index * 30.f + 1.5f, Font, 1.05f);
		DrawText(Lines[Index], FLinearColor(0.96f, 0.97f, 0.90f), LX, Y + Index * 30.f, Font, 1.05f);
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
		// Keep the navigation marker clear of the bottom-left rulebook and subtitle strip.
		const float MaxY = Position.X < 520.f ? H - 212.f : H - 195.f;
		Position.Y = FMath::Clamp(Position.Y, 286.f, MaxY);
	}
	else
	{
		FHitResult Occlusion;
		FCollisionQueryParams Params(TEXT("GuideMarkerView"), false, Player);
		Params.AddIgnoredActor(Guide->Current.Target.Get());
		if (GetWorld()->LineTraceSingleByChannel(Occlusion, Player->FirstPersonCamera->GetComponentLocation(), Target, ECC_Visibility, Params)) { return; }
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const float PulseR = 13.f + 2.5f * FMath::Sin(Now * 6.5f);
	const FVector2D Side(-Direction.Y, Direction.X);
	const FVector2D Tip = Position + Direction * 15.f;
	const FLinearColor Accent(1.f, 0.86f, 0.30f);

	// Pulsing tactical diamond beacon around waypoint
	DrawLine(Position.X, Position.Y - PulseR, Position.X + PulseR, Position.Y, Accent, 2.f);
	DrawLine(Position.X + PulseR, Position.Y, Position.X, Position.Y + PulseR, Accent, 2.f);
	DrawLine(Position.X, Position.Y + PulseR, Position.X - PulseR, Position.Y, Accent, 2.f);
	DrawLine(Position.X - PulseR, Position.Y, Position.X, Position.Y - PulseR, Accent, 2.f);

	// Directional chevron pointer
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 8.f + Side.X * 11.f, Position.Y - Direction.Y * 8.f + Side.Y * 11.f, Accent, 3.5f);
	DrawLine(Tip.X, Tip.Y, Position.X - Direction.X * 8.f - Side.X * 11.f, Position.Y - Direction.Y * 8.f - Side.Y * 11.f, Accent, 3.5f);

	const FShiftTaskGuidance& G = Guide->Current;
	const FString Name = Guide->Status != EShiftRouteStatus::Arrived ? TEXT("FOLLOW AISLE") :
		(G.Action == EShiftGuideAction::CollectCarton ? TEXT("WAREHOUSE CARTON") :
		(G.Action == EShiftGuideAction::CollectMop ? TEXT("MOP") : G.TaskLabel));
	const int32 Meters = FMath::Max(1, FMath::RoundToInt(FVector::Dist2D(Player->GetActorLocation(), G.Location) / 100.f));
	const FString Tag = FString::Printf(TEXT("%s · %dm"), *ShiftLoc::Text(Name), Meters);
	float TW = 0.f, TH = 0.f;
	GetTextSize(Tag, TW, TH, InterfaceFont(), 0.85f);
	const float X = FMath::Clamp(Position.X - TW * 0.5f, 18.f, W - TW - 18.f);
	const float Y = FMath::Clamp(Position.Y + 18.f, 18.f, H - TH - 52.f);
	DrawStyledPanel(X - 8.f, Y - 3.f, TW + 16.f, TH + 6.f, Accent, FLinearColor(0.015f, 0.028f, 0.030f, 0.88f), false, 0.f);
	DrawText(Tag, Accent, X, Y, InterfaceFont(), 0.85f);
}
