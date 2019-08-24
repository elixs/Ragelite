// Fill out your copyright notice in the Description page of Project Settings.

#include "RlButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
//#include "Styling/SlateTypes.h"

URlButton::URlButton(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	auto a = 3;
}

void URlButton::NativeConstruct()
{
	Super::NativeConstruct();

	//if (IsValid())
	//{
	//	ButtonStyle = Button->WidgetStyle;

	//	TextBlock->TextDelegate.BindDynamic(this, &URlButton::Meh);
	//}

	//TextBlock->Text = Text;

	////bIsFocusable = true;
}

void URlButton::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);

	//if (IsValid())
	//{
	//	Button->WidgetStyle.Normal = ButtonStyle.Hovered;
	//}
}

void URlButton::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);

	//if (IsValid())
	//{
	//	Button->WidgetStyle.Normal = ButtonStyle.Normal;
	//}
}

void URlButton::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	//SetKeyboardFocus();
}

bool URlButton::Initialize()
{
	bool da = Super::Initialize();

	//auto a = 5;

	return da;
}

FText URlButton::Meh()
{
	return Text;
}

bool URlButton::IsValid()
{
	return false;
	//return Button && TextBlock;
}

void URlButton::SetIsEnabled(bool bInIsEnabled)
{
	Super::SetIsEnabled(bInIsEnabled);

	//if (IsValid())
	//{
	//	Button->SetIsEnabled(bInIsEnabled);
	//}
}
#if WITH_EDITOR
void URlButton::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	//if (PropertyName == GET_MEMBER_NAME_CHECKED(URlButton, Text))
	//{
	//	if (IsValid())
	//	{
	//		TextBlock->Text = Text;
	//	}
	//}
}
#endif