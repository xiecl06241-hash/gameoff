#include "ShiftHUD.h"
#include "ShiftStoreGameMode.h"
#include "ShiftPlayerCharacter.h"
#include "ShiftTaskTarget.h"
#include "ShiftCustomer.h"
#include "ShiftDirector.h"
#include "ShiftItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void AShiftHUD::DrawBar(float X, float Y, float W, float H, float Pct, FLinearColor Fill, FLinearColor Back)
{
	DrawRect(Back, X, Y, W, H);
	DrawRect(Fill, X + 2.f, Y + 2.f, FMath::Clamp(Pct, 0.f, 1.f) * (W - 4.f), H - 4.f);
}

void AShiftHUD::DrawCenteredText(const FString& Str, float Y, FLinearColor Color, float Scale)
{
	if (!Canvas) { return; }
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	float TW = 0.f, TH = 0.f;
	GetTextSize(Str, TW, TH, Font, Scale);
	DrawText(Str, Color, (Canvas->ClipX - TW) * 0.5f, Y, Font, Scale);
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
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;

	// Full-screen overlay phases (Title, NightIntro, Dawn, GameOver, Ending)
	if (GM->Phase != EShiftPhase::Working && GM->Phase != EShiftPhase::Cleanup)
	{
		DrawRect(FLinearColor(0.02f, 0.03f, 0.03f, 0.90f), 0.f, 0.f, W, H);
		DrawRect(FLinearColor(0.18f, 0.55f, 0.48f, 1.f), W * 0.15f, H * 0.22f, W * 0.70f, 3.f);

		if (GM->Phase == EShiftPhase::Title)
		{
			DrawCenteredText(TEXT("S H I F T  /  2 4"), H * 0.27f, FLinearColor(0.82f, 0.95f, 0.90f), 2.4f);
			DrawCenteredText(TEXT("Convenience Store Night-Shift Horror — GitHub Game Off 2026 (Theme: SHIFT)"), H * 0.36f, FLinearColor(0.60f, 0.75f, 0.70f), 1.1f);
			DrawCenteredText(TEXT("Complete clerk chores before timers expire. Serve every customer."), H * 0.45f, FLinearColor(0.85f, 0.88f, 0.86f), 1.15f);
			DrawCenteredText(TEXT("When an entity reveals itself, the store SHIFTS into an abnormal state."), H * 0.50f, FLinearColor(0.95f, 0.45f, 0.40f), 1.15f);
			DrawCenteredText(TEXT("Find the item it demands and deliver it to the Checkout Register to restore the store."), H * 0.55f, FLinearColor(0.85f, 0.88f, 0.86f), 1.15f);
			DrawCenteredText(TEXT("Controls: [WASD] Move   [Mouse] Look   [E / LMB] Interact / Hold Chore   [F / RMB] Throw Item   [G] Drop"), H * 0.65f, FLinearColor(0.65f, 0.82f, 0.78f), 1.05f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] to Start Shift"), H * 0.76f, FLinearColor(1.f, 0.90f, 0.45f), 1.45f);
		}
		else if (GM->Phase == EShiftPhase::NightIntro)
		{
			const FShiftNightSpec& N = GM->CurrentNightSpec();
			DrawCenteredText(N.Title, H * 0.28f, FLinearColor(0.82f, 0.95f, 0.90f), 2.2f);
			TArray<FString> Lines;
			N.Intro.ParseIntoArrayLines(Lines);
			for (int32 i = 0; i < Lines.Num(); ++i)
			{
				DrawCenteredText(Lines[i], H * 0.40f + i * 34.f, FLinearColor(0.88f, 0.90f, 0.88f), 1.2f);
			}
			DrawCenteredText(FString::Printf(TEXT("Customers Expected Tonight: %d   |   Auto-Save Active"), N.Customers.Num()), H * 0.58f, FLinearColor(0.65f, 0.82f, 0.78f), 1.1f);
			DrawCenteredText(TEXT("Press [SPACE] or [E] to Begin Night"), H * 0.74f, FLinearColor(1.f, 0.90f, 0.45f), 1.4f);
		}
		else if (GM->Phase == EShiftPhase::Dawn)
		{
			DrawCenteredText(TEXT("06:00 AM — DAWN"), H * 0.28f, FLinearColor(0.95f, 0.88f, 0.55f), 2.2f);
			DrawCenteredText(FString::Printf(TEXT("%s COMPLETE — Store Handed Over to Day Shift"), *GM->CurrentNightSpec().Title), H * 0.38f, FLinearColor(0.82f, 0.95f, 0.90f), 1.3f);
			DrawCenteredText(FString::Printf(TEXT("Sales Revenue: $%.2f   |   Merchandise Losses: -$%.2f   |   Progress Saved"), GM->Income, GM->Losses), H * 0.48f, FLinearColor(0.85f, 0.88f, 0.86f), 1.2f);
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
			DrawCenteredText(FString::Printf(TEXT("Final Revenue: $%.2f   |   Total Losses: -$%.2f   |   Net: $%.2f"),
				GM->Income, GM->Losses, GM->Income - GM->Losses), H * 0.50f, FLinearColor(0.95f, 0.88f, 0.55f), 1.3f);
			DrawCenteredText(TEXT("Thank you for playing SHIFT / 24! Press [SPACE] to Play Again."), H * 0.72f, FLinearColor(1.f, 0.90f, 0.45f), 1.35f);
		}
		return;
	}

	// --- Top-Left Status Panel: Sanity, Ledger, Shift State ---
	const bool bShifted = GM->Director && GM->Director->IsShifted();
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.78f), 24.f, 24.f, 360.f, 134.f);
	DrawText(FString::Printf(TEXT("%s  |  Customers: %d / %d  |  %s"),
		*GM->CurrentNightSpec().Title, GM->CustomersServed, GM->CurrentNightSpec().Customers.Num(),
		bShifted ? TEXT("STATE: SHIFTED [ABNORMAL]") : TEXT("STATE: NORMAL")),
		bShifted ? FLinearColor(1.f, 0.32f, 0.28f) : FLinearColor(0.75f, 0.92f, 0.86f), 36.f, 32.f, Font, 1.0f);

	// Sanity bar
	const float SanityPct = GM->Sanity / ShiftData::MaxSanity;
	DrawText(FString::Printf(TEXT("SANITY: %d%%"), FMath::RoundToInt(GM->Sanity)),
		SanityPct < 0.35f ? FLinearColor(1.f, 0.3f, 0.25f) : FLinearColor(0.85f, 0.92f, 0.90f), 36.f, 60.f, Font, 0.95f);
	DrawBar(165.f, 63.f, 205.f, 16.f, SanityPct,
		SanityPct < 0.35f ? FLinearColor(0.92f, 0.22f, 0.18f, 1.f) : FLinearColor(0.25f, 0.80f, 0.68f, 1.f),
		FLinearColor(0.12f, 0.16f, 0.16f, 0.9f));

	// Store ledger / loss limit
	const float LossPct = GM->Losses / ShiftData::LossLimit;
	DrawText(FString::Printf(TEXT("SALES: +$%.1f   LOSS: -$%.1f / -$%.0f"), GM->Income, GM->Losses, ShiftData::LossLimit),
		LossPct > 0.7f ? FLinearColor(1.f, 0.4f, 0.3f) : FLinearColor(0.85f, 0.90f, 0.85f), 36.f, 90.f, Font, 0.92f);
	DrawBar(36.f, 116.f, 334.f, 12.f, LossPct, FLinearColor(0.90f, 0.42f, 0.18f, 1.f), FLinearColor(0.12f, 0.15f, 0.15f, 0.9f));

	// Held item indicator
	if (Player)
	{
		const FString HeldStr = Player->HeldItem.IsValid()
			? FString::Printf(TEXT("CARRYING: %s  ([F/RMB] Throw  [G] Drop)"), *Player->HeldItem->DisplayName())
			: TEXT("HANDS: Empty  ([E] Pick up / Interact)");
		DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.72f), 24.f, 166.f, 360.f, 32.f);
		DrawText(HeldStr, FLinearColor(0.92f, 0.90f, 0.70f), 36.f, 173.f, Font, 0.92f);
	}

	// --- Top-Right Panel: Priority Objective, Customer / Entity Demand, Active Chores ---
	const float RX = W - 430.f;
	float RY = 24.f;
	DrawRect(FLinearColor(0.02f, 0.04f, 0.04f, 0.78f), RX, RY, 406.f, 220.f);
	DrawText(TEXT("SHIFT CHECKLIST & ALERTS"), FLinearColor(0.75f, 0.92f, 0.86f), RX + 14.f, RY + 10.f, Font, 1.0f);
	RY += 38.f;

	if (GM->ActiveCustomer.IsValid())
	{
		AShiftCustomer* C = GM->ActiveCustomer.Get();
		if (C->IsMonsterActive())
		{
			DrawText(TEXT("!! ABNORMAL ENTITY IN STORE !!"), FLinearColor(1.f, 0.25f, 0.22f), RX + 14.f, RY, Font, 1.05f);
			RY += 24.f;
			DrawText(FString::Printf(TEXT("Demand: Find [%s] (Red Glow)"), *ShiftData::ProductLabel(C->Spec.MonsterSku)),
				FLinearColor(1.f, 0.82f, 0.45f), RX + 14.f, RY, Font, 0.95f);
			RY += 22.f;
			DrawText(TEXT("-> Deliver it to the Checkout Register!"), FLinearColor(0.95f, 0.90f, 0.85f), RX + 14.f, RY, Font, 0.95f);
			RY += 26.f;
		}
		else if (C->State == ECustomerState::WaitingForHelp)
		{
			DrawText(FString::Printf(TEXT("* Customer Help: Bring [%s]"), *ShiftData::ProductLabel(C->Spec.WantedSku)),
				FLinearColor(1.f, 0.88f, 0.40f), RX + 14.f, RY, Font, 0.98f);
			RY += 26.f;
		}
		else if (C->State == ECustomerState::WaitingAtCheckout)
		{
			DrawText(TEXT("* Customer waiting at Checkout Register [E]"),
				FLinearColor(0.45f, 1.f, 0.65f), RX + 14.f, RY, Font, 0.98f);
			RY += 26.f;
		}
		else
		{
			DrawText(TEXT("* Customer in store (Watch them closely)"),
				FLinearColor(0.80f, 0.92f, 0.88f), RX + 14.f, RY, Font, 0.95f);
			RY += 26.f;
		}
	}

	if (GM->Phase == EShiftPhase::Cleanup)
	{
		DrawText(TEXT("DAWN CLEANUP (Equip Mop in Back Corridor):"), FLinearColor(0.95f, 0.88f, 0.45f), RX + 14.f, RY, Font, 0.95f);
		RY += 24.f;
	}

	for (const TWeakObjectPtr<AShiftTaskTarget>& TP : GM->ActiveTasks)
	{
		if (const AShiftTaskTarget* T = TP.Get())
		{
			const bool bOver = T->IsOverdue();
			const FString TimeStr = (T->Deadline > 0.f)
				? (bOver ? TEXT("OVERDUE! (-Sanity)") : FString::Printf(TEXT("%ds"), FMath::CeilToInt(T->TimeLeft)))
				: TEXT("Required");
			DrawText(FString::Printf(TEXT("- %s  [%s]"), *T->Label, *TimeStr),
				bOver ? FLinearColor(1.f, 0.30f, 0.25f) : FLinearColor(0.86f, 0.90f, 0.88f),
				RX + 14.f, RY, Font, 0.92f);
			RY += 22.f;
		}
	}

	// Status banner (center top)
	if (GM->BannerTimer > 0.f && !GM->BannerText.IsEmpty())
	{
		DrawRect(FLinearColor(0.05f, 0.02f, 0.02f, 0.82f), W * 0.25f, H * 0.14f, W * 0.50f, 38.f);
		DrawCenteredText(GM->BannerText, H * 0.14f + 8.f, FLinearColor(1.f, 0.85f, 0.45f), 1.15f);
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
}
