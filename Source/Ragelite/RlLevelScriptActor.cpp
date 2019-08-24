// Fill out your copyright notice in the Description page of Project Settings.

#include "RlLevelScriptActor.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine.h"
#include "Paper2D/Classes/PaperTileMapActor.h"
#include "Paper2D/Classes/PaperTileMapComponent.h"
#include "Paper2D/Classes/PaperTileMap.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/KismetMathLibrary.h"

ARlLevelScriptActor::ARlLevelScriptActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARlLevelScriptActor::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters ActorSpawnParameters;
	MainCamera = GetWorld()->SpawnActor<ACameraActor>(FVector(0.f), FRotator(0.f, 270.f, 0.f), ActorSpawnParameters);

	MainCamera->GetCameraComponent()->ProjectionMode = ECameraProjectionMode::Orthographic;
	MainCamera->GetCameraComponent()->OrthoWidth = 480;
	MainCamera->GetCameraComponent()->OrthoNearClipPlane = -100;

	MainCamera->GetCameraComponent()->bConstrainAspectRatio = false;

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PlayerController->SetViewTargetWithBlend(MainCamera);
	}
}