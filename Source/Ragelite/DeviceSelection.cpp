// Fill out your copyright notice in the Description page of Project Settings.

#include "DeviceSelection.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/PanelWidget.h"
#include "Kismet/GameplayStatics.h"
#include "RlCharacter.h"
#include "FocusButton.h"
#include "RlGameInstance.h"
#include "WidgetManager.h"
#include "LevelManager.h"
#include "RlGameInstance.h"
#include "RlGameMode.h"
#include "WidgetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogStatus, Log, All);

UDeviceSelection::UDeviceSelection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AccumulatedTime = 0;
	LastFocusedButton = nullptr;
	bPopUp = false;
}

void UDeviceSelection::NativeConstruct()
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->Text = FText::FromString("Meh");
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->AddChild(TextBlock);
	Devices->AddChild(Button);

	UTextBlock* TextBlock2 = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock2->Text = FText::FromString("Meh2");
	UButton* Button2 = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button2->AddChild(TextBlock2);
	//Devices->AddChild(Button2);
	InsertChildAt(Devices, 1, Button2);

	//DevicesMap.Add(FString("aa:bb:cc"), 16);
	//DevicesMap.Add(FString("aa:bb:dd"), 16);

	Update();

	//ClearDevices();

	bTryingToConnect = false;

	//Cancel->OnClicked.RemoveDynamic(this, &UDeviceSelection::OnCancel);
	Cancel->OnClicked.AddDynamic(this, &UDeviceSelection::OnCancel);

	//Yes->OnClicked.RemoveDynamic(this, &UDeviceSelection::OnYes);
	Yes->OnClicked.AddDynamic(this, &UDeviceSelection::OnYes);

	//No->OnClicked.RemoveDynamic(this, &UDeviceSelection::OnNo);
	No->OnClicked.AddDynamic(this, &UDeviceSelection::OnNo);

	Super::NativeConstruct();
}

void UDeviceSelection::Update()
{
	if (!DevicesMap.Num())
	{
		if (Devices->GetChildrenCount() == 1)
		{
			UFocusButton* Button = Cast<UFocusButton>(Devices->GetChildAt(0));
			if (Button && !Button->GetIsEnabled())
			{
				return;
			}
		}
		while (Devices->HasAnyChildren())
		{
			Devices->RemoveChildAt(0);
		}

		AddDeviceButton(FString("No nearby devices"));

		Cancel->SetIsEnabled(true);

		LastFocusedButton = Cancel;
		if (!bPopUp)
		{
			Cancel->SetKeyboardFocus();
		}

		return;
	}

	Cancel->SetIsEnabled(false);

	if (Devices->HasAnyChildren())
	{
		UFocusButton* Button = Cast<UFocusButton>(Devices->GetChildAt(0));
		if (Button && !Button->GetIsEnabled())
		{
			Devices->RemoveChildAt(0);
		}
	}

	UFocusButton* FocusedButton = nullptr;

	int32 CurrentIndex = 0;

	for (auto It = DevicesMap.CreateConstIterator(); It;)
	{
		auto Device = It.Key();

		//if (DevicesArray.IsValidIndex(CurrentArrayIndex))
		if (Devices->GetChildrenCount() > CurrentIndex)
		{
			bool bValidChildren = true;

			UFocusButton* Button = Cast<UFocusButton>(Devices->GetChildAt(CurrentIndex));
			if (Button && Button->GetChildrenCount() == 1)
			{
				UTextBlock* TextBlock = Cast<UTextBlock>(Button->GetChildAt(0));
				if (TextBlock)
				{
					int32 Comparison = Device.Compare(TextBlock->Text.ToString());

					if (!Comparison)
					{
						if (Button->HasKeyboardFocus() || (bPopUp && Button == LastFocusedButton))
						{
							FocusedButton = Button;
						}

						++It;
						++CurrentIndex;
						continue;
					}
					else if (Comparison > 0)
					{
						AddDeviceButton(Device, CurrentIndex++);
						++It;
						continue;
					}
					else
					{
						if ((Button->HasKeyboardFocus() || (bPopUp && Button == LastFocusedButton)) && CurrentIndex)
						{
							FocusedButton = Cast<UFocusButton>(Devices->GetChildAt(CurrentIndex - 1));
						}
					}
				}
			}

			Devices->RemoveChildAt(CurrentIndex);
		}
		else
		{
			AddDeviceButton(Device);
			++It;
			++CurrentIndex;
		}
	}

	while (Devices->GetChildrenCount() > CurrentIndex)
	{
		Devices->RemoveChildAt(CurrentIndex);
	}

	// Rebuild UI
	auto Children = Devices->GetAllChildren();

	while (Devices->HasAnyChildren())
	{
		Devices->RemoveChildAt(0);
	}

	for (auto& Child : Children)
	{
		Devices->AddChild(Child);
	}

	if (!FocusedButton)
	{
		FocusedButton = Cast<UFocusButton>(Devices->GetChildAt(0));
	}

	LastFocusedButton = FocusedButton;
	if (!bPopUp)
	{
		FocusedButton->SetKeyboardFocus();
	}
}

void UDeviceSelection::AddDevice(FString Device)
{
	if (DevicesMap.Contains(Device))
	{
		DevicesMap[Device] = InitialLife;
		return;
	}
	DevicesMap.Add(Device, InitialLife);

	Update();
}

void UDeviceSelection::DeviceConnectionFailed()
{
	Connecting->SetVisibility(ESlateVisibility::Hidden);
	Failed->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	RlCharacter->bConnectionAccepted = false;

	GetWorld()->GetTimerManager().SetTimer(FailedHandle, this, &UDeviceSelection::ResetDeviceConnection, 2.f);
}

void UDeviceSelection::ResetDeviceConnection()
{
	Failed->SetVisibility(ESlateVisibility::Hidden);

	auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PC->SetInputMode(FInputModeUIOnly());
	UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->EnableInput(PC);
}

void UDeviceSelection::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AccumulatedTime += InDeltaTime;

	if (AccumulatedTime > 1.f)
	{
		AccumulatedTime = 0;

		bool bUpdate = false;

		TArray<FString> DevicesToRemove;

		for (auto& Device : DevicesMap)
		{
			if (!--DevicesMap[Device.Key])
			{
				DevicesToRemove.Add(Device.Key);
				bUpdate = true;
			}
		}

		for (auto& Device : DevicesToRemove)
		{
			DevicesMap.Remove(Device);
		}

		if (bUpdate)
		{
			Update();
		}
	}
}

void UDeviceSelection::AddDeviceButton(FString Device, int32 Index /*= -1*/)
{
	//UWidgetManager* WM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->WidgetManager;

	UFocusButton* Button = WidgetTree->ConstructWidget<UFocusButton>(UFocusButton::StaticClass());
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

	TextBlock->Text = FText::FromString(Device);
	Button->AddChild(TextBlock);

	if (!DevicesMap.Num())
	{
		Button->SetIsEnabled(false);
	}
	else
	{
		Button->OnClickDelegate.AddDynamic(this, &UDeviceSelection::OnButtonClicked);
	}

	if (Index == -1)
	{
		Devices->AddChild(Button);
	}
	else
	{
		InsertChildAt(Devices, Index, Button);
	}
}

void UDeviceSelection::ClearDevices()
{
	while (Devices->HasAnyChildren())
	{
		Devices->RemoveChildAt(0);
	}

	AddDeviceButton(FString("No nearby devices"));

	Cancel->SetIsEnabled(true);
}

void UDeviceSelection::OnButtonClicked(UFocusButton* Button)
{
	if (Button && Button->GetChildrenCount() == 1)
	{
		UTextBlock* TextBlock = Cast<UTextBlock>(Button->GetChildAt(0));
		if (TextBlock)
		{
			//ULevelManager* LM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->LevelManager;
			//if (LM->DeviceSelection && !LM->DeviceSelection->bTryingToConnect)
			if (!bTryingToConnect)
			{
				bTryingToConnect = true;
				ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

				// TODO Reply with a confirmation of the conection or try again
				RlCharacter->Connect(TextBlock->Text.ToString());

				Connecting->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

				auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
				PC->SetInputMode(FInputModeGameOnly());
				UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->DisableInput(PC);
			}
		}
	}
}

void UDeviceSelection::InsertChildAt(UPanelWidget* Widget, int32 Index, UWidget* Content)
{
	UPanelSlot* NewSlot = Widget->AddChild(Content);

	int32 CurrentIndex = Widget->GetChildIndex(Content);

	TArray<UPanelSlot*> Slots = Widget->GetSlots();

	Slots.RemoveAt(CurrentIndex);
	Slots.Insert(Content->Slot, FMath::Clamp(Index, 0, Slots.Num()));
}

void UDeviceSelection::OnCancel()
{
	bPopUp = true;
	Confirmation->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	No->SetKeyboardFocus();
}

void UDeviceSelection::OnYes()
{
	ARlCharacter* RlCharacter = Cast<ARlCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	RlCharacter->CloseConnection();

	//auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	//PC->SetInputMode(FInputModeGameOnly());

	//UE_LOG(LogStatus, Log, TEXT("Game Started"));

	//if (ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode()))
	//{
	//	GameMode->bGameStarted = true;
	//}

	RemoveFromParent();

	UWidgetManager* WM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->WidgetManager;

	WM->StartIntro();



}

void UDeviceSelection::OnNo()
{
	bPopUp = false;
	Confirmation->SetVisibility(ESlateVisibility::Hidden);
	LastFocusedButton->SetKeyboardFocus();
}