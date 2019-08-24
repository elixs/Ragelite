// Fill out your copyright notice in the Description page of Project Settings.


#include "Hazard.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"

AHazard::AHazard()
{
	UPaperSpriteComponent* InRenderComponent = GetRenderComponent();
	InRenderComponent->SetMobility(EComponentMobility::Movable);
	InRenderComponent->SetCollisionProfileName(FName("NoCollision"));
	InRenderComponent->SetGenerateOverlapEvents(false);

	TileSize = 32;
}

void AHazard::Move(FVector Location, EHazardLocation HazardLocation)
{
	float HazardSize = TileSize / 4;
	float HalfHazardSize = HazardSize / 2;

	Location.X -= 3 * HalfHazardSize;
	Location.Z += 3 * HalfHazardSize;
	FRotator Rotation(0.f);

	switch (HazardLocation)
	{
	case EHazardLocation::B1:
	case EHazardLocation::T2:
		Location.X += HazardSize;
		break;
	case EHazardLocation::B2:
	case EHazardLocation::T1:
		Location.X += 2 * HazardSize;
		break;
	case EHazardLocation::B3:
	case EHazardLocation::T0:
	case EHazardLocation::R0:
	case EHazardLocation::R1:
	case EHazardLocation::R2:
	case EHazardLocation::R3:
		Location.X += 3 * HazardSize;
		break;
	default:
		break;
	}

	switch (HazardLocation)
	{
	case EHazardLocation::L1:
	case EHazardLocation::R2:
		Location.Z -= HazardSize;
		break;
	case EHazardLocation::L2:
	case EHazardLocation::R1:
		Location.Z -= 2 * HazardSize;
		break;
	case EHazardLocation::L3:
	case EHazardLocation::R0:
	case EHazardLocation::B0:
	case EHazardLocation::B1:
	case EHazardLocation::B2:
	case EHazardLocation::B3:
		Location.Z -= 3 * HazardSize;
		break;
	default:
		break;
	}

	switch (HazardLocation)
	{
	case EHazardLocation::R0:
	case EHazardLocation::R1:
	case EHazardLocation::R2:
	case EHazardLocation::R3:
		Rotation.Pitch = 90.f;
		break;
	case EHazardLocation::T0:
	case EHazardLocation::T1:
	case EHazardLocation::T2:
	case EHazardLocation::T3:
		Rotation.Pitch = 180.f;
		break;
	case EHazardLocation::L0:
	case EHazardLocation::L1:
	case EHazardLocation::L2:
	case EHazardLocation::L3:
		Rotation.Pitch = 270.f;
		break;
	default:
		break;
	}

	SetActorLocationAndRotation(Location, Rotation);
}
