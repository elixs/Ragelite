// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RlCharacterMovementComponent.h"
#include "TimerManager.h"
#include "RlCharacter.generated.h"

class UPaperFlipbookComponent;
class URlCharacterMovementComponent;
class UBoxComponent;
class UArrowComponent;
class FDebugDisplayInfo;
class UPaperFlipbook;
class UParticleSystem;
class UParticleSystemComponent;
class FSocket;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRlCharacterReachedApexSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRlLandedSignature, const FHitResult&, Hit);

//DECLARE_LOG_CATEGORY_EXTERN(LogHeartRate, Log, All);
//DECLARE_LOG_CATEGORY_EXTERN(LogDevice, Log, All);



enum class ERlAnimationState : int8
{
	Idle,
	ToIdle,
	ToIdleR,
	Running,
	ToJumping,
	Jumping,
	ToFalling,
	Falling,
	Landing
};

struct FRlTransition
{

	bool(*Predicate)(ARlCharacter*); // Give this as an argument.

	ERlAnimationState NextState;

	FRlTransition() {};

	FRlTransition(bool(*InPredicate)(ARlCharacter*), ERlAnimationState InNextState)
	{
		Predicate = InPredicate;
		NextState = InNextState;
	}

};

struct FRlAnimation
{

	//UPaperFlipbook* Animation;
	UPaperFlipbook* (*Animation)(ARlCharacter*); // Give this as an argument.
	//float AnimationSpeed; // maybe convert to a lambda?
	bool bLoop;
	bool bReverse;
	int Start; // -1 to play from start
	int End; // -1 to play from end
	bool bKeepFrame; // True to play from current frame

	TArray<FRlTransition> Transitions; // First transitions have priority

	FRlAnimation() {};

	FRlAnimation(UPaperFlipbook* (*InAnimation)(ARlCharacter*), bool bInLoop, bool bInReverse, bool bInKeepFrame = false, int InStart = -1, int InEnd = -1)
	{
		Animation = InAnimation;
		bLoop = bInLoop;
		bReverse = bInReverse;
		Start = InStart;
		End = InEnd;
		bKeepFrame = bInKeepFrame;
		
	}
};



/** RlMovementBaseUtility provides utilities for working with movement bases, for which we may need relative positioning info. */
namespace RlMovementBaseUtility
{
	/** Determine whether MovementBase can possibly move. */
	RAGELITE_API bool IsDynamicBase(const UPrimitiveComponent* MovementBase);

	/** Determine if we should use relative positioning when based on a component (because it may move). */
	FORCEINLINE bool UseRelativeLocation(const UPrimitiveComponent* MovementBase)
	{
		return IsDynamicBase(MovementBase);
	}

	/** Ensure that BasedObjectTick ticks after NewBase */
	RAGELITE_API void AddTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* NewBase);

	/** Remove tick dependency of BasedObjectTick on OldBase */
	RAGELITE_API void RemoveTickDependency(FTickFunction& BasedObjectTick, UPrimitiveComponent* OldBase);

	/** Get the velocity of the given component, first checking the ComponentVelocity and falling back to the physics velocity if necessary. */
	RAGELITE_API FVector GetMovementBaseVelocity(const UPrimitiveComponent* MovementBase, const FName BoneName);

	/** Get the tangential velocity at WorldLocation for the given component. */
	RAGELITE_API FVector GetMovementBaseTangentialVelocity(const UPrimitiveComponent* MovementBase, const FName BoneName, const FVector& WorldLocation);

	/** Get the transforms for the given MovementBase, optionally at the location of a bone. Returns false if MovementBase is nullptr, or if BoneName is not a valid bone. */
	RAGELITE_API bool GetMovementBaseTransform(const UPrimitiveComponent* MovementBase, const FName BoneName, FVector& OutLocation, FQuat& OutQuat);
}

/** Struct to hold information about the "base" object the character is standing on. */
USTRUCT()
struct FRlBasedMovementInfo
{
	GENERATED_USTRUCT_BODY()

	/** Component we are based on */
	UPROPERTY()
	UPrimitiveComponent* MovementBase;

	/** Bone name on component, for skeletal meshes. NAME_None if not a skeletal mesh or if bone is invalid. */
	UPROPERTY()
	FName BoneName;

	/** Location relative to MovementBase. Only valid if HasRelativeLocation() is true. */
	UPROPERTY()
	FVector_NetQuantize100 Location;

	/** Rotation: relative to MovementBase if HasRelativeRotation() is true, absolute otherwise. */
	UPROPERTY()
	FRotator Rotation;

	/** Whether the server says that there is a base. On clients, the component may not have resolved yet. */
	UPROPERTY()
	bool bServerHasBaseComponent;

	/** Whether rotation is relative to the base or absolute. It can only be relative if location is also relative. */
	UPROPERTY()
	bool bRelativeRotation;

	/** Whether there is a velocity on the server. Used for forcing replication when velocity goes to zero. */
	UPROPERTY()
	bool bServerHasVelocity;

	/** Is location relative? */
	FORCEINLINE bool HasRelativeLocation() const
	{
		return RlMovementBaseUtility::UseRelativeLocation(MovementBase);
	}

	/** Is rotation relative or absolute? It can only be relative if location is also relative. */
	FORCEINLINE bool HasRelativeRotation() const
	{
		return bRelativeRotation && HasRelativeLocation();
	}

	/** Return true if the client should have MovementBase, but it hasn't replicated (possibly component has not streamed in). */
	FORCEINLINE bool IsBaseUnresolved() const
	{
		return (MovementBase == nullptr) && bServerHasBaseComponent;
	}
};


/**
 * 
 */ 
//UCLASS()
UCLASS(config=Game, BlueprintType, meta=(ShortTooltip="A character is a type of Pawn that includes the ability to walk around."))
class RAGELITE_API ARlCharacter : public APawn
{
	GENERATED_BODY()
public:
	/** Default UObject constructor. */
	ARlCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

private:
	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category = Character, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UPaperFlipbookComponent* Sprite;
public:

	/** Movement component used for movement logic in various movement modes (walking, falling, etc), containing relevant settings and functions to control movement. */
	UPROPERTY(Category = Character, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	URlCharacterMovementComponent* RlCharacterMovement;

	///** Movement component used for movement logic in various movement modes (walking, falling, etc), containing relevant settings and functions to control movement. */
	//UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	//URlCharacterMovementComponent* CharacterMovement;

	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category = Character, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BoxComponent;

	UPROPERTY(Category = Character, VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* DustComponent;

#if WITH_EDITORONLY_DATA
	/** Component shown in the editor only to indicate character facing */
	UPROPERTY()
	UArrowComponent* ArrowComponent;
#endif

public:

	/** Returns Mesh subobject **/
	FORCEINLINE UPaperFlipbookComponent* GetSprite() const { return Sprite; }

	/** Name of the SpriteComponent. Use this name if you want to prevent creation of the component (with ObjectInitializer.DoNotCreateDefaultSubobject). */
	static FName SpriteComponentName;


	/** Returns RlCharacterMovement subobject **/
	FORCEINLINE URlCharacterMovementComponent* GetRlCharacterMovement() const { return RlCharacterMovement; }

	/** Name of the CharacterMovement component. Use this name if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass). */
	static FName RlCharacterMovementComponentName;

	/** Returns BoxComponent subobject **/
	FORCEINLINE UBoxComponent* GetBoxComponent() const { return BoxComponent; }

	/** Name of the CapsuleComponent. */
	static FName BoxComponentName;

#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif


	
protected:
	// The animation to play while running around
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* RunningRightAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* ToIdleRightAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* IdleRightAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* IdleRightAnimationExtras;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* JumpRightAnimation;

	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* WallWalkRightAnimation;



	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* RunningLeftAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* ToIdleLeftAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* IdleLeftAnimation;


	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* IdleLeftAnimationExtras;


	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* JumpLeftAnimation;

	UPROPERTY(EditAnywhere, Category = Animations)
	UPaperFlipbook* WallWalkLeftAnimation;



	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
	UParticleSystem* DeathAnimation;

	UParticleSystem* DustRight;

	UParticleSystem* DustLeft;



	/** Called for side to side input */
	void MoveRight(float Value);

	/** Called to choose the correct animation to play based on the character's movement state */
	void UpdateAnimation();

	void Up();





	///** Handle touch inputs. */
	//void TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location);

	///** Handle touch stop event. */
	//void TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location);

		/** Called for side to side input */
	void Level();

	FTimerHandle IgnoreTimerHandle;

	FTimerHandle DeathHandle;

	void Skip();
	void Reset();

	void DifficultyDebug(float Value);
	void DifficultyAddDebug(float Value);

	bool bCanDA = false;

private:
	ERlAnimationState CurrentState;

	void SetCurrentState(ERlAnimationState State);

	//UPROPERTY()
	TMap<ERlAnimationState, FRlAnimation> AnimationLibrary;

	int GetFlipbookLengthInFrames(UPaperFlipbook* (*Animation)(ARlCharacter*));

	float NextAnimationPostion();

	bool AboveFrame(int32 Frame);
	bool BelowFrame(int32 Frame);

public:

	bool bDeath;
	bool bInvincible;

	void Death();

	bool bDust;


	// Animation
	bool bIsLastDirectionRight;
	float WalkSpeed;
	float CurrentDeltaTime;
	bool bIsChangingDirection;


	float PreviousVelocityX;

	bool bIsReversing;

	bool bIsUsingGamepad;

	void Vibrate();

	void Vibrate(uint16 Milliseconds);

	void WriteMessage(uint8* Message, uint32 MessageSize);

	void HearRate(bool bStart);

	void AdvertisementWatcher(bool bStart);

	void Connect(FString Device);

	bool bDeviceConnected;

	class FTcpListener* TcpListener;
	FSocket* ConnectionSocket;

	bool OnConnection(FSocket* Socket, const struct FIPv4Endpoint& IPv4Endpoint);

	bool bConnectionAccepted;

	void CheckServer();

	FTimerHandle DeviceConnectionHandle;

	void CloseConnection();

	// For Test Only
	UPROPERTY(EditAnywhere)
	float Difficulty;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;



	/** Sets the component the Character is walking on, used by CharacterMovement walking movement to be able to follow dynamic objects. */
	virtual void SetBase(UPrimitiveComponent* NewBase, const FName BoneName = NAME_None, bool bNotifyActor=true);

protected:
	/** Info about our current movement base (object we are standing on). */
	UPROPERTY()
	struct FRlBasedMovementInfo BasedMovement;

protected:
	/** Saved translation offset of mesh. */
	UPROPERTY()
	FVector BaseTranslationOffset;

	/** Saved rotation offset of mesh. */
	UPROPERTY()
	FQuat BaseRotationOffset;

	/** Event called after actor's base changes (if SetBase was requested to notify us with bNotifyPawn). */
	virtual void BaseChange();

public:

	/** Accessor for BasedMovement */
	FORCEINLINE const FRlBasedMovementInfo& GetBasedMovement() const { return BasedMovement; }

	/** Save a new relative location in BasedMovement and a new rotation with is either relative or absolute. */
	void SaveRelativeBasedMovement(const FVector& NewRelativeLocation, const FRotator& NewRotation, bool bRelativeRotation);

	/** Get the saved translation offset of mesh. This is how much extra offset is applied from the center of the capsule. */
	UFUNCTION(BlueprintCallable, Category=Character)
	FVector GetBaseTranslationOffset() const { return BaseTranslationOffset; }

	/** Get the saved rotation offset of mesh. This is how much extra rotation is applied from the capsule rotation. */
	virtual FQuat GetBaseRotationOffset() const { return BaseRotationOffset; }

	/** Get the saved rotation offset of mesh. This is how much extra rotation is applied from the capsule rotation. */
	UFUNCTION(BlueprintCallable, Category=Character, meta=(DisplayName="GetBaseRotationOffset", ScriptName="GetBaseRotationOffset"))
	FRotator GetBaseRotationOffsetRotator() const { return GetBaseRotationOffset().Rotator(); }

	/** When true, player wants to jump */
	uint32 bWantJump:1;

	uint32 bIsPressingJump:1;

	uint32 bStoredJump : 1;

	uint32 bWantWallWalk : 1;

	uint32 bWallWalkToggle : 1;

	UPROPERTY(EditAnywhere, Category = Character)
	uint32 bRunEnabled : 1;

	UPROPERTY(EditAnywhere, Category = Character)
	uint32 bJumpEnabled : 1;

	UPROPERTY(EditAnywhere, Category = Character)
	uint32 bLongJumpEnabled : 1;

	UPROPERTY(EditAnywhere, Category = Character)
	uint32 bWallWalkEnabled : 1;

	/** Disable simulated gravity (set when character encroaches geometry on client, to keep him from falling through floors) */
	UPROPERTY()
	uint32 bSimGravityDisabled:1;

	/** Tracks whether or not the character was already jumping last frame. */
	UPROPERTY(VisibleInstanceOnly, Category=Character)
	uint32 bWasJumping : 1;

	uint32 bWasWallWalking : 1;

	uint32 bIsSprinting : 1;

	/** 
	 * Jump key Held Time.
	 * This is the time that the player has held the jump key, in seconds.
	 */
	UPROPERTY(VisibleInstanceOnly, Category=Character)
	float JumpKeyHoldTime;

	/** Amount of jump force time remaining, if JumpMaxHoldTime > 0. */
	UPROPERTY(VisibleInstanceOnly, Category=Character)
	float JumpForceTimeRemaining;

	/** 
	 * The max time the jump key can be held.
	 * Note that if StopJumping() is not called before the max jump hold time is reached,
	 * then the character will carry on receiving vertical velocity. Therefore it is usually 
	 * best to call StopJumping() when jump input has ceased (such as a button up event).
	 */
	UPROPERTY(EditAnywhere, Category=Character, Meta=(ClampMin=0.0, UIMin=0.0))
	float JumpMaxHoldTime;

	UPROPERTY(EditAnywhere, Category=Character, Meta=(ClampMin=0.0, UIMin=0.0))
	float JumpHoldForceFactor;

    /**
     * The max number of jumps the character can perform.
     * Note that if JumpMaxHoldTime is non zero and StopJumping is not called, the player
     * may be able to perform and unlimited number of jumps. Therefore it is usually
     * best to call StopJumping() when jump input has ceased (such as a button up event).
     */
    UPROPERTY(EditAnywhere, Category=Character)
    int32 JumpMaxCount;

    /**
     * Tracks the current number of jumps performed.
     * This is incremented in CheckJumpInput, used in CanJump_Implementation, and reset in OnMovementModeChanged.
     * When providing overrides for these methods, it's recommended to either manually
     * increment / reset this value, or call the Super:: method.
     */
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=Character)
    int32 JumpCurrentCount;

	float WallWalkHoldTime;

	UPROPERTY(EditAnywhere, Category = Character, Meta = (ClampMin = 0.0, UIMin = 0.0))
	float WallWalkMaxHoldTime;

	/** Incremented every time there is an Actor overlap event (start or stop) on this actor. */
	uint32 NumActorOverlapEventsCounter;

	//~ Begin AActor Interface.
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ClearCrossLevelReferences() override;
	//virtual void PreNetReceive() override;
	//virtual void PostNetReceive() override;
	//virtual void OnRep_ReplicatedMovement() override;
	//virtual void PostNetReceiveLocationAndRotation() override;
	//virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const override;
	//virtual UActorComponent* FindComponentByClass(const TSubclassOf<UActorComponent> ComponentClass) const override;
	//virtual void TornOff() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor);
	virtual void NotifyActorEndOverlap(AActor* OtherActor);
	//~ End AActor Interface

	//~ Begin INavAgentInterface Interface
	//virtual FVector GetNavAgentLocation() const override;
	//~ End INavAgentInterface Interface

	//~ Begin APawn Interface.
	virtual void PostInitializeComponents() override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;
	virtual UPrimitiveComponent* GetMovementBase() const override final { return BasedMovement.MovementBase; }
	virtual float GetDefaultHalfHeight() const override;
	virtual void TurnOff() override;
	virtual void Restart() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	//~ End APawn Interface

	/** Apply momentum caused by damage. */
	virtual void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser);

	// This only adds vertical velocity
	/** 
	 * Make the character jump on the next update.	 
	 * If you want your character to jump according to the time that the jump key is held,
	 * then you can set JumpKeyHoldTime to some non-zero value. Make sure in this case to
	 * call StopJumping() when you want the jump's z-velocity to stop being applied (such 
	 * as on a button up event), otherwise the character will carry on receiving the 
	 * velocity until JumpKeyHoldTime is reached.
	 */
	virtual void Jump(FKey Key);

	/** 
	 * Stop the character from jumping on the next update. 
	 * Call this from an input event (such as a button 'up' event) to cease applying
	 * jump Z-velocity. If this is not called, then jump z-velocity will be applied
	 * until JumpMaxHoldTime is reached.
	 */
	virtual void StopJumping();

	/**
	 * Check if the character can jump in the current state.
	 *
	 * The default implementation may be overridden or extended by implementing the custom CanJump event in Blueprints. 
	 * 
	 * @Return Whether the character can jump in the current state. 
	 */
	bool CanJump() const;

	bool CanWallWalk() const;

public:

	/** Marks character as not trying to jump */
	virtual void ResetJumpState();

	/**
	 * True if jump is actively providing a force, such as when the jump key is held and the time it has been held is less than JumpMaxHoldTime.
	 * @see CharacterMovement->IsFalling
	 */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual bool IsJumpProvidingForce() const;

	/**
	 * Set a pending launch velocity on the Character. This velocity will be processed on the next CharacterMovementComponent tick,
	 * and will set it to the "falling" state. Triggers the OnLaunched event.
	 * @PARAM LaunchVelocity is the velocity to impart to the Character
	 * @PARAM bXYOverride if true replace the XY part of the Character's velocity instead of adding to it.
	 * @PARAM bZOverride if true replace the Z component of the Character's velocity instead of adding to it.
	 */
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride);

	/**
	 * Called when pawn's movement is blocked
	 * @param Impact describes the blocking hit.
	 */
	virtual void MoveBlockedBy(const FHitResult& Impact) {};

	/** Trigger jump if jump button has been pressed. */
	virtual void CheckJumpInput(float DeltaTime);

	/** Update jump input state after having checked input. */
	virtual void ClearJumpInput(float DeltaTime);

	/**
	 * Get the maximum jump time for the character.
	 * Note that if StopJumping() is not called before the max jump hold time is reached,
	 * then the character will carry on receiving vertical velocity. Therefore it is usually 
	 * best to call StopJumping() when jump input has ceased (such as a button up event).
	 * 
	 * @return Maximum jump time for the character
	 */
	virtual float GetJumpMaxHoldTime() const;
};
