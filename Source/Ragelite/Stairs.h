// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Paper2D/Classes/PaperSpriteActor.h"
#include "Stairs.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

UCLASS()
class RAGELITE_API AStairs : public APaperSpriteActor
{
	GENERATED_BODY()

public:
	AStairs();

	UBoxComponent* GetTriggerComponent();

	bool bCanEnter;

private:

	UPROPERTY(Category = Trigger, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* TriggerComponent;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
