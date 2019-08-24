// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimerManager.h"
#include "InputTutorial.generated.h"

class APaperTileMapActor;
class UPaperTileMap;
class ULevelManager;
class ARlCharacter;
class UWorld;
class URlGameInstance;

/**
 * 
 */
UCLASS()
class RAGELITE_API UInputTutorial : public UObject
{
	GENERATED_BODY()

public:
	UInputTutorial();

	~UInputTutorial();

	void Init(APaperTileMapActor* InTileMapActor, TArray<UPaperTileMap*> InTileMaps, URlGameInstance* InRlGameInstance);

	virtual UWorld* GetWorld() const override;

	void Update(int32 CurrentLevelIndex, bool bIsUsingGamepad);

private:
	bool bInit;

	APaperTileMapActor* TileMapActor;

	TArray<UPaperTileMap*> TileMaps;

	void HideLayer(int32 Layer);
	void ShowLayer(int32 Layer);
	void SetLayerColor(int32 Layer, FLinearColor Color);
	void SetLayerVisibility(int32 Layer, bool bVisible);
	bool GetLayerVisibility(int32 Layer);
	void ToggleLayers(int32 FirstLayer, int32 SecondLayer);
	void ShowLayers(int32 KeyboardLayer, int32 GamepadLayer);
	void ShowOnlyLayers(int32 KeyboardLayer, int32 GamepadLayer);

	UPaperTileMap* LastTileMap;
	int32 LastLevel;
	bool bWasUsingGamepad;


	// Animations
	FTimerHandle MainAnimationHandle;
	FTimerHandle SecondaryAnimationHandle;

	void LevelOneOne();
	void LevelOneTwo();
	void LevelOneThree();
	void LevelOneStairs();
	bool bLevelOneRight;

	void LevelTwoOne();
	void LevelTwoTwo();
	void LevelTwoThree();

	//void LevelThreeOne();
	//void LevelThreeTwo();

	void LevelThreeOne();
	void LevelThreeTwo();
	void LevelThreeThree();
	void LevelThreeFour();

	void LevelFourOne();
	void LevelFourTwo();



private:
	URlGameInstance* RlGameInstance;
	
};
