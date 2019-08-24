// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "FocusButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClickDelegate, UFocusButton*, Button);

/**
 * 
 */
UCLASS()
class RAGELITE_API UFocusButton : public UButton
{
	GENERATED_BODY()

public:

	UFocusButton(const FObjectInitializer& ObjectInitializer);

	UPROPERTY()
	FOnClickDelegate OnClickDelegate;

private:

	UFUNCTION()
	void OnClick();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//virtual void NativeConstruct() override;

private:
	FButtonStyle ButtonStyle;

	UFUNCTION()
	void Test();

	UFUNCTION()
	void OnFocused();

	UFUNCTION()
	void OnUnfocused();

	UFUNCTION()
	void SetFocus();
};
