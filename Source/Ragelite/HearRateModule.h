// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HearRateModule.generated.h"


class ARlGameMode;
/**
 * 
 */
UCLASS()
class RAGELITE_API UHearRateModule : public UObject
{
	GENERATED_BODY()
	
public:
	UHearRateModule();

	TArray<uint8> HeartRates;

	// Estimation of heart rate peaks
	float ScopePercentage;

	// Used to adjust Min and Max
	float DeadZoneFactor;

	float HeartRateMin;
	float HeartRateMax;

	float HeartRateMedian;

	//uint64 HeartRateSum;

	void AddHeartRate(uint8 HearRate);
	void AddHeartRate(FString HearRate);

	void StartCalibration();
	void EndCalibration();

	bool bCalibration;


	// For debug
	UPROPERTY()
	ARlGameMode* GameMode;


//private:
	bool bEnabled;
};
