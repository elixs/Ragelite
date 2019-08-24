// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Hazard.h"
#include "Stone.generated.h"

/**
 * 
 */
UCLASS()
class RAGELITE_API AStone : public AHazard
{
	GENERATED_BODY()

public:
	AStone();

	void Move(FVector Location, EHazardLocation HazardLocation) override;
	
};
