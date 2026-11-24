#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShiftTypes.h"
#include "ShiftItem.generated.h"

class UStaticMeshComponent;

/** A physics object the player can carry, hand over, deliver or throw. */
UCLASS()
class SHIFTSTORE_API AShiftItem : public AActor
{
	GENERATED_BODY()

public:
	AShiftItem();

	void InitProduct(FName InSku);
	void InitKind(EShiftItemKind InKind);

	void SetHeld(bool bHeld);
	void Throw(const FVector& Velocity, AActor* Thrower);
	FString DisplayName() const;
	bool CanSimulate() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;

	EShiftItemKind Kind = EShiftItemKind::Product;
	FName Sku;
	bool bInFlight = false;
	/** Set for the out-of-place item a revealed monster wants. */
	bool bLostItem = false;
	/** Hot oden cup: disposable, never counted as loose shelf merchandise. */
	bool bHotCup = false;

protected:
	UFUNCTION() void OnHit(UPrimitiveComponent* HitComp, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);
	virtual void Tick(float DeltaSeconds) override;

private:
	float FlightTime = 0.f;
	float GlowTime = 0.f;
	float PulseCueTime = 1.5f;
	TObjectPtr<class UPointLightComponent> Glow;
	TObjectPtr<UStaticMeshComponent> BeaconMesh;
};
