// Fill out your copyright notice in the Description page of Project Settings.


#include "Stone.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"

AStone::AStone()
{
	UPaperSpriteComponent* InRenderComponent = GetRenderComponent();

	InRenderComponent->SetGenerateOverlapEvents(false);
	InRenderComponent->SetCollisionProfileName(FName("BlockAllDynamic"));
}

void AStone::Move(FVector Location, EHazardLocation HazardLocation)
{
	float HazardSize = TileSize / 2;
	float HalfHazardSize = HazardSize / 2;

	Location.X -= HalfHazardSize;
	Location.Z += HalfHazardSize;

	switch (HazardLocation)
	{
	case EHazardLocation::T0:
	case EHazardLocation::T1:
	case EHazardLocation::B2:
	case EHazardLocation::B3:
	case EHazardLocation::R0:
	case EHazardLocation::R1:
	case EHazardLocation::R2:
	case EHazardLocation::R3:
		Location.X += HazardSize;
		break;
	default:
		break;
	}

	switch (HazardLocation)
	{
	case EHazardLocation::R0:
	case EHazardLocation::R1:
	case EHazardLocation::L2:
	case EHazardLocation::L3:
	case EHazardLocation::B0:
	case EHazardLocation::B1:
	case EHazardLocation::B2:
	case EHazardLocation::B3:
		Location.Z -= HazardSize;
		break;
	default:
		break;
	}

	SetActorLocationAndRotation(Location, FRotator(0.f));
}
