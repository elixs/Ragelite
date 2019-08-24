// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WidgetManager.generated.h"

class UIntro;
class UDeviceSelection;
class URlGameInstance;

/**
 * 
 */
UCLASS(Blueprintable)
class RAGELITE_API UWidgetManager : public UObject
{
	GENERATED_BODY()

public:

	void Init(URlGameInstance* InRlGI);

	URlGameInstance* RlGI;


	UPROPERTY(Category = Devices, EditAnywhere)
	TSubclassOf<UIntro> IntroClass;
	
	UIntro* Intro;

	UPROPERTY(Category = Devices, EditAnywhere)
	TSubclassOf<UDeviceSelection> DeviceSelectionClass;

	UPROPERTY()
	UDeviceSelection* DeviceSelection;

	void StartIntro();

	void EndIntro();

	void EndGame();

};
