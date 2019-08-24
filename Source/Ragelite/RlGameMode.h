// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RlGameMode.generated.h"

class UHearRateModule;

/**
 * The GameMode defines the game being played. It governs the game rules, scoring, what actors
 * are allowed to exist in this game type, and who may enter the game.
 *
 * This game mode just sets the default pawn to be the MyCharacter asset, which is a subclass of RageliteCharacter
 */
UCLASS(minimalapi)
class /*RAGELITE_API*/ ARlGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARlGameMode();

	UPROPERTY(EditAnywhere, Category = Config)
	float Difficulty;

	bool bGameStarted;

	UPROPERTY()
	UHearRateModule* HeartRateModule;


//public:
//	virtual void StartPlay() override;

	//virtual void Tick() override;

protected:
	virtual void BeginPlay() override;

//private:
//	FTimerHandle TimeHandle;
//
//	void TimeTick();
};
