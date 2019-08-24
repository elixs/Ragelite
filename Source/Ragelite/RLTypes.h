// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RLTypes.generated.h"


class UPaperTileMap;

UENUM()
enum class ELevelState : uint8
{
	Start,
	Next,
	Reset
};

UENUM(Meta = (Bitflags))
enum class EHazardLocation : int32
{
	// Bottom
	B0,
	B1,
	B2,
	B3,
	// Right
	R0,
	R1,
	R2,
	R3,
	// Top
	T0,
	T1,
	T2,
	T3,
	// Left
	L0,
	L1,
	L2,
	L3
};

UENUM()
enum class EHazardType : uint8
{
	Spikes,
	Darts,
	Stones
};

USTRUCT()
struct FHazardsData
{
	GENERATED_BODY()

	FHazardsData();

	// 2D coordintes on the grid
	UPROPERTY(EditAnywhere)
	FVector2D Coords;

	// Difficulty of all the hazards
	UPROPERTY(EditAnywhere)
	float DifficultyFactor;

	UPROPERTY(EditAnywhere, Meta = (Bitmask, BitmaskEnum = "EHazardLocation"))
	int32 HazardsLocations;

	UPROPERTY(EditAnywhere)
	EHazardType HazardsType;

	int32 Num() const;

	// Darts

	UPROPERTY(EditAnywhere)
	float DartsDelay;

	UPROPERTY(EditAnywhere)
	float DartsCooldown;

	UPROPERTY(EditAnywhere)
	float DartsSpeed;

	UPROPERTY(EditAnywhere)
	bool bSpawnsFrom;

};

USTRUCT()
struct FRlLevel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Levels|Level")
	UPaperTileMap* PaperTileMap;

	UPROPERTY(EditAnywhere, Category = "Levels|Level")
	FVector2D Start;

	UPROPERTY(EditAnywhere, Category = "Levels|Level")
	FVector2D Stairs;

	UPROPERTY(EditAnywhere, Category = "Levels|Level")
	TArray<FHazardsData> Hazards;
};