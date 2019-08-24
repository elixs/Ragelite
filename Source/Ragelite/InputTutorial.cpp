// Fill out your copyright notice in the Description page of Project Settings.

#include "InputTutorial.h"
#include "Paper2D/Classes/PaperTileMapActor.h"
#include "Paper2D/Classes/PaperTileMapComponent.h"
#include "Paper2D/Classes/PaperTileMap.h"
#include "RlCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "LevelManager.h"
#include "RlGameInstance.h"

UInputTutorial::UInputTutorial()
{
	bWasUsingGamepad = false;

	bLevelOneRight = false;

	bInit = false;
}

UInputTutorial::~UInputTutorial()
{
	bWasUsingGamepad = false;

	bLevelOneRight = false;

	bInit = false;
}

void UInputTutorial::Init(APaperTileMapActor* InTileMapActor, TArray<UPaperTileMap*> InTileMaps, URlGameInstance* InRlGameInstance)
{
	TileMapActor = InTileMapActor;
	TileMaps = InTileMaps;

	RlGameInstance = InRlGameInstance;

	GetWorld()->GetTimerManager().ClearTimer(MainAnimationHandle);
	GetWorld()->GetTimerManager().ClearTimer(SecondaryAnimationHandle);

	bInit = true;
}

UWorld* UInputTutorial::GetWorld() const
{
	return RlGameInstance->GetWorld();
}

void UInputTutorial::Update(int32 CurrentLevelIndex, bool bIsUsingGamepad)
{
	bool bLastWasUsingGamepad = bWasUsingGamepad;
	bWasUsingGamepad = bIsUsingGamepad;
	if (bInit)
	{
		if (TileMaps.IsValidIndex(CurrentLevelIndex - 1))
		{
			UPaperTileMap* CurrentTileMap = TileMaps[CurrentLevelIndex - 1];
			if (CurrentTileMap != LastTileMap || LastLevel != CurrentLevelIndex)
			{
				LastTileMap = CurrentTileMap;
				LastLevel = CurrentLevelIndex;

				GetWorld()->GetTimerManager().ClearTimer(MainAnimationHandle);
				GetWorld()->GetTimerManager().ClearTimer(SecondaryAnimationHandle);

				UPaperTileMapComponent* RenderComponent = TileMapActor->GetRenderComponent();
				RenderComponent->SetTileMap(TileMaps[CurrentLevelIndex - 1]);
				RenderComponent->SetVisibility(true);

				for (int32 i = 0; i < RenderComponent->TileMap->TileLayers.Num(); ++i)
				{
					HideLayer(i);
				}
				switch (CurrentLevelIndex)
				{
				case 1:
					LevelOneOne();
					break;
				case 2:
				case 5:
					LevelTwoOne();
					break;
				case 3:
					LevelThreeOne();
					break;
				case 4:
					LevelFourOne();
					break;
				default:
					break;
				}
			}
			else if (bLastWasUsingGamepad != bWasUsingGamepad)
			{
				switch (CurrentLevelIndex)
				{
				case 1:
					ToggleLayers(0, 3);
					ToggleLayers(1, 4);
					ToggleLayers(2, 5);

					ToggleLayers(6, 8);
					ToggleLayers(7, 9);
					break;
				case 2:
				case 5:
					ToggleLayers(0, 3);
					ToggleLayers(1, 4);
					ToggleLayers(2, 5);
					break;
				case 3:
				case 4:
					ToggleLayers(0, 2);
					ToggleLayers(1, 3);
					//ToggleLayers(0, 4);
					//ToggleLayers(1, 5);
					//ToggleLayers(2, 6);
					//ToggleLayers(3, 7);
					break;
				default:
					break;
				}
			}
		}
		else
		{
			TileMapActor->GetRenderComponent()->SetVisibility(false);
		}
	}
}

void UInputTutorial::HideLayer(int32 Layer)
{
	SetLayerVisibility(Layer, false);
	//SetLayerColor(Layer, FLinearColor(1.f, 1.f, 1.f, 0.f));
}

void UInputTutorial::ShowLayer(int32 Layer)
{
	SetLayerVisibility(Layer, true);
	//SetLayerColor(Layer, FLinearColor(1.f, 1.f, 1.f, 1.f));
}

void UInputTutorial::SetLayerColor(int32 Layer, FLinearColor Color)
{
	if (TileMapActor)
	{
		UPaperTileMapComponent* RenderComponent = TileMapActor->GetRenderComponent();
		if (RenderComponent->TileMap->TileLayers.IsValidIndex(Layer))
		{
			RenderComponent->TileMap->TileLayers[Layer]->SetLayerColor(Color);
			RenderComponent->MarkRenderStateDirty();
		}
	}
}

void UInputTutorial::SetLayerVisibility(int32 Layer, bool bVisible)
{
	if (TileMapActor)
	{
		UPaperTileMapComponent* RenderComponent = TileMapActor->GetRenderComponent();
		if (RenderComponent->TileMap->TileLayers.IsValidIndex(Layer))
		{
#if WITH_EDITOR
			RenderComponent->TileMap->TileLayers[Layer]->SetShouldRenderInEditor(bVisible);
#else
			RenderComponent->TileMap->TileLayers[Layer]->SetLayerColor(FLinearColor(1.f, 1.f, 1.f, bVisible ? 1.f : 0.f));
#endif
			RenderComponent->MarkRenderStateDirty();
		}
	}
}

bool UInputTutorial::GetLayerVisibility(int32 Layer)
{
	if (TileMapActor)
	{
		UPaperTileMapComponent* RenderComponent = TileMapActor->GetRenderComponent();
		if (RenderComponent->TileMap->TileLayers.IsValidIndex(Layer))
		{
#if WITH_EDITOR
			return RenderComponent->TileMap->TileLayers[Layer]->ShouldRenderInEditor();
#else
			return RenderComponent->TileMap->TileLayers[Layer]->GetLayerColor().A == 1.f;
#endif
		}
	}
	return false;
}

void UInputTutorial::ToggleLayers(int32 KeyboardLayer, int32 GamepadLayer)
{
	if (bWasUsingGamepad)
	{
		if (GetLayerVisibility(KeyboardLayer))
		{
			HideLayer(KeyboardLayer);
			ShowLayer(GamepadLayer);
		}
	}
	else
	{
		if (GetLayerVisibility(GamepadLayer))
		{
			HideLayer(GamepadLayer);
			ShowLayer(KeyboardLayer);
		}
	}
}

void UInputTutorial::ShowLayers(int32 KeyboardLayer, int32 GamepadLayer)
{
	if (bWasUsingGamepad)
	{
		HideLayer(KeyboardLayer);
		ShowLayer(GamepadLayer);
	}
	else
	{
		HideLayer(GamepadLayer);
		ShowLayer(KeyboardLayer);
	}
}

void UInputTutorial::ShowOnlyLayers(int32 KeyboardLayer, int32 GamepadLayer)
{
	for (int32 i = 0; i < TileMapActor->GetRenderComponent()->TileMap->TileLayers.Num(); ++i)
	{
		HideLayer(i);
	}
	ShowLayers(KeyboardLayer, GamepadLayer);
}

void UInputTutorial::LevelOneOne()
{
	HideLayer(1);
	HideLayer(2);
	HideLayer(4);
	HideLayer(5);
	HideLayer(7);
	HideLayer(9);

	ShowLayers(0, 3);
	ShowLayers(6, 8);

	if (bLevelOneRight)
	{
		GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelOneThree, 0.5f);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelOneTwo, 0.5f);
	}
	GetWorld()->GetTimerManager().SetTimer(SecondaryAnimationHandle, this, &UInputTutorial::LevelOneStairs, 0.5f);
}

void UInputTutorial::LevelOneTwo()
{
	bLevelOneRight = true;

	HideLayer(0);
	HideLayer(3);

	ShowLayers(1, 4);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelOneOne, 0.5f);
}

void UInputTutorial::LevelOneThree()
{
	bLevelOneRight = false;

	HideLayer(0);
	HideLayer(3);

	ShowLayers(2, 5);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelOneOne, 0.5f);
}

void UInputTutorial::LevelOneStairs()
{
	HideLayer(6);
	HideLayer(8);

	if (bWasUsingGamepad)
	{
		ShowLayer(9);
	}
	else
	{
		ShowLayer(7);
	}
}

void UInputTutorial::LevelTwoOne()
{
	ShowOnlyLayers(0, 3);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelTwoTwo, 0.25f);
}

void UInputTutorial::LevelTwoTwo()
{
	ShowOnlyLayers(1, 4);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelTwoThree, 0.25f);
}

void UInputTutorial::LevelTwoThree()
{
	ShowOnlyLayers(2, 5);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelTwoOne, 1.f);
}

//void UInputTutorial::LevelThreeOne()
//{
//	ShowOnlyLayers(0, 2);
//
//	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelThreeTwo, 0.5f);
//}
//
//void UInputTutorial::LevelThreeTwo()
//{
//	ShowOnlyLayers(1, 3);
//
//	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelThreeOne, 0.25f);
//}

void UInputTutorial::LevelThreeOne()
{
	ShowOnlyLayers(0, 4);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelThreeTwo, 0.25f);
}

void UInputTutorial::LevelThreeTwo()
{
	ShowOnlyLayers(1, 5);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelThreeThree, 0.25f);
}

void UInputTutorial::LevelThreeThree()
{
	ShowOnlyLayers(2, 6);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelThreeFour, 0.25f);
}

void UInputTutorial::LevelThreeFour()
{
	ShowOnlyLayers(3, 7);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelThreeOne, 0.25f);
}

void UInputTutorial::LevelFourOne()
{
	ShowOnlyLayers(0, 2);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelFourTwo, 0.5f);
}

void UInputTutorial::LevelFourTwo()
{
	ShowOnlyLayers(1, 3);

	GetWorld()->GetTimerManager().SetTimer(MainAnimationHandle, this, &UInputTutorial::LevelFourOne, 1.f);
}