// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimerManager.h"
#include "RLTypes.h"
#include "Engine/World.h"
#include "LevelManager.generated.h"


class URlGameInstance;
class UPaperTileMap;
class UPaperTileMapComponent;
class APaperTileMapActor;
class UPaperSprite;
class APaperSpriteActor;
class UMaterialInterface;
class UWorld;
class AStairs;
class UPrimitiveComponent;
class AHazardPool;
class ARlSpikes;
class UInputTutorial;
class UUserWidget;

/**
 * 
 */
UCLASS(Blueprintable)
class RAGELITE_API ULevelManager : public UObject
{
	GENERATED_BODY()

	ULevelManager(const FObjectInitializer& ObjectInitializer);

public:
	void Init(URlGameInstance* InRlGameInstance);

	virtual UWorld* GetWorld() const override;

	UPaperSprite* GetSpikeSprite() const;
	UPaperSprite* GetDartSprite() const;
	UPaperSprite* GetStoneSprite() const;

public:
	UPROPERTY(Category = Levels, VisibleAnywhere)
	FVector SpawnLocation;

	UPROPERTY(Category = Levels, EditAnywhere)
	TArray<FRlLevel> Levels;

	UPROPERTY(Category = Sprites, EditAnywhere)
	UPaperSprite* StairsSprite;

	UPROPERTY(Category = Sprites, EditAnywhere)
	TArray<UPaperSprite*> SpikesSprites;

	UPROPERTY(Category = Sprites, EditAnywhere)
	UPaperSprite* DartSprite;

	UPROPERTY(Category = Sprites, EditAnywhere)
	TArray<UPaperSprite*> DartsSprites;

	UPROPERTY(Category = Sprites, EditAnywhere)
	TArray<UPaperSprite*> StonesSprites;

	UPROPERTY(Category = Input, EditAnywhere)
	TArray<UPaperTileMap*> InputTutorialTileMaps;

	UPROPERTY(Category = Sprites, EditAnywhere)
	UMaterialInterface* Material;

	UPROPERTY(Category = Sprites, EditAnywhere)
	int32 TileSize;

	UPROPERTY(Category = Camera, EditAnywhere)
	float FadeTime;

	UPROPERTY(Category = Debug, EditAnywhere)
	float CurrentDifficulty_Debug = 0.5;

	int32 CurrentLevelIndex;

	APaperTileMapActor* CurrentLevel;

	APaperTileMapActor* InputTutorialTileMapActor;

	AHazardPool* HazardPool;

	UPROPERTY()
	UInputTutorial* InputTutorial;

public:

	UFUNCTION()
	void EndGame();

	void SetLevel(ELevelState State);

	void RestartLevel();


	FVector GetRelativeLocation(FVector2D Coords, int32 Y = 0.f);

	void UpdateInputTutorial();

private:
	URlGameInstance* RlGameInstance;

	FRotator SpawnRotation;
	FActorSpawnParameters SpawnInfo;

	AStairs* CurrentStairs;

	UFUNCTION()
	void StartLevel(ELevelState State);


	void SetLevelTileMap();
	void SpawnObstacles();
	void MoveActors();

	FTimerHandle FadeTimerHandle;
	void FadeIn(FTimerDelegate TimerDelegate);
	void FadeOut();
	void FadeOutCallback();

	void DestroyProjectiles();

	bool bTimeStarted;
};
