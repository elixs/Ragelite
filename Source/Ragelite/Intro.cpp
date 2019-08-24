// Fill out your copyright notice in the Description page of Project Settings.


#include "Intro.h"
#include "Animation/WidgetAnimation.h"
#include "RlGameInstance.h"
#include "WidgetManager.h"
#include "LevelManager.h"

void UIntro::NativeConstruct()
{

}

void UIntro::MehStart()
{
	PlayAnimation(Meh);
	//GetWorld()->GetTimerManager().SetTimer(MehHandle, this, &UIntro::MehEnd, 1.f);
	GetWorld()->GetTimerManager().SetTimer(MehHandle, this, &UIntro::MehEnd, 68.f);
}

void UIntro::MehStart2()
{
	PlayAnimation(Meh2);
}

void UIntro::MehEnd()
{
	UWidgetManager* WM = Cast<URlGameInstance>(GetGameInstance())->WidgetManager;
	WM->EndIntro();

	ULevelManager* LM = Cast<URlGameInstance>(GetGameInstance())->LevelManager;
	LM->SetLevel(ELevelState::Start);
}
