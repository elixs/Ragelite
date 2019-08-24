// Fill out your copyright notice in the Description page of Project Settings.

#include "RlGameInstance.h"
#include "LevelManager.h"
#include "WidgetManager.h"
#include "AudioManager.h"
#include "SpriteTextActor.h"
#include "RlSpriteHUD.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY_STATIC(LogStatus, Log, All);

URlGameInstance::URlGameInstance(const FObjectInitializer& ObjectInitializer)
{
	bUseDevice = !FParse::Param(FCommandLine::Get(), TEXT("nodevice"));
	//bUseDevice = false;

	UE_LOG(LogStatus, Log, TEXT("Use device? %i"), bUseDevice);

	bDynamicDifficulty = !FParse::Param(FCommandLine::Get(), TEXT("nochange"));
	//bDynamicDifficulty = false;

	bIntro = !FParse::Param(FCommandLine::Get(), TEXT("nointro"));
	//bIntro = false;

	UE_LOG(LogStatus, Log, TEXT("Use dynamic difficulty? %i"), bDynamicDifficulty);
}

void URlGameInstance::Init()
{
	LevelManager = LevelManagerPtr->GetDefaultObject<ULevelManager>();
	LevelManager->Init(this);

	WidgetManager = WidgetManagerClass->GetDefaultObject<UWidgetManager>();
	WidgetManager->Init(this);

	//AudioManager = AudioManagerClass->GetDefaultObject<AAudioManager>();

	//FRotator SpawnRotation(0.f);
	//FActorSpawnParameters SpawnInfo;
	//AudioManager = GetWorld()->SpawnActor<AAudioManager>(AudioManagerClass, FVector::ZeroVector, SpawnRotation, SpawnInfo);
	//AudioManager->Init(this);
}

void URlGameInstance::Shutdown()
{
	OnShutdown.ExecuteIfBound();
}
