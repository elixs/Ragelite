// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "DefaultMovementComponent.h"
//#include "GameFramework/ProjectileMovementComponent.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"
#include "RlCharacter.h"
#include "TimerManager.h"
#include "Engine/World.h"

AProjectile::AProjectile()
{
	MovementComponent = CreateDefaultSubobject<UDefaultMovementComponent>("Movement Component");
	//MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("Movement Component");

	UPaperSpriteComponent* InRenderComponent = GetRenderComponent();

	InRenderComponent->SetMobility(EComponentMobility::Movable);
	InRenderComponent->SetCollisionProfileName(FName("Projectile"));
	InRenderComponent->SetGenerateOverlapEvents(false);

	InRenderComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

	InitialSpeed = 100.f;

	PrimaryActorTick.bCanEverTick = true;
}

void AProjectile::Tick(float DeltaSeconds)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(DestroyHandle))
	{
		return;
	}
	const FVector Delta = GetActorUpVector() * InitialSpeed * DeltaSeconds;

	FHitResult Hit(1.f);
	MovementComponent->SafeMoveUpdatedComponent(Delta, RootComponent->GetComponentQuat(), true, Hit);
	
	//float LastMoveTimeSlice = DeltaSeconds;
	//
	if (Hit.bBlockingHit)
	{
		MovementComponent->Velocity = FVector::ZeroVector;
		GetRenderComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		GetWorld()->GetTimerManager().SetTimer(DestroyHandle, this, &AProjectile::DestroyWrapper, 1.f, false);
	}
	//
	//if (Hit.bStartPenetrating)
	//{
	//	// Allow this hit to be used as an impact we can deflect off, otherwise we do nothing the rest of the update and appear to hitch.
	//	SlideAlongSurface(Delta, 1.f, Hit.Normal, Hit, true);
	//
	//	if (Hit.bStartPenetrating)
	//	{
	//		OnCharacterStuckInGeometry(&Hit);
	//	}
	//}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ARlCharacter* RlCharacter = Cast<ARlCharacter>(OtherActor))
	{
		if (!RlCharacter->bDeath && !RlCharacter->bInvincible)
		{
			RlCharacter->Death();
		}
		OnRlCharacterHit.ExecuteIfBound();

		Destroy();
	}
}

void AProjectile::DestroyWrapper()
{
	Destroy();
}
