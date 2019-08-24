// Fill out your copyright notice in the Description page of Project Settings.


#include "AudioManager.h"
#include "Sound/SoundCue.h"
#include "RlGameInstance.h"
#include "Components/AudioComponent.h"

AAudioManager::AAudioManager(const FObjectInitializer& ObjectInitializer)
{
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio Component"));
	AudioComponent->bAutoActivate = false;

	AudioComponent->SetupAttachment(RootComponent);
}

void AAudioManager::Init(URlGameInstance* InRlGI)
{
	RlGI = InRlGI;

	//if (Music)
	//{
	//	AudioComponent->SetSound(Music);
	//}
}

void AAudioManager::Play()
{
	AudioComponent->Play();
}

void AAudioManager::SetIntro()
{
	if (Intro)
	{
		AudioComponent->SetSound(Intro);
	}
}

void AAudioManager::SetMusic()
{
	if (Music)
	{
		AudioComponent->SetSound(Music);
	}
}

void AAudioManager::SetWin()
{
	if (Win)
	{
		AudioComponent->SetSound(Win);
	}
}
