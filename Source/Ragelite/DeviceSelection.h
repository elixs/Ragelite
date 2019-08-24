// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "DeviceSelection.generated.h"

class UScrollBox;
class UFocusButton;
class UCanvasPanel;
class UPanelWidget;

/**
 * 
 */
UCLASS()
class RAGELITE_API UDeviceSelection : public UUserWidget
{
	GENERATED_BODY()

public:
	UDeviceSelection(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UScrollBox* Devices;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UFocusButton* Cancel;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UCanvasPanel* Confirmation;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UFocusButton* Yes;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UFocusButton* No;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UCanvasPanel* Connecting;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UCanvasPanel* Failed;

	FTimerHandle FailedHandle;

	void Update();


	void AddDevice(FString Device);

	bool bTryingToConnect;

	void DeviceConnectionFailed();

	void ResetDeviceConnection();


protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// MAC and life of the device
	TSortedMap<FString, uint8> DevicesMap;

	float AccumulatedTime;

	uint8 InitialLife = 30;

	void AddDeviceButton(FString Device, int32 Index = -1);

	void ClearDevices();

	UFUNCTION()
	void OnButtonClicked(UFocusButton* Button);

	void InsertChildAt(UPanelWidget* Widget, int32 Index, UWidget* Content);

	UFUNCTION()
	void OnCancel();

	UFUNCTION()
	void OnYes();

	UFUNCTION()
	void OnNo();

	UFocusButton* LastFocusedButton;

	bool bPopUp;

};
