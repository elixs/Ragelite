// Fill out your copyright notice in the Description page of Project Settings.

#include "RlGameMode.h"
#include "RlCharacter.h"
#include "RlGameInstance.h"
#include "RlPlayerController.h"
#include "Engine/World.h"
#include "SlateApplication.h"
#include "NavigationConfig.h"
#include "DeviceSelection.h"
#include "LevelManager.h"
#include "RlGameInstance.h"
#include "RlPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "HearRateModule.h"
#include "WidgetManager.h"
#include "AudioManager.h"
#include "Components/AudioComponent.h"

ARlGameMode::ARlGameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = ARlCharacter::StaticClass();
	PlayerControllerClass = ARlPlayerController::StaticClass();

	Difficulty = 0.5;

	bGameStarted = false;

	HeartRateModule = NewObject<UHearRateModule>();
	HeartRateModule->GameMode = this;
}

void ARlGameMode::BeginPlay()
{
	Super::BeginPlay();

	FSlateApplication& SlateApplication = FSlateApplication::Get();
	FNavigationConfig& NavigationConfig = SlateApplication.GetNavigationConfig().Get();

	NavigationConfig.KeyEventRules.Remove(EKeys::Left);
	NavigationConfig.KeyEventRules.Remove(EKeys::Right);
	NavigationConfig.KeyEventRules.Remove(EKeys::Up);
	NavigationConfig.KeyEventRules.Remove(EKeys::Down);

	NavigationConfig.KeyEventRules.Add(EKeys::A, EUINavigation::Left);
	NavigationConfig.KeyEventRules.Add(EKeys::D, EUINavigation::Right);
	NavigationConfig.KeyEventRules.Add(EKeys::W, EUINavigation::Up);
	NavigationConfig.KeyEventRules.Add(EKeys::S, EUINavigation::Down);

	URlGameInstance* RlGI = Cast<URlGameInstance>(GetWorld()->GetGameInstance());

	FRotator SpawnRotation(0.f);
	FActorSpawnParameters SpawnInfo;
	RlGI->AudioManager = GetWorld()->SpawnActor<AAudioManager>(RlGI->AudioManagerClass, FVector::ZeroVector, SpawnRotation, SpawnInfo);
	RlGI->AudioManager->Init(RlGI);

	RlGI->AudioManager->SetIntro();
	RlGI->AudioManager->Play();

	if (RlGI->bUseDevice)
	{
		UWidgetManager* WM = RlGI->WidgetManager;

		if (WM->DeviceSelectionClass)
		{
			WM->DeviceSelection = CreateWidget<UDeviceSelection>(GetWorld(), WM->DeviceSelectionClass);

			WM->DeviceSelection->AddToViewport();
			auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			PC->SetInputMode(FInputModeUIOnly());
			return;
		}
	}
	else
	{
		UWidgetManager* WM = RlGI->WidgetManager;

		// Swap to skip intro
		if (RlGI->bIntro)
		{
			WM->StartIntro();
		}
		else
		{
			if (RlGI->AudioManager)
			{
				RlGI->AudioManager->SetMusic();
				RlGI->AudioManager->Play();
			}

			ULevelManager* LM = Cast<URlGameInstance>(GetGameInstance())->LevelManager;
			LM->SetLevel(ELevelState::Start);
		}
	}
}