// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Intro.generated.h"

class UWidgetAnimation;

/**
 * 
 */
UCLASS()
class RAGELITE_API UIntro : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;
	
	UPROPERTY(meta = (BindWidgetAnim))
	UWidgetAnimation* Meh;

	UPROPERTY(meta = (BindWidgetAnim))
	UWidgetAnimation* Meh2;

	void MehStart();
	void MehStart2();


private:
	FTimerHandle MehHandle;

	void MehEnd();
};
