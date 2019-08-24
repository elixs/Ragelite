// Fill out your copyright notice in the Description page of Project Settings.

#include "Spike.h"
#include "RlCharacter.h"
#include "Components/BoxComponent.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"

ASpike::ASpike()
{
	UPaperSpriteComponent* InRenderComponent = GetRenderComponent();

	InRenderComponent->SetCollisionProfileName(FName("BlockAllDynamic"));
	InRenderComponent->OnComponentHit.AddDynamic(this, &ASpike::OnHit);
}

void ASpike::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ARlCharacter* RlCharacter = Cast<ARlCharacter>(OtherActor))
	{
		if (!RlCharacter->bDeath && !RlCharacter->bInvincible)
		{
			RlCharacter->Death();
		}
		OnRlCharacterHit.ExecuteIfBound();
	}
}