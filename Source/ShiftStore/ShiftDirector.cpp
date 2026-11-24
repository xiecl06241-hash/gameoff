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
#include "Components/DecalComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AShiftDirector::AShiftDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMat(TEXT("/Game/ShiftStore/Materials/M_OtherworldWall.M_OtherworldWall"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FloorMat(TEXT("/Game/ShiftStore/Materials/M_OtherworldFloor.M_OtherworldFloor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CeilMat(TEXT("/Game/ShiftStore/Materials/M_OtherworldCeiling.M_OtherworldCeiling"));
	if (WallMat.Succeeded()) { OtherworldWallMat = WallMat.Object; }
	if (FloorMat.Succeeded()) { OtherworldFloorMat = FloorMat.Object; }
	if (CeilMat.Succeeded()) { OtherworldCeilingMat = CeilMat.Object; }
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

		// Repeated display stock: move individual instances that sit on a moving gondola,
		// and hide modern store merchandise when shifting into the ancient Chinese ancestral hall Otherworld.
		TArray<UInstancedStaticMeshComponent*> Instanced;
		A->GetComponents(Instanced);
		for (UInstancedStaticMeshComponent* ISM : Instanced)
		{
			NormalOnlyISMs.Add(ISM);
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

		if (ATextRenderActor* TRA = Cast<ATextRenderActor>(A))
		{
			// Hide all modern store text signs (CHECKOUT, AISLE 01-03, Tag_* prices) inside the Chinese ancestral hall!
			NormalOnlyActors.Add(TRA);
		}

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
			// Collect architectural shell for material swap vs modern convenience-store fixtures to hide in Otherworld!
			const FString AName = A->GetName();
			const bool bIsModernFixture =
				AName.Contains(TEXT("Gondola_")) || AName.Contains(TEXT("WallShelf"))
				|| AName.Contains(TEXT("FloorBrand")) || AName.Contains(TEXT("FloorCarton"))
				|| AName.Contains(TEXT("Fridge")) || AName.Contains(TEXT("Drinks"))
				|| AName.Contains(TEXT("Counter")) || AName.Contains(TEXT("Register"))
				|| AName.Contains(TEXT("Scanner")) || AName.Contains(TEXT("Oden"))
				|| AName.Contains(TEXT("Checkout")) || AName.Contains(TEXT("Coffee"))
				|| AName.Contains(TEXT("HotCase")) || AName.Contains(TEXT("Display"))
				|| AName.Contains(TEXT("Aisle")) || AName.Contains(TEXT("Price"))
				|| AName.Contains(TEXT("Tag_")) || AName.Contains(TEXT("Hero_"))
				|| AName.Contains(TEXT("Extinguisher")) || AName.Contains(TEXT("FrontGlass"))
				|| AName.Contains(TEXT("FrontBase")) || AName.Contains(TEXT("ExitSign"))
				|| AName.Contains(TEXT("OfficeSign")) || AName.Contains(TEXT("RestroomSign"))
				|| AName.Contains(TEXT("WarehouseSign")) || AName.Contains(TEXT("ExteriorSign"))
				|| AName.Contains(TEXT("Shelf")) || AName.Contains(TEXT("Bin"))
				|| AName.Contains(TEXT("Poster")) || AName.Contains(TEXT("Banner"))
				|| AName.Contains(TEXT("StockRack")) || AName.Contains(TEXT("OfficeComputer"))
				|| AName.Contains(TEXT("OfficeChair")) || AName.Contains(TEXT("StaffLocker"))
				|| AName.Contains(TEXT("RestroomSink")) || AName.Contains(TEXT("Toilet"))
				|| AName.Contains(TEXT("Mirror")) || AName.Contains(TEXT("Bucket"))
				|| AName.Contains(TEXT("Broom")) || AName.Contains(TEXT("Mop"));

			if (bIsModernFixture && (!SM || SM != WelcomeMesh) && (!SM || SM != FixtureMesh))
			{
				NormalOnlyActors.Add(A);
			}
			else if (C)
			{
				UMaterialInterface* ShiftMat = nullptr;
				if (AName.Contains(TEXT("Floor_")) && OtherworldFloorMat) { ShiftMat = OtherworldFloorMat; }
				else if ((AName.Contains(TEXT("Ceiling_")) || AName.Contains(TEXT("CeilingRail"))) && OtherworldCeilingMat) { ShiftMat = OtherworldCeilingMat; }
				else if ((AName.Contains(TEXT("Wall")) || AName.Contains(TEXT("WindowMullion"))) && OtherworldWallMat) { ShiftMat = OtherworldWallMat; }
				if (ShiftMat)
				{
					for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot)
					{
						SwappedSurfaces.Add({ C, Slot, C->GetMaterial(Slot), ShiftMat });
					}
				}
			}
		}
		if (APostProcessVolume* PPV = Cast<APostProcessVolume>(A)) { PostProcess = PPV; }

		TArray<URectLightComponent*> Rects;
		A->GetComponents(Rects);
		for (URectLightComponent* L : Rects)
		{
			const FVector P = L->GetComponentLocation();
			const bool bMain = P.Y < 1050.f;          // sales floor main lights
			// Crisp, bright, clean modern 24h convenience store in Normal -> eerie ghostly cyan-green dim light in Otherworld!
			const float NormalInt = bMain ? (L->Intensity * 0.96f) : (L->Intensity * 0.72f);
			const float ShiftInt = bMain ? 72.f : 88.f;
			const FLinearColor NormCol = bMain ? FLinearColor(0.96f, 0.97f, 0.99f, 1.f) : FLinearColor(0.90f, 0.92f, 0.88f, 1.f);
			const FLinearColor ShiftCol = bMain ? FLinearColor(0.12f, 0.72f, 0.44f, 1.f) : FLinearColor(0.55f, 0.52f, 0.12f, 1.f);
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

	SpawnChineseHorrorCrimeScene();
	Apply(0.f);
	UE_LOG(LogTemp, Log, TEXT("ShiftDirector: %d actors, %d instances, %d rect lights, %d accent lights, %d fixtures, %d crime props"),
		Moved.Num(), MovedInstances.Num(), Lights.Num(), AccentLights.Num(), Fixtures.Num(), CrimeSceneActors.Num());
}

void AShiftDirector::SpawnChineseHorrorCrimeScene()
{
	UWorld* W = GetWorld();
	if (!W) { return; }

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* SpillBaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ShiftStore/Materials/M_FloorSpill.M_FloorSpill"));
	UTexture2D* IchorD = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Ichor_D.T_Spill_Ichor_D"));
	UTexture2D* IchorN = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Ichor_N.T_Spill_Ichor_N"));
	UTexture2D* IchorM = LoadObject<UTexture2D>(nullptr, TEXT("/Game/ShiftStore/Textures/Spills/T_Spill_Ichor_M.T_Spill_Ichor_M"));
	UMaterialInterface* LitBaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	auto SpawnColoredProp = [&](const FVector& Loc, const FRotator& Rot, const FVector& Scale, UStaticMesh* Mesh, const FLinearColor& Color, float EmissiveBoost = 0.f) -> AActor*
	{
		if (!Mesh) { return nullptr; }
		AStaticMeshActor* Prop = W->SpawnActor<AStaticMeshActor>(Loc, Rot);
		if (!Prop) { return nullptr; }
		UStaticMeshComponent* Comp = Prop->GetStaticMeshComponent();
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(Mesh);
		Comp->SetWorldScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		if (LitBaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(LitBaseMat, Prop))
			{
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(Color.R * (1.f + EmissiveBoost), Color.G * (1.f + EmissiveBoost), Color.B * (1.f + EmissiveBoost), 1.f));
				Comp->SetMaterial(0, MID);
			}
		}
		Prop->SetActorHiddenInGame(true);
		CrimeSceneActors.Add(Prop);
		return Prop;
	};

	// 1. Floor PBR bloodstain & drag-trail decals (suggests an old murder scene without spelling out details)
	if (SpillBaseMat && IchorD && IchorN && IchorM)
	{
		struct FStainSpec { FVector Pos; float SizeX; float SizeY; float Yaw; FLinearColor Tint; };
		const FStainSpec Stains[] = {
			{ FVector(800.f, 560.f, 4.f),  135.f, 115.f,  25.f, FLinearColor(0.55f, 0.04f, 0.04f, 1.f) }, // Main aisle incident pool
			{ FVector(805.f, 710.f, 4.f),   95.f,  55.f,  92.f, FLinearColor(0.48f, 0.03f, 0.03f, 1.f) }, // Drag trail #1
			{ FVector(795.f, 845.f, 4.f),   88.f,  48.f,  86.f, FLinearColor(0.45f, 0.03f, 0.03f, 1.f) }, // Drag trail #2 toward backroom
			{ FVector(1265.f, 220.f, 4.f),  82.f,  74.f, 140.f, FLinearColor(0.52f, 0.04f, 0.04f, 1.f) }, // Behind checkout counter
		};
		for (const FStainSpec& S : Stains)
		{
			AActor* Holder = W->SpawnActor<AActor>(S.Pos, FRotator::ZeroRotator);
			if (!Holder) { continue; }
			UDecalComponent* Decal = NewObject<UDecalComponent>(Holder);
			Decal->RegisterComponent();
			Holder->SetRootComponent(Decal);
			Decal->SetWorldLocation(S.Pos);
			Decal->SetWorldRotation(FRotator(-90.f, S.Yaw, 0.f));
			Decal->DecalSize = FVector(32.f, S.SizeY, S.SizeX);
			Decal->FadeScreenSize = 0.001f;
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(SpillBaseMat, Holder))
			{
				MID->SetTextureParameterValue(TEXT("AlbedoTex"), IchorD);
				MID->SetTextureParameterValue(TEXT("NormalTex"), IchorN);
				MID->SetTextureParameterValue(TEXT("MaskTex"), IchorM);
				MID->SetVectorParameterValue(TEXT("TintColor"), S.Tint);
				MID->SetScalarParameterValue(TEXT("WetnessScale"), 0.85f);
				MID->SetScalarParameterValue(TEXT("CleanProgress"), 0.f);
				Decal->SetDecalMaterial(MID);
			}
			Holder->SetActorHiddenInGame(true);
			CrimeSceneActors.Add(Holder);
		}
	}

	// 2. Full forensic chalk body outline around the center aisle stain (head + torso + outstretched limbs)
	const FLinearColor ChalkCol(0.90f, 0.88f, 0.82f, 1.f);
	SpawnColoredProp(FVector(782.f, 520.f, 1.8f), FRotator(0.f, 10.f, 0.f),  FVector(0.24f, 0.24f, 0.008f), SphereMesh, ChalkCol); // head outline
	SpawnColoredProp(FVector(790.f, 558.f, 1.8f), FRotator(0.f, 14.f, 0.f),  FVector(0.72f, 0.032f, 0.008f), CubeMesh, ChalkCol);  // torso left
	SpawnColoredProp(FVector(818.f, 554.f, 1.8f), FRotator(0.f, 14.f, 0.f),  FVector(0.72f, 0.032f, 0.008f), CubeMesh, ChalkCol);  // torso right
	SpawnColoredProp(FVector(758.f, 546.f, 1.8f), FRotator(0.f, 65.f, 0.f),  FVector(0.58f, 0.028f, 0.008f), CubeMesh, ChalkCol);  // left arm
	SpawnColoredProp(FVector(842.f, 536.f, 1.8f), FRotator(0.f, -55.f, 0.f), FVector(0.56f, 0.028f, 0.008f), CubeMesh, ChalkCol);  // right arm
	SpawnColoredProp(FVector(786.f, 616.f, 1.8f), FRotator(0.f, 22.f, 0.f),  FVector(0.75f, 0.028f, 0.008f), CubeMesh, ChalkCol);  // left leg
	SpawnColoredProp(FVector(826.f, 614.f, 1.8f), FRotator(0.f, -18.f, 0.f), FVector(0.75f, 0.028f, 0.008f), CubeMesh, ChalkCol);  // right leg

	// 2b. Trail of 10 dried bloody footprints leading from behind the checkout counter -> main aisle -> backroom door
	const FLinearColor BloodStepCol(0.42f, 0.03f, 0.03f, 1.f);
	const FVector Footprints[] = {
		FVector(1220.f, 255.f, 1.6f), FVector(1145.f, 305.f, 1.6f), FVector(1065.f, 355.f, 1.6f),
		FVector(980.f,  405.f, 1.6f), FVector(900.f,  460.f, 1.6f), FVector(835.f,  525.f, 1.6f),
		FVector(808.f,  660.f, 1.6f), FVector(792.f,  740.f, 1.6f), FVector(812.f,  830.f, 1.6f),
		FVector(800.f,  935.f, 1.6f)
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(Footprints); ++i)
	{
		const float StepYaw = (i < 6) ? 145.f : 90.f;
		SpawnColoredProp(Footprints[i], FRotator(0.f, StepYaw + ((i % 2 == 0) ? -12.f : 12.f), 0.f), FVector(0.24f, 0.11f, 0.006f), CubeMesh, BloodStepCol);
	}

	// 3. Entrance crimson funerary drapes + weathered yellow seal tape + Chinese vermilion paper talismans (黄符 / 封条)
	const FLinearColor SealYellow(0.86f, 0.66f, 0.14f, 1.f);
	const FLinearColor SealRed(0.78f, 0.08f, 0.06f, 1.f);
	const FLinearColor DrapeCrimson(0.36f, 0.03f, 0.03f, 1.f);
	// Crimson drapes hanging inside the front glass doorway (visual dread, zero collision)
	SpawnColoredProp(FVector(720.f, 52.f, 175.f), FRotator(0.f, 0.f, 2.f),  FVector(0.85f, 0.03f, 2.45f), CubeMesh, DrapeCrimson);
	SpawnColoredProp(FVector(880.f, 52.f, 175.f), FRotator(0.f, 0.f, -2.f), FVector(0.85f, 0.03f, 2.45f), CubeMesh, DrapeCrimson);
	// Crossed seal strips across entrance & middle aisle
	SpawnColoredProp(FVector(800.f, 65.f, 130.f),  FRotator(10.f, 0.f, 0.f), FVector(2.3f, 0.015f, 0.08f), CubeMesh, SealYellow);
	SpawnColoredProp(FVector(800.f, 65.f, 112.f),  FRotator(-9.f, 0.f, 0.f), FVector(2.3f, 0.015f, 0.08f), CubeMesh, SealYellow);
	SpawnColoredProp(FVector(800.f, 445.f, 118.f), FRotator(8.f, 0.f, 0.f),  FVector(2.1f, 0.015f, 0.07f), CubeMesh, SealYellow);
	SpawnColoredProp(FVector(800.f, 445.f, 102.f), FRotator(-6.f, 0.f, 0.f), FVector(2.1f, 0.015f, 0.07f), CubeMesh, SealYellow);

	// Paper talismans affixed across counter wall, coolers, and aisles
	const FVector TalismanSpots[] = {
		FVector(1220.f, 125.f, 160.f), FVector(1290.f, 125.f, 172.f), FVector(1355.f, 125.f, 152.f),
		FVector(420.f,  950.f, 165.f), FVector(320.f,  950.f, 175.f), FVector(520.f,  950.f, 155.f),
		FVector(690.f,  450.f, 165.f), FVector(910.f,  450.f, 162.f), FVector(690.f,  750.f, 168.f),
		FVector(910.f,  750.f, 160.f), FVector(800.f, 1010.f, 195.f), FVector(855.f, 1010.f, 185.f)
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(TalismanSpots); ++i)
	{
		const float Tilt = (i % 2 == 0) ? 4.f : -5.f;
		SpawnColoredProp(TalismanSpots[i], FRotator(0.f, 0.f, Tilt), FVector(0.11f, 0.008f, 0.36f), CubeMesh, SealYellow);
		SpawnColoredProp(TalismanSpots[i] + FVector(0.f, 0.8f, 0.f), FRotator(0.f, 0.f, Tilt), FVector(0.038f, 0.01f, 0.24f), CubeMesh, SealRed);
	}

	// 4. Six hanging crimson Chinese paper lanterns with tassels casting vivid vermilion pools against the ghostly green ceiling lights
	const FVector LanternPositions[] = {
		FVector(800.f,  210.f, 255.f),
		FVector(800.f,  580.f, 250.f),
		FVector(800.f,  920.f, 255.f),
		FVector(1240.f, 260.f, 255.f),
		FVector(410.f,  580.f, 255.f),
		FVector(1190.f, 620.f, 255.f),
	};
	for (const FVector& LP : LanternPositions)
	{
		if (AActor* Lantern = SpawnColoredProp(LP, FRotator::ZeroRotator, FVector(0.36f, 0.36f, 0.46f), SphereMesh, FLinearColor(0.96f, 0.08f, 0.05f, 1.f), 2.2f))
		{
			UPointLightComponent* Light = NewObject<UPointLightComponent>(Lantern);
			Light->RegisterComponent();
			Light->AttachToComponent(Lantern->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			Light->SetRelativeLocation(FVector(0.f, 0.f, -38.f));
			Light->SetIntensityUnits(ELightUnits::Lumens);
			Light->SetIntensity(0.f);
			Light->SetLightColor(FLinearColor(1.0f, 0.12f, 0.06f, 1.f));
			Light->SetAttenuationRadius(720.f);
			Light->SetCastShadows(false);
			CrimeLanternLights.Add(Light);
		}
		// Top & bottom dark wooden lantern caps + hanging red tassel
		SpawnColoredProp(LP + FVector(0.f, 0.f, 23.f),  FRotator::ZeroRotator, FVector(0.18f, 0.18f, 0.05f), CubeMesh, FLinearColor(0.05f, 0.03f, 0.02f, 1.f));
		SpawnColoredProp(LP + FVector(0.f, 0.f, -23.f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 0.05f), CubeMesh, FLinearColor(0.05f, 0.03f, 0.02f, 1.f));
		SpawnColoredProp(LP + FVector(0.f, 0.f, -38.f), FRotator::ZeroRotator, FVector(0.035f, 0.035f, 0.28f), CubeMesh, SealRed, 0.6f);
	}

	// 5. Large wall blood-splatter decals projected directly onto the decayed Otherworld walls
	if (SpillBaseMat && IchorD && IchorN && IchorM)
	{
		struct FWallStain { FVector Pos; FRotator Rot; float SizeY; float SizeZ; };
		const FWallStain WallStains[] = {
			{ FVector(12.f,   560.f, 150.f), FRotator(0.f, 0.f,   0.f), 140.f, 130.f }, // West wall
			{ FVector(1588.f, 520.f, 145.f), FRotator(0.f, 180.f, 0.f), 140.f, 130.f }, // East wall
			{ FVector(1280.f, 128.f, 150.f), FRotator(0.f, 90.f,  0.f), 120.f, 110.f }, // Behind counter
			{ FVector(800.f, 1005.f, 155.f), FRotator(0.f, -90.f, 0.f), 135.f, 120.f }, // Rear corridor wall
		};
		for (const FWallStain& WS : WallStains)
		{
			AActor* Holder = W->SpawnActor<AActor>(WS.Pos, FRotator::ZeroRotator);
			if (!Holder) { continue; }
			UDecalComponent* Decal = NewObject<UDecalComponent>(Holder);
			Decal->RegisterComponent();
			Holder->SetRootComponent(Decal);
			Decal->SetWorldLocation(WS.Pos);
			Decal->SetWorldRotation(WS.Rot);
			Decal->DecalSize = FVector(45.f, WS.SizeY, WS.SizeZ);
			Decal->FadeScreenSize = 0.001f;
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(SpillBaseMat, Holder))
			{
				MID->SetTextureParameterValue(TEXT("AlbedoTex"), IchorD);
				MID->SetTextureParameterValue(TEXT("NormalTex"), IchorN);
				MID->SetTextureParameterValue(TEXT("MaskTex"), IchorM);
				MID->SetVectorParameterValue(TEXT("TintColor"), FLinearColor(0.58f, 0.03f, 0.03f, 1.f));
				MID->SetScalarParameterValue(TEXT("WetnessScale"), 0.95f);
				MID->SetScalarParameterValue(TEXT("CleanProgress"), 0.f);
				Decal->SetDecalMaterial(MID);
			}
			Holder->SetActorHiddenInGame(true);
			CrimeSceneActors.Add(Holder);
		}
	}

	// 6. Overhead hanging funeral white/yellow spirit banners (招魂白幡) along the central aisle (no collision)
	const FLinearColor FuneralCloth(0.76f, 0.72f, 0.62f, 1.f);
	const FVector BannerSpots[] = {
		FVector(735.f, 320.f, 225.f), FVector(865.f, 320.f, 222.f),
		FVector(735.f, 520.f, 224.f), FVector(865.f, 520.f, 220.f),
		FVector(735.f, 740.f, 225.f), FVector(865.f, 740.f, 221.f),
	};
	for (int32 i = 0; i < UE_ARRAY_COUNT(BannerSpots); ++i)
	{
		SpawnColoredProp(BannerSpots[i], FRotator(0.f, (i % 2 == 0) ? 10.f : -10.f, 0.f), FVector(0.24f, 0.012f, 1.05f), CubeMesh, FuneralCloth);
		SpawnColoredProp(BannerSpots[i] + FVector(0.f, 1.2f, 8.f), FRotator(0.f, (i % 2 == 0) ? 10.f : -10.f, 0.f), FVector(0.06f, 0.014f, 0.68f), CubeMesh, SealRed);
	}

	// 7. Distant Chinese funerary shrine / memorial table silhouette at the rear aisle end with ghostly jade-green candles
	const FVector AltarBase(800.f, 972.f, 0.f);
	const FLinearColor DarkWood(0.09f, 0.04f, 0.03f, 1.f);
	const FLinearColor LacquerRed(0.32f, 0.045f, 0.035f, 1.f);
	const FLinearColor StoneGrey(0.14f, 0.14f, 0.13f, 1.f);
	const FLinearColor CoffinBlack(0.04f, 0.035f, 0.032f, 1.f);

	SpawnColoredProp(AltarBase + FVector(0.f, 0.f, 44.f),  FRotator::ZeroRotator, FVector(1.35f, 0.48f, 0.88f), CubeMesh, DarkWood);
	SpawnColoredProp(AltarBase + FVector(0.f, -8.f, 118.f), FRotator::ZeroRotator, FVector(0.32f, 0.06f, 0.52f), CubeMesh, CoffinBlack);
	SpawnColoredProp(AltarBase + FVector(0.f, -12.f, 118.f), FRotator::ZeroRotator, FVector(0.18f, 0.02f, 0.36f), CubeMesh, SealRed, 0.4f);
	for (float Side : { -42.f, 42.f })
	{
		SpawnColoredProp(AltarBase + FVector(Side, -14.f, 100.f), FRotator::ZeroRotator, FVector(0.05f, 0.05f, 0.24f), CubeMesh, SealRed);
		if (AActor* Flame = SpawnColoredProp(AltarBase + FVector(Side, -14.f, 116.f), FRotator::ZeroRotator, FVector(0.07f, 0.07f, 0.11f), SphereMesh, FLinearColor(0.22f, 0.98f, 0.56f, 1.f), 3.0f))
		{
			UPointLightComponent* GhostFlame = NewObject<UPointLightComponent>(Flame);
			GhostFlame->RegisterComponent();
			GhostFlame->AttachToComponent(Flame->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			GhostFlame->SetIntensityUnits(ELightUnits::Lumens);
			GhostFlame->SetIntensity(0.f);
			GhostFlame->SetLightColor(FLinearColor(0.18f, 0.96f, 0.52f, 1.f));
			GhostFlame->SetAttenuationRadius(420.f);
			GhostFlame->SetCastShadows(false);
			CrimeLanternLights.Add(GhostFlame);
		}
	}

	// 8. ANCIENT CHINESE ANCESTRAL HALL / MOURNING HALL ARCHITECTURE REPLACING THE STORE SHELVES & COUNTER!
	// 8a. Six pairs of vermilion lacquered wooden pillars + carved dark wood lattice screens aligned 1:1 with the 6 gondolas
	const FVector GondolaCenters[] = {
		FVector(400.f, 450.f, 0.f), FVector(800.f, 450.f, 0.f), FVector(1200.f, 450.f, 0.f),
		FVector(400.f, 750.f, 0.f), FVector(800.f, 750.f, 0.f), FVector(1200.f, 750.f, 0.f),
	};
	for (int32 idx = 0; idx < UE_ARRAY_COUNT(GondolaCenters); ++idx)
	{
		const FVector& GC = GondolaCenters[idx];
		// Two floor-to-ceiling crimson wooden pillars with stone plinths at either end of each shelf footprint
		for (float OffX : { -95.f, 95.f })
		{
			SpawnColoredProp(GC + FVector(OffX, 0.f, 148.f), FRotator::ZeroRotator, FVector(0.28f, 0.28f, 2.96f), CylinderMesh, LacquerRed);
			SpawnColoredProp(GC + FVector(OffX, 0.f, 12.f),  FRotator::ZeroRotator, FVector(0.38f, 0.38f, 0.24f), CubeMesh, StoneGrey);
		}
		// Top lacquered lintel beam connecting the pair of pillars
		SpawnColoredProp(GC + FVector(0.f, 0.f, 268.f), FRotator::ZeroRotator, FVector(2.20f, 0.24f, 0.22f), CubeMesh, LacquerRed);

		if (idx == 1)
		{
			// Center front footprint: a heavy black lacquered Chinese coffin (黑漆灵柩) sealed with crossed yellow talisman tape!
			SpawnColoredProp(GC + FVector(0.f, 0.f, 42.f), FRotator::ZeroRotator, FVector(1.85f, 0.72f, 0.74f), CubeMesh, CoffinBlack);
			CoffinLidActor = SpawnColoredProp(GC + FVector(0.f, 0.f, 82.f), FRotator::ZeroRotator, FVector(1.94f, 0.78f, 0.14f), CubeMesh, LacquerRed);
			CoffinSealA = SpawnColoredProp(GC + FVector(0.f, 0.f, 90.f), FRotator(0.f, 18.f, 0.f),  FVector(1.60f, 0.08f, 0.015f), CubeMesh, SealYellow);
			CoffinSealB = SpawnColoredProp(GC + FVector(0.f, 0.f, 90.f), FRotator(0.f, -18.f, 0.f), FVector(1.60f, 0.08f, 0.015f), CubeMesh, SealYellow);
		}
		else
		{
			// Dark aged wooden partition screen + lattice bars replacing the modern supermarket shelves
			SpawnColoredProp(GC + FVector(0.f, 0.f, 92.f),  FRotator::ZeroRotator, FVector(1.82f, 0.18f, 1.78f), CubeMesh, DarkWood);
			SpawnColoredProp(GC + FVector(0.f, 0.f, 182.f), FRotator::ZeroRotator, FVector(1.88f, 0.24f, 0.08f), CubeMesh, LacquerRed);
			SpawnColoredProp(GC + FVector(0.f, -11.f, 118.f), FRotator(0.f, 0.f, 3.f),  FVector(0.12f, 0.01f, 0.38f), CubeMesh, SealYellow);
			SpawnColoredProp(GC + FVector(0.f,  11.f, 118.f), FRotator(0.f, 0.f, -4.f), FVector(0.12f, 0.01f, 0.38f), CubeMesh, SealYellow);
		}
	}

	// 8b. Checkout counter footprint (1270, 180): stark, high-impact minimalist Chinese ancestral shrine (no cluttered props)
	const FVector CounterShrine(1270.f, 180.f, 0.f);
	const FLinearColor GoldTrim(0.76f, 0.56f, 0.16f, 1.f);
	const FLinearColor PaperWindowRed(0.22f, 0.03f, 0.025f, 1.f);

	// Monolithic floor-to-ceiling black-lacquered shrine backdrop wall (completely erases modern checkout feel)
	SpawnColoredProp(CounterShrine + FVector(0.f, -46.f, 150.f), FRotator::ZeroRotator, FVector(3.30f, 0.10f, 3.00f), CubeMesh, CoffinBlack);
	// Heavy dark-lacquered altar table with stark crimson top & full crimson altar cloth
	SpawnColoredProp(CounterShrine + FVector(0.f, 0.f, 46.f),    FRotator::ZeroRotator, FVector(2.90f, 0.88f, 0.92f), CubeMesh, CoffinBlack);
	SpawnColoredProp(CounterShrine + FVector(0.f, 0.f, 94.f),    FRotator::ZeroRotator, FVector(3.00f, 0.96f, 0.08f), CubeMesh, LacquerRed);
	SpawnColoredProp(CounterShrine + FVector(0.f, 45.f, 48.f),   FRotator::ZeroRotator, FVector(2.50f, 0.03f, 0.86f), CubeMesh, DrapeCrimson);

	// Twin tall crimson shrine pillars & heavy overhanging dark-red roof eaves
	for (float SideX : { -145.f, 145.f })
	{
		SpawnColoredProp(CounterShrine + FVector(SideX, 20.f, 148.f), FRotator::ZeroRotator, FVector(0.24f, 0.24f, 2.96f), CylinderMesh, LacquerRed);
	}
	SpawnColoredProp(CounterShrine + FVector(0.f, -4.f, 262.f),  FRotator::ZeroRotator, FVector(3.50f, 1.25f, 0.16f), CubeMesh, LacquerRed);

	// Imposing oversized ancestral tablet dead-center (strong silhouette against the black backdrop)
	SpawnColoredProp(CounterShrine + FVector(0.f, -20.f, 104.f), FRotator::ZeroRotator, FVector(0.88f, 0.36f, 0.14f), CubeMesh, LacquerRed);
	SpawnColoredProp(CounterShrine + FVector(0.f, -22.f, 168.f), FRotator::ZeroRotator, FVector(0.56f, 0.10f, 1.12f), CubeMesh, CoffinBlack);
	SpawnColoredProp(CounterShrine + FVector(0.f, -16.f, 168.f), FRotator::ZeroRotator, FVector(0.40f, 0.03f, 0.92f), CubeMesh, SealRed, 0.85f);
	SpawnColoredProp(CounterShrine + FVector(0.f, -14.f, 168.f), FRotator::ZeroRotator, FVector(0.05f, 0.035f, 0.72f), CubeMesh, GoldTrim);

	// Bronze incense burner & 3->2->1 progressive smoldering incense sticks
	SpawnColoredProp(CounterShrine + FVector(0.f, 16.f, 108.f), FRotator::ZeroRotator, FVector(0.28f, 0.28f, 0.18f), CylinderMesh, StoneGrey);
	for (float IX : { -6.f, 0.f, 6.f })
	{
		SpawnColoredProp(CounterShrine + FVector(IX, 16.f, 126.f), FRotator::ZeroRotator, FVector(0.015f, 0.015f, 0.22f), CylinderMesh, FLinearColor(0.35f, 0.28f, 0.18f, 1.f));
		if (AActor* Ember = SpawnColoredProp(CounterShrine + FVector(IX, 16.f, 138.f), FRotator::ZeroRotator, FVector(0.032f, 0.032f, 0.032f), SphereMesh, FLinearColor(1.0f, 0.22f, 0.04f, 1.f), 4.0f))
		{
			IncenseEmbers.Add(Ember);
		}
	}

	// Twin dramatic crimson candlelights framing the giant tablet
	for (float CX : { -86.f, 86.f })
	{
		SpawnColoredProp(CounterShrine + FVector(CX, 8.f, 118.f), FRotator::ZeroRotator, FVector(0.08f, 0.08f, 0.36f), CylinderMesh, SealRed);
		if (AActor* CandleFlame = SpawnColoredProp(CounterShrine + FVector(CX, 8.f, 140.f), FRotator::ZeroRotator, FVector(0.065f, 0.065f, 0.11f), SphereMesh, FLinearColor(1.0f, 0.45f, 0.08f, 1.f), 4.0f))
		{
			UPointLightComponent* CandleLight = NewObject<UPointLightComponent>(CandleFlame);
			CandleLight->RegisterComponent();
			CandleLight->AttachToComponent(CandleFlame->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			CandleLight->SetIntensityUnits(ELightUnits::Lumens);
			CandleLight->SetIntensity(0.f);
			CandleLight->SetLightColor(FLinearColor(1.0f, 0.24f, 0.08f, 1.f));
			CandleLight->SetAttenuationRadius(420.f);
			CandleLight->SetCastShadows(false);
			CrimeLanternLights.Add(CandleLight);
		}
	}

	// 8b2. Clean monolithic crimson paper-screen shutters sealing off the modern storefront glass windows (Y = 14)
	for (float WinX : { 280.f, 1280.f })
	{
		SpawnColoredProp(FVector(WinX, 14.f, 155.f), FRotator::ZeroRotator, FVector(6.2f, 0.08f, 3.00f), CubeMesh, PaperWindowRed);
		SpawnColoredProp(FVector(WinX, 19.f, 155.f), FRotator::ZeroRotator, FVector(6.25f, 0.08f, 0.10f), CubeMesh, CoffinBlack);
		for (float XOff : { -200.f, 0.f, 200.f })
		{
			SpawnColoredProp(FVector(WinX + XOff, 19.f, 155.f), FRotator::ZeroRotator, FVector(0.10f, 0.08f, 3.00f), CubeMesh, CoffinBlack);
		}
	}

	// 8c. Overhead heavy dark-red ancestral hall roof beams spanning the entire store ceiling
	for (float BeamY : { 210.f, 450.f, 750.f, 960.f })
	{
		SpawnColoredProp(FVector(800.f, BeamY, 282.f), FRotator::ZeroRotator, FVector(15.6f, 0.26f, 0.28f), CubeMesh, LacquerRed);
	}
	// Longitudinal beams
	for (float BeamX : { 400.f, 800.f, 1200.f })
	{
		SpawnColoredProp(FVector(BeamX, 560.f, 286.f), FRotator::ZeroRotator, FVector(0.24f, 10.5f, 0.24f), CubeMesh, DarkWood);
	}

	// 9. BACK-OF-HOUSE THREE-ROOM OTHERWORLD STAGING (stark silhouettes & high-impact lighting)
	auto SpawnDecal = [&](const FVector& Pos, const FRotator& Rot, const FVector& Size, const FLinearColor& Tint)
	{
		if (!SpillBaseMat || !IchorD || !IchorN || !IchorM) { return; }
		AActor* Holder = W->SpawnActor<AActor>(Pos, FRotator::ZeroRotator);
		if (!Holder) { return; }
		UDecalComponent* Decal = NewObject<UDecalComponent>(Holder);
		Decal->RegisterComponent();
		Holder->SetRootComponent(Decal);
		Decal->SetWorldLocation(Pos);
		Decal->SetWorldRotation(Rot);
		Decal->DecalSize = Size;
		Decal->FadeScreenSize = 0.001f;
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(SpillBaseMat, Holder))
		{
			MID->SetTextureParameterValue(TEXT("AlbedoTex"), IchorD);
			MID->SetTextureParameterValue(TEXT("NormalTex"), IchorN);
			MID->SetTextureParameterValue(TEXT("MaskTex"), IchorM);
			MID->SetVectorParameterValue(TEXT("TintColor"), Tint);
			MID->SetScalarParameterValue(TEXT("WetnessScale"), 0.92f);
			MID->SetScalarParameterValue(TEXT("CleanProgress"), 0.f);
			Decal->SetDecalMaterial(MID);
		}
		Holder->SetActorHiddenInGame(true);
		CrimeSceneActors.Add(Holder);
	};

	// 9a. Warehouse (X=0..800, Y=1200..1600): Ancestral Mortuary Storage
	for (float WX : { 170.f, 610.f })
	{
		for (float WY : { 1280.f, 1525.f })
		{
			SpawnColoredProp(FVector(WX, WY, 148.f), FRotator::ZeroRotator, FVector(0.24f, 0.24f, 2.96f), CylinderMesh, LacquerRed);
		}
	}
	SpawnColoredProp(FVector(390.f, 1400.f, 276.f), FRotator::ZeroRotator, FVector(7.2f, 0.24f, 0.24f), CubeMesh, LacquerRed);
	// Two heavy sealed dark-wood coffin crates along the north warehouse wall with crossed yellow talismans
	for (float CrateX : { 230.f, 540.f })
	{
		SpawnColoredProp(FVector(CrateX, 1515.f, 44.f), FRotator::ZeroRotator, FVector(1.65f, 0.68f, 0.88f), CubeMesh, CoffinBlack);
		SpawnColoredProp(FVector(CrateX, 1480.f, 52.f), FRotator(0.f, 0.f, 18.f), FVector(1.30f, 0.015f, 0.07f), CubeMesh, SealYellow);
		SpawnColoredProp(FVector(CrateX, 1480.f, 52.f), FRotator(0.f, 0.f, -18.f), FVector(1.30f, 0.015f, 0.07f), CubeMesh, SealYellow);
	}
	// Warehouse hanging crimson lantern
	if (AActor* WLantern = SpawnColoredProp(FVector(390.f, 1400.f, 248.f), FRotator::ZeroRotator, FVector(0.34f, 0.34f, 0.44f), SphereMesh, FLinearColor(0.96f, 0.08f, 0.05f, 1.f), 2.2f))
	{
		UPointLightComponent* WLight = NewObject<UPointLightComponent>(WLantern);
		WLight->RegisterComponent();
		WLight->AttachToComponent(WLantern->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		WLight->SetRelativeLocation(FVector(0.f, 0.f, -34.f));
		WLight->SetIntensityUnits(ELightUnits::Lumens);
		WLight->SetIntensity(0.f);
		WLight->SetLightColor(FLinearColor(1.0f, 0.12f, 0.06f, 1.f));
		WLight->SetAttenuationRadius(650.f);
		WLight->SetCastShadows(false);
		CrimeLanternLights.Add(WLight);
	}
	SpawnDecal(FVector(345.f, 1340.f, 4.f), FRotator(-90.f, 75.f, 0.f), FVector(32.f, 75.f, 135.f), FLinearColor(0.50f, 0.03f, 0.03f, 1.f));
	SpawnDecal(FVector(385.f, 1588.f, 145.f), FRotator(0.f, -90.f, 0.f), FVector(45.f, 145.f, 125.f), FLinearColor(0.56f, 0.03f, 0.03f, 1.f));

	// 9b. Manager's Office (X=800..1300, Y=1200..1600): Old Accounting & Incident Room
	SpawnColoredProp(FVector(880.f, 1550.f, 148.f), FRotator::ZeroRotator, FVector(0.22f, 0.22f, 2.96f), CylinderMesh, LacquerRed);
	SpawnColoredProp(FVector(1240.f, 1550.f, 148.f), FRotator::ZeroRotator, FVector(0.22f, 0.22f, 2.96f), CylinderMesh, LacquerRed);
	// Black-lacquered shroud over the office desk surface + towering dark funerary scroll behind desk
	SpawnColoredProp(FVector(1070.f, 1510.f, 80.f), FRotator::ZeroRotator, FVector(1.66f, 0.86f, 0.04f), CubeMesh, LacquerRed);
	SpawnColoredProp(FVector(1070.f, 1584.f, 165.f), FRotator::ZeroRotator, FVector(1.30f, 0.06f, 2.10f), CubeMesh, CoffinBlack);
	SpawnColoredProp(FVector(1070.f, 1580.f, 165.f), FRotator::ZeroRotator, FVector(0.18f, 0.03f, 1.55f), CubeMesh, SealRed, 0.75f);
	SpawnDecal(FVector(1070.f, 1588.f, 150.f), FRotator(0.f, -90.f, 0.f), FVector(45.f, 150.f, 130.f), FLinearColor(0.55f, 0.03f, 0.03f, 1.f));
	if (AActor* OfficeGhostFlame = SpawnColoredProp(FVector(1070.f, 1510.f, 110.f), FRotator::ZeroRotator, FVector(0.08f, 0.08f, 0.12f), SphereMesh, FLinearColor(0.22f, 0.98f, 0.56f, 1.f), 3.0f))
	{
		UPointLightComponent* OLight = NewObject<UPointLightComponent>(OfficeGhostFlame);
		OLight->RegisterComponent();
		OLight->AttachToComponent(OfficeGhostFlame->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		OLight->SetIntensityUnits(ELightUnits::Lumens);
		OLight->SetIntensity(0.f);
		OLight->SetLightColor(FLinearColor(0.18f, 0.96f, 0.52f, 1.f));
		OLight->SetAttenuationRadius(480.f);
		OLight->SetCastShadows(false);
		CrimeLanternLights.Add(OLight);
	}

	// Normal-world desk telephone on Manager's Office desk (rings on Night 2 to pull player into back-of-house!)
	auto SpawnNormalProp = [&](const FVector& Loc, const FRotator& Rot, const FVector& Scale, UStaticMesh* Mesh, const FLinearColor& Color, float EmissiveBoost = 0.f) -> AActor*
	{
		if (!Mesh) { return nullptr; }
		AStaticMeshActor* Prop = W->SpawnActor<AStaticMeshActor>(Loc, Rot);
		if (!Prop) { return nullptr; }
		UStaticMeshComponent* Comp = Prop->GetStaticMeshComponent();
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetStaticMesh(Mesh);
		Comp->SetWorldScale3D(Scale);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		if (LitBaseMat)
		{
			if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(LitBaseMat, Prop))
			{
				MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(Color.R * (1.f + EmissiveBoost), Color.G * (1.f + EmissiveBoost), Color.B * (1.f + EmissiveBoost), 1.f));
				Comp->SetMaterial(0, MID);
			}
		}
		NormalOnlyActors.Add(Prop);
		return Prop;
	};
	const FVector PhonePos = GetOfficePhoneLocation();
	if (AActor* PhoneBase = SpawnNormalProp(PhonePos, FRotator(0.f, -18.f, 0.f), FVector(0.22f, 0.18f, 0.10f), CubeMesh, FLinearColor(0.06f, 0.06f, 0.07f, 1.f)))
	{
		SpawnNormalProp(PhonePos + FVector(0.f, -2.f, 7.5f), FRotator(0.f, -18.f, 0.f), FVector(0.24f, 0.06f, 0.055f), CubeMesh, FLinearColor(0.04f, 0.04f, 0.05f, 1.f));
		UPointLightComponent* RingLight = NewObject<UPointLightComponent>(PhoneBase);
		RingLight->RegisterComponent();
		RingLight->AttachToComponent(PhoneBase->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		RingLight->SetRelativeLocation(FVector(0.f, 0.f, 14.f));
		RingLight->SetIntensityUnits(ELightUnits::Lumens);
		RingLight->SetIntensity(0.f);
		RingLight->SetLightColor(FLinearColor(1.0f, 0.14f, 0.08f, 1.f));
		RingLight->SetAttenuationRadius(260.f);
		RingLight->SetCastShadows(false);
		OfficePhoneLight = RingLight;
	}

	// 9c. Staff Restroom (X=1300..1600, Y=1200..1600): Cursed Stone Well & Blood Basin
	const FVector WellPos(1475.f, 1445.f, 0.f);
	SpawnColoredProp(WellPos + FVector(0.f, 0.f, 34.f), FRotator::ZeroRotator, FVector(0.92f, 0.92f, 0.68f), CylinderMesh, StoneGrey);
	SpawnColoredProp(WellPos + FVector(0.f, 0.f, 68.f), FRotator::ZeroRotator, FVector(0.76f, 0.76f, 0.04f), CylinderMesh, CoffinBlack);
	SpawnDecal(WellPos + FVector(0.f, 0.f, 4.f), FRotator(-90.f, 30.f, 0.f), FVector(32.f, 118.f, 118.f), FLinearColor(0.55f, 0.03f, 0.03f, 1.f));
	if (AActor* WellGlow = SpawnColoredProp(WellPos + FVector(0.f, 0.f, 74.f), FRotator::ZeroRotator, FVector(0.18f, 0.18f, 0.04f), SphereMesh, FLinearColor(0.16f, 0.96f, 0.48f, 1.f), 3.0f))
	{
		UPointLightComponent* WellLight = NewObject<UPointLightComponent>(WellGlow);
		WellLight->RegisterComponent();
		WellLight->AttachToComponent(WellGlow->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		WellLight->SetRelativeLocation(FVector(0.f, 0.f, 24.f));
		WellLight->SetIntensityUnits(ELightUnits::Lumens);
		WellLight->SetIntensity(0.f);
		WellLight->SetLightColor(FLinearColor(0.16f, 0.96f, 0.48f, 1.f));
		WellLight->SetAttenuationRadius(420.f);
		WellLight->SetCastShadows(false);
		CrimeLanternLights.Add(WellLight);
	}
	for (float TZ : { 148.f, 174.f })
	{
		SpawnColoredProp(FVector(1585.f, 1445.f, TZ), FRotator(0.f, 90.f, 4.f), FVector(0.12f, 0.01f, 0.38f), CubeMesh, SealYellow);
	}
}

void AShiftDirector::SetOfficePhoneRinging(bool bRinging)
{
	bOfficePhoneRinging = bRinging;
	if (!bRinging)
	{
		if (UPointLightComponent* L = OfficePhoneLight.Get()) { L->SetIntensity(0.f); }
	}
}

void AShiftDirector::SetShifted(bool bInShift)
{
	bShift = bInShift;
	bPlayerPhaseShift = false;
	bSnappyTransition = false;
	PhaseBurstLeft = 0.f;
	if (!bInShift)
	{
		BreakerOverrideLeft = 0.f;
		BreakerCooldownLeft = 0.f;
		PlayerPhaseCooldown = 0.f;
	}
	ClearMicroShifts();
}

void AShiftDirector::ClearPlayerPhaseShift()
{
	bPlayerPhaseShift = false;
	PhaseBurstLeft = 0.f;
	PlayerPhaseCooldown = 0.f;
}

bool AShiftDirector::TogglePlayerPhaseShift()
{
	if (PlayerPhaseCooldown > 0.f) { return false; }
	if (bShift)
	{
		// During a monster-forced Shift, [Q] yanks the store back to Normal layout for 2.5s (slides gondolas!).
		if (BreakerOverrideLeft > 0.f || PhaseBurstLeft > 0.f) { return false; }
		PhaseBurstLeft = 2.5f;
		PlayerPhaseCooldown = 7.0f;
		bSnappyTransition = true;
		Flicker(0.18f);
		return true;
	}
	bPlayerPhaseShift = !bPlayerPhaseShift;
	bSnappyTransition = true;
	PlayerPhaseCooldown = 0.65f;
	Flicker(0.12f);
	return true;
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
	if (PlayerPhaseCooldown > 0.f) { PlayerPhaseCooldown = FMath::Max(0.f, PlayerPhaseCooldown - DeltaSeconds); }
	if (BreakerOverrideLeft > 0.f)
	{
		BreakerOverrideLeft = FMath::Max(0.f, BreakerOverrideLeft - DeltaSeconds);
		if (BreakerOverrideLeft == 0.f && bShift) { Flicker(0.45f); }
	}
	if (PhaseBurstLeft > 0.f)
	{
		PhaseBurstLeft = FMath::Max(0.f, PhaseBurstLeft - DeltaSeconds);
		if (PhaseBurstLeft == 0.f && bShift) { Flicker(0.22f); }
	}
	const float Target = IsEffectiveShifted() ? 1.f : 0.f;
	const float Speed = (bSnappyTransition || PhaseBurstLeft > 0.f) ? 4.2f : (Target > CurrentAlpha ? 0.8f : 0.65f);
	CurrentAlpha = FMath::FInterpConstantTo(CurrentAlpha, Target, DeltaSeconds, Speed);
	const bool bWasFlickering = FlickerLeft > 0.f;
	FlickerLeft = FMath::Max(0.f, FlickerLeft - DeltaSeconds);

	const bool bTransition = CurrentAlpha > 0.f && CurrentAlpha < 1.f;
	if (bTransition || FlickerLeft > 0.f || (bWasFlickering && FlickerLeft == 0.f) || AppliedAlpha != CurrentAlpha)
	{
		Apply(CurrentAlpha);
	}
	if (UPointLightComponent* PhoneL = OfficePhoneLight.Get())
	{
		if (bOfficePhoneRinging && !bSurfacesSwapped)
		{
			OfficePhonePulseTime += DeltaSeconds;
			const float Pulse = 0.5f + 0.5f * FMath::Sin(OfficePhonePulseTime * 9.0f);
			PhoneL->SetIntensity(120.f + Pulse * 360.f);
		}
		else
		{
			PhoneL->SetIntensity(0.f);
		}
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

void AShiftDirector::ConfigureCrimeSceneForNight(int32 NightIndex)
{
	CurrentCrimeNightIndex = FMath::Clamp(NightIndex, 0, 2);
	const FVector CoffinBase(800.f, 450.f, 0.f);
	if (AActor* Lid = CoffinLidActor.Get())
	{
		if (CurrentCrimeNightIndex == 0)
		{
			Lid->SetActorLocationAndRotation(CoffinBase + FVector(0.f, 0.f, 82.f), FRotator::ZeroRotator);
		}
		else if (CurrentCrimeNightIndex == 1)
		{
			// Night 2: coffin lid pushed slightly ajar revealing a dark seam
			Lid->SetActorLocationAndRotation(CoffinBase + FVector(12.f, -22.f, 82.f), FRotator(0.f, 14.f, 0.f));
		}
		else
		{
			// Night 3: coffin lid overturned beside the open empty coffin!
			Lid->SetActorLocationAndRotation(CoffinBase + FVector(0.f, -56.f, 20.f), FRotator(0.f, 8.f, 34.f));
		}
	}
	if (bSurfacesSwapped)
	{
		// Re-apply visibility rules for seals & incense embers immediately if already in Otherworld
		if (AActor* SealA = CoffinSealA.Get()) { SealA->SetActorHiddenInGame(CurrentCrimeNightIndex >= 1); }
		if (AActor* SealB = CoffinSealB.Get()) { SealB->SetActorHiddenInGame(CurrentCrimeNightIndex >= 2); }
		const int32 LitIncense = FMath::Clamp(3 - CurrentCrimeNightIndex, 1, 3);
		for (int32 i = 0; i < IncenseEmbers.Num(); ++i)
		{
			if (AActor* Ember = IncenseEmbers[i].Get()) { Ember->SetActorHiddenInGame(i >= LitIncense); }
		}
	}
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
		// Clean, crystal-clear modern store air in Normal -> thick ghostly jade-green mist lit by crimson lanterns in Otherworld!
		Fog->SetFogDensity(FMath::Lerp(0.0045f, 0.038f, Smooth));
		Fog->SetStartDistance(FMath::Lerp(480.f, 75.f, Smooth));
		Fog->SetFogInscatteringColor(FMath::Lerp(FLinearColor(0.04f, 0.042f, 0.045f, 1.f), FLinearColor(0.025f, 0.12f, 0.075f, 1.f), Smooth));
	}
	const bool bShowCrimeScene = Smooth > 0.25f;
	if (bSurfacesSwapped != bShowCrimeScene)
	{
		bSurfacesSwapped = bShowCrimeScene;
		for (const FSwappedSurface& S : SwappedSurfaces)
		{
			if (UStaticMeshComponent* C = S.Comp.Get())
			{
				UMaterialInterface* TargetMat = bShowCrimeScene ? S.ShiftMat.Get() : S.NormalMat.Get();
				if (TargetMat) { C->SetMaterial(S.Slot, TargetMat); }
			}
		}
		for (const TWeakObjectPtr<AActor>& NPtr : NormalOnlyActors)
		{
			if (AActor* NA = NPtr.Get()) { NA->SetActorHiddenInGame(bShowCrimeScene); }
		}
		for (const TWeakObjectPtr<UInstancedStaticMeshComponent>& ISMPtr : NormalOnlyISMs)
		{
			if (UInstancedStaticMeshComponent* ISM = ISMPtr.Get()) { ISM->SetVisibility(!bShowCrimeScene); }
		}
	}
	for (const TWeakObjectPtr<AActor>& PropPtr : CrimeSceneActors)
	{
		if (AActor* Prop = PropPtr.Get())
		{
			Prop->SetActorHiddenInGame(!bShowCrimeScene);
		}
	}
	if (bShowCrimeScene)
	{
		if (AActor* SealA = CoffinSealA.Get()) { SealA->SetActorHiddenInGame(CurrentCrimeNightIndex >= 1); }
		if (AActor* SealB = CoffinSealB.Get()) { SealB->SetActorHiddenInGame(CurrentCrimeNightIndex >= 2); }
		const int32 LitIncense = FMath::Clamp(3 - CurrentCrimeNightIndex, 1, 3);
		for (int32 i = 0; i < IncenseEmbers.Num(); ++i)
		{
			if (AActor* Ember = IncenseEmbers[i].Get()) { Ember->SetActorHiddenInGame(i >= LitIncense); }
		}
	}
	for (const TWeakObjectPtr<UPointLightComponent>& LP : CrimeLanternLights)
	{
		if (UPointLightComponent* Light = LP.Get())
		{
			Light->SetIntensity(Smooth * 560.f);
		}
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
		PPV->Settings.ColorSaturation = FMath::Lerp(FVector4(0.96f, 0.96f, 0.96f, 1.f), FVector4(1.18f, 0.84f, 0.72f, 1.f), Smooth);
		PPV->Settings.bOverride_ColorContrast = true;
		PPV->Settings.ColorContrast = FMath::Lerp(FVector4(1.04f, 1.04f, 1.05f, 1.f), FVector4(1.36f, 1.26f, 1.28f, 1.f), Smooth);
		PPV->Settings.bOverride_ColorGamma = true;
		PPV->Settings.ColorGamma = FMath::Lerp(FVector4(1.00f, 1.00f, 1.00f, 1.f), FVector4(1.04f, 0.92f, 0.85f, 1.f), Smooth);
		PPV->Settings.bOverride_BloomIntensity = true;
		PPV->Settings.BloomIntensity = FMath::Lerp(0.45f, 1.85f, Smooth);
		PPV->Settings.bOverride_SceneFringeIntensity = true;
		PPV->Settings.SceneFringeIntensity = FMath::Lerp(0.05f, 3.20f, Smooth);
		PPV->Settings.bOverride_FilmGrainIntensity = true;
		PPV->Settings.FilmGrainIntensity = FMath::Lerp(0.04f, 0.42f, Smooth);
		PPV->Settings.bOverride_VignetteIntensity = true;
		PPV->Settings.VignetteIntensity = FMath::Lerp(0.22f, 0.68f, Smooth);
	}
}
