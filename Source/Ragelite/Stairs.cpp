// Fill out your copyright notice in the Description page of Project Settings.

#include "Stairs.h"
#include "Components/BoxComponent.h"
#include "RlCharacter.h"
#include "Paper2D/Classes/PaperSpriteComponent.h"

AStairs::AStairs()
{
	TriggerComponent = CreateDefaultSubobject<UBoxComponent>("Collision Component");
	TriggerComponent->InitBoxExtent(FVector(16.f));
	TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &AStairs::OnOverlapBegin);
	TriggerComponent->OnComponentEndOverlap.AddDynamic(this, &AStairs::OnOverlapEnd);

	RootComponent = TriggerComponent;

	UPaperSpriteComponent* InRenderComponent = GetRenderComponent();
	InRenderComponent->SetMobility(EComponentMobility::Movable);
	InRenderComponent->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
	InRenderComponent->SetCollisionProfileName(FName("NoCollision"));

	InRenderComponent->SetupAttachment(TriggerComponent);

	bCanEnter = false;
}

UBoxComponent* AStairs::GetTriggerComponent()
{
	return TriggerComponent;
}

void AStairs::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ARlCharacter>(OtherActor))
	{
		bCanEnter = true;
	}
}

void AStairs::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<ARlCharacter>(OtherActor))
	{
		bCanEnter = false;
	}
}