#include "ShiftDirector.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"

AShiftDirector::AShiftDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

FVector AShiftDirector::OffsetFor(const FVector& P)
{
	// Footprints of Gondola_0_1 (800,450) and Gondola_1_1 (800,750); see generate_store.py.
	if (FMath::Abs(P.X - 800.f) > 125.f || P.Z < -1.f || P.Z > 200.f) { return FVector::ZeroVector; }
	if (FMath::Abs(P.Y - 450.f) <= 60.f) { return FVector(30.f, 0.f, 0.f); }
	if (FMath::Abs(P.Y - 750.f) <= 60.f) { return FVector(-30.f, 0.f, 0.f); }
	return FVector::ZeroVector;
}

void AShiftDirector::BeginPlay()
{
	Super::BeginPlay();
	WelcomeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ShiftStore/Meshes/SM_Sign_Welcome.SM_Sign_Welcome"));
	NoticeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ShiftStore/Meshes/SM_Sign_ShiftNotice.SM_Sign_ShiftNotice"));
	DarkLens = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShiftStore/Materials/M_Charcoal.M_Charcoal"));
	Collect();
}

void AShiftDirector::Collect()
{
	UStaticMesh* FixtureMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ShiftStore/Meshes/SM_LightFixture.SM_LightFixture"));
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (A == this || A->ActorHasTag(TEXT("Shift.Item"))) { continue; }

		// Repeated display stock: move individual instances that sit on a moving gondola.
		TArray<UInstancedStaticMeshComponent*> Instanced;
		A->GetComponents(Instanced);
		for (UInstancedStaticMeshComponent* ISM : Instanced)
		{
			bool bMadeMovable = false;
			for (int32 i = 0; i < ISM->GetInstanceCount(); ++i)
			{
				FTransform T;
				ISM->GetInstanceTransform(i, T, true);
				const FVector Off = OffsetFor(T.GetLocation());
				if (Off.IsZero()) { continue; }
				if (!bMadeMovable) { ISM->SetMobility(EComponentMobility::Movable); bMadeMovable = true; }
				MovedInstances.Add({ ISM, i, T, Off });
			}
		}
		if (Instanced.Num() > 0) { continue; }

		if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(A))
		{
			UStaticMeshComponent* C = SMA->GetStaticMeshComponent();
			const UStaticMesh* SM = C ? C->GetStaticMesh() : nullptr;
			if (SM && SM == FixtureMesh && C->GetNumMaterials() > 1)
			{
				Fixtures.Add(C);
				FixtureNormalMats.Add(C->GetMaterial(1));
			}
			if (SM && SM == WelcomeMesh)
			{
				C->SetMobility(EComponentMobility::Movable);
				WelcomeSign = C;
			}
		}
		if (APostProcessVolume* PPV = Cast<APostProcessVolume>(A)) { PostProcess = PPV; }

		TArray<URectLightComponent*> Rects;
		A->GetComponents(Rects);
		for (URectLightComponent* L : Rects)
		{
			const FVector P = L->GetComponentLocation();
			const bool bMain = P.Y < 1050.f;          // sales floor main lights
			// Moody 2:30 AM convenience store: dimmer rear corners, sickly vintage fluorescent warmth
			const float CornerDim = (bMain && (P.Y > 750.f || P.X < 450.f)) ? 0.56f : 0.72f;
			const float NormalInt = bMain ? (L->Intensity * CornerDim) : (L->Intensity * 0.55f);
			const float ShiftInt = bMain ? 22.f : 65.f;
			const FLinearColor NormCol = bMain ? FLinearColor(0.92f, 0.86f, 0.72f, 1.f) : FLinearColor(0.76f, 0.80f, 0.70f, 1.f);
			const FLinearColor ShiftCol = bMain ? FLinearColor(0.65f, 0.14f, 0.12f, 1.f) : FLinearColor(0.85f, 0.12f, 0.10f, 1.f);
			Lights.Add({ L, NormalInt, ShiftInt, NormCol, ShiftCol });
		}

		TArray<UPointLightComponent*> Points;
		A->GetComponents(Points);
		for (UPointLightComponent* PL : Points)
		{
			if (A->IsA<APawn>() || A->ActorHasTag(TEXT("Shift.Item"))) { continue; }
			const FString Name = A->GetName();
			if (Name.Contains(TEXT("ColdLamp")) || Name.Contains(TEXT("DrinksLamp")))
			{
				// Stark clinical icy-blue refrigerators cutting against warm fluorescent aisles
				AccentLights.Add({ PL, 185.f, 220.f, FLinearColor(0.32f, 0.72f, 0.96f, 1.f), FLinearColor(0.18f, 0.55f, 0.88f, 1.f) });
			}
			else if (Name.Contains(TEXT("Emergency")))
			{
				// Lonely warm amber counter/corridor glow in Normal -> deep blood-crimson horror glow in Shift
				AccentLights.Add({ PL, 130.f, 420.f, FLinearColor(0.98f, 0.58f, 0.24f, 1.f), FLinearColor(0.95f, 0.08f, 0.06f, 1.f) });
			}
		}

		if (Cast<AStaticMeshActor>(A) || Cast<ATextRenderActor>(A))
		{
			const FVector Off = OffsetFor(A->GetActorLocation());
			if (!Off.IsZero() && A->GetRootComponent())
			{
				A->GetRootComponent()->SetMobility(EComponentMobility::Movable);
				Moved.Add({ A, A->GetActorLocation(), Off });
			}
		}
	}

	// Spawn or reuse atmospheric height fog so the convenience store has genuine horror depth & dread
	AExponentialHeightFog* FogActor = nullptr;
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It) { FogActor = *It; break; }
	if (!FogActor)
	{
		FogActor = GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(800.f, 800.f, 100.f), FRotator::ZeroRotator);
	}
	if (FogActor && FogActor->GetComponent())
	{
		HorrorFog = FogActor->GetComponent();
		HorrorFog->SetFogHeightFalloff(0.15f);
		HorrorFog->SetFogMaxOpacity(0.82f);
	}

	Apply(0.f);
	UE_LOG(LogTemp, Log, TEXT("ShiftDirector: %d actors, %d instances, %d rect lights, %d accent lights, %d fixtures"), Moved.Num(), MovedInstances.Num(), Lights.Num(), AccentLights.Num(), Fixtures.Num());
}

void AShiftDirector::SetShifted(bool bInShift)
{
	bShift = bInShift;
	if (!bInShift)
	{
		BreakerOverrideLeft = 0.f;
		BreakerCooldownLeft = 0.f;
	}
	ClearMicroShifts();
}

bool AShiftDirector::TriggerBreakerOverride(float Duration)
{
	if (!bShift || BreakerOverrideLeft > 0.f || BreakerCooldownLeft > 0.f) { return false; }
	BreakerOverrideLeft = Duration;
	BreakerCooldownLeft = Duration + 20.f;
	Flicker(0.35f);
	return true;
}

void AShiftDirector::SetMicroShift(int32 AnomalyIndex, bool bActive)
{
	if (AnomalyIndex == 0) { bMicroSignWarped = bActive; }
	else if (AnomalyIndex == 1) { bMicroGondolaShifted = bActive; }
	if (!bActive) { FlickerLeft = 0.f; }
	Apply(CurrentAlpha);
}

void AShiftDirector::ClearMicroShifts()
{
	if (bMicroSignWarped || bMicroGondolaShifted)
	{
		bMicroSignWarped = false;
		bMicroGondolaShifted = false;
		AppliedAlpha = -1.f;
	}
}

void AShiftDirector::Flicker(float Duration)
{
	FlickerLeft = Duration;
}

void AShiftDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (BreakerCooldownLeft > 0.f) { BreakerCooldownLeft = FMath::Max(0.f, BreakerCooldownLeft - DeltaSeconds); }
	if (BreakerOverrideLeft > 0.f)
	{
		BreakerOverrideLeft = FMath::Max(0.f, BreakerOverrideLeft - DeltaSeconds);
		if (BreakerOverrideLeft == 0.f && bShift) { Flicker(0.45f); }
	}
	const float Target = (bShift && BreakerOverrideLeft <= 0.f) ? 1.f : 0.f;
	CurrentAlpha = FMath::FInterpConstantTo(CurrentAlpha, Target, DeltaSeconds, Target > CurrentAlpha ? 0.8f : 0.65f);
	const bool bWasFlickering = FlickerLeft > 0.f;
	FlickerLeft = FMath::Max(0.f, FlickerLeft - DeltaSeconds);

	const bool bTransition = CurrentAlpha > 0.f && CurrentAlpha < 1.f;
	if (bTransition || FlickerLeft > 0.f || (bWasFlickering && FlickerLeft == 0.f) || AppliedAlpha != CurrentAlpha)
	{
		Apply(CurrentAlpha);
	}
}

bool AShiftDirector::ValidateState(bool bExpectedShifted, FString& OutError) const
{
	const float ExpectedAlpha = bExpectedShifted ? 1.f : 0.f;
	if (!FMath::IsNearlyEqual(CurrentAlpha, ExpectedAlpha) || !FMath::IsNearlyEqual(AppliedAlpha, ExpectedAlpha) || FlickerLeft > 0.f)
	{
		OutError = TEXT("The transition or flicker has not finished");
		return false;
	}
	if (Moved.IsEmpty() || MovedInstances.IsEmpty() || Lights.IsEmpty() || Fixtures.IsEmpty() || !WelcomeSign.IsValid())
	{
		OutError = TEXT("The scene is missing moving actors, stock, lighting or signage");
		return false;
	}
	for (const FMovedActor& M : Moved)
	{
		const AActor* Actor = M.Actor.Get();
		if (!Actor || !Actor->GetActorLocation().Equals(M.Base + M.Offset * ExpectedAlpha, 0.1f))
		{
			OutError = TEXT("A moving actor has the wrong position");
			return false;
		}
	}
	for (const FMovedInstance& M : MovedInstances)
	{
		const UInstancedStaticMeshComponent* C = M.Comp.Get();
		FTransform Actual;
		if (!C || !C->GetInstanceTransform(M.Index, Actual, true) || !Actual.GetLocation().Equals(M.Base.GetLocation() + M.Offset * ExpectedAlpha, 0.1f))
		{
			OutError = TEXT("A stock instance has the wrong position");
			return false;
		}
	}
	for (const FLightState& L : Lights)
	{
		const ULightComponent* C = L.Light.Get();
		if (!C || !FMath::IsNearlyEqual(C->Intensity, bExpectedShifted ? L.Shift : L.Normal, 0.1f))
		{
			OutError = TEXT("A light has the wrong intensity");
			return false;
		}
	}
	for (int32 i = 0; i < Fixtures.Num(); ++i)
	{
		const UStaticMeshComponent* C = Fixtures[i].Get();
		if (!C || C->GetMaterial(1) != (bExpectedShifted ? DarkLens.Get() : FixtureNormalMats[i].Get()))
		{
			OutError = TEXT("A fixture has the wrong lens material");
			return false;
		}
	}
	if (WelcomeSign->GetStaticMesh() != (bExpectedShifted ? NoticeMesh.Get() : WelcomeMesh.Get()))
	{
		OutError = TEXT("The welcome sign has the wrong mesh");
		return false;
	}
	const APostProcessVolume* PPV = PostProcess.Get();
	if (!PPV || !FMath::IsNearlyEqual(PPV->Settings.CameraISO, bExpectedShifted ? 1600.f : 100.f, 0.1f))
	{
		OutError = TEXT("The scene has the wrong exposure");
		return false;
	}
	OutError.Reset();
	return true;
}

void AShiftDirector::Apply(float A)
{
	AppliedAlpha = A;
	const float Smooth = FMath::SmoothStep(0.f, 1.f, A);

	for (const FMovedActor& M : Moved)
	{
		if (AActor* Actor = M.Actor.Get())
		{
			const float LocalAlpha = (!bShift && bMicroGondolaShifted && M.Base.Y < 600.f) ? 0.65f : Smooth;
			Actor->SetActorLocation(M.Base + M.Offset * LocalAlpha);
		}
	}
	TSet<UInstancedStaticMeshComponent*> Dirty;
	for (const FMovedInstance& M : MovedInstances)
	{
		if (UInstancedStaticMeshComponent* C = M.Comp.Get())
		{
			const float LocalAlpha = (!bShift && bMicroGondolaShifted && M.Base.GetLocation().Y < 600.f) ? 0.65f : Smooth;
			FTransform T = M.Base;
			T.AddToTranslation(M.Offset * LocalAlpha);
			C->UpdateInstanceTransform(M.Index, T, true, false, true);
			Dirty.Add(C);
		}
	}
	for (UInstancedStaticMeshComponent* C : Dirty) { C->MarkRenderStateDirty(); }

	// Lights stutter while changing state, or during a scripted flicker.
	const bool bStutter = (A > 0.02f && A < 0.98f) || FlickerLeft > 0.f;
	const float Stutter = bStutter ? (FMath::FRand() < 0.35f ? 0.05f : 1.f) : 1.f;
	for (const FLightState& L : Lights)
	{
		if (ULightComponent* C = L.Light.Get())
		{
			C->SetIntensity(FMath::Lerp(L.Normal, L.Shift, Smooth) * Stutter);
			C->SetLightColor(FMath::Lerp(L.NormalColor, L.ShiftColor, Smooth));
		}
	}
	for (const FAccentLightState& AL : AccentLights)
	{
		if (UPointLightComponent* C = AL.Light.Get())
		{
			C->SetIntensity(FMath::Lerp(AL.Normal, AL.Shift, Smooth));
			C->SetLightColor(FMath::Lerp(AL.NormalColor, AL.ShiftColor, Smooth));
		}
	}
	if (UExponentialHeightFogComponent* Fog = HorrorFog.Get())
	{
		Fog->SetFogDensity(FMath::Lerp(0.014f, 0.052f, Smooth));
		Fog->SetStartDistance(FMath::Lerp(260.f, 55.f, Smooth));
		Fog->SetFogInscatteringColor(FMath::Lerp(FLinearColor(0.032f, 0.034f, 0.030f, 1.f), FLinearColor(0.055f, 0.008f, 0.008f, 1.f), Smooth));
	}
	const bool bDark = Smooth > 0.5f;
	for (int32 i = 0; i < Fixtures.Num(); ++i)
	{
		if (UStaticMeshComponent* C = Fixtures[i].Get()) { C->SetMaterial(1, bDark || Stutter < 1.f ? DarkLens.Get() : FixtureNormalMats[i].Get()); }
	}
	if (UStaticMeshComponent* Sign = WelcomeSign.Get())
	{
		Sign->SetStaticMesh((bDark || bMicroSignWarped) ? NoticeMesh.Get() : WelcomeMesh.Get());
	}
	if (APostProcessVolume* PPV = PostProcess.Get())
	{
		PPV->Settings.CameraISO = FMath::Lerp(100.f, 1600.f, Smooth);
		PPV->Settings.CameraShutterSpeed = FMath::Lerp(60.f, 30.f, Smooth);
		PPV->Settings.bOverride_ColorSaturation = true;
		PPV->Settings.ColorSaturation = FMath::Lerp(FVector4(0.72f, 0.70f, 0.68f, 1.f), FVector4(0.62f, 0.44f, 0.44f, 1.f), Smooth);
		PPV->Settings.bOverride_ColorContrast = true;
		PPV->Settings.ColorContrast = FMath::Lerp(FVector4(1.16f, 1.15f, 1.18f, 1.f), FVector4(1.32f, 1.26f, 1.28f, 1.f), Smooth);
		PPV->Settings.bOverride_ColorGamma = true;
		PPV->Settings.ColorGamma = FMath::Lerp(FVector4(0.93f, 0.92f, 0.89f, 1.f), FVector4(0.86f, 0.81f, 0.81f, 1.f), Smooth);
		PPV->Settings.bOverride_BloomIntensity = true;
		PPV->Settings.BloomIntensity = FMath::Lerp(0.80f, 1.55f, Smooth);
		PPV->Settings.bOverride_SceneFringeIntensity = true;
		PPV->Settings.SceneFringeIntensity = FMath::Lerp(0.35f, 3.0f, Smooth);
		PPV->Settings.bOverride_FilmGrainIntensity = true;
		PPV->Settings.FilmGrainIntensity = FMath::Lerp(0.14f, 0.45f, Smooth);
		PPV->Settings.bOverride_VignetteIntensity = true;
		PPV->Settings.VignetteIntensity = FMath::Lerp(0.45f, 0.82f, Smooth);
	}
}
