// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "RlLevelScriptActor.generated.h"

/**
 * 
 */
UCLASS()
class RAGELITE_API ARlLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class ACameraActor* MainCamera;

public:
	ARlLevelScriptActor();

protected:
	virtual void BeginPlay() override;
	
};
