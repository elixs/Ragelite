// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RlSpriteHUD.generated.h"

class ASpriteTextActor;

UCLASS()
class RAGELITE_API ARlSpriteHUD : public AActor
{
	GENERATED_BODY()
	
public:

	void IncreaseScore();
	void IncreaseLevel();
	void IncreaseTime();
	void IncreaseDeaths();

	void ResetHUD();

	void PostInitializeComponents() override;


protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	ASpriteTextActor* LevelText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	ASpriteTextActor* ScoreText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	ASpriteTextActor* TimeText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	ASpriteTextActor* DeathsText;

	UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	AActor* Test;

private:
	FTimerHandle TimeHandle;

	void TimeTick();

};
