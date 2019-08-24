// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Hazard.h"
#include "Dart.generated.h"

/**
 * 
 */
UCLASS()
class RAGELITE_API ADart : public AHazard
{
	GENERATED_BODY()

public:
	ADart();

	void Enable(float InDelay = 0.f, float InCoolDown = 1.f, float InSpeed = 100.f);

	void Disable();

private:
	UPROPERTY(EditAnywhere, Category = Dart, meta = (AllowPrivateAccess = "true"))
	bool bEnabled;

	FTimerHandle SpawnDartHandle;

	void SpawnDart();

	float Delay;
	float Cooldown;
	float Speed;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	
};
