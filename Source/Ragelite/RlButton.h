// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
//#include "Styling/SlateTypes.h"
#include "RlButton.generated.h"

class UButton;
class UTextBlock;
//class ;

/**
 * 
 */
UCLASS()
class RAGELITE_API URlButton : public UUserWidget
{
	GENERATED_BODY()

public:

	URlButton(const FObjectInitializer&  ObjectInitializer);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UButton* Button = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* TextBlock = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;
	
	bool IsValid();


	virtual void SetIsEnabled(bool bInIsEnabled) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual bool Initialize() override;

private: 
	
	FButtonStyle ButtonStyle;

	UFUNCTION()
	FText Meh();
};
