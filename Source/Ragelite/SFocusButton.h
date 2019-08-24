// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Input/SButton.h"

class RAGELITE_API SFocusButton : public SButton
{
public:
	FSimpleDelegate OnFocused;

	FSimpleDelegate OnUnfocused;

	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;

	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;
};
