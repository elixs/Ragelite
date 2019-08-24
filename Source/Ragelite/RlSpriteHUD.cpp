// Fill out your copyright notice in the Description page of Project Settings.


#include "RlSpriteHUD.h"
#include "SpriteTextActor.h"
#include "RlGameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "RlGameMode.h"

void ARlSpriteHUD::IncreaseLevel()
{
	if (URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance()))
	{
		if (LevelText)
		{
			LevelText->SetText(FString::FromInt(++RlGI->Level));
		}
	}
}

void ARlSpriteHUD::IncreaseScore()
{
	if (URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance()))
	{
		if (ScoreText)
		{
			ScoreText->SetText(FString::FromInt(++RlGI->Score));
		}
	}
}

void ARlSpriteHUD::IncreaseTime()
{
	if (URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance()))
	{
		if (TimeText)
		{
			int32 TempTime = ++RlGI->Time;
			const int32 Seconds = TempTime % 60;
			TempTime /= 60;
			const int32 Minutes = TempTime % 60;
			const int32 Hours = TempTime /= 60;
			if (Hours)
			{
				TimeText->SetText(FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds));
			}
			else
			{
				TimeText->SetText(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
			}
		}
	}
}

void ARlSpriteHUD::IncreaseDeaths()
{
	if (URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance()))
	{
		if (DeathsText)
		{
			DeathsText->SetText(FString::FromInt(++RlGI->Deaths));
		}
	}


}

void ARlSpriteHUD::ResetHUD()
{
	if (URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance()))
	{
		RlGI->Level = 1;
		RlGI->Score = 0;
		RlGI->Time = 0;
		RlGI->Deaths = 0;

		if (ScoreText && TimeText  && DeathsText)
		{
			LevelText->SetText(FString::FromInt(1));
			ScoreText->SetText(FString::FromInt(0));
			TimeText->SetText(FString("00:00"));
			DeathsText->SetText(FString::FromInt(0));
		}
	}
}

void ARlSpriteHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance()))
	{
		RlGI->RlSpriteHUD = this;
	}
}

void ARlSpriteHUD::BeginPlay()
{
	Super::BeginPlay();

	ResetHUD();

	GetWorld()->GetTimerManager().ClearTimer(TimeHandle);
	GetWorld()->GetTimerManager().SetTimer(TimeHandle, this, &ARlSpriteHUD::TimeTick, 1.f, false);
}

void ARlSpriteHUD::TimeTick()
{
	if (ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (GameMode->bGameStarted)
		{
			IncreaseTime();
		}
	}

	GetWorld()->GetTimerManager().SetTimer(TimeHandle, this, &ARlSpriteHUD::TimeTick, 1.f, false);
}