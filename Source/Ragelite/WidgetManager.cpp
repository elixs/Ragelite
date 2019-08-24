// Fill out your copyright notice in the Description page of Project Settings.


#include "WidgetManager.h"
#include "Intro.h"
#include "Kismet/GameplayStatics.h"
#include "RlGameInstance.h"
#include "RlGameMode.h"
#include "AudioManager.h"
#include "Components/AudioComponent.h"
#include "RlCharacter.h"
#include "PaperFlipbookComponent.h"
#include "HearRateModule.h"
#include "DeviceSelection.h"

DEFINE_LOG_CATEGORY_STATIC(LogStatus, Log, All);


void UWidgetManager::Init(URlGameInstance* InRlGI)
{
	RlGI = InRlGI;
}


void UWidgetManager::StartIntro()
{
	if (IntroClass)
	{
		Intro = CreateWidget<UIntro>(RlGI->GetWorld(), IntroClass);

		Intro->AddToViewport();

		Intro->MehStart();

		auto PC = UGameplayStatics::GetPlayerController(RlGI->GetWorld(), 0);
		PC->SetInputMode(FInputModeGameOnly());

		UGameplayStatics::GetPlayerPawn(RlGI->GetWorld(), 0)->DisableInput(PC);
	}
}

void UWidgetManager::EndIntro()
{
	if (Intro)
	{
		Intro->RemoveFromParent();
	}

	auto PC = UGameplayStatics::GetPlayerController(RlGI->GetWorld(), 0);

	UGameplayStatics::GetPlayerPawn(RlGI->GetWorld(), 0)->EnableInput(PC);

	UE_LOG(LogStatus, Log, TEXT("Game Started"));

	if (RlGI->AudioManager)
	{
		RlGI->AudioManager->SetMusic();
		RlGI->AudioManager->Play();
	}
}

void UWidgetManager::EndGame()
{
	Cast<ARlGameMode>(RlGI->GetWorld()->GetAuthGameMode())->HeartRateModule->bEnabled = false;

	if (IntroClass)
	{
		Intro = CreateWidget<UIntro>(RlGI->GetWorld(), IntroClass);

		Intro->AddToViewport();

		Intro->MehStart2();

		auto PC = UGameplayStatics::GetPlayerController(RlGI->GetWorld(), 0);
		PC->SetInputMode(FInputModeGameOnly());

		UGameplayStatics::GetPlayerPawn(RlGI->GetWorld(), 0)->DisableInput(PC);

		Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(RlGI->GetWorld(), 0))->GetSprite()->ToggleVisibility();

		UE_LOG(LogStatus, Log, TEXT("Game Ended"));

		if (RlGI->AudioManager)
		{
			RlGI->AudioManager->SetWin();
			RlGI->AudioManager->Play();
		}
	}
}
