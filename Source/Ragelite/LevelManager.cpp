// Fill out your copyright notice in the Description page of Project Settings.

#include "LevelManager.h"
#include "RlGameInstance.h"
#include "Paper2D/Classes/PaperTileMap.h"
#include "Paper2D/Classes/PaperTileMapComponent.h"
#include "Paper2D/Classes/PaperTileMapActor.h"
#include "Paper2D/Classes/PaperSprite.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"
#include "Paper2D/Classes/PaperSpriteActor.h"
#include "Materials/Material.h"
#include "ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "RlCharacter.h"
#include "Stairs.h"
#include "Components/BoxComponent.h"
#include "RlCharacterMovementComponent.h"
#include "RlPlayerController.h"
#include "Hazard.h"
#include "Spike.h"
#include "HazardPool.h"
#include "RlGameMode.h"
#include "Paper2D/Classes/PaperFlipbookComponent.h"
#include "RlGameInstance.h"
#include "RlSpriteHUD.h"
#include "Projectile.h"
#include "InputTutorial.h"
#include "RlGameMode.h"
#include "HearRateModule.h"
#include "WidgetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogStatus, Log, All);

ULevelManager::ULevelManager(const FObjectInitializer& ObjectInitializer)
{
	CurrentLevel = nullptr;
	CurrentStairs = nullptr;
	HazardPool = nullptr;

	CurrentLevelIndex = 0;

	SpawnLocation = FVector(-208.f, -15.0f, 89.f);
	SpawnRotation = FRotator(0.f);

	TileSize = 16;

	FadeTime = 0.5f;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialRef(TEXT("/Game/Materials/MaskedUnlitSpriteMaterial"));

	if (MaterialRef.Object)
	{
		Material = MaterialRef.Object;
	}

	bTimeStarted = false;
}

void ULevelManager::Init(URlGameInstance* InRlGameInstance)
{
	RlGameInstance = InRlGameInstance;
	RlGameInstance->OnShutdown.BindUFunction(this, FName("EndGame"));
}

UWorld* ULevelManager::GetWorld() const
{
	return RlGameInstance->GetWorld();
}

UPaperSprite* ULevelManager::GetSpikeSprite() const
{
	if (SpikesSprites.Num())
	{
		return SpikesSprites[FMath::RandRange(0, SpikesSprites.Num() - 1)];
	}
	return nullptr;
}

UPaperSprite* ULevelManager::GetDartSprite() const
{
	if (DartsSprites.Num())
	{
		return DartsSprites[FMath::RandRange(0, DartsSprites.Num() - 1)];
	}
	return nullptr;
}

UPaperSprite* ULevelManager::GetStoneSprite() const
{
	if (StonesSprites.Num())
	{
		return StonesSprites[FMath::RandRange(0, StonesSprites.Num() - 1)];
	}
	return nullptr;
}

void ULevelManager::EndGame()
{
	if (CurrentLevel && !CurrentLevel->IsPendingKill())
	{
		CurrentLevel->Destroy();
	}
	CurrentLevel = nullptr;

	if (InputTutorialTileMapActor && !InputTutorialTileMapActor->IsPendingKill())
	{
		InputTutorialTileMapActor->Destroy();
	}
	InputTutorialTileMapActor = nullptr;

	if (CurrentStairs && !CurrentStairs->IsPendingKill())
	{
		//CurrentStairs->OnRlCharacterOverlapBegin.Unbind();
		CurrentStairs->Destroy();
	}
	CurrentStairs = nullptr;

	if (HazardPool && !HazardPool->IsPendingKill())
	{
		HazardPool->Destroy();
	}
	HazardPool = nullptr;

	if (InputTutorial)
	{
		delete InputTutorial;
	}
	InputTutorial = nullptr;

	bTimeStarted = false;
}

void ULevelManager::SetLevel(ELevelState State)
{
	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (State == ELevelState::Start)
	{
		// Change to one to skip debug level
		CurrentLevelIndex = 1;
		RlGameInstance->Level = 1;
		UE_LOG(LogStatus, Log, TEXT("Current Level: %i"), CurrentLevelIndex);
	}
	if (State == ELevelState::Next)
	{
		++CurrentLevelIndex;
		UE_LOG(LogStatus, Log, TEXT("Current Level: %i"), CurrentLevelIndex);
		UE_LOG(LogStatus, Log, TEXT("Deaths: %i"), RlGameInstance->Deaths);
		UE_LOG(LogStatus, Log, TEXT("Time: %i"), RlGameInstance->Time);

	}
	if (CurrentLevelIndex < Levels.Num())
	{
		if (CurrentLevelIndex == 1)
		{
			RlCharacter->bRunEnabled = false;
			RlCharacter->bJumpEnabled = false;
			RlCharacter->bLongJumpEnabled = false;
			RlCharacter->bWallWalkEnabled = false;
		}
		if (CurrentLevelIndex == 2)
		{
			RlCharacter->bRunEnabled = true;
		}
		if (CurrentLevelIndex == 3)
		{
			RlCharacter->bJumpEnabled = true;
		}
		if (CurrentLevelIndex == 4)
		{
			RlCharacter->bLongJumpEnabled = true;
		}
		if (CurrentLevelIndex == 5)
		{
			RlCharacter->bWallWalkEnabled = true;
		}
		if (CurrentLevelIndex == 6)
		{
			if (ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode()))
			{
				if (GameMode->HeartRateModule->bCalibration)
				{
					GameMode->HeartRateModule->EndCalibration();
				}
			}
		}
		if (CurrentLevelIndex == Levels.Num() - 1)
		{
			// Disable input
			// Hide character
			// Show last picture
			RlGameInstance->WidgetManager->EndGame();
		}
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUFunction(this, FName("StartLevel"), State);
		FadeIn(TimerDelegate);
	}
	else
	{
		--CurrentLevelIndex;
		// Finished game
	}
}

void ULevelManager::RestartLevel()
{
	SetLevel(ELevelState::Reset);
}

void ULevelManager::SetLevelTileMap()
{
	if (Levels[CurrentLevelIndex].PaperTileMap && CurrentLevel)
	{
		CurrentLevel->GetRenderComponent()->SetTileMap(Levels[CurrentLevelIndex].PaperTileMap);
	}
}

void ULevelManager::SpawnObstacles()
{
	int32 SpikesNum = 0;
	int32 DartsNum = 0;
	int32 StonesNum = 0;

	for (auto HazardsData : Levels[CurrentLevelIndex].Hazards)
	{
		if (HazardsData.HazardsType == EHazardType::Spikes)
		{
			SpikesNum += HazardsData.Num();
		}
		else if (HazardsData.HazardsType == EHazardType::Darts)
		{
			DartsNum += HazardsData.Num();
		}
		else if (HazardsData.HazardsType == EHazardType::Stones)
		{
			StonesNum += HazardsData.Num();
		}
	}

	// We have to check the correct amount of spikes
	while (SpikesNum > HazardPool->Spikes.Num())
	{
		//HazardPool->AddSpikesUntil(Spikes);
		HazardPool->AddSpikes(HazardPool->InitialSpikes);
	}

	while (DartsNum > HazardPool->Darts.Num())
	{
		HazardPool->AddDarts(HazardPool->InitialDarts);
	}

	while (StonesNum > HazardPool->Stones.Num())
	{
		HazardPool->AddStones(HazardPool->InitialStones);
	}
}

void ULevelManager::MoveActors()
{
	// Move RlCharacter
	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	RlCharacter->GetRlCharacterMovement()->StopActiveMovement();
	RlCharacter->GetRlCharacterMovement()->bJustTeleported = true;
	RlCharacter->SetActorLocation(GetRelativeLocation(Levels[CurrentLevelIndex].Start) - FVector(0.f, 0.f, 4.f));

	CurrentStairs->SetActorLocation(GetRelativeLocation(Levels[CurrentLevelIndex].Stairs, -10.f));

	HazardPool->ResetHazards(Levels[CurrentLevelIndex].Hazards);
}

void ULevelManager::UpdateInputTutorial()
{
	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	InputTutorial->Update(CurrentLevelIndex, RlCharacter->bIsUsingGamepad);
}

FVector ULevelManager::GetRelativeLocation(FVector2D Coords, int32 Y)
{
	return FVector(SpawnLocation.X + Coords.X * TileSize, Y, SpawnLocation.Z - Coords.Y * TileSize/* + 4.f*/);
}

void ULevelManager::StartLevel(ELevelState State)
{
	if (State == ELevelState::Start)
	{
		EndGame();

		//CurrentLevelIndex = 0;

		CurrentStairs = GetWorld()->SpawnActor<AStairs>(GetRelativeLocation(Levels[CurrentLevelIndex].Stairs, -10.f), SpawnRotation, SpawnInfo);

		UPaperSpriteComponent* CurrentStairsRC = CurrentStairs->GetRenderComponent();
		CurrentStairsRC->SetSprite(StairsSprite);
		CurrentStairsRC->SetMaterial(0, Material);

		CurrentLevel = GetWorld()->SpawnActor<APaperTileMapActor>(SpawnLocation + FVector(-8.f, 0.f, 8.f), SpawnRotation, SpawnInfo);
		CurrentLevel->GetRenderComponent()->SetMaterial(0, Material);

		InputTutorialTileMapActor = GetWorld()->SpawnActor<APaperTileMapActor>(SpawnLocation + FVector(-8.f, 14.f, 0.f), SpawnRotation, SpawnInfo);
		InputTutorialTileMapActor->GetRenderComponent()->SetMaterial(0, Material);

		HazardPool = GetWorld()->SpawnActor<AHazardPool>(FVector::ZeroVector, SpawnRotation, SpawnInfo);


		InputTutorial = NewObject<UInputTutorial>();
		InputTutorial->Init(InputTutorialTileMapActor, InputTutorialTileMaps, RlGameInstance);
	}
	
	if (State != ELevelState::Reset)
	{
		SetLevelTileMap();
		SpawnObstacles();
	}
	if (State == ELevelState::Next)
	{
		if (ARlSpriteHUD* RlSHUD = RlGameInstance->RlSpriteHUD)
		{
			RlSHUD->IncreaseLevel();
		}
	}

	DestroyProjectiles();

	MoveActors();

	UpdateInputTutorial();

	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (RlCharacter->bDeath)
	{
		RlCharacter->bDeath = false;
		RlCharacter->GetSprite()->ToggleVisibility();
	}

	FadeOut();
}

void ULevelManager::FadeIn(FTimerDelegate TimerDelegate)
{
	if (ARlPlayerController* RlPlayerController = Cast<ARlPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		RlPlayerController->FadeInBack(FadeTime / 2.f);
		//RlPlayerController->SetIgnoreMoveInput(true);
		GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, TimerDelegate, FadeTime / 2.f, false);
	}
}

void ULevelManager::FadeOut()
{
	if (ARlPlayerController* RlPlayerController = Cast<ARlPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		RlPlayerController->FadeOutBack(FadeTime / 2.f);
	}

	GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, this, &ULevelManager::FadeOutCallback, FadeTime / 2.f, false);
}

void ULevelManager::FadeOutCallback()
{
	if (ARlPlayerController* RlPlayerController = Cast<ARlPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		RlPlayerController->SetIgnoreMoveInput(false);
	}

	if (!bTimeStarted)
	{
		if (ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GameMode->bGameStarted = true;
			bTimeStarted = true;
		}
	}
	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	RlCharacter->bInvincible = false;
}

void ULevelManager::DestroyProjectiles()
{
	TArray<AActor*> Projectiles;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProjectile::StaticClass(), Projectiles);
	for (AActor* Projectile : Projectiles)
	{
		Projectile->Destroy();
	}
}