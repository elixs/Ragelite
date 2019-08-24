// Fill out your copyright notice in the Description page of Project Settings.

// This file has utf-8 enconding

#include "RlCharacter.h"
#include "RlCharacterMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperFlipbook.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/InputComponent.h"
#include "Engine/CollisionProfile.h"
#include "DisplayDebugHelpers.h"
#include "Engine/Canvas.h"
#include "GameFramework/DamageType.h"
#include "RlGameInstance.h"
#include "LevelManager.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "RlPlayerController.h"
#include "RlSpriteHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Stairs.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleEmitter.h"
#include "ConstructorHelpers.h"
#include "DeviceSelection.h"
#include "RlGameMode.h"
#include "HearRateModule.h"
#include "WidgetManager.h"

#include "Runtime/Networking/Public/Common/TcpSocketBuilder.h"
#include "Runtime/Networking/Public/Common/TcpListener.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Endpoint.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h"

#include <string>
#include <sstream>

DEFINE_LOG_CATEGORY_STATIC(LogCharacter, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogAvatar, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogHeartRate, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogDevice, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogStatus, Log, All);

FName ARlCharacter::SpriteComponentName(TEXT("Sprite0"));
FName ARlCharacter::RlCharacterMovementComponentName(TEXT("RlCharMoveComp"));
FName ARlCharacter::BoxComponentName(TEXT("CollisionBox"));

ARlCharacter::ARlCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//PrimaryActorTick.bCanEverTick = true;

	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FName ID_Characters;
		FText NAME_Characters;
		FConstructorStatics()
			: ID_Characters(TEXT("Characters"))
			, NAME_Characters(NSLOCTEXT("SpriteCategory", "Characters", "Characters"))
		{}
	};
	static FConstructorStatics ConstructorStatics;

	// Character rotation only changes in Yaw, to prevent the capsule from changing orientation.
	// Ask the Controller for the full rotation if desired (ie for aiming).
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(ARlCharacter::BoxComponentName);
	//BoxComponent->InitBoxExtent(FVector(16.f));
	BoxComponent->InitBoxExtent(FVector(9.f, 8.f, 10.f));
	BoxComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	BoxComponent->CanCharacterStepUpOn = ECB_No;
	BoxComponent->SetShouldUpdatePhysicsVolume(true);
	//BoxComponent->bCheckAsyncSceneOnMove = false;
	BoxComponent->SetCanEverAffectNavigation(false);
	BoxComponent->bDynamicObstacle = true;
	RootComponent = BoxComponent;

	JumpMaxHoldTime = 0.2f;

	JumpHoldForceFactor = 7.f;

	JumpMaxCount = 1;
	JumpCurrentCount = 0;
	bWasJumping = false;

	WallWalkHoldTime = 0.f;
	WallWalkMaxHoldTime = 1.f;

	//AnimRootMotionTranslationScale = 1.0f;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Characters;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Characters;
		ArrowComponent->SetupAttachment(BoxComponent);
		ArrowComponent->bIsScreenSizeScaled = true;
	}
#endif // WITH_EDITORONLY_DATA

	RlCharacterMovement = CreateDefaultSubobject<URlCharacterMovementComponent>(ARlCharacter::RlCharacterMovementComponentName);
	if (RlCharacterMovement)
	{
		RlCharacterMovement->UpdatedComponent = RootComponent;
	}

	Sprite = CreateOptionalDefaultSubobject<UPaperFlipbookComponent>(ARlCharacter::SpriteComponentName);
	if (Sprite)
	{
		Sprite->AlwaysLoadOnClient = true;
		Sprite->AlwaysLoadOnServer = true;
		Sprite->bOwnerNoSee = false;
		Sprite->bAffectDynamicIndirectLighting = true;
		Sprite->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Sprite->SetupAttachment(GetBoxComponent());
		static FName CollisionProfileName(TEXT("NoCollision"));
		Sprite->SetCollisionProfileName(CollisionProfileName);
		Sprite->SetGenerateOverlapEvents(false);
		Sprite->SetRelativeLocation(FVector(0.f, 0.f, 3.f));
	}

	DustComponent = CreateDefaultSubobject<UParticleSystemComponent>(FName("Dust"));
	DustComponent->SetRelativeLocation(FVector(0.f, 0.f, -12.f));
	DustComponent->SetAutoActivate(false);

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DustRightRef(TEXT("/Game/Particles/P_DustRight"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem> DustLeftRef(TEXT("/Game/Particles/P_DustLeft"));

	if (DustRightRef.Object)
	{
		DustRight = DustRightRef.Object;
	}
	if (DustLeftRef.Object)
	{
		DustLeft = DustLeftRef.Object;
	}

	BaseRotationOffset = FQuat::Identity;

	// Animation

	bDeath = false;
	bInvincible = false;

	WalkSpeed = RlCharacterMovement->NormalMaxWalkSpeed / 2.f;

	bIsLastDirectionRight = true;

	PreviousVelocityX = 0.f;

	bIsChangingDirection = false;

	// Enable features
	bRunEnabled = true;
	bJumpEnabled = true;
	bLongJumpEnabled = true;
	bWallWalkEnabled = true;

	bIsSprinting = false;

	bIsUsingGamepad = false;

	bDust = false;

	bDeviceConnected = false;

	bConnectionAccepted = false;
}

// meh
void ARlCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Animation

	auto IdleLambda = [](ARlCharacter* This) -> UPaperFlipbook* { return This->bIsLastDirectionRight ? This->IdleRightAnimation : This->IdleLeftAnimation; };
	auto ToIdleLambda = [](ARlCharacter* This) -> UPaperFlipbook* { return This->bIsLastDirectionRight ? This->ToIdleRightAnimation : This->ToIdleLeftAnimation; };
	auto RunningLambda = [](ARlCharacter* This) -> UPaperFlipbook*
	{
		if (This->RlCharacterMovement->IsWallWalking())
		{
			return This->bIsLastDirectionRight ? This->WallWalkRightAnimation : This->WallWalkLeftAnimation;
		}
		else
		{
			return This->bIsLastDirectionRight ? This->RunningRightAnimation : This->RunningLeftAnimation;
		}
	};

	// Jump
	auto JumpingLambda = [](ARlCharacter* This) -> UPaperFlipbook* { return This->bIsLastDirectionRight ? This->JumpRightAnimation : This->JumpLeftAnimation; };

	FRlAnimation IA(IdleLambda, true, false);
	FRlAnimation TIA(ToIdleLambda, false, false, true);
	FRlAnimation TIRA(ToIdleLambda, false, true, true);
	FRlAnimation RA(RunningLambda, true, false);

	FRlAnimation TJA(JumpingLambda, false, false, true, 0, 5);
	FRlAnimation JA(JumpingLambda, true, false, false, 6, 6);
	FRlAnimation TFA(JumpingLambda, false, false, false, 7, 8);
	FRlAnimation FA(JumpingLambda, true, false, false, 9, 9);
	FRlAnimation LA(JumpingLambda, false, false, false, 10, 11);

	// Reached Start or End with bLoop false
	auto AnimationStopped = [](ARlCharacter* This) -> bool { return !This->Sprite->IsPlaying(); };
	auto JumpStart = [](ARlCharacter* This) -> bool { return This->bStoredJump; };
	auto FallStart = [](ARlCharacter* This) -> bool { return This->RlCharacterMovement->Velocity.Z < 0.f; };

	IA.Transitions.Add(FRlTransition(FallStart, ERlAnimationState::ToFalling));
	IA.Transitions.Add(FRlTransition(JumpStart, ERlAnimationState::ToJumping));
	IA.Transitions.Add(FRlTransition([](ARlCharacter* This) -> bool
	{
		return This->RlCharacterMovement->GetCurrentAcceleration().X != 0.f;
	}, ERlAnimationState::ToIdleR));

	TIA.Transitions.Add(FRlTransition(FallStart, ERlAnimationState::ToFalling));
	TIA.Transitions.Add(FRlTransition(JumpStart, ERlAnimationState::ToJumping));
	TIA.Transitions.Add(FRlTransition(AnimationStopped, ERlAnimationState::Idle));
	TIA.Transitions.Add(FRlTransition([](ARlCharacter* This) -> bool
	{
		return FMath::Abs(This->RlCharacterMovement->Velocity.X) > This->WalkSpeed && This->RlCharacterMovement->GetCurrentAcceleration().X != 0.f;
	}, ERlAnimationState::ToIdleR));

	TIRA.Transitions.Add(FRlTransition(FallStart, ERlAnimationState::ToFalling));
	TIRA.Transitions.Add(FRlTransition(JumpStart, ERlAnimationState::ToJumping));
	TIRA.Transitions.Add(FRlTransition(AnimationStopped, ERlAnimationState::Running));
	TIRA.Transitions.Add(FRlTransition([](ARlCharacter* This) -> bool
	{
		return FMath::Abs(This->RlCharacterMovement->Velocity.X) < This->WalkSpeed && This->RlCharacterMovement->GetCurrentAcceleration().X == 0.f;
	}, ERlAnimationState::ToIdle));

	RA.Transitions.Add(FRlTransition(FallStart, ERlAnimationState::ToFalling));
	RA.Transitions.Add(FRlTransition(JumpStart, ERlAnimationState::ToJumping));
	RA.Transitions.Add(FRlTransition([](ARlCharacter* This) -> bool
	{
		return !This->RlCharacterMovement->IsWallWalking() && This->RlCharacterMovement->GetCurrentAcceleration().X == 0.f && !This->bIsChangingDirection && FMath::Abs(This->RlCharacterMovement->Velocity.X) < This->WalkSpeed;
	}, ERlAnimationState::ToIdle));

	// Jumping
	TJA.Transitions.Add(FRlTransition(AnimationStopped, ERlAnimationState::Jumping));

	JA.Transitions.Add(FRlTransition([](ARlCharacter* This) -> bool
	{
		return This->RlCharacterMovement->Velocity.Z <= 0.f;
	}, ERlAnimationState::ToFalling));

	TFA.Transitions.Add(FRlTransition(AnimationStopped, ERlAnimationState::Falling));

	FA.Transitions.Add(FRlTransition([](ARlCharacter* This) -> bool
	{
		return This->RlCharacterMovement->Velocity.Z == 0.f;
	}, ERlAnimationState::Landing));

	LA.Transitions.Add(FRlTransition(AnimationStopped, ERlAnimationState::Running));

	AnimationLibrary.Add(ERlAnimationState::Idle, IA);
	AnimationLibrary.Add(ERlAnimationState::ToIdle, TIA);
	AnimationLibrary.Add(ERlAnimationState::ToIdleR, TIRA);
	AnimationLibrary.Add(ERlAnimationState::Running, RA);

	AnimationLibrary.Add(ERlAnimationState::ToJumping, TJA);
	AnimationLibrary.Add(ERlAnimationState::Jumping, JA);
	AnimationLibrary.Add(ERlAnimationState::ToFalling, TFA);
	AnimationLibrary.Add(ERlAnimationState::Falling, FA);
	AnimationLibrary.Add(ERlAnimationState::Landing, LA);

	SetCurrentState(ERlAnimationState::Idle);

	if (!IsPendingKill())
	{
		if (Sprite)
		{
			// force animation tick after movement component updates
			if (Sprite->PrimaryComponentTick.bCanEverTick && RlCharacterMovement)
			{
				Sprite->PrimaryComponentTick.AddPrerequisite(RlCharacterMovement, RlCharacterMovement->PrimaryComponentTick);
			}
		}

		if (Controller == nullptr)
		{
			if (RlCharacterMovement && RlCharacterMovement->bRunPhysicsWithNoController)
			{
				RlCharacterMovement->SetDefaultMovementMode();
			}
		}
	}
}

// meh
void ARlCharacter::BeginPlay()
{
	Super::BeginPlay();

	URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance());

	if (RlGI->bUseDevice)
	{
		//UKismetSystemLibrary::PrintString(GetWorld(), FString("Start Server"));
		UE_LOG(LogStatus, Log, TEXT("Start Server"));
		FIPv4Endpoint IPv4Endpoint(FIPv4Address(127, 0, 0, 1), 1242);

		TcpListener = new FTcpListener(IPv4Endpoint);

		TcpListener->OnConnectionAccepted().BindUObject(this, &ARlCharacter::OnConnection);

		// This will start the client of the band, so HRM must be running. Move to a separated method later.
		AdvertisementWatcher(true);

		//UGameplayStatics::GetPlayerController(0);
	}
}

void ARlCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance());

	if (RlGI->bUseDevice)
	{
		HearRate(false);

		if (!bDeviceConnected)
		{
			AdvertisementWatcher(false);
		}
	}

	//UKismetSystemLibrary::PrintString(GetWorld(), FString("Stop"));
	//UE_LOG(LogStatus, Log, TEXT("Stop"));

	CloseConnection();

	UKismetSystemLibrary::PrintString(GetWorld(), FString("Stop Success"));
	UE_LOG(LogTemp, Warning, TEXT("Stop Success"));
}

void ARlCharacter::CloseConnection()
{
	if (TcpListener)
	{
		TcpListener->Stop();
		delete TcpListener;
		TcpListener = nullptr;
	}
	if (ConnectionSocket)
	{
		ConnectionSocket->Close();
		ConnectionSocket = nullptr;
	}
}

UPawnMovementComponent* ARlCharacter::GetMovementComponent() const
{
	return RlCharacterMovement;
}

// meh
float ARlCharacter::GetDefaultHalfHeight() const
{
	UBoxComponent* DefaultBox = GetClass()->GetDefaultObject<ARlCharacter>()->BoxComponent;
	if (DefaultBox)
	{
		return DefaultBox->GetScaledBoxExtent().Z / 2.f;
	}
	else
	{
		return Super::GetDefaultHalfHeight();
	}
}

// meh
void ARlCharacter::Jump(FKey Key)
{
	//bIsUsingGamepad = Key.IsGamepadKey();

	//Cast<URlGameInstance>(GetGameInstance())->LevelManager->UpdateInputTutorial();

	if (!IsMoveInputIgnored() && bJumpEnabled)
	{
		bIsPressingJump = true;

		if (!bStoredJump && !bWantJump && !bWantWallWalk && RlCharacterMovement->Velocity.Z == 0.f)
		{
			bStoredJump = true;
		}

		if (!bStoredJump && !bWantJump)
		{
			bWantWallWalk = true;
		}

		JumpKeyHoldTime = 0.f;
		WallWalkHoldTime = 0.f;
	}
}

// meh
void ARlCharacter::StopJumping()
{
	ResetJumpState();
}

// meh
void ARlCharacter::ResetJumpState()
{
	bIsPressingJump = false;
	bWantWallWalk = false;
	bWantJump = false;
	bWasJumping = false;
	JumpKeyHoldTime = 0.0f;
	JumpForceTimeRemaining = 0.0f;
	WallWalkHoldTime = 0.f;

	if (RlCharacterMovement && !RlCharacterMovement->IsFalling())
	{
		JumpCurrentCount = 0;
	}
}

// meh
bool ARlCharacter::CanJump() const
{
	// Can only jump from the ground, or multi-jump if already falling, not wall walking.
	bool bCanJump = RlCharacterMovement && (RlCharacterMovement->IsMovingOnGround() || RlCharacterMovement->IsFalling());

	if (bCanJump)
	{
		// Ensure JumpHoldTime and JumpCount are valid.
		if (!bWasJumping || GetJumpMaxHoldTime() <= 0.0f)
		{
			if (JumpCurrentCount == 0 && RlCharacterMovement->IsFalling())
			{
				bCanJump = JumpCurrentCount + 1 < JumpMaxCount;
			}
			else
			{
				bCanJump = JumpCurrentCount < JumpMaxCount;
			}
		}
		else
		{
			// Only consider JumpKeyHoldTime as long as:
			// A) The jump limit hasn't been met OR
			// B) The jump limit has been met AND we were already jumping
			const bool bJumpKeyHeld = (bWantJump && JumpKeyHoldTime < GetJumpMaxHoldTime());
			bCanJump = bLongJumpEnabled && bJumpKeyHeld && ((JumpCurrentCount < JumpMaxCount) || (bWasJumping && JumpCurrentCount == JumpMaxCount));
		}
	}

	return bCanJump;
}

// meh
bool ARlCharacter::CanWallWalk() const
{
	bool bCanWallWalk = RlCharacterMovement && RlCharacterMovement->IsFalling();

	bCanWallWalk &= bWallWalkEnabled && bWantWallWalk && bWallWalkToggle && !CanJump() && WallWalkHoldTime < WallWalkMaxHoldTime;

	return bCanWallWalk;
}

// meh
void ARlCharacter::CheckJumpInput(float DeltaTime)
{
	if (RlCharacterMovement)
	{
		if (bWantJump)
		{
			if (!bIsPressingJump)
			{
				bWantJump = false;
			}

			// If this is the first jump and we're already falling,
			// then increment the JumpCount to compensate.
			const bool bFirstJump = JumpCurrentCount == 0;
			if (bFirstJump && RlCharacterMovement->IsFalling())
			{
				JumpCurrentCount++;
			}

			const bool bDidJump = CanJump() && RlCharacterMovement->DoJump();
			if (bDidJump)
			{
				// Transition from not (actively) jumping to jumping.
				if (!bWasJumping)
				{
					JumpCurrentCount++;
					JumpForceTimeRemaining = GetJumpMaxHoldTime();
				}
			}

			bWasJumping = bDidJump;
		}

		if (bWantWallWalk)
		{
			const bool bDidWallWalk = CanWallWalk() && RlCharacterMovement->DoWallWalk();
			if (bDidWallWalk)
			{
				if (!bWasWallWalking)
				{
					WallWalkHoldTime = 0.f;
				}
			}

			bWasWallWalking = bDidWallWalk;
		}
	}
}

// meh
bool ARlCharacter::IsJumpProvidingForce() const
{
	return JumpForceTimeRemaining > 0.0f;
}

// meh
void ARlCharacter::ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser)
{
	UDamageType const* const DmgTypeCDO = DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>();
	float const ImpulseScale = DmgTypeCDO->DamageImpulse;

	if ((ImpulseScale > 3.f) && (RlCharacterMovement != nullptr))
	{
		FHitResult HitInfo;
		FVector ImpulseDir;
		DamageEvent.GetBestHitInfo(this, PawnInstigator, HitInfo, ImpulseDir);

		FVector Impulse = ImpulseDir * ImpulseScale;
		bool const bMassIndependentImpulse = !DmgTypeCDO->bScaleMomentumByMass;

		// limit Z momentum added if already going up faster than jump (to avoid blowing character way up into the sky)
		{
			FVector MassScaledImpulse = Impulse;
			if (!bMassIndependentImpulse && RlCharacterMovement->Mass > SMALL_NUMBER)
			{
				MassScaledImpulse = MassScaledImpulse / RlCharacterMovement->Mass;
			}

			if ((RlCharacterMovement->Velocity.Z > GetDefault<URlCharacterMovementComponent>(RlCharacterMovement->GetClass())->JumpZVelocity) && (MassScaledImpulse.Z > 0.f))
			{
				Impulse.Z *= 0.5f;
			}
		}

		RlCharacterMovement->AddImpulse(Impulse, bMassIndependentImpulse);
	}
}

void ARlCharacter::ClearCrossLevelReferences()
{
	if (BasedMovement.MovementBase != nullptr && GetOutermost() != BasedMovement.MovementBase->GetOutermost())
	{
		SetBase(nullptr);
	}

	Super::ClearCrossLevelReferences();
}

namespace RlMovementBaseUtility
{
	// Checked
	bool IsDynamicBase(const UPrimitiveComponent* MovementBase)
	{
		return (MovementBase && MovementBase->Mobility == EComponentMobility::Movable);
	}

	void AddTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* NewBase)
	{
		if (NewBase && RlMovementBaseUtility::UseRelativeLocation(NewBase))
		{
			if (NewBase->PrimaryComponentTick.bCanEverTick)
			{
				BasedObjectTick.AddPrerequisite(NewBase, NewBase->PrimaryComponentTick);
			}

			AActor* NewBaseOwner = NewBase->GetOwner();
			if (NewBaseOwner)
			{
				if (NewBaseOwner->PrimaryActorTick.bCanEverTick)
				{
					BasedObjectTick.AddPrerequisite(NewBaseOwner, NewBaseOwner->PrimaryActorTick);
				}

				// @TODO: We need to find a more efficient way of finding all ticking components in an actor.
				for (UActorComponent* Component : NewBaseOwner->GetComponents())
				{
					// Dont allow a based component (e.g. a particle system) to push us into a different tick group
					if (Component && Component->PrimaryComponentTick.bCanEverTick && Component->PrimaryComponentTick.TickGroup <= BasedObjectTick.TickGroup)
					{
						BasedObjectTick.AddPrerequisite(Component, Component->PrimaryComponentTick);
					}
				}
			}
		}
	}

	void RemoveTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* OldBase)
	{
		if (OldBase && RlMovementBaseUtility::UseRelativeLocation(OldBase))
		{
			BasedObjectTick.RemovePrerequisite(OldBase, OldBase->PrimaryComponentTick);
			AActor* OldBaseOwner = OldBase->GetOwner();
			if (OldBaseOwner)
			{
				BasedObjectTick.RemovePrerequisite(OldBaseOwner, OldBaseOwner->PrimaryActorTick);

				// @TODO: We need to find a more efficient way of finding all ticking components in an actor.
				for (UActorComponent* Component : OldBaseOwner->GetComponents())
				{
					if (Component && Component->PrimaryComponentTick.bCanEverTick)
					{
						BasedObjectTick.RemovePrerequisite(Component, Component->PrimaryComponentTick);
					}
				}
			}
		}
	}

	FVector GetMovementBaseVelocity(const UPrimitiveComponent* MovementBase, const FName BoneName)
	{
		FVector BaseVelocity = FVector::ZeroVector;
		if (RlMovementBaseUtility::IsDynamicBase(MovementBase))
		{
			if (BoneName != NAME_None)
			{
				const FBodyInstance* BodyInstance = MovementBase->GetBodyInstance(BoneName);
				if (BodyInstance)
				{
					BaseVelocity = BodyInstance->GetUnrealWorldVelocity();
					return BaseVelocity;
				}
			}

			BaseVelocity = MovementBase->GetComponentVelocity();
			if (BaseVelocity.IsZero())
			{
				// Fall back to actor's Root component
				const AActor* Owner = MovementBase->GetOwner();
				if (Owner)
				{
					// Component might be moved manually (not by simulated physics or a movement component), see if the root component of the actor has a velocity.
					BaseVelocity = MovementBase->GetOwner()->GetVelocity();
				}
			}

			// Fall back to physics velocity.
			if (BaseVelocity.IsZero())
			{
				if (FBodyInstance* BaseBodyInstance = MovementBase->GetBodyInstance())
				{
					BaseVelocity = BaseBodyInstance->GetUnrealWorldVelocity();
				}
			}
		}

		return BaseVelocity;
	}

	FVector GetMovementBaseTangentialVelocity(const UPrimitiveComponent* MovementBase, const FName BoneName, const FVector& WorldLocation)
	{
		if (RlMovementBaseUtility::IsDynamicBase(MovementBase))
		{
			if (const FBodyInstance* BodyInstance = MovementBase->GetBodyInstance(BoneName))
			{
				const FVector BaseAngVelInRad = BodyInstance->GetUnrealWorldAngularVelocityInRadians();
				if (!BaseAngVelInRad.IsNearlyZero())
				{
					FVector BaseLocation;
					FQuat BaseRotation;
					if (RlMovementBaseUtility::GetMovementBaseTransform(MovementBase, BoneName, BaseLocation, BaseRotation))
					{
						const FVector RadialDistanceToBase = WorldLocation - BaseLocation;
						const FVector TangentialVel = BaseAngVelInRad ^ RadialDistanceToBase;
						return TangentialVel;
					}
				}
			}
		}

		return FVector::ZeroVector;
	}

	// Checked
	bool GetMovementBaseTransform(const UPrimitiveComponent* MovementBase, const FName BoneName, FVector& OutLocation, FQuat& OutQuat)
	{
		if (MovementBase)
		{
			if (BoneName != NAME_None)
			{
				bool bFoundBone = false;
				if (MovementBase)
				{
					// Check if this socket or bone exists (DoesSocketExist checks for either, as does requesting the transform).
					if (MovementBase->DoesSocketExist(BoneName))
					{
						MovementBase->GetSocketWorldLocationAndRotation(BoneName, OutLocation, OutQuat);
						bFoundBone = true;
					}
					else
					{
						UE_LOG(LogCharacter, Warning, TEXT("GetMovementBaseTransform(): Invalid bone or socket '%s' for PrimitiveComponent base %s"), *BoneName.ToString(), *GetPathNameSafe(MovementBase));
					}
				}

				if (!bFoundBone)
				{
					OutLocation = MovementBase->GetComponentLocation();
					OutQuat = MovementBase->GetComponentQuat();
				}
				return bFoundBone;
			}

			// No bone supplied
			OutLocation = MovementBase->GetComponentLocation();
			OutQuat = MovementBase->GetComponentQuat();
			return true;
		}

		// nullptr MovementBase
		OutLocation = FVector::ZeroVector;
		OutQuat = FQuat::Identity;
		return false;
	}
}

/**	Change the Pawn's base. */
void ARlCharacter::SetBase(UPrimitiveComponent* NewBaseComponent, const FName InBoneName, bool bNotifyPawn)
{
	// If NewBaseComponent is nullptr, ignore bone name.
	const FName BoneName = (NewBaseComponent ? InBoneName : NAME_None);

	// See what changed.
	const bool bBaseChanged = (NewBaseComponent != BasedMovement.MovementBase);
	const bool bBoneChanged = (BoneName != BasedMovement.BoneName);

	if (bBaseChanged || bBoneChanged)
	{
		// Verify no recursion.
		APawn* Loop = (NewBaseComponent ? Cast<APawn>(NewBaseComponent->GetOwner()) : nullptr);
		while (Loop)
		{
			if (Loop == this)
			{
				UE_LOG(LogCharacter, Warning, TEXT(" SetBase failed! Recursion detected. Pawn %s already based on %s."), *GetName(), *NewBaseComponent->GetName()); //-V595
				return;
			}
			if (UPrimitiveComponent* LoopBase = Loop->GetMovementBase())
			{
				Loop = Cast<APawn>(LoopBase->GetOwner());
			}
			else
			{
				break;
			}
		}

		// Set base.
		UPrimitiveComponent* OldBase = BasedMovement.MovementBase;
		BasedMovement.MovementBase = NewBaseComponent;
		BasedMovement.BoneName = BoneName;

		if (RlCharacterMovement)
		{
			const bool bBaseIsSimulating = NewBaseComponent && NewBaseComponent->IsSimulatingPhysics();
			if (bBaseChanged)
			{
				RlMovementBaseUtility::RemoveTickDependency(RlCharacterMovement->PrimaryComponentTick, OldBase);
				// We use a special post physics function if simulating, otherwise add normal tick prereqs.
				if (!bBaseIsSimulating)
				{
					RlMovementBaseUtility::AddTickDependency(RlCharacterMovement->PrimaryComponentTick, NewBaseComponent);
				}
			}

			if (NewBaseComponent)
			{
				// Update OldBaseLocation/Rotation as those were referring to a different base
				// ... but not when handling replication for proxies (since they are going to copy this data from the replicated values anyway)
				//if (!bInBaseReplication)
				//{
					// Force base location and relative position to be computed since we have a new base or bone so the old relative offset is meaningless.
				RlCharacterMovement->SaveBaseLocation();
				//}
			}
			else
			{
				BasedMovement.BoneName = NAME_None; // None, regardless of whether user tried to set a bone name, since we have no base component.
				BasedMovement.bRelativeRotation = false;
				RlCharacterMovement->CurrentFloor.Clear();
			}

			if (Role == ROLE_Authority || Role == ROLE_AutonomousProxy)
			{
				BasedMovement.bServerHasBaseComponent = (BasedMovement.MovementBase != nullptr); // Also set on proxies for nicer debugging.
				UE_LOG(LogCharacter, Verbose, TEXT("Setting base on %s for '%s' to '%s'"), Role == ROLE_Authority ? TEXT("Server") : TEXT("AutoProxy"), *GetName(), *GetFullNameSafe(NewBaseComponent));
			}
			else
			{
				UE_LOG(LogCharacter, Verbose, TEXT("Setting base on Client for '%s' to '%s'"), *GetName(), *GetFullNameSafe(NewBaseComponent));
			}
		}

		// Notify this actor of his new floor.
		if (bNotifyPawn)
		{
			BaseChange();
		}
	}
}

void ARlCharacter::SaveRelativeBasedMovement(const FVector& NewRelativeLocation, const FRotator& NewRotation, bool bRelativeRotation)
{
	checkSlow(BasedMovement.HasRelativeLocation());
	BasedMovement.Location = NewRelativeLocation;
	BasedMovement.Rotation = NewRotation;
	BasedMovement.bRelativeRotation = bRelativeRotation;
}

// meh
void ARlCharacter::TurnOff()
{
	if (RlCharacterMovement != nullptr)
	{
		RlCharacterMovement->StopMovementImmediately();
		RlCharacterMovement->DisableMovement();
	}

	Super::TurnOff();
}

// meh
void ARlCharacter::Restart()
{
	Super::Restart();

	JumpCurrentCount = 0;

	bWantJump = false;
	ResetJumpState();

	if (RlCharacterMovement)
	{
		RlCharacterMovement->SetDefaultMovementMode();
	}
}

// meh
void ARlCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	NumActorOverlapEventsCounter++;
	Super::NotifyActorBeginOverlap(OtherActor);
}

// meh
void ARlCharacter::NotifyActorEndOverlap(AActor* OtherActor)
{
	NumActorOverlapEventsCounter++;
	Super::NotifyActorEndOverlap(OtherActor);
}

void ARlCharacter::BaseChange()
{
	if (RlCharacterMovement)
	{
		AActor* ActualMovementBase = GetMovementBaseActor(this);
		if ((ActualMovementBase != nullptr) && !ActualMovementBase->CanBeBaseForCharacter(this))
		{
			RlCharacterMovement->JumpOff(ActualMovementBase);
		}
	}
}

// meh
void ARlCharacter::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	float Indent = 0.f;

	static FName NAME_Physics = FName(TEXT("Physics"));
	if (DebugDisplay.IsDisplayOn(NAME_Physics))
	{
		FIndenter PhysicsIndent(Indent);

		FString BaseString;
		if (RlCharacterMovement == nullptr || BasedMovement.MovementBase == nullptr)
		{
			BaseString = "Not Based";
		}
		else
		{
			BaseString = BasedMovement.MovementBase->IsWorldGeometry() ? "World Geometry" : BasedMovement.MovementBase->GetName();
			BaseString = FString::Printf(TEXT("Based On %s"), *BaseString);
		}

		FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
		DisplayDebugManager.DrawString(FString::Printf(TEXT("RelativeLoc: %s Rot: %s %s"), *BasedMovement.Location.ToCompactString(), *BasedMovement.Rotation.ToCompactString(), *BaseString), Indent);

		if (RlCharacterMovement != nullptr)
		{
			//RlCharacterMovement->DisplayDebug(Canvas, DebugDisplay, YL, YPos);
		}
		const bool Crouched = RlCharacterMovement && RlCharacterMovement->IsCrouching();
		FString T = FString::Printf(TEXT("Crouched %i"), Crouched);
		DisplayDebugManager.DrawString(T, Indent);
	}
}

// meh
void ARlCharacter::LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
{
	UE_LOG(LogCharacter, Verbose, TEXT("ARlCharacter::LaunchCharacter '%s' (%f,%f,%f)"), *GetName(), LaunchVelocity.X, LaunchVelocity.Y, LaunchVelocity.Z);

	if (RlCharacterMovement)
	{
		FVector FinalVel = LaunchVelocity;
		const FVector Velocity = GetVelocity();

		if (!bXYOverride)
		{
			FinalVel.X += Velocity.X;
			FinalVel.Y += Velocity.Y;
		}
		if (!bZOverride)
		{
			FinalVel.Z += Velocity.Z;
		}

		RlCharacterMovement->Launch(FinalVel);
	}
}

void ARlCharacter::ClearJumpInput(float DeltaTime)
{
	if (bWantJump)
	{
		JumpKeyHoldTime += DeltaTime;

		// Don't disable bPressedJump right away if it's still held.
		// Don't modify JumpForceTimeRemaining because a frame of update may be remaining.
		if (JumpKeyHoldTime >= GetJumpMaxHoldTime())
		{
			bWantJump = false;
		}
	}
	else
	{
		JumpForceTimeRemaining = 0.0f;
		bWasJumping = false;
	}
}

// meh
float ARlCharacter::GetJumpMaxHoldTime() const
{
	return JumpMaxHoldTime;
}

// meh
void ARlCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentDeltaTime = DeltaTime;

	UpdateAnimation();

	URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance());

	if (RlGI->bUseDevice)
	{
		CheckServer();

		if (!bDeviceConnected && !bConnectionAccepted)
		{
			AdvertisementWatcher(true);
		}
	}
}

// meh
void ARlCharacter::MoveRight(float Value)
{
	if (bIsSprinting)
	{
		if (Value > 0)
		{
			Value = FMath::Lerp(0.5f, 1.f, Value);
		}
		if (Value < 0)
		{
			Value = FMath::Lerp(-0.5f, -1.f, -Value);
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("%f"), Value);

	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

// meh
void ARlCharacter::UpdateAnimation()
{
	// Dust update

	float VX = RlCharacterMovement->Velocity.X;
	float AX = RlCharacterMovement->GetCurrentAcceleration().X;
	float NMWS = RlCharacterMovement->NormalMaxWalkSpeed;

	if (bDust && (!bIsSprinting || bIsChangingDirection || FMath::Abs(VX) < NMWS || RlCharacterMovement->IsFalling()))
	{
		bDust = false;
		DustComponent->Deactivate();
	}

	if (!bDust && bIsSprinting && FMath::Abs(VX) > NMWS && !RlCharacterMovement->IsFalling())
	{
		bDust = true;
		DustComponent->SetTemplate(AX > 0 ? DustRight : DustLeft);
		DustComponent->Activate(true);
	}

	FRlAnimation CurrentAnimationInfo = AnimationLibrary[CurrentState];

	// Check Start and End
	if (CurrentAnimationInfo.bReverse)
	{
		if (CurrentAnimationInfo.Start != -1 && BelowFrame(CurrentAnimationInfo.Start))
		{
			if (CurrentAnimationInfo.bLoop)
			{
				Sprite->SetPlaybackPositionInFrames(CurrentAnimationInfo.End, false);
			}
			else
			{
				Sprite->Stop();
			}
		}
	}
	else
	{
		if (CurrentAnimationInfo.End != -1 && AboveFrame(CurrentAnimationInfo.End))
		{
			if (CurrentAnimationInfo.bLoop)
			{
				Sprite->SetPlaybackPositionInFrames(CurrentAnimationInfo.Start, false);
			}
			else
			{
				Sprite->Stop();
			}
		}
	}

	const float VelocityX = RlCharacterMovement->Velocity.X;
	const float VelocityZ = RlCharacterMovement->Velocity.Z;

	//UE_LOG(LogTemp, Warning, TEXT("X: %f    Z: %f    S: %i"), VelocityX, VelocityZ, (int)CurrentState);

	bool bOldIsLastDirectionRight = bIsLastDirectionRight;

	if (VelocityX > 0.f)
	{
		bIsLastDirectionRight = true;
	}
	else if (VelocityX < 0.f)
	{
		bIsLastDirectionRight = false;
	}

	const bool bSwitchDirection = bIsLastDirectionRight != bOldIsLastDirectionRight;

	if (FMath::Sign(VelocityX * RlCharacterMovement->GetCurrentAcceleration().X) < 0.f)
	{
		bIsChangingDirection = true;
	}

	if (FMath::Abs(VelocityX) > WalkSpeed || RlCharacterMovement->GetCurrentAcceleration().X == 0.f)
	{
		bIsChangingDirection = false;
	}

	if (bSwitchDirection)
	{
		SetCurrentState(CurrentState);

		//bIsChangingDirection = false;
	}

	if (AnimationLibrary.Contains(CurrentState))
	{
		for (FRlTransition Transition : AnimationLibrary[CurrentState].Transitions)
		{
			if (Transition.Predicate(this))
			{
				SetCurrentState(Transition.NextState);
				break;
			}
		}
	}

	// Change start of TJA to match state of TIRA

	if (CurrentState == ERlAnimationState::Running)
	{
		FScopedFlipbookMutator ScopedFlipbookMutator(Sprite->GetFlipbook());
		ScopedFlipbookMutator.FramesPerSecond = FMath::Max(FMath::RoundToInt((WalkSpeed * FMath::Abs(RlCharacterMovement->Velocity.X)) / RlCharacterMovement->NormalMaxWalkSpeed), 10);
	}

	if (CurrentState == ERlAnimationState::Idle)
	{
		UPaperFlipbook* CurrentAnimation = AnimationLibrary[CurrentState].Animation(this);
		UPaperFlipbook* CurrentAnimationExtras = bIsLastDirectionRight ? IdleRightAnimationExtras : IdleLeftAnimationExtras;

		const float EffectiveDeltaTime = CurrentDeltaTime * Sprite->GetPlayRate();
		float NewPosition = Sprite->GetPlaybackPosition() + EffectiveDeltaTime;

		if (CurrentAnimation  && CurrentAnimationExtras && NewPosition > Sprite->GetFlipbookLength())
		{
			FScopedFlipbookMutator ScopedFlipbookMutator(CurrentAnimation);
			ScopedFlipbookMutator.KeyFrames[1] = CurrentAnimationExtras->GetKeyFrameChecked(FMath::RandRange(0, 1));
			ScopedFlipbookMutator.KeyFrames[3] = CurrentAnimationExtras->GetKeyFrameChecked(FMath::RandRange(2, 4));
		}
	}
}

// meh
void ARlCharacter::SetCurrentState(ERlAnimationState State)
{
	ERlAnimationState OldState = CurrentState;
	CurrentState = State;

	UPaperFlipbook* OldAnimation = AnimationLibrary[OldState].Animation(this);
	UPaperFlipbook* CurrentAnimation = AnimationLibrary[CurrentState].Animation(this);
	float OldFrame = Sprite->GetPlaybackPositionInFrames();

	bool bChangedAnimation = OldAnimation != CurrentAnimation;

	if (CurrentState == ERlAnimationState::Jumping)
	{
		if (bStoredJump)
		{
			bStoredJump = false;
			bWantJump = true;
		}
	}

	Sprite->SetFlipbook(CurrentAnimation);

	Sprite->SetLooping(AnimationLibrary[CurrentState].bLoop);

	if (OldState == ERlAnimationState::Idle && CurrentState == ERlAnimationState::ToJumping)
	{
		bChangedAnimation = false;
		Sprite->SetPlaybackPositionInFrames(2, false);
	}

	if ((OldState == ERlAnimationState::ToIdle || OldState == ERlAnimationState::ToIdleR) && CurrentState == ERlAnimationState::ToJumping)
	{
		bChangedAnimation = false;
		Sprite->SetPlaybackPositionInFrames(OldFrame, false);
	}

	if (AnimationLibrary[CurrentState].bReverse)
	{
		if (!AnimationLibrary[CurrentState].bKeepFrame || bChangedAnimation)
		{
			const int End = AnimationLibrary[CurrentState].End;
			Sprite->SetPlaybackPositionInFrames(End != -1 ? End : Sprite->GetFlipbookLengthInFrames(), false);
		}
		Sprite->Reverse();
	}
	else
	{
		if (!AnimationLibrary[CurrentState].bKeepFrame || bChangedAnimation)
		{
			const int Start = AnimationLibrary[CurrentState].Start;
			Sprite->SetPlaybackPositionInFrames(Start != -1 ? Start : 0.f, false);
		}
		Sprite->Play();
	}

	//UE_LOG(LogTemp, Warning, TEXT("%i"), CurrentState);
}

// meh
int ARlCharacter::GetFlipbookLengthInFrames(UPaperFlipbook* (*Animation)(ARlCharacter*))
{
	UPaperFlipbook* PaperFlipbook = Animation(this);
	return PaperFlipbook ? PaperFlipbook->GetNumFrames() : 0;
}

// meh
float ARlCharacter::NextAnimationPostion()
{
	const float EffectiveDeltaTime = CurrentDeltaTime * Sprite->GetPlayRate();
	return Sprite->GetPlaybackPosition() + EffectiveDeltaTime;
}

// meh
bool ARlCharacter::AboveFrame(int32 Frame)
{
	return NextAnimationPostion() > Frame / Sprite->GetFlipbookFramerate();
}

// meh
bool ARlCharacter::BelowFrame(int32 Frame)
{
	return NextAnimationPostion() < Frame / Sprite->GetFlipbookFramerate();
}

// meh
void ARlCharacter::Level()
{
	ULevelManager* LM = Cast<URlGameInstance>(GetGameInstance())->LevelManager;
	LM->SetLevel(ELevelState::Start);
}

// meh
void ARlCharacter::Skip()
{
	ULevelManager* LM = Cast<URlGameInstance>(GetGameInstance())->LevelManager;
	if (LM->CurrentLevel)
	{
		LM->SetLevel(ELevelState::Next);
	}
}

// meh
void ARlCharacter::Reset()
{
	ULevelManager* LM = Cast<URlGameInstance>(GetGameInstance())->LevelManager;
	if (LM->CurrentLevel)
	{
		LM->RestartLevel();
	}
}

// meh
void ARlCharacter::DifficultyDebug(float Value)
{
	if (!Value)
	{
		return;
	}
	if (Value == -1.f)
	{
		Value = 0;
	}

	ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode());

	if (Value == GameMode->Difficulty)
	{
		return;
	}

	GameMode->Difficulty = Value;

	UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Difficulty: %f"), GameMode->Difficulty));
}

// meh
void ARlCharacter::DifficultyAddDebug(float Value)
{
	if (!Value)
	{
		bCanDA = true;
		return;
	}
	if (bCanDA)
	{
		bCanDA = false;

		ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode());
		GameMode->Difficulty += Value;

		UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Difficulty: %f"), GameMode->Difficulty));
	}
}

// meh
void ARlCharacter::Death()
{
	if (!bDeath)
	{
		Vibrate(100);

		bDeath = true;
		Sprite->ToggleVisibility();

		ResetJumpState();
		bStoredJump = false;

		if (ARlPlayerController* RlPlayerController = Cast<ARlPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
		{
			RlPlayerController->SetIgnoreMoveInput(true);
		}

		URlGameInstance* RlGI = Cast<URlGameInstance>(GetGameInstance());

		if (ARlSpriteHUD* RlSHUD = RlGI->RlSpriteHUD)
		{
			RlSHUD->IncreaseDeaths();
		}

		//std::ostringstream oss;
		//oss << '\0' << RlGI->Deaths << '\0' << "Deaths" << '\0' << '\0';
		//std::string Message = oss.str();

		//WriteMessage((uint8*)Message.c_str(), Message.size());

		// Timer to call LM->RestartLevel() on end of death animation

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathAnimation, GetActorLocation() + FVector(0.f, 0.f, -6.f), FRotator(0.f, bIsLastDirectionRight ? 0.f : 180.f, 0.f));

		ULevelManager* LM = RlGI->LevelManager;
		GetWorld()->GetTimerManager().SetTimer(DeathHandle, LM, &ULevelManager::RestartLevel, 0.5f, false);
		//LM->RestartLevel();
	}
}

// meh
void ARlCharacter::Up()
{
	//WriteMessage((uint8 *)u8"· o ·)>", 9);

	//HearRate(true);

	if (!bDeath)
	{
		TArray<AActor*> Stairs;
		GetOverlappingActors(Stairs, AStairs::StaticClass());
		if (Stairs.Num())
		{
			AStairs* OverlappingStairs = Cast<AStairs>(Stairs.Pop());
			if (OverlappingStairs->bCanEnter && !bInvincible)
			{
				ULevelManager* LM = Cast<URlGameInstance>(GetGameInstance())->LevelManager;
				LM->SetLevel(ELevelState::Next);
				bInvincible = true;
			}
		}
	}
}

// meh
void ARlCharacter::Vibrate(uint16 Milliseconds)
{
	int32 Sent;
	const uint8 Id = 0x04;
	//const uint8 Data[] = { 0x00, (Milliseconds >> 8) & 0xff, Milliseconds & 0xff };

	FIPv4Endpoint IPv4Endpoint(FIPv4Address(127, 0, 0, 1), 1243);

	FSocket* Socket = FTcpSocketBuilder(TEXT("FTcpListener server"));
	Socket->Connect(*IPv4Endpoint.ToInternetAddr());
	Socket->Send(&Id, 1, Sent);
	Socket->Send((const uint8*)&Milliseconds, 2, Sent);
	Socket->Close();
}

// meh
void ARlCharacter::WriteMessage(uint8* Message, uint32 MessageSize)
{
	//Message.
	int32 Sent;
	const uint8 Id = 0x02;

	FIPv4Endpoint IPv4Endpoint(FIPv4Address(127, 0, 0, 1), 1243);

	FSocket* Socket = FTcpSocketBuilder(TEXT("FTcpListener server"));
	Socket->Connect(*IPv4Endpoint.ToInternetAddr());
	Socket->Send(&Id, 1, Sent);
	Socket->Send((const uint8*)&MessageSize, 4, Sent);
	Socket->Send(Message, MessageSize, Sent);

	Socket->Close();
}

// meh
void ARlCharacter::HearRate(bool bStart)
{
	int32 Sent;
	const uint8 Id = 0x03;

	FIPv4Endpoint IPv4Endpoint(FIPv4Address(127, 0, 0, 1), 1243);

	FSocket* Socket = FTcpSocketBuilder(TEXT("FTcpListener server"));
	Socket->Connect(*IPv4Endpoint.ToInternetAddr());
	Socket->Send(&Id, 1, Sent);
	Socket->Send((const uint8*)&bStart, 1, Sent);

	Socket->Close();
}

// meh
void ARlCharacter::AdvertisementWatcher(bool bStart)
{
	int32 Sent;
	const uint8 Id = 0x00;

	FIPv4Endpoint IPv4Endpoint(FIPv4Address(127, 0, 0, 1), 1243);

	FSocket* Socket = FTcpSocketBuilder(TEXT("FTcpListener server"));
	Socket->Connect(*IPv4Endpoint.ToInternetAddr());
	Socket->Send(&Id, 1, Sent);
	Socket->Send((const uint8*)&bStart, 1, Sent);

	Socket->Close();
}

// meh
void ARlCharacter::Connect(FString Device)
{
	int32 Sent;
	const uint8 Id = 0x01;

	////auto a = Device.GetCharArray();

	//auto a = *Device;

	uint32 MessageSize = Device.Len();

	char* Message = TCHAR_TO_ANSI(*Device);

	auto a = Device;

	FIPv4Endpoint IPv4Endpoint(FIPv4Address(127, 0, 0, 1), 1243);

	FSocket* Socket = FTcpSocketBuilder(TEXT("FTcpListener server"));
	Socket->Connect(*IPv4Endpoint.ToInternetAddr());
	Socket->Send(&Id, 1, Sent);
	Socket->Send((const uint8*)&MessageSize, 4, Sent);
	Socket->Send((const uint8*)Message, MessageSize, Sent);

	Socket->Close();

	UWidgetManager* WM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->WidgetManager;
	if (WM->DeviceSelection)
	{
		GetWorld()->GetTimerManager().SetTimer(DeviceConnectionHandle, WM->DeviceSelection, &UDeviceSelection::DeviceConnectionFailed, 20.f);
	}
}

// meh
void ARlCharacter::Vibrate()
{
	int32 Sent;
	const uint8 Id = 0x05;

	FIPv4Endpoint IPv4Endpoint(FIPv4Address(127, 0, 0, 1), 1243);

	FSocket* Socket = FTcpSocketBuilder(TEXT("FTcpListener server"));
	Socket->Connect(*IPv4Endpoint.ToInternetAddr());
	Socket->Send(&Id, 1, Sent);
	Socket->Close();
}

// meh	
bool ARlCharacter::OnConnection(FSocket* Socket, const struct FIPv4Endpoint& IPv4Endpoint)
{
	bConnectionAccepted = true;
	//UKismetSystemLibrary::PrintString(GetWorld(), FString("Connection accepted"));
	UE_LOG(LogStatus, Log, TEXT("Connection accepted"));
	ConnectionSocket = Socket;
	int32 NewSize;
	if (!bDeviceConnected)
	{
		ConnectionSocket->SetReceiveBufferSize(17 * sizeof(uint8), NewSize);
	}
	else
	{
		ConnectionSocket->SetReceiveBufferSize(2 * sizeof(uint8), NewSize);
	}
	return true;
}

// meh	
void ARlCharacter::CheckServer()
{
	if (!ConnectionSocket)
	{
		return;
	}

	TArray<uint8> ReceivedData;

	uint32 Size;
	while (ConnectionSocket->HasPendingData(Size))
	{
		ReceivedData.Init(0, FMath::Min(Size, 65507u));

		int32 Read = 0;
		ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
	}

	if (ReceivedData.Num() <= 0)
	{
		return;
	}

	ReceivedData.Add(0);
	const FString ReceivedUE4String = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));

	ULevelManager* LM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->LevelManager;
	UWidgetManager* WM = Cast<URlGameInstance>(GetWorld()->GetGameInstance())->WidgetManager;

	if (!bDeviceConnected)
	{
		if (!ReceivedUE4String.Compare("Connected"))
		{
			GetWorld()->GetTimerManager().ClearTimer(DeviceConnectionHandle);

			bDeviceConnected = true;

			if (WM->DeviceSelection)
			{
				WM->DeviceSelection->RemoveFromParent();
			}

			UE_LOG(LogStatus, Log, TEXT("Hear Rate Measurement Started"));

			HearRate(true);

			if (ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode()))
			{
				GameMode->HeartRateModule->StartCalibration();
			}

			if (Cast<URlGameInstance>(GetWorld()->GetGameInstance())->bIntro)
			{
				WM->StartIntro();
			}
			else
			{
				LM->SetLevel(ELevelState::Start);
			}

			//auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			//PC->SetInputMode(FInputModeGameOnly());
			//EnableInput(PC);
		}
		else if (WM->DeviceSelection)
		{
			UE_LOG(LogDevice, Log, TEXT("%s"), *ReceivedUE4String);
			WM->DeviceSelection->AddDevice(ReceivedUE4String);
		}
	}
	else
	{
		if (ReceivedUE4String.Len() == 2)
		{
			if (ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode()))
			{
				GameMode->HeartRateModule->AddHeartRate(ReceivedUE4String);
			}
		}
		//UE_LOG(LogHeartRate, Log, TEXT("%s"), *ReceivedUE4String);
	}

	//UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Heart Rate: %s"), *ReceivedUE4String), true, true, FLinearColor(0.0, 0.66, 1.0), 20.f);
}

// meh
void ARlCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ARlCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ARlCharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveRight", this, &ARlCharacter::MoveRight);

	// TODO Remove
	PlayerInputComponent->BindAxis("DifficultyDebug", this, &ARlCharacter::DifficultyDebug);
	PlayerInputComponent->BindAxis("DifficultyAddDebug", this, &ARlCharacter::DifficultyAddDebug);

	PlayerInputComponent->BindAction("Up", IE_Pressed, this, &ARlCharacter::Up);

	PlayerInputComponent->BindAction("Level", IE_Pressed, this, &ARlCharacter::Level);

	PlayerInputComponent->BindAction("Skip", IE_Pressed, this, &ARlCharacter::Skip);

	PlayerInputComponent->BindAction("Reset", IE_Pressed, this, &ARlCharacter::Reset);

	PlayerInputComponent->BindAction("Vibrate", IE_Pressed, this, &ARlCharacter::Vibrate);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, RlCharacterMovement, &URlCharacterMovementComponent::SprintStart);

	PlayerInputComponent->BindAction("Sprint", IE_Released, RlCharacterMovement, &URlCharacterMovementComponent::SprintStop);
}

#if WITH_EDITOR
void ARlCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ARlCharacter, Difficulty))
	{
		ARlGameMode* GameMode = Cast<ARlGameMode>(GetWorld()->GetAuthGameMode());
		GameMode->Difficulty = Difficulty;
	}
}
#endif