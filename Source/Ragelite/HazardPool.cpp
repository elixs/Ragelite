// Fill out your copyright notice in the Description page of Project Settings.

#include "HazardPool.h"
#include "Hazard.h"
#include "Spike.h"
#include "Dart.h"
#include "Stone.h"
#include "Engine/World.h"
#include "RlGameInstance.h"
#include "RlGameMode.h"
#include "LevelManager.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"

AHazardPool::AHazardPool()
{
	InitialSpikes = 30;
	InitialDarts = 10;
	InitialStones = 10;
}

void AHazardPool::AddSpikes(int32 SpikesNum)
{
	FRotator SpawnRotation(0.f);
	FActorSpawnParameters SpawnInfo;

	ULevelManager* LM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->LevelManager;

	for (int32 i = 0; i < SpikesNum; ++i)
	{
		ASpike* Spike = GetWorld()->SpawnActor<ASpike>(FVector(10000.f), SpawnRotation, SpawnInfo);

		UPaperSpriteComponent* RenderComponent = Spike->GetRenderComponent();
		RenderComponent->SetSprite(LM->GetSpikeSprite());
		RenderComponent->SetMaterial(0, LM->Material);

		Spikes.Add(Spike);
	}
}

void AHazardPool::AddSpikesUntil(int32 SpikesNum)
{
	AddSpikes(FMath::Max(0, SpikesNum - Spikes.Num()));
}

void AHazardPool::AddDarts(int32 DartsNum)
{
	FRotator SpawnRotation(0.f);
	FActorSpawnParameters SpawnInfo;

	ULevelManager* LM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->LevelManager;

	for (int32 i = 0; i < DartsNum; ++i)
	{
		ADart* Dart = GetWorld()->SpawnActor<ADart>(FVector(10000.f), SpawnRotation, SpawnInfo);

		UPaperSpriteComponent* RenderComponent = Dart->GetRenderComponent();
		RenderComponent->SetSprite(LM->DartSprite);
		RenderComponent->SetMaterial(0, LM->Material);

		Darts.Add(Dart);
	}
}

void AHazardPool::AddDartsUntil(int32 DartssNum)
{
	AddDarts(FMath::Max(0, DartssNum - Darts.Num()));
}

void AHazardPool::AddStones(int32 StonesNum)
{
	FRotator SpawnRotation(0.f);
	FActorSpawnParameters SpawnInfo;

	ULevelManager* LM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->LevelManager;

	for (int32 i = 0; i < StonesNum; ++i)
	{
		AStone* Stone = GetWorld()->SpawnActor<AStone>(FVector(10000.f), SpawnRotation, SpawnInfo);

		UPaperSpriteComponent* RenderComponent = Stone->GetRenderComponent();
		RenderComponent->SetSprite(LM->GetStoneSprite());
		RenderComponent->SetMaterial(0, LM->Material);

		Stones.Add(Stone);
	}
}

void AHazardPool::AddStonesUntil(int32 StonesNum)
{
	AddStones(FMath::Max(0, StonesNum - Stones.Num()));
}

void AHazardPool::ResetHazards(TArray<FHazardsData> HazardsData)
{
	ARlGameMode* RlGameMode = Cast<ARlGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	float CurrentDifficulty = RlGameMode->Difficulty;

	ULevelManager* LM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->LevelManager;

	int32 SpikesInUse = 0;
	int32 DartsInUse = 0;
	int32 StonesInUse = 0;

	for (int32 i = 0; i < HazardsData.Num(); ++i)
	{

		// If bSpawnsFrom, the hazard will appear from the DifficultyFactor and beyond.
		// If !bSpawsFrom, the hazard will appear until the DifficultyFactor.


		if ((!HazardsData[i].bSpawnsFrom && HazardsData[i].DifficultyFactor > CurrentDifficulty) 
			|| (HazardsData[i].bSpawnsFrom && HazardsData[i].DifficultyFactor <= CurrentDifficulty))
		{
			int32 Count = 0;

			for (int32 Number = HazardsData[i].HazardsLocations; Number; Number >>= 1, ++Count)
			{
				if (Number & 1)
				{
					if (HazardsData[i].HazardsType == EHazardType::Spikes)
					{
						Spikes[SpikesInUse++]->Move(LM->GetRelativeLocation(HazardsData[i].Coords, -5.f), static_cast<EHazardLocation>(Count));
					}
					else if (HazardsData[i].HazardsType == EHazardType::Darts)
					{
						Darts[DartsInUse]->Move(LM->GetRelativeLocation(HazardsData[i].Coords, -5.f), static_cast<EHazardLocation>(Count));
						Darts[DartsInUse]->Enable(HazardsData[i].DartsDelay, HazardsData[i].DartsCooldown, HazardsData[i].DartsSpeed);
						++DartsInUse;
					}
					else if (HazardsData[i].HazardsType == EHazardType::Stones)
					{
						Stones[StonesInUse++]->Move(LM->GetRelativeLocation(HazardsData[i].Coords, -5.f), static_cast<EHazardLocation>(Count));
					}
				}
			}
		}
	}

	for (int32 i = SpikesInUse; i < Spikes.Num(); ++i)
	{
		Spikes[i]->SetActorLocation(FVector(10000.f));
	}

	for (int32 i = DartsInUse; i < Darts.Num(); ++i)
	{
		Darts[i]->SetActorLocation(FVector(10000.f));
		Darts[i]->Disable();
	}

	for (int32 i = StonesInUse; i < Stones.Num(); ++i)
	{
		Stones[i]->SetActorLocation(FVector(10000.f));
	}
}

void AHazardPool::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AddSpikes(InitialSpikes);
	AddDarts(InitialDarts);
}

void AHazardPool::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (int32 i = 0; i < Spikes.Num(); ++i)
	{
		Spikes[i]->Destroy();
	}

	for (int32 i = 0; i < Darts.Num(); ++i)
	{
		Darts[i]->Destroy();
	}

	for (int32 i = 0; i < Stones.Num(); ++i)
	{
		Stones[i]->Destroy();
	}
}