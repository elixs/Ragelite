// Fill out your copyright notice in the Description page of Project Settings.

#include "RlPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RlCharacter.h"
#include "Engine/World.h"
#include "RlGameInstance.h"
#include "LevelManager.h"

void ARlPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("Exit", IE_Pressed, this, &ARlPlayerController::Exit);
}

void ARlPlayerController::FadeInBack(float Seconds)
{
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(0.f, 1.f, Seconds, FLinearColor::Black, false, true);
	}
}

void ARlPlayerController::FadeOutBack(float Seconds)
{
	PlayerCameraManager->StartCameraFade(1.f, 0.f, Seconds, FLinearColor::Black, false, false);
}

void ARlPlayerController::Exit()
{
	bMouseToggle = !bMouseToggle;
	bShowMouseCursor = bMouseToggle;
	ConsoleCommand("quit");
}

bool ARlPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	bool Result = Super::InputKey(Key, EventType, AmountDepressed, bGamepad);

	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (RlCharacter)
	{
		//RlCharacter->bIsUsingGamepad = Key.IsGamepadKey();
		RlCharacter->bIsUsingGamepad = bGamepad;
		ULevelManager* LM = Cast<URlGameInstance>(GetGameInstance())->LevelManager;
		if (LM->InputTutorial)
		{
			LM->UpdateInputTutorial();
		}
	}

	return Result;
}