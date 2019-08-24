// Fill out your copyright notice in the Description page of Project Settings.

#include "Dart.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Projectile.h"
#include "RlGameInstance.h"
#include "LevelManager.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"

ADart::ADart()
{
	bEnabled = false;
	Delay = 0.f;
	Cooldown = 1.f;
	Speed = 100.f;
}

void ADart::Enable(float InDelay, float InCooldown, float InSpeed)
{
	bEnabled = true;
	Delay = InDelay;
	Cooldown = InCooldown;
	if (InSpeed != 0.f)
	{
		Speed = InSpeed;
	}

	if (Delay == 0.f)
	{
		SpawnDart();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(SpawnDartHandle, this, &ADart::SpawnDart, Delay, false);
	}
}

void ADart::Disable()
{
	bEnabled = false;

	GetWorld()->GetTimerManager().ClearTimer(SpawnDartHandle);
}

void ADart::SpawnDart()
{
	FRotator SpawnRotation(0.f);
	FActorSpawnParameters SpawnInfo;

	ULevelManager* LM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->LevelManager;

	AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(GetActorLocation() + GetActorUpVector(), GetActorRotation(), SpawnInfo);
	Projectile->InitialSpeed = Speed;
	UPaperSpriteComponent* InRenderComponent = Projectile->GetRenderComponent();
	InRenderComponent->SetSprite(LM->GetDartSprite());
	InRenderComponent->SetMaterial(0, LM->Material);

	GetWorld()->GetTimerManager().SetTimer(SpawnDartHandle, this, &ADart::SpawnDart, Cooldown, false);
}

#if WITH_EDITOR
void ADart::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ADart, bEnabled))
	{
		if (bEnabled)
		{
			Enable();
		}
		else
		{
			Disable();
		}
	}
}
#endif