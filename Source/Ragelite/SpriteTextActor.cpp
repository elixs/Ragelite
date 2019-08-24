// Fill out your copyright notice in the Description page of Project Settings.

#include "SpriteTextActor.h"
#include "ConstructorHelpers.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Materials/Material.h"

#include "Kismet/KismetSystemLibrary.h"

ASpriteTextActor::ASpriteTextActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	RootComponent = SceneComponent;
	STSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TextSceneComponent"));
	STSceneComponent->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialRef(TEXT("/Paper2D/MaskedUnlitSpriteMaterial"));

	if (MaterialRef.Object)
	{
		Material = MaterialRef.Object;
	}

	Text = FString::FromInt(0);
	TileWidth = 8;
	TileHeight = 16;
	TextLenght = 1;

	//for (TCHAR i = 48; i < 91; ++i)
	//{
	//	CharacterMap.Add(FString(1, &i), nullptr);
	//}
}

void ASpriteTextActor::SetText(FString InText)
{
	Text = InText;

	UpdateHorizontal();
	UpdateSprites();
	UpdateAlignment();
}

void ASpriteTextActor::UpdateMaterial()
{
	for (UPaperSpriteComponent* PaperSpriteComponent : SpriteTextComponents)
	{
		PaperSpriteComponent->SetMaterial(0, Material);
	}
}

void ASpriteTextActor::UpdateSprites()
{
	for (int i = 0; i < SpriteTextComponents.Num(); ++i)
	{
		UPaperSpriteComponent* PaperSpriteComponent = SpriteTextComponents[i];
		FString Key = FString::Chr(Text[i]);
		PaperSpriteComponent->SetSprite(CharacterMap.Contains(Key) ? CharacterMap[Key] : nullptr);
	}
}

void ASpriteTextActor::UpdateLocationHorizontal()
{
	for (int i = 0; i < SpriteTextComponents.Num(); ++i)
	{
		SpriteTextComponents[i]->SetRelativeLocation(FVector(TileWidth * i, 0.f, 0.f));
	}
}

void ASpriteTextActor::UpdateHorizontal()
{
	while (SpriteTextComponents.Num() < Text.Len())
	{
		UPaperSpriteComponent* PaperSpriteComponent = NewObject<UPaperSpriteComponent>(this);
		PaperSpriteComponent->SetMaterial(0, Material);
		FString Key = FString::Chr(Text[SpriteTextComponents.Num()]);
		if (CharacterMap.Contains(Key) && CharacterMap[Key])
		{
			PaperSpriteComponent->SetSprite(CharacterMap[Key]);
		}
		PaperSpriteComponent->SetRelativeLocation(FVector(TileWidth * SpriteTextComponents.Num(), 0.f, 0.f));
		PaperSpriteComponent->SetupAttachment(STSceneComponent);
		PaperSpriteComponent->RegisterComponent();
		SpriteTextComponents.Add(PaperSpriteComponent);
	}

	while (SpriteTextComponents.Num() > Text.Len())
	{
		UPaperSpriteComponent* PaperSpriteComponent = SpriteTextComponents.Pop();
		PaperSpriteComponent->DestroyComponent();
	}

	TextLenght = Text.Len();

	UpdateLocationHorizontal();
}

void ASpriteTextActor::UpdateAlignment()
{
	FVector Location(0.f);

	switch (HorizontalAlignment)
	{
	default:
	case EHorizontalSpriteTextAligment::Left:
		break;
	case EHorizontalSpriteTextAligment::Center:
		Location.X -= (TileWidth * TextLenght + 1) / 2;
		break;
	case EHorizontalSpriteTextAligment::Right:
		Location.X -= TileWidth * TextLenght;
		break;
	}

	switch (VerticalAlignment)
	{
	default:
	case EVerticalSpriteTextAligment::Top:
		break;
	case EVerticalSpriteTextAligment::Center:
		Location.Z += (TileHeight + 1) / 2;
		break;
	case EVerticalSpriteTextAligment::Bottom:
		Location.Z += TileHeight;
		break;
	}

	STSceneComponent->SetRelativeLocation(Location);
}

void ASpriteTextActor::PostLoad()
{
	Super::PostLoad();

	UpdateHorizontal();
	UpdateMaterial();
	UpdateSprites();
	UpdateLocationHorizontal();
	UpdateAlignment();
}

#if WITH_EDITOR
void ASpriteTextActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASpriteTextActor, Text))
	{
		UpdateHorizontal();
		UpdateSprites();
		UpdateAlignment();
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASpriteTextActor, Material))
	{
		UpdateMaterial();
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASpriteTextActor, TileWidth)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASpriteTextActor, TileHeight))
	{
		UpdateLocationHorizontal();
		UpdateAlignment();
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASpriteTextActor, CharacterMap))
	{
		UpdateSprites();
		UpdateLocationHorizontal();
	}
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ASpriteTextActor, HorizontalAlignment)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ASpriteTextActor, VerticalAlignment))
	{
		UpdateAlignment();
	}
}
#endif