// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Hazard.h"
#include "Spike.generated.h"

DECLARE_DELEGATE(FHitSignature)

/**
 * 8 x 8 spike.
 */
UCLASS()
class RAGELITE_API ASpike : public AHazard
{
	GENERATED_BODY()

public:
	ASpike();

	FHitSignature OnRlCharacterHit;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
