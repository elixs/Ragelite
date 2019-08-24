// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enums.h"
#include "SpriteTextActor.generated.h"

USTRUCT()
struct FSpriteCharacter
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(Category = SpriteText, VisibleAnywhere)
	FName Name;

	UPROPERTY(Category = SpriteText, EditAnywhere)
	class UPaperSprite* Sprite;

	FSpriteCharacter() : Sprite(nullptr) {};

	FSpriteCharacter(FName InName) : Name(InName), Sprite(nullptr) {};
};

class UPaperSprite;

UCLASS()
class RAGELITE_API ASpriteTextActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	void SetText(FString InText);

private:

	UPROPERTY(Category = SpriteText, VisibleAnywhere)
	class USceneComponent* SceneComponent;

	class USceneComponent* STSceneComponent;

	UPROPERTY(Category = SpriteText, EditAnywhere/*, meta = (MultiLine = "true")*/) // Add multiLine support if needed.
	FString Text;

	UPROPERTY(Category = SpriteText, EditAnywhere)
	int TileWidth;

	UPROPERTY(Category = SpriteText, EditAnywhere)
	int TileHeight;

	UPROPERTY(Category = SpriteText, EditAnywhere)
	EHorizontalSpriteTextAligment HorizontalAlignment;

	UPROPERTY(Category = SpriteText, EditAnywhere)
	EVerticalSpriteTextAligment VerticalAlignment;

	UPROPERTY(Category = SpriteText, EditAnywhere)
	class UMaterialInterface* Material;

	// Only use one character keys
	UPROPERTY(Category = SpriteText, EditAnywhere)
	TMap<FString, UPaperSprite*> CharacterMap;

	UPROPERTY(Category = SpriteText, VisibleAnywhere)
	TArray<class UPaperSpriteComponent*> SpriteTextComponents;

	int TextLenght;

	void UpdateMaterial();

	void UpdateSprites();

	void UpdateLocationHorizontal();

	void UpdateHorizontal();

	void UpdateAlignment();

public:

	virtual void PostLoad() override;


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
