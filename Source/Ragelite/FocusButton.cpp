// Fill out your copyright notice in the Description page of Project Settings.


#include "FocusButton.h"
#include "SFocusButton.h"
#include "Components/ButtonSlot.h"

UFocusButton::UFocusButton(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	OnClicked.AddDynamic(this, &UFocusButton::OnClick);
	OnHovered.AddDynamic(this, &UFocusButton::SetFocus);
	//OnUnhovered.AddDynamic(this, &UFocusButton::OnUnfocused);

	ButtonStyle = WidgetStyle;
}

void UFocusButton::OnClick()
{
	OnClickDelegate.Broadcast(this);
}

TSharedRef<SWidget> UFocusButton::RebuildWidget()
{
	MyButton = SNew(SFocusButton)
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleClicked))
		.OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressed))
		.OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleased))
		.OnHovered_UObject(this, &ThisClass::SlateHandleHovered)
		.OnUnhovered_UObject(this, &ThisClass::SlateHandleUnhovered)
		.ButtonStyle(&WidgetStyle)
		.ClickMethod(ClickMethod)
		.TouchMethod(TouchMethod)
		.PressMethod(PressMethod)
		.IsFocusable(IsFocusable)
		;

	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}

	SFocusButton* FocusButton = static_cast<SFocusButton*>(MyButton.Get());

	FocusButton->OnFocused.BindUObject(this, &UFocusButton::OnFocused);
	FocusButton->OnUnfocused.BindUObject(this, &UFocusButton::OnUnfocused);

	return MyButton.ToSharedRef();
}

//void UFocusButton::NativeConstruct()
//{
//	Super::NativeConstruct();
//
//	ButtonStyle = WidgetStyle;
//}

void UFocusButton::Test()
{
	auto a = 5;
}

void UFocusButton::OnFocused()
{
	WidgetStyle.Normal = ButtonStyle.Hovered;
	// TODO: Remove focus to other focus buttons
 }

void UFocusButton::OnUnfocused()
{
	WidgetStyle.Normal = ButtonStyle.Normal;

}

void UFocusButton::SetFocus()
{
	SetKeyboardFocus();
}

