// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
//#include "GameFramework/CharacterMovementComponent.h" // FRlFindFloorResult
#include "RlCharacterMovementComponent.generated.h"

class ARlCharacter;
class USceneComponent;

/** Movement modes for RlCharacters. */
UENUM(BlueprintType)
enum class ERlMovementMode : uint8
{
	/** None (movement is disabled). */
	None		UMETA(DisplayName = "None"),

	/** Walking on a surface. */
	Walking		UMETA(DisplayName = "Walking"),

	/** Falling under the effects of gravity, such as after jumping or walking off the edge of a surface. */
	Falling		UMETA(DisplayName = "Falling"),

	/** Walking across the background wall. */
	WallWalking	UMETA(DisplayName = "WallWalking"),

	MAX			UMETA(Hidden),
};

/** Data about the floor for walking movement, used by CharacterMovementComponent. */
USTRUCT(BlueprintType)
struct RAGELITE_API FRlFindFloorResult
{
	GENERATED_USTRUCT_BODY()

	/**
	* True if there was a blocking hit in the floor test that was NOT in initial penetration.
	* The HitResult can give more info about other circumstances.
	*/
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	uint32 bBlockingHit:1;

	/** True if the hit found a valid walkable floor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	uint32 bWalkableFloor:1;

	/** True if the hit found a valid walkable floor using a line trace (rather than a sweep test, which happens when the sweep test fails to yield a walkable surface). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	uint32 bLineTrace:1;

	/** The distance to the floor, computed from the swept box trace. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	float FloorDist;
	
	/** The distance to the floor, computed from the trace. Only valid if bLineTrace is true. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	float LineDist;

	/** Hit result of the test that found a floor. Includes more specific data about the point of impact and surface normal at that point. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	FHitResult HitResult;

public:

	FRlFindFloorResult()
		: bBlockingHit(false)
		, bWalkableFloor(false)
		, bLineTrace(false)
		, FloorDist(0.f)
		, LineDist(0.f)
		, HitResult(1.f)
	{
	}

	/** Returns true if the floor result hit a walkable surface. */
	bool IsWalkableFloor() const
	{
		return bBlockingHit && bWalkableFloor;
	}

	void Clear()
	{
		bBlockingHit = false;
		bWalkableFloor = false;
		bLineTrace = false;
		FloorDist = 0.f;
		LineDist = 0.f;
		HitResult.Reset(1.f, false);
	}

	/** Gets the distance to floor, either LineDist or FloorDist. */
	float GetDistanceToFloor() const
	{
		// When the floor distance is set using SetFromSweep, the LineDist value will be reset.
		// However, when SetLineFromTrace is used, there's no guarantee that FloorDist is set.
		return bLineTrace ? LineDist : FloorDist;
	}

	void SetFromSweep(const FHitResult& InHit, const float InSweepFloorDist, const bool bIsWalkableFloor);
	void SetFromLineTrace(const FHitResult& InHit, const float InSweepFloorDist, const float InLineDist, const bool bIsWalkableFloor);
};


/**
 *
 */
UCLASS()
class RAGELITE_API URlCharacterMovementComponent : public UPawnMovementComponent
{
	GENERATED_UCLASS_BODY()

protected:

	/** Character movement component belongs to */
	UPROPERTY()
	ARlCharacter* CharacterOwner;

public:

	/** Get the Character that owns UpdatedComponent. */
	ARlCharacter* GetCharacterOwner() const;

	/**
	 * Actor's current movement mode (walking, falling, etc).
	 *    - walking:  Walking on a surface, under the effects of friction, and able to "step up" barriers. Vertical velocity is zero.
	 *    - falling:  Falling under the effects of gravity, after jumping or walking off the edge of a surface.
	 *    - 
	 */
	UPROPERTY(Category = "Character Movement: MovementMode", VisibleAnywhere)
	ERlMovementMode MovementMode;

	/**
	 * Change movement mode.
	 *
	 * @param NewMovementMode	The new movement mode
	 */
	virtual void SetMovementMode(ERlMovementMode NewMovementMode);







	// Character Movement: General Settings

	/** Max Acceleration (rate of change of velocity) */
	UPROPERTY(Category = "Character Movement: General Settings", EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
	float MaxAcceleration;

	/**
	* Factor used to multiply actual value of friction used when braking.
	* This applies to any friction value that is currently used, which may depend on bUseSeparateBrakingFriction.
	* @note This is 2 by default for historical reasons, a value of 1 gives the true drag equation.
	* @see bUseSeparateBrakingFriction, GroundFriction, BrakingFriction
	*/
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, meta=(ClampMin="0", UIMin="0"))
	float BrakingFrictionFactor;

	/**
	 * Friction (drag) coefficient applied when braking (whenever Acceleration = 0, or if character is exceeding max speed); actual value used is this multiplied by BrakingFrictionFactor.
	 * When braking, this property allows you to control how much friction is applied when moving across the ground, applying an opposing force that scales with current velocity.
	 * Braking is composed of friction (velocity-dependent drag) and constant deceleration.
	 * This is the current value, used in all movement modes; if this is not desired, override it or bUseSeparateBrakingFriction when movement mode changes.
	 * @note Only used if bUseSeparateBrakingFriction setting is true, otherwise current friction such as GroundFriction is used.
	 * @see bUseSeparateBrakingFriction, BrakingFrictionFactor, GroundFriction, BrakingDecelerationWalking
	 */
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, meta=(ClampMin="0", UIMin="0", EditCondition="bUseSeparateBrakingFriction"))
	float BrakingFriction;

	/**
	 * Time substepping when applying braking friction. Smaller time steps increase accuracy at the slight cost of performance, especially if there are large frame times.
	 */
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, AdvancedDisplay, meta=(ClampMin="0.0166", ClampMax="0.05", UIMin="0.0166", UIMax="0.05"))
	float BrakingSubStepTime;

	/**
	* If true, high-level movement updates will be wrapped in a movement scope that accumulates updates and defers a bulk of the work until the end.
	* When enabled, touch and hit events will not be triggered until the end of multiple moves within an update, which can improve performance.
	*
	* @see FScopedMovementUpdate
	*/
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, AdvancedDisplay)
	uint8 bEnableScopedMovementUpdates:1;


	/**
	 * If true, movement will be performed even if there is no Controller for the Character owner.
	 * Normally without a Controller, movement will be aborted and velocity and acceleration are zeroed if the character is walking.
	 * Characters that are spawned without a Controller but with this flag enabled will initialize the movement mode to DefaultLandMovementMode or DefaultWaterMovementMode appropriately.
	 * @see DefaultLandMovementMode, DefaultWaterMovementMode
	 */
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, AdvancedDisplay)
	uint8 bRunPhysicsWithNoController:1;


	/**
	* If true, BrakingFriction will be used to slow the character to a stop (when there is no Acceleration).
	* If false, braking uses the same friction passed to CalcVelocity() (ie GroundFriction when walking), multiplied by BrakingFrictionFactor.
	* This setting applies to all movement modes; if only desired in certain modes, consider toggling it when movement modes change.
	* @see BrakingFriction
	*/
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere)
	uint8 bUseSeparateBrakingFriction:1;

	/** Mass of pawn (for when momentum is imparted to it). */
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float Mass;


	/**
	 * Default movement mode when not in water. Used at player startup or when teleported.
	 * @see DefaultWaterMovementMode
	 * @see bRunPhysicsWithNoController
	 */
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere)
	ERlMovementMode DefaultLandMovementMode;


private:
	/**
	 * Ground movement mode to switch to after falling and resuming ground movement.
	 * Only allowed values are: MOVE_Walking, MOVE_NavWalking.
	 * @see SetGroundMovementMode(), GetGroundMovementMode()
	 */
	UPROPERTY()
	ERlMovementMode GroundMovementMode;

public:

	/**
	 * Get current GroundMovementMode value.
	 * @return current GroundMovementMode
	 * @see GroundMovementMode, SetGroundMovementMode()
	 */
	 ERlMovementMode GetGroundMovementMode() const { return GroundMovementMode; }




	/** Used by movement code to determine if a change in position is based on normal movement or a teleport. If not a teleport, velocity can be recomputed based on the change in position. */
	UPROPERTY(Category="Character Movement: General Settings", Transient, VisibleInstanceOnly, BlueprintReadWrite)
	uint8 bJustTeleported:1;


	/**
	* Whether the character ignores changes in rotation of the base it is standing on.
	* If true, the character maintains current world rotation.
	* If false, the character rotates with the moving base.
	*/
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite)
	uint8 bIgnoreBaseRotation:1;












	// Character Movement: Walking

	/**
	 * Setting that affects movement control. Higher values allow faster changes in direction.
	 * If bUseSeparateBrakingFriction is false, also affects the ability to stop more quickly when braking (whenever Acceleration is zero), where it is multiplied by BrakingFrictionFactor.
	 * When braking, this property allows you to control how much friction is applied when moving across the ground, applying an opposing force that scales with current velocity.
	 * This can be used to simulate slippery surfaces such as ice or oil by changing the value (possibly based on the material pawn is standing on).
	 * @see BrakingDecelerationWalking, BrakingFriction, bUseSeparateBrakingFriction, BrakingFrictionFactor
	 */
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
	float GroundFriction;


	/** Saved location of object we are standing on, for UpdateBasedMovement() to determine if base moved in the last frame, and therefore pawn needs an update. */
	FQuat OldBaseQuat;

	/** Saved location of object we are standing on, for UpdateBasedMovement() to determine if base moved in the last frame, and therefore pawn needs an update. */
	FVector OldBaseLocation;

	/** The maximum ground speed when walking. Also determines maximum lateral speed when falling. */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, meta=(ClampMin="0", UIMin="0"))
	float MaxWalkSpeed;

	/** The ground speed that we should accelerate up to when walking at minimum analog stick tilt */
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, meta = (ClampMin = "0", UIMin = "0"))
	float MinAnalogWalkSpeed;

	/**
	* Deceleration when walking and not applying acceleration. This is a constant opposing force that directly lowers velocity by a constant value.
	* @see GroundFriction, MaxAcceleration
	*/
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationWalking;

	/**
	* Whether or not the character should sweep for collision geometry while walking.
	* @see USceneComponent::MoveComponent.
	*/
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere)
	uint8 bSweepWhileNavWalking:1;

	/**
	* Force the Character in MOVE_Walking to do a check for a valid floor even if he hasn't moved. Cleared after next floor check.
	* Normally if bAlwaysCheckFloor is false we try to avoid the floor check unless some conditions are met, but this can be used to force the next check to always run.
	*/
	UPROPERTY(Category="Character Movement: Walking", VisibleInstanceOnly, AdvancedDisplay)
	uint8 bForceNextFloorCheck:1;



	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere)
	float NormalMaxAcceleration;

	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere)
	float NormalMaxWalkSpeed;

	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere)
	float SprintMaxAcceleration;

	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere)
	float SprintMaxWalkSpeed;

	void SprintStart();

	void SprintStop();

	bool bSprintStop;

	float LastSpeed;



	// Floor

	/** Information about the floor the Character is standing on (updated only during walking movement). */
	UPROPERTY(Category="Character Movement: Walking", VisibleInstanceOnly, BlueprintReadOnly)
	FRlFindFloorResult CurrentFloor;


	bool CheckSpikes();













	// Character Movement: Jumping / Falling

	/** Custom gravity scale. Gravity is multiplied by this amount for the character. */
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere)
	float GravityScale;

	/** Initial velocity (instantaneous vertical acceleration) when jumping. */
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, meta=(ClampMin="0", UIMin="0"))
	float JumpZVelocity;

	/** Fraction of JumpZVelocity to use when automatically "jumping off" of a base actor that's not allowed to be a base for a character. (For example, if you're not allowed to stand on other players.) */
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, AdvancedDisplay, meta=(ClampMin="0", UIMin="0"))
	float JumpOffJumpZFactor;

	/**
	* Lateral deceleration when falling and not applying acceleration.
	* @see MaxAcceleration
	*/
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationFalling;

	/**
	* Friction to apply to lateral air movement when falling.
	* If bUseSeparateBrakingFriction is false, also affects the ability to stop more quickly when braking (whenever Acceleration is zero).
	* @see BrakingFriction, bUseSeparateBrakingFriction
	*/
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, meta=(ClampMin="0", UIMin="0"))
	float FallingLateralFriction;

	/** If true, impart the base actor's X velocity when falling off it (which includes jumping) */
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere)
	uint8 bImpartBaseVelocityX:1;

	/** If true, impart the base actor's Y velocity when falling off it (which includes jumping) */
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere)
	uint8 bImpartBaseVelocityY:1;

	/** If true, impart the base actor's Z velocity when falling off it (which includes jumping) */
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere)
	uint8 bImpartBaseVelocityZ:1;

	/**
	 * If true, impart the base component's tangential components of angular velocity when jumping or falling off it.
	 * Only those components of the velocity allowed by the separate component settings (bImpartBaseVelocityX etc) will be applied.
	 * @see bImpartBaseVelocityX, bImpartBaseVelocityY, bImpartBaseVelocityZ
	 */
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere)
	uint8 bImpartBaseAngularVelocity:1;

	UPROPERTY(Category = "Character Movement: Jumping / Falling", EditAnywhere)
	float TerminalVelocity;

	UPROPERTY(Category = "Character Movement: Jumping / Falling", EditAnywhere)
	bool bCustomTerminalVelocity;




protected:

	/**
	 * True during movement update.
	 * Used internally so that attempts to change CharacterOwner and UpdatedComponent are deferred until after an update.
	 * @see IsMovementInProgress()
	 */
	UPROPERTY()
	uint8 bMovementInProgress:1;





public:

	

	/** Ignores size of acceleration component, and forces max acceleration to drive character at full velocity. */
	UPROPERTY()
	uint8 bForceMaxAccel:1;    




public:

	/** What to update CharacterOwner and UpdatedComponent after movement ends */
	UPROPERTY()
	USceneComponent* DeferredUpdatedMoveComponent;




	

protected:

	/**
	 * Current acceleration vector (with magnitude).
	 * This is calculated each update based on the input vector and the constraints of MaxAcceleration and the current movement mode.
	 */
	UPROPERTY()
	FVector Acceleration;

	/**
	 * Rotation after last PerformMovement or SimulateMovement update.
	 */
	UPROPERTY()
	FQuat LastUpdateRotation;

	/**
	 * Location after last PerformMovement or SimulateMovement update. Used internally to detect changes in position from outside character movement to try to validate the current floor.
	 */
	UPROPERTY()
	FVector LastUpdateLocation;

	/**
	 * Velocity after last PerformMovement or SimulateMovement update. Used internally to detect changes in velocity from external sources.
	 */
	UPROPERTY()
	FVector LastUpdateVelocity;

	/** Accumulated impulse to be added next tick. */
	UPROPERTY()
	FVector PendingImpulseToApply;

	/** Accumulated force to be added next tick. */
	UPROPERTY()
	FVector PendingForceToApply;

	/**
	 * Modifier to applied to values such as acceleration and max speed due to analog input.
	 */
	UPROPERTY()
	float AnalogInputModifier;

	/** Computes the analog input modifier based on current input vector and/or acceleration. */
	virtual float ComputeAnalogInputModifier() const;



public:

	/** Returns the location at the end of the last tick. */
	FVector GetLastUpdateLocation() const { return LastUpdateLocation; }

	/** Returns the rotation at the end of the last tick. */
	FRotator GetLastUpdateRotation() const { return LastUpdateRotation.Rotator(); }

	/** Returns the rotation Quat at the end of the last tick. */
	FQuat GetLastUpdateQuat() const { return LastUpdateRotation; }

	/** Returns the velocity at the end of the last tick. */
	FVector GetLastUpdateVelocity() const { return LastUpdateVelocity; }



	/**
	 * Compute remaining time step given remaining time and current iterations.
	 * The last iteration (limited by MaxSimulationIterations) always returns the remaining time, which may violate MaxSimulationTimeStep.
	 *
	 * @param RemainingTime		Remaining time in the tick.
	 * @param Iterations		Current iteration of the tick (starting at 1).
	 * @return The remaining time step to use for the next sub-step of iteration.
	 * @see MaxSimulationTimeStep, MaxSimulationIterations
	 */
	float GetSimulationTimeStep(float RemainingTime, int32 Iterations) const;

	/**
	 * Max time delta for each discrete simulation step.
	 * Used primarily in the the more advanced movement modes that break up larger time steps (usually those applying gravity such as falling and walking).
	 * Lowering this value can address issues with fast-moving objects or complex collision scenarios, at the cost of performance.
	 *
	 * WARNING: if (MaxSimulationTimeStep * MaxSimulationIterations) is too low for the min framerate, the last simulation step may exceed MaxSimulationTimeStep to complete the simulation.
	 * @see MaxSimulationIterations
	 */
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, BlueprintReadWrite, AdvancedDisplay, meta=(ClampMin="0.0166", ClampMax="0.50", UIMin="0.0166", UIMax="0.50"))
	float MaxSimulationTimeStep;

	/**
	 * Max number of iterations used for each discrete simulation step.
	 * Used primarily in the the more advanced movement modes that break up larger time steps (usually those applying gravity such as falling and walking).
	 * Increasing this value can address issues with fast-moving objects or complex collision scenarios, at the cost of performance.
	 *
	 * WARNING: if (MaxSimulationTimeStep * MaxSimulationIterations) is too low for the min framerate, the last simulation step may exceed MaxSimulationTimeStep to complete the simulation.
	 * @see MaxSimulationTimeStep
	 */
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, BlueprintReadWrite, AdvancedDisplay, meta=(ClampMin="1", ClampMax="25", UIMin="1", UIMax="25"))
	int32 MaxSimulationIterations;







	/**
	* Max distance we allow simulated proxies to depenetrate when moving out of anything but Pawns.
	* This is generally more tolerant than with Pawns, because other geometry is either not moving, or is moving predictably with a bit of delay compared to on the server.
	* @see MaxDepenetrationWithGeometryAsProxy, MaxDepenetrationWithPawn, MaxDepenetrationWithPawnAsProxy
	*/
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, BlueprintReadWrite, AdvancedDisplay, meta=(ClampMin="0", UIMin="0"))
	float MaxDepenetrationWithGeometry;

	/**
	* Max distance we are allowed to depenetrate when moving out of other Pawns.
	* @see MaxDepenetrationWithGeometry, MaxDepenetrationWithGeometryAsProxy, MaxDepenetrationWithPawnAsProxy
	*/
	UPROPERTY(Category="Character Movement: General Settings", EditAnywhere, BlueprintReadWrite, AdvancedDisplay, meta=(ClampMin="0", UIMin="0"))
	float MaxDepenetrationWithPawn;





public:



	UPROPERTY(Category="Character Movement: Physics Interaction", EditAnywhere)
	uint8 bEnablePhysicsInteraction:1;



	/** 
	 * Set this to true if riding on a moving base that you know is clear from non-moving world obstructions.
	 * Optimization to avoid sweeps during based movement, use with care.
	 */
	UPROPERTY()
	uint8 bFastAttachedMove:1;

	/**
	 * Whether we always force floor checks for stationary Characters while walking.
	 * Normally floor checks are avoided if possible when not moving, but this can be used to force them if there are use-cases where they are being skipped erroneously
	 * (such as objects moving up into the character from below).
	 */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	uint8 bAlwaysCheckFloor:1;

	///**
	// * Performs floor checks as if the character is using a shape with a flat base.
	// * This avoids the situation where characters slowly lower off the side of a ledge (as their capsule 'balances' on the edge).
	// */
	//UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	//uint8 bUseFlatBaseForFloorChecks:1;

	/** Used to prevent reentry of JumpOff() */
	UPROPERTY()
	uint8 bPerformingJumpOff:1;

	/** Temporarily holds launch velocity when pawn is to be launched so it happens at end of movement. */
	UPROPERTY()
	FVector PendingLaunchVelocity;


protected:

	/** Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner. */
	virtual void OnMovementModeChanged(ERlMovementMode PreviousMovementMode);

public:

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void PostLoad() override;
	virtual void Deactivate() override;
	//End UActorComponent Interface

	//BEGIN UMovementComponent Interface
	virtual float GetMaxSpeed() const override;
	virtual void StopActiveMovement() override;
	virtual bool IsFalling() const override;
	virtual bool IsWallWalking() const;
	virtual bool IsMovingOnGround() const override;
	virtual float GetGravityZ() const override;
	virtual void AddRadialForce(const FVector& Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff) override;
	virtual void AddRadialImpulse(const FVector& Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bVelChange) override;
	//END UMovementComponent Interface

	/** Returns true if the character is in the 'Walking' movement mode. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	bool IsWalking() const;

	/**
	 * Returns true if currently performing a movement update.
	 * @see bMovementInProgress
	 */
	bool IsMovementInProgress() const { return bMovementInProgress; }

	/** Make movement impossible (sets movement mode to MOVE_None). */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	virtual void DisableMovement();

	/** Return true if we have a valid CharacterOwner and UpdatedComponent. */
	virtual bool HasValidData() const;

	/** Transition from walking to falling */
	virtual void StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc);

	/** Adjust distance from floor, trying to maintain a slight offset from the floor when walking (based on CurrentFloor). */
	virtual void AdjustFloorHeight();




	/** Return PrimitiveComponent we are based on (standing and walking on). */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	UPrimitiveComponent* GetMovementBase() const;



	/** true to update CharacterOwner and UpdatedComponent after movement ends */
	UPROPERTY()
	uint8 bDeferUpdateMoveComponent:1;

	/** Update controller's view rotation as pawn's base rotates */
	virtual void UpdateBasedRotation(FRotator& FinalRotation, const FRotator& ReducedRotation);


	/** Update OldBaseLocation and OldBaseQuat if there is a valid movement base, and store the relative location/rotation if necessary. Ignores bDeferUpdateBasedMovement and forces the update. */
	virtual void SaveBaseLocation();

	/** changes physics based on MovementMode */
	virtual void StartNewPhysics(float DeltaTime, int32 Iterations);
	
	/**
	 * Perform jump. Called by Character when a jump has been detected because Character->bPressedJump was true. Checks CanJump().
	 * Note that you should usually trigger a jump through Character::Jump() instead.
	 * @return	True if the jump was triggered successfully.
	 */
	virtual bool DoJump();

	virtual bool DoWallWalk();

	/** Queue a pending launch with velocity LaunchVel. */
	virtual void Launch(FVector const& LaunchVel);

	/** Handle a pending launch during an update. Returns true if the launch was triggered. */
	virtual bool HandlePendingLaunch();

	/**
	 * If we have a movement base, get the velocity that should be imparted by that base, usually when jumping off of it.
	 * Only applies the components of the velocity enabled by bImpartBaseVelocityX, bImpartBaseVelocityY, bImpartBaseVelocityZ.
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	virtual FVector GetImpartedMovementBaseVelocity() const;

	/** Force this pawn to bounce off its current base, which isn't an acceptable base for it. */
	virtual void JumpOff(AActor* MovementBaseActor);

	/** Can be overridden to choose to jump based on character velocity, base actor dimensions, etc. */
	virtual FVector GetBestDirectionOffActor(AActor* BaseActor) const; // Calculates the best direction to go to "jump off" an actor.






	/** 
	 * Updates Velocity and Acceleration based on the current state, applying the effects of friction and acceleration or deceleration. Does not apply gravity.
	 * This is used internally during movement updates. Normally you don't need to call this from outside code, but you might want to use it for custom movement modes.
	 *
	 * @param	DeltaTime						time elapsed since last frame.
	 * @param	Friction						coefficient of friction when not accelerating, or in the direction opposite acceleration.
	 * @param	BrakingDeceleration				deceleration applied when not accelerating, or when exceeding max velocity.
	 */
	virtual void CalcVelocity(float DeltaTime, float Friction, float BrakingDeceleration);
	
	/**
	 *	Compute the max jump height based on the JumpZVelocity velocity and gravity.
	 *	This does not take into account the CharacterOwner's MaxJumpHoldTime.
	 */
	virtual float GetMaxJumpHeight() const;

	/**
	 *	Compute the max jump height based on the JumpZVelocity velocity and gravity.
	 *	This does take into account the CharacterOwner's MaxJumpHoldTime.
	 */
	virtual float GetMaxJumpHeightWithJumpTime() const;

	/** Returns maximum acceleration for the current state. */
	virtual float GetMinAnalogSpeed() const;

	/** Returns maximum acceleration for the current state. */
	virtual float GetMaxAcceleration() const;

	/** Returns maximum deceleration for the current state when braking (ie when there is no acceleration). */
	virtual float GetMaxBrakingDeceleration() const;

	/** Returns current acceleration, computed from input vector each update. */
	FVector GetCurrentAcceleration() const;

	/** Returns modifier [0..1] based on the magnitude of the last input vector, which is used to modify the acceleration and max speed during movement. */
	float GetAnalogInputModifier() const;
	



	/** Update the base of the character, which is the PrimitiveComponent we are standing on. */
	virtual void SetBase(UPrimitiveComponent* NewBase, const FName BoneName = NAME_None, bool bNotifyActor=true);

	/**
	 * Update the base of the character, using the given floor result if it is walkable, or null if not. Calls SetBase().
	 */
	void SetBaseFromFloor(const FRlFindFloorResult& FloorResult);


	
	/** Applies momentum accumulated through AddImpulse() and AddForce(), then clears those forces. Does *not* use ClearAccumulatedForces() since that would clear pending launch velocity as well. */
	virtual void ApplyAccumulatedForces(float DeltaSeconds);

	/** Clears forces accumulated through AddImpulse() and AddForce(), and also pending launch velocity. */
	virtual void ClearAccumulatedForces();




	/** Handle falling movement. */
	virtual void PhysFalling(float DeltaTime, int32 Iterations);

protected:

	/** Handle landing against Hit surface over remaingTime and iterations, calling SetPostLandedPhysics() and starting the new movement mode. */
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations);








	/** Handle wall walking movement. */
	virtual void PhysWallWalking(float DeltaTime, int32 Iterations);
	



public:

	/** Called by owning Character upon successful teleport from AActor::TeleportTo(). */
	virtual void OnTeleported() override;


	/** Check if pawn is falling */
	virtual bool CheckFall(const FRlFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float remainingTime, float timeTick, int32 Iterations);
	
	/** 
	 *  Revert to previous position OldLocation, return to being based on OldBase.
	 *  if bFailMove, stop movement and notify controller
	 */	
	void RevertMove(const FVector& OldLocation, UPrimitiveComponent* OldBase, const FVector& InOldBaseLocation, const FRlFindFloorResult& OldFloor, bool bFailMove);

	/** Set movement mode to the default based on the current physics volume. */
	virtual void SetDefaultMovementMode();
	
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
	
	/** Returns MovementMode string */
	virtual FString GetMovementName() const;

	/** 
	 * Add impulse to character. Impulses are accumulated each tick and applied together
	 * so multiple calls to this function will accumulate.
	 * An impulse is an instantaneous force, usually applied once. If you want to continually apply
	 * forces each frame, use AddForce().
	 * Note that changing the momentum of characters like this can change the movement mode.
	 * 
	 * @param	Impulse				Impulse to apply.
	 * @param	bVelocityChange		Whether or not the impulse is relative to mass.
	 */
	virtual void AddImpulse( FVector Impulse, bool bVelocityChange = false );

	/** 
	 * Add force to character. Forces are accumulated each tick and applied together
	 * so multiple calls to this function will accumulate.
	 * Forces are scaled depending on timestep, so they can be applied each frame. If you want an
	 * instantaneous force, use AddImpulse.
	 * Adding a force always takes the actor's mass into account.
	 * Note that changing the momentum of characters like this can change the movement mode.
	 * 
	 * @param	Force			Force to apply.
	 */
	virtual void AddForce( FVector Force );





	/** Return true if the hit result should be considered a walkable surface for the character. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	virtual bool IsWalkable(const FHitResult& Hit) const;


protected:
	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysWalking(float DeltaTime, int32 Iterations);

	/**
	 * Move along the floor, using CurrentFloor and ComputeGroundMovementDelta() to get a movement direction.
	 * If a second walkable surface is hit, it will also be moved along using the same approach.
	 *
	 * @param InVelocity:			Velocity of movement
	 * @param DeltaSeconds:			Time over which movement occurs
	 */
	virtual void MoveHorizontal(const FVector& InVelocity, float DeltaSeconds, bool bAlongFloor = true);

	/** Notification that the character is stuck in geometry.  Only called during walking movement. */
	virtual void OnCharacterStuckInGeometry(const FHitResult* Hit);

	/**
	 * Adjusts velocity when walking so that Z velocity is zero.
	 * When bMaintainHorizontalGroundVelocity is false, also rescales the velocity vector to maintain the original magnitude, but in the horizontal direction.
	 */
	virtual void MaintainHorizontalGroundVelocity();



	/** Overridden to enforce max distances based on hit geometry. */
	virtual FVector GetPenetrationAdjustment(const FHitResult& Hit) const override;

	/** Overridden to set bJustTeleported to true, so we don't make incorrect velocity calculations based on adjusted movement. */
	virtual bool ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation) override;

	/** Custom version of SlideAlongSurface that handles different movement modes separately; namely during walking physics we might not want to slide up slopes. */
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact) override;


	/**
	 * Calculate slide vector along a surface.
	 * Has special treatment when falling, to avoid boosting up slopes (calling HandleSlopeBoosting() in this case).
	 *
	 * @param Delta:	Attempted move.
	 * @param Time:		Amount of move to apply (between 0 and 1).
	 * @param Normal:	Normal opposed to movement. Not necessarily equal to Hit.Normal (but usually is).
	 * @param Hit:		HitResult of the move that resulted in the slide.
	 * @return			New deflected vector of movement.
	 */
	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const override;

	/** 
	 * Limit the slide vector when falling if the resulting slide might boost the character faster upwards.
	 * @param SlideResult:	Vector of movement for the slide (usually the result of ComputeSlideVector)
	 * @param Delta:		Original attempted move
	 * @param Time:			Amount of move to apply (between 0 and 1).
	 * @param Normal:		Normal opposed to movement. Not necessarily equal to Hit.Normal (but usually is).
	 * @param Hit:			HitResult of the move that resulted in the slide.
	 * @return:				New slide result.
	 */
	virtual FVector HandleSlopeBoosting(const FVector& SlideResult, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const;

	/** Slows towards stop. */
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration);









public:

	/**
	 * Return true if the 2D distance to the impact point is inside the edge tolerance (CapsuleRadius minus a small rejection threshold).
	 * Useful for rejecting adjacent hits when finding a floor or landing spot.
	 */
	virtual bool IsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const;





	/**
	 * Sweeps a vertical trace to find the floor for the capsule at the given location. Will attempt to perch if ShouldComputePerchResult() returns true for the downward sweep result.
	 * No floor will be found if collision is disabled on the capsule!
	 *
	 * @param CapsuleLocation		Location where the capsule sweep should originate
	 * @param OutFloorResult		[Out] Contains the result of the floor check. The HitResult will contain the valid sweep or line test upon success, or the result of the sweep upon failure.
	 * @param bCanUseCachedLocation If true, may use a cached value (can be used to avoid unnecessary floor tests, if for example the capsule was not moving since the last test).
	 * @param DownwardSweepResult	If non-null and it contains valid blocking hit info, this will be used as the result of a downward sweep test instead of doing it as part of the update.
	 */
	virtual void FindFloor(const FVector& CapsuleLocation, FRlFindFloorResult& OutFloorResult, bool bCanUseCachedLocation, const FHitResult* DownwardSweepResult = NULL) const;


	/**
	 * Compute distance to the floor from bottom box and store the result in OutFloorResult.
	 * This distance is the swept distance of the box to the first point impacted by bottom, or distance from the bottom of the box in the case of a line trace.
	 * This function does not care if collision is disabled on the capsule (unlike FindFloor).
	 * @see FindFloor
	 *
	 * @param BoxLocation:		Location of the box used for the query
	 * @param LineDistance:		If non-zero, max distance to test for a simple line check from the capsule base. Used only if the sweep test fails to find a walkable floor, and only returns a valid result if the impact normal is a walkable normal.
	 * @param SweepDistance:	If non-zero, max distance to use when sweeping a capsule downwards for the test. MUST be greater than or equal to the line distance.
	 * @param OutFloorResult:	Result of the floor check. The HitResult will contain the valid sweep or line test upon success, or the result of the sweep upon failure.
	 * @param SweepRadius:		The radius to use for sweep tests. Should be <= capsule radius.
	 * @param DownwardSweepResult:	If non-null and it contains valid blocking hit info, this will be used as the result of a downward sweep test instead of doing it as part of the update.
	 */
	virtual void ComputeFloorDist(const FVector& BoxLocation, float LineDistance, float SweepDistance, FRlFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult = NULL) const;

	/**
	 * Sweep against the world and return the first blocking hit.
	 * Intended for tests against the floor, because it may change the result of impacts on the lower area of the test (especially if bUseFlatBaseForFloorChecks is true).
	 *
	 * @param OutHit			First blocking hit found.
	 * @param Start				Start location of the capsule.
	 * @param End				End location of the capsule.
	 * @param TraceChannel		The 'channel' that this trace is in, used to determine which components to hit.
	 * @param CollisionShape	Box collision shape.
	 * @param Params			Additional parameters used for the trace.
	 * @param ResponseParam		ResponseContainer to be used for this trace.
	 * @return True if OutHit contains a blocking hit entry.
	 */
	virtual bool FloorSweepTest(
		struct FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		ECollisionChannel TraceChannel,
		const struct FCollisionShape& CollisionShape,
		const struct FCollisionQueryParams& Params,
		const struct FCollisionResponseParams& ResponseParam
		) const;

	/** Verify that the supplied hit result is a valid landing spot when falling. */
	virtual bool IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const;

	/**
	 * Determine whether we should try to find a valid landing spot after an impact with an invalid one (based on the Hit result).
	 * For example, landing on the lower portion of the capsule on the edge of geometry may be a walkable surface, but could have reported an unwalkable impact normal.
	 */
	virtual bool ShouldCheckForValidLandingSpot(float DeltaTime, const FVector& Delta, const FHitResult& Hit) const;


protected:

	///** Called when the collision capsule touches another primitive component */
	//UFUNCTION()
	//virtual void CapsuleTouched(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Enum used to control GetPawnCapsuleExtent behavior
	enum EShrinkBoxExtent
	{
		SHRINK_None,			// Don't change the size of the capsule
		SHRINK_XCustom,			// Change only X, based on a supplied param
		SHRINK_YCustom,			// Change only X, based on a supplied param
		SHRINK_ZCustom,			// Change only X, based on a supplied param
		SHRINK_AllCustom,		// Change both radius and height, based on a supplied param
	};

	/** Get the box extent for the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 * @param ShrinkMode			Controls the way the box is resized.
	 * @param CustomShrinkAmount	The amount to shrink the box, used only for ShrinkModes that specify custom.
	 * @return The capsule extent of the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 */
	FVector GetPawnBoxExtent(const EShrinkBoxExtent ShrinkMode, const float CustomShrinkAmount = 0.f) const;
	
	/** Get the collision shape for the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 * @param ShrinkMode			Controls the way the box is resized.
	 * @param CustomShrinkAmount	The amount to shrink the box, used only for ShrinkModes that specify custom.
	 * @return The capsule extent of the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 */
	FCollisionShape GetPawnBoxCollisionShape(const EShrinkBoxExtent ShrinkMode, const float CustomShrinkAmount = 0.f) const;





	/** Enforce constraints on input given current state. For instance, don't move upwards if walking and looking up. */
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const;

	/** Scale input acceleration, based on movement acceleration rate. */
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const;

public:

	/** React to instantaneous change in position. Invalidates cached floor recomputes it if possible if there is a current movement base. */
	virtual void UpdateFloorFromAdjustment();









protected:

	// Movement functions broken out based on owner's network Role.
	// TickComponent calls the correct version based on the Role.
	// These may be called during move playback and correction during network updates.
	//

	/** Perform movement on an autonomous client */
	virtual void PerformMovement(float DeltaTime);






public:

	/** Minimum delta time considered when ticking. Delta times below this are not considered. This is a very small non-zero value to avoid potential divide-by-zero in simulation code. */
	static const float MIN_TICK_TIME;

	/** Minimum acceptable distance for Character capsule to float above floor when walking. */
	static const float MIN_FLOOR_DIST;

	/** Maximum acceptable distance for Character capsule to float above floor when walking. */
	static const float MAX_FLOOR_DIST;

	/** Reject sweep impacts that are this close to the edge of the vertical portion of the capsule when performing vertical sweeps, and try again with a smaller capsule. */
	static const float SWEEP_EDGE_REJECT_DISTANCE;

	/** Stop completely when braking and velocity magnitude is lower than this. */
	static const float BRAKE_TO_STOP_VELOCITY;
};


FORCEINLINE ARlCharacter* URlCharacterMovementComponent::GetCharacterOwner() const
{
	return CharacterOwner;
}

FORCEINLINE_DEBUGGABLE bool URlCharacterMovementComponent::IsWalking() const
{
	return IsMovingOnGround();
}