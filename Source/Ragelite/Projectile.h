// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaperSpriteActor.h"
#include "Projectile.generated.h"

DECLARE_DELEGATE(FHitSignature)

//class UProjectileMovementComponent;
class UDefaultMovementComponent;

/**
 * 
 */
UCLASS()
class RAGELITE_API AProjectile : public APaperSpriteActor
{
	GENERATED_BODY()

public:
	AProjectile();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(Category = Character, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UDefaultMovementComponent* MovementComponent;
	//UProjectileMovementComponent* MovementComponent;

	FHitSignature OnRlCharacterHit;

	FTimerHandle DestroyHandle;

	float InitialSpeed;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void DestroyWrapper();
	
};
