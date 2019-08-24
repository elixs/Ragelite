// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RlGameInstance.generated.h"

DECLARE_DELEGATE(FShutdownSignature)

class ULevelManager;
class UWidgetManager;
class AAudioManager;
class ASpriteTextActor;
class ARlSpriteHUD;

/**
 * 
 */
UCLASS(Blueprintable)
class RAGELITE_API URlGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
	URlGameInstance(const FObjectInitializer& ObjectInitializer);

public:

	virtual void Init() override;

	virtual void Shutdown() override;

	FShutdownSignature OnShutdown;

	// HUD variables
	int32 Level;
	int32 Score;
	int32 Time;
	int32 Deaths;

	ARlSpriteHUD* RlSpriteHUD;

	bool bUseDevice;

	bool bDynamicDifficulty;

	bool bIntro;

public:

	ULevelManager* LevelManager;
	UWidgetManager* WidgetManager;
	UPROPERTY()
	AAudioManager* AudioManager;

private:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Levels", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ULevelManager> LevelManagerPtr;	

	UPROPERTY(EditDefaultsOnly, Category = "Widgets", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UWidgetManager> WidgetManagerClass;

public:

	UPROPERTY(EditDefaultsOnly, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AAudioManager> AudioManagerClass;

};

