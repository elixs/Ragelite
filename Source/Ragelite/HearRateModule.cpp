// Fill out your copyright notice in the Description page of Project Settings.

#include "HearRateModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "RlGameMode.h"
#include "RlGameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogHeartRateModule, Log, All);

UHearRateModule::UHearRateModule()
{
	//ScopePercentage = 0.2f;
	ScopePercentage = 0.15f;
	//ScopePercentage = 0.1f;

	//DeadZoneFactor = 0.25;
	DeadZoneFactor = 0.375;
	//DeadZoneFactor = 0.5;

	HeartRateMin = -1;
	HeartRateMax = 0;

	//HeartRateMean = 0.f;
	HeartRateMedian = -1 / 2;

	bEnabled = false;
	bCalibration = false;
}

void UHearRateModule::AddHeartRate(uint8 HeartRate)
{

	if (bEnabled && HeartRate)
	{

		//UKismetSystemLibrary::PrintString(GameMode->GetWorld(), FString::Printf(TEXT("Heart Rate: %i"), HeartRate), true, true, FLinearColor(0.0, 0.66, 1.0), 20.f);

		UE_LOG(LogHeartRateModule, Log, TEXT("Heart Rate: %i"), HeartRate);

		if (bCalibration)
		{
			HeartRates.Add(HeartRate);
		}
		else
		{
			float Change = HeartRate - HeartRateMedian;
			// The difficulty is calculated comparing the current heart rate with the min and max.
			// Also the median and scope are adjusted.
			// The median can vary at most one per input received

			// Si su pulso se estabiliza en un nuevo valor debería considerarse estable luego de 2 minutos (30 mediciones aprox)

			HeartRateMedian += Change / 30;

			// El focus se reajusta de tal manera que a lo más cambie un 5% por minuto
			// Para que esto ocurra al menos la mitad de las mediciones debieron estar al menos a un 100% de distancia de la deadzone

			float DeadZone = HeartRateMedian * ScopePercentage * DeadZoneFactor;

			float PercentageOutside = FMath::Clamp((FMath::Abs(Change) - DeadZone) / DeadZone, -1.f, 1.f);

			ScopePercentage += 0.05f * PercentageOutside / 15;

			HeartRateMin = (1.f - ScopePercentage) * HeartRateMedian;
			HeartRateMax = (1.f + ScopePercentage) * HeartRateMedian;

			if (GameMode)
			{
				float TargetDifficulty = FMath::Clamp((HeartRateMax - HeartRate) / (HeartRateMax - HeartRateMin), 0.f, 1.f);

				URlGameInstance* RlGI = Cast<URlGameInstance>(GameMode->GetGameInstance());

				if (RlGI->bDynamicDifficulty)
				{
					GameMode->Difficulty = TargetDifficulty;
				}

				//UKismetSystemLibrary::PrintString(GameMode->GetWorld(), FString::Printf(TEXT("HR Module: M %f P %f D %f"), HeartRateMedian, ScopePercentage, GameMode->Difficulty), true, true, FLinearColor(0.0, 0.66, 1.0), 20.f);
				UE_LOG(LogHeartRateModule, Log, TEXT("HR Module: M %f P %f D %f"), HeartRateMedian, ScopePercentage, TargetDifficulty);
			}
		}
	}
}

void UHearRateModule::AddHeartRate(FString HearRate)
{
	AddHeartRate(FCString::Atoi(*HearRate));
}

void UHearRateModule::StartCalibration()
{
	UE_LOG(LogHeartRateModule, Log, TEXT("Calibrarion Started"));
	bEnabled = bCalibration = true;
}

void UHearRateModule::EndCalibration()
{
	UE_LOG(LogHeartRateModule, Log, TEXT("Calibrarion Ended"));
	bCalibration = false;

	if (HeartRates.Num())
	{
		HeartRates.Sort();

		HeartRateMedian = HeartRates[(HeartRates.Num() - 1) / 2];

		float FirstHalfMean = 0.f;
		float SecondHalfMean = 0.f;

		for (int32 i = 0; i < HeartRates.Num(); ++i)
		{
			if (i < HeartRates.Num() / 2)
			{
				FirstHalfMean += HeartRates[i];
			}
			else
			{
				SecondHalfMean += HeartRates[i];
			}
		}

		FirstHalfMean /= HeartRates.Num() / 2;
		SecondHalfMean /= HeartRates.Num() - (HeartRates.Num() / 2);

		float LowerDeadZone = HeartRateMedian - FirstHalfMean;
		float HigherDeadZone = SecondHalfMean - HeartRateMedian;

		float DeadZone = HeartRateMedian * ScopePercentage * DeadZoneFactor;

		float PercentageFix = (FMath::Max(LowerDeadZone - DeadZone, 0.f) + FMath::Max(HigherDeadZone - DeadZone, 0.f)) / 2.f;

		ScopePercentage += PercentageFix / HeartRateMedian;

		HeartRateMin = (1.f - ScopePercentage) * HeartRateMedian;
		HeartRateMax = (1.f + ScopePercentage) * HeartRateMedian;

		UE_LOG(LogHeartRateModule, Log, TEXT("Result: %f -> %f - %f"), ScopePercentage, HeartRateMin, HeartRateMax);
	}
}