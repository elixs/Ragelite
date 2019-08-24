// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperSpriteActor.h"
#include "RLTypes.h"
#include "Hazard.generated.h"

/**
 * A hazard.
 */
UCLASS()
class RAGELITE_API AHazard : public APaperSpriteActor
{
	GENERATED_BODY()

public:
	AHazard();

	/** Size of the tile containing the hazard. A single tile can have up to 16 hazards. */
	UPROPERTY(Category = Sprites, EditAnywhere)
	int32 TileSize;

	virtual void Move(FVector Location, EHazardLocation HazardLocation);
	
};
