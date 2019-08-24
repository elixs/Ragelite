// Fill out your copyright notice in the Description page of Project Settings.

#include "RLTypes.h"

FHazardsData::FHazardsData()
{
	Coords = FVector2D(0.f, 0.f);
	DifficultyFactor = 0.f;
	HazardsLocations = 0;
	HazardsType = EHazardType::Spikes;

	DartsDelay = 0.f;
	DartsCooldown = 1.f;
	DartsSpeed = 100.f;

	bSpawnsFrom = true;
}

int32 FHazardsData::Num() const
{
	int32 Count = 0;

	for (int32 Number = HazardsLocations; Number; ++Count)
	{
		Number = Number & (Number - 1);
	}

	return Count;
}