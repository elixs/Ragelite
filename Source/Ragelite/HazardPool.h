// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RLTypes.h"
#include "HazardPool.generated.h"

class AHazard;
class ASpike;
class ADart;
class AStone;

UCLASS()
class RAGELITE_API AHazardPool : public AActor
{
	GENERATED_BODY()
	
public:	
	AHazardPool();

	UPROPERTY(EditAnywhere, Category = Spikes)
	int32 InitialSpikes;

	TArray<ASpike*> Spikes;

	UPROPERTY(EditAnywhere, Category = Darts)
	int32 InitialDarts;

	TArray<ADart*> Darts;

	UPROPERTY(EditAnywhere, Category = Darts)
	int32 InitialStones;

	TArray<AStone*> Stones;


public:
	void AddSpikes(int32 SpikesNum);

	void AddSpikesUntil(int32 SpikesNum);

	void AddDarts(int32 DartsNum);

	void AddDartsUntil(int32 DartsNum);

	void AddStones(int32 StonesNum);

	void AddStonesUntil(int32 StonesNum);

	void ResetHazards(TArray<FHazardsData> HazardsData);

	virtual void PostInitializeComponents() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;




};
