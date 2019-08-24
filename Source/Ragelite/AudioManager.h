// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AudioManager.generated.h"

class URlGameInstance;
class USoundCue;
class UAudioComponent;

/**
 * 
 */
UCLASS(Blueprintable)
class RAGELITE_API AAudioManager : public AActor
{
	GENERATED_BODY()

public:

	AAudioManager(const FObjectInitializer& ObjectInitializer);

	void Init(URlGameInstance* InRlGI);

	void Play();

	void SetIntro();

	void SetMusic();

	void SetWin();

	URlGameInstance* RlGI;

	UPROPERTY(VisibleAnywhere)
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* Intro;
	
	UPROPERTY(EditAnywhere)
	USoundCue* Music;

	UPROPERTY(EditAnywhere)
	USoundCue* Win;

	UPROPERTY(EditAnywhere)
	USoundCue* Jump;

	UPROPERTY(EditAnywhere)
	USoundCue* Run;

	UPROPERTY(EditAnywhere)
	USoundCue* Walk;

	UPROPERTY(EditAnywhere)
	USoundCue* Death;

	UPROPERTY(EditAnywhere)
	USoundCue* Dart;
};
