// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RlPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class RAGELITE_API ARlPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Allows the PlayerController to set up custom input bindings. */
	virtual void SetupInputComponent() override;
	
public:
	void FadeInBack(float Seconds);

	void FadeOutBack(float Seconds);

private:
	void Exit();

	bool bMouseToggle = false;

public:
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;
};
