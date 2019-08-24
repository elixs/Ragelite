// Fill out your copyright notice in the Description page of Project Settings.

#include "RlCharacterMovementComponent.h"
#include "RlCharacter.h"
#include "GameFramework/PhysicsVolume.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Spike.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterMovement, Log, All);

// MAGIC NUMBERS
const float MAX_STEP_SIDE_Z = 0.08f;	// maximum z value for the normal on the vertical side of steps
const float SWIMBOBSPEED = -80.f;
const float VERTICAL_SLOPE_NORMAL_Z = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle normals slightly off horizontal for vertical surface.

const float URlCharacterMovementComponent::MIN_TICK_TIME = 1e-6f;
const float URlCharacterMovementComponent::MIN_FLOOR_DIST = 2.f;
const float URlCharacterMovementComponent::MAX_FLOOR_DIST = 2.4f;
const float URlCharacterMovementComponent::BRAKE_TO_STOP_VELOCITY = 10.f;
const float URlCharacterMovementComponent::SWEEP_EDGE_REJECT_DISTANCE = 0.15f;

void FRlFindFloorResult::SetFromSweep(const FHitResult& InHit, const float InSweepFloorDist, const bool bIsWalkableFloor)
{
	bBlockingHit = InHit.IsValidBlockingHit();
	bWalkableFloor = bIsWalkableFloor;
	bLineTrace = false;
	FloorDist = InSweepFloorDist;
	LineDist = 0.f;
	HitResult = InHit;
}

void FRlFindFloorResult::SetFromLineTrace(const FHitResult& InHit, const float InSweepFloorDist, const float InLineDist, const bool bIsWalkableFloor)
{
	// We require a sweep that hit if we are going to use a line result.
	check(HitResult.bBlockingHit);
	if (HitResult.bBlockingHit && InHit.bBlockingHit)
	{
		// Override most of the sweep result with the line result, but save some values
		FHitResult OldHit(HitResult);
		HitResult = InHit;

		// Restore some of the old values. We want the new normals and hit actor, however.
		HitResult.Time = OldHit.Time;
		HitResult.ImpactPoint = OldHit.ImpactPoint;
		HitResult.Location = OldHit.Location;
		HitResult.TraceStart = OldHit.TraceStart;
		HitResult.TraceEnd = OldHit.TraceEnd;

		bLineTrace = true;
		FloorDist = InSweepFloorDist;
		LineDist = InLineDist;
		bWalkableFloor = bIsWalkableFloor;
	}
}

// meh
URlCharacterMovementComponent::URlCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bConstrainToPlane = true;
	SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);

	GravityScale = 1.5f;

	JumpZVelocity = 400.f;
	JumpOffJumpZFactor = 0.5f;

	NormalMaxAcceleration = 150.f;
	NormalMaxWalkSpeed = 50.f;
	SprintMaxAcceleration = 300.f;
	SprintMaxWalkSpeed = 100.f;
	bSprintStop = false;

	LastSpeed = SprintMaxWalkSpeed;

	MaxAcceleration = NormalMaxAcceleration;
	BrakingFrictionFactor = 1.f;
	BrakingFriction = 4.f;
	bUseSeparateBrakingFriction = true;

	GroundFriction = 6.f;
	MaxWalkSpeed = NormalMaxWalkSpeed;
	BrakingDecelerationWalking = 1.f;

	bCustomTerminalVelocity = false;
	TerminalVelocity = 50.f;

	//bConstrainToPlane = true
	//SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);

	//HorizontalVelocityFactor = 1.f;

	MaxSimulationTimeStep = 0.05f;
	MaxSimulationIterations = 8;

	//MaxDepenetrationWithGeometry = 500.f;
	//MaxDepenetrationWithPawn = 100.f;

	FallingLateralFriction = 0.f;
	BrakingSubStepTime = 1.0f / 33.0f;
	BrakingDecelerationFalling = 0.f;

	Mass = 10.0f;
	bJustTeleported = true;

	LastUpdateRotation = FQuat::Identity;
	LastUpdateVelocity = FVector::ZeroVector;
	PendingImpulseToApply = FVector::ZeroVector;
	PendingLaunchVelocity = FVector::ZeroVector;
	DefaultLandMovementMode = ERlMovementMode::Walking;
	GroundMovementMode = ERlMovementMode::Walking;
	bForceNextFloorCheck = true;

	bImpartBaseVelocityX = true;
	bImpartBaseVelocityY = true;
	bImpartBaseVelocityZ = true;
	bImpartBaseAngularVelocity = true;
	bAlwaysCheckFloor = true;

	OldBaseQuat = FQuat::Identity;
	OldBaseLocation = FVector::ZeroVector;
}

// meh
void URlCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	const FVector InputVector = ConsumeInputVector();
	if (!HasValidData() || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	if (CharacterOwner->bDeath)
	{
		Velocity = FVector::ZeroVector;
		return;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Super tick may destroy/invalidate CharacterOwner or UpdatedComponent, so we need to re-check.
	if (!HasValidData() || !CharacterOwner->CheckStillInWorld())
	{
		return;
	}

	if (CharacterOwner->IsLocallyControlled() || (!CharacterOwner->Controller && bRunPhysicsWithNoController))
	{
		// We need to check the jump state before adjusting input acceleration, to minimize latency
		// and to make sure acceleration respects our potentially new falling state.
		CharacterOwner->CheckJumpInput(DeltaTime);

		// apply input to acceleration
		Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector));

		AnalogInputModifier = ComputeAnalogInputModifier();

		PerformMovement(DeltaTime);
	}

	//UE_LOG(LogTemp, Warning, TEXT("Velocity: %f"), Velocity.Size());
}

// meh
void URlCharacterMovementComponent::PerformMovement(float DeltaSeconds)
{
	const UWorld* MyWorld = GetWorld();
	if (!HasValidData() || MyWorld == nullptr)
	{
		return;
	}

	// Force floor update if we've moved outside of CharacterMovement since last update.
	bForceNextFloorCheck |= (IsMovingOnGround() && UpdatedComponent->GetComponentLocation() != LastUpdateLocation);

	FVector OldVelocity;
	FVector OldLocation;

	// Scoped updates can improve performance of multiple MoveComponent calls.
	{
		FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

		OldVelocity = Velocity;
		OldLocation = UpdatedComponent->GetComponentLocation();

		ApplyAccumulatedForces(DeltaSeconds);

		// Character::LaunchCharacter() has been deferred until now.
		// Launch means overwrite the current velocity.
		HandlePendingLaunch();
		ClearAccumulatedForces();

		// Clear jump input now, to allow movement events to trigger it for next update.
		CharacterOwner->ClearJumpInput(DeltaSeconds);

		// change position
		StartNewPhysics(DeltaSeconds, 0);

		if (!HasValidData())
		{
			return;
		}
	} // End scoped movement update

	// Here the scoped movement is complete, we can create a delegate and call it here if needed

	SaveBaseLocation();
	// Update component velocity in case events want to read it
	UpdateComponentVelocity();

	const FVector NewLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
	const FQuat NewRotation = UpdatedComponent ? UpdatedComponent->GetComponentQuat() : FQuat::Identity;

	LastUpdateLocation = NewLocation;
	LastUpdateRotation = NewRotation;
	LastUpdateVelocity = Velocity;
}

// meh
void URlCharacterMovementComponent::StartNewPhysics(float DeltaTime, int32 Iterations)
{
	if ((DeltaTime < MIN_TICK_TIME) || (Iterations >= MaxSimulationIterations) || !HasValidData())
	{
		return;
	}

	//UE_LOG(LogTemp, Warning, TEXT("%s"), *Velocity.ToString());

	const bool bSavedMovementInProgress = bMovementInProgress;
	bMovementInProgress = true;

	switch (MovementMode)
	{
	case ERlMovementMode::Walking:
		PhysWalking(DeltaTime, Iterations);
		break;
	case ERlMovementMode::Falling:
		PhysFalling(DeltaTime, Iterations);
		break;
	case ERlMovementMode::WallWalking:
		PhysWallWalking(DeltaTime, Iterations);
		break;
	}

	bMovementInProgress = bSavedMovementInProgress;
	if (bDeferUpdateMoveComponent)
	{
		SetUpdatedComponent(DeferredUpdatedMoveComponent);
	}
}

// meh
void URlCharacterMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsQueryCollisionEnabled())
	{
		return;
	}

	bJustTeleported = false;
	bool bCheckedFall = false;
	float RemainingTime = DeltaTime;

	// Perform the move
	while ((RemainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController))
	{
		Iterations++;
		bJustTeleported = false;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		// Save current values
		UPrimitiveComponent * const OldBase = GetMovementBase();
		const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FRlFindFloorResult OldFloor = CurrentFloor;

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;
		Acceleration.Z = 0.f;

		CalcVelocity(TimeTick, GroundFriction, GetMaxBrakingDeceleration());

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = TimeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();

		if (bZeroDelta)
		{
			RemainingTime = 0.f;
		}
		else
		{
			if (CheckSpikes())
			{
				return;
			}

			// try to move forward
			MoveHorizontal(MoveVelocity, TimeTick);

			if (IsFalling())
			{
				// pawn decided to jump up
				const float DesiredDist = Delta.Size();
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					RemainingTime += TimeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));
				}
				StartNewPhysics(RemainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);

		// Validate the floor check
		if (CurrentFloor.IsWalkableFloor())
		{
			AdjustFloorHeight();
			SetBase(CurrentFloor.HitResult.Component.Get(), CurrentFloor.HitResult.BoneName);
		}
		else if (CurrentFloor.HitResult.bStartPenetrating && RemainingTime <= 0.f)
		{
			// The floor check failed because it started in penetration
			// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
			FHitResult Hit(CurrentFloor.HitResult);
			Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
			const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
			ResolvePenetration(RequestedAdjustment, Hit, UpdatedComponent->GetComponentQuat());
			bForceNextFloorCheck = true;
		}

		// See if we need to start falling.
		if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
		{
			if (!bCheckedFall && CheckFall(OldFloor, CurrentFloor.HitResult, Delta, OldLocation, RemainingTime, TimeTick, Iterations))
			{
				return;
			}
			bCheckedFall = true;
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			RemainingTime = 0.f;
			break;
		}
	}

	if (IsMovingOnGround())
	{
		MaintainHorizontalGroundVelocity();
	}
}


void URlCharacterMovementComponent::PhysFalling(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	FVector FallAcceleration(Acceleration.X, Acceleration.Y, 0.f);

	float RemainingTime = DeltaTime;
	while ((RemainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
		bJustTeleported = false;

		FVector OldVelocity = Velocity;
		FVector VelocityNoAirControl = Velocity;

		// Apply input
		const float MaxDecel = GetMaxBrakingDeceleration();
		// Compute VelocityNoAirControl
		{
			// Find velocity *without* acceleration.
			TGuardValue<FVector> RestoreAcceleration(Acceleration, FVector::ZeroVector);
			TGuardValue<FVector> RestoreVelocity(Velocity, Velocity);
			Velocity.Z = 0.f;
			CalcVelocity(TimeTick, FallingLateralFriction, MaxDecel);
			VelocityNoAirControl = FVector(Velocity.X, Velocity.Y, OldVelocity.Z);
		}

		// Compute Velocity
		{
			// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
			TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
			Velocity.Z = 0.f;
			CalcVelocity(TimeTick, FallingLateralFriction, MaxDecel);
			Velocity.Z = OldVelocity.Z;
		}

		// Apply gravity
		const FVector Gravity(0.f, 0.f, GetGravityZ());
		float GravityTime = TimeTick;

		// If jump is providing force, gravity may be affected.
		if (CharacterOwner->JumpForceTimeRemaining > 0.0f)
		{
			// Update Character state
			CharacterOwner->JumpForceTimeRemaining -= FMath::Min(CharacterOwner->JumpForceTimeRemaining, TimeTick);
			if (CharacterOwner->JumpForceTimeRemaining <= 0.0f)
			{
				CharacterOwner->ResetJumpState();
			}
		}

		Velocity += Gravity * GravityTime;
		VelocityNoAirControl += Gravity * GravityTime;

		// Move
		FHitResult Hit(1.f);
		FVector Adjusted = 0.5f*(OldVelocity + Velocity) * TimeTick;
		SafeMoveUpdatedComponent(Adjusted, PawnRotation, true, Hit);

		if (!HasValidData())
		{
			return;
		}

		float LastMoveTimeSlice = TimeTick;
		float subTimeTickRemaining = TimeTick * (1.f - Hit.Time);

		if (Hit.bBlockingHit)
		{
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				RemainingTime += subTimeTickRemaining;
				ProcessLanded(Hit, RemainingTime, Iterations);
				return;
			}
			else
			{
				// Compute impact deflection based on final velocity, not integration step.
				// This allows us to compute a new velocity from the deflected vector, and ensures the full gravity effect is included in the slide result.
				Adjusted = Velocity * TimeTick;

				if (Hit.Normal.Z < 0)
				{
					CharacterOwner->JumpForceTimeRemaining = 0.f;
					CharacterOwner->ResetJumpState();
				}

				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;
				FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

				// Compute velocity after deflection (only gravity component for RootMotion)
				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
				{
					const FVector NewVelocity = (Delta / subTimeTickRemaining);
					Velocity = NewVelocity;
				}

				if (subTimeTickRemaining > KINDA_SMALL_NUMBER && (Delta | Adjusted) > 0.f)
				{
					// Move in deflected direction.
					SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);

					if (Hit.bBlockingHit)
					{
						// hit second wall
						LastMoveTimeSlice = subTimeTickRemaining;
						subTimeTickRemaining = subTimeTickRemaining * (1.f - Hit.Time);

						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							RemainingTime += subTimeTickRemaining;
							ProcessLanded(Hit, RemainingTime, Iterations);
							return;
						}

						const FVector LastMoveNoAirControl = VelocityNoAirControl * LastMoveTimeSlice;
						Delta = ComputeSlideVector(LastMoveNoAirControl, 1.f, OldHitNormal, Hit);

						FVector PreTwoWallDelta = Delta;
						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// Compute velocity after deflection (only gravity component for RootMotion)
						if (subTimeTickRemaining > KINDA_SMALL_NUMBER && !bJustTeleported)
						{
							const FVector NewVelocity = (Delta / subTimeTickRemaining);
							Velocity = NewVelocity;
						}
						SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
					}
				}
			}
		}

		if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}
	}
}

// meh
void URlCharacterMovementComponent::PhysWallWalking(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	float RemainingTime = DeltaTime;
	while ((RemainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations))
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		if (CharacterOwner->WallWalkMaxHoldTime > 0.0f)
		{
			// Update Character state
			CharacterOwner->WallWalkHoldTime += TimeTick;
			if (CharacterOwner->WallWalkHoldTime > CharacterOwner->WallWalkMaxHoldTime || !CharacterOwner->bIsPressingJump)
			{
				CharacterOwner->ResetJumpState();
				SetMovementMode(ERlMovementMode::Falling);
				StartNewPhysics(DeltaTime, Iterations);
				return;
			}
		}

		if (Acceleration.X == 0.f || Velocity.X == 0.f)
		{
			CharacterOwner->ResetJumpState();
			SetMovementMode(ERlMovementMode::Falling);
			StartNewPhysics(DeltaTime, Iterations);
			return;
		}

		// Ensure velocity is horizontal.
		Velocity.Z = 0.f;
		Acceleration.Z = 0.f;

		CalcVelocity(TimeTick, GroundFriction, GetMaxBrakingDeceleration());

		Velocity.X = FMath::Sign(Velocity.X) * FMath::Min(FMath::Abs(Velocity.X), LastSpeed);
		LastSpeed = FMath::Abs(Velocity.X);

		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = TimeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();

		if (bZeroDelta)
		{
			RemainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveHorizontal(MoveVelocity, TimeTick, false);
		}
	}
}

// meh
void URlCharacterMovementComponent::PostLoad()
{
	Super::PostLoad();

	CharacterOwner = Cast<ARlCharacter>(PawnOwner);
}

// meh
void URlCharacterMovementComponent::Deactivate()
{
	Super::Deactivate();

	if (!IsActive())
	{
		ClearAccumulatedForces();
		if (CharacterOwner)
		{
			CharacterOwner->ResetJumpState();
		}
	}
}

// meh
void URlCharacterMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent)
{
	if (NewUpdatedComponent)
	{
		const ARlCharacter* NewCharacterOwner = Cast<ARlCharacter>(NewUpdatedComponent->GetOwner());
		if (NewCharacterOwner == NULL)
		{
			UE_LOG(LogCharacterMovement, Error, TEXT("%s owned by %s must update a component owned by a RlCharacter"), *GetName(), *GetNameSafe(NewUpdatedComponent->GetOwner()));
			return;
		}

		// check that UpdatedComponent is a Box
		if (Cast<UBoxComponent>(NewUpdatedComponent) == NULL)
		{
			UE_LOG(LogCharacterMovement, Error, TEXT("%s owned by %s must update a box component"), *GetName(), *GetNameSafe(NewUpdatedComponent->GetOwner()));
			return;
		}
	}

	if (bMovementInProgress)
	{
		// failsafe to avoid crashes in CharacterMovement.
		bDeferUpdateMoveComponent = true;
		DeferredUpdatedMoveComponent = NewUpdatedComponent;
		return;
	}
	bDeferUpdateMoveComponent = false;
	DeferredUpdatedMoveComponent = NULL;

	USceneComponent* OldUpdatedComponent = UpdatedComponent;
	UPrimitiveComponent* OldPrimitive = Cast<UPrimitiveComponent>(UpdatedComponent);

	Super::SetUpdatedComponent(NewUpdatedComponent);
	CharacterOwner = Cast<ARlCharacter>(PawnOwner);

	if (UpdatedComponent != OldUpdatedComponent)
	{
		ClearAccumulatedForces();
	}

	if (UpdatedComponent == NULL)
	{
		StopActiveMovement();
	}

	const bool bValidUpdatedPrimitive = IsValid(UpdatedPrimitive);
}

// meh
bool URlCharacterMovementComponent::HasValidData() const
{
	const bool bIsValid = UpdatedComponent && IsValid(CharacterOwner);
	return bIsValid;
}

// meh
FCollisionShape URlCharacterMovementComponent::GetPawnBoxCollisionShape(const EShrinkBoxExtent ShrinkMode, const float CustomShrinkAmount) const
{
	FVector Extent = GetPawnBoxExtent(ShrinkMode, CustomShrinkAmount);
	return FCollisionShape::MakeBox(Extent);
}

// meh
FVector URlCharacterMovementComponent::GetPawnBoxExtent(const EShrinkBoxExtent ShrinkMode, const float CustomShrinkAmount) const
{
	check(CharacterOwner);

	FVector BoxExtent = CharacterOwner->GetBoxComponent()->GetScaledBoxExtent();

	float XEpsilon = 0.f;
	float YEpsilon = 0.f;
	float ZEpsilon = 0.f;

	switch (ShrinkMode)
	{
	case SHRINK_None:
		return BoxExtent;

	case SHRINK_XCustom:
		XEpsilon = CustomShrinkAmount;
		break;

	case SHRINK_YCustom:
		YEpsilon = CustomShrinkAmount;
		break;

	case SHRINK_ZCustom:
		ZEpsilon = CustomShrinkAmount;
		break;

	case SHRINK_AllCustom:
		XEpsilon = CustomShrinkAmount;
		YEpsilon = CustomShrinkAmount;
		ZEpsilon = CustomShrinkAmount;
		break;

	default:
		UE_LOG(LogCharacterMovement, Warning, TEXT("Unknown EShrinkCapsuleExtent in URlCharacterMovementComponent::GetCapsuleExtent"));
		break;
	}

	// Don't shrink to zero extent.
	const float MinExtent = KINDA_SMALL_NUMBER * 10.f;
	BoxExtent.X = FMath::Max(BoxExtent.X - XEpsilon, MinExtent);
	BoxExtent.Y = FMath::Max(BoxExtent.Y - YEpsilon, MinExtent);
	BoxExtent.Z = FMath::Max(BoxExtent.Z - ZEpsilon, MinExtent);

	return BoxExtent;
}

// meh
bool URlCharacterMovementComponent::DoJump()
{
	if (CharacterOwner && CharacterOwner->CanJump())
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			float HoldJumpKeyFactor = 0.f;

			if (CharacterOwner->bWasJumping && CharacterOwner->JumpMaxHoldTime > 0.f)
			{
				HoldJumpKeyFactor = FMath::Pow(1.f - (CharacterOwner->JumpForceTimeRemaining / CharacterOwner->JumpMaxHoldTime), CharacterOwner->JumpHoldForceFactor);
			}

			Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity * (1.f - HoldJumpKeyFactor));
			SetMovementMode(ERlMovementMode::Falling);
			return true;
		}
	}

	return false;
}

// meh
bool URlCharacterMovementComponent::DoWallWalk()
{
	if (CharacterOwner && CharacterOwner->CanWallWalk())
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			//Velocity.X = FMath::Max(Velocity.Z, JumpZVelocity * (1.f - HoldJumpKeyFactor));
			SetMovementMode(ERlMovementMode::WallWalking);
			return true;
		}
	}

	return false;
}

// meh
FVector URlCharacterMovementComponent::GetImpartedMovementBaseVelocity() const
{
	FVector Result = FVector::ZeroVector;
	if (CharacterOwner)
	{
		UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
		if (RlMovementBaseUtility::IsDynamicBase(MovementBase))
		{
			FVector BaseVelocity = RlMovementBaseUtility::GetMovementBaseVelocity(MovementBase, CharacterOwner->GetBasedMovement().BoneName);

			if (bImpartBaseAngularVelocity)
			{
				const FVector CharacterBasePosition = (UpdatedComponent->GetComponentLocation() - FVector(0.f, 0.f, CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().Z));
				const FVector BaseTangentialVel = RlMovementBaseUtility::GetMovementBaseTangentialVelocity(MovementBase, CharacterOwner->GetBasedMovement().BoneName, CharacterBasePosition);
				BaseVelocity += BaseTangentialVel;
			}

			if (bImpartBaseVelocityX)
			{
				Result.X = BaseVelocity.X;
			}
			if (bImpartBaseVelocityY)
			{
				Result.Y = BaseVelocity.Y;
			}
			if (bImpartBaseVelocityZ)
			{
				Result.Z = BaseVelocity.Z;
			}
		}
	}

	return Result;
}

// meh
void URlCharacterMovementComponent::Launch(FVector const& LaunchVel)
{
	if (IsActive() && HasValidData())
	{
		PendingLaunchVelocity = LaunchVel;
	}
}

// meh
bool URlCharacterMovementComponent::HandlePendingLaunch()
{
	if (!PendingLaunchVelocity.IsZero() && HasValidData())
	{
		Velocity = PendingLaunchVelocity;
		SetMovementMode(ERlMovementMode::Falling);
		PendingLaunchVelocity = FVector::ZeroVector;
		bForceNextFloorCheck = true;
		return true;
	}

	return false;
}

// meh
void URlCharacterMovementComponent::JumpOff(AActor* MovementBaseActor)
{
	if (!bPerformingJumpOff)
	{
		bPerformingJumpOff = true;
		if (CharacterOwner)
		{
			const float MaxSpeed = GetMaxSpeed() * 0.85f;
			Velocity += MaxSpeed * GetBestDirectionOffActor(MovementBaseActor);
			if (Velocity.Size2D() > MaxSpeed)
			{
				Velocity = MaxSpeed * Velocity.GetSafeNormal();
			}
			Velocity.Z = JumpOffJumpZFactor * JumpZVelocity;
			SetMovementMode(ERlMovementMode::Falling);
		}
		bPerformingJumpOff = false;
	}
}

// meh
FVector URlCharacterMovementComponent::GetBestDirectionOffActor(AActor* BaseActor) const
{
	// By default, just pick a random direction.  Derived character classes can choose to do more complex calculations,
	// such as finding the shortest distance to move in based on the BaseActor's Bounding Volume.
	const float RandAngle = FMath::DegreesToRadians(FMath::SRand() * 360.f);
	return FVector(FMath::Cos(RandAngle), FMath::Sin(RandAngle), 0.5f).GetSafeNormal();
}

// meh
void URlCharacterMovementComponent::SetDefaultMovementMode()
{
	if (!CharacterOwner || MovementMode != DefaultLandMovementMode)
	{
		const float SavedVelocityZ = Velocity.Z;
		SetMovementMode(DefaultLandMovementMode);

		// Avoid 1-frame delay if trying to walk but walking fails at this location.
		if (MovementMode == ERlMovementMode::Walking && GetMovementBase() == NULL)
		{
			Velocity.Z = SavedVelocityZ; // Prevent temporary walking state from zeroing Z velocity.
			SetMovementMode(ERlMovementMode::Falling);
		}
	}
}

// meh
void URlCharacterMovementComponent::SetMovementMode(ERlMovementMode NewMovementMode)
{
	// Do nothing if nothing is changing.
	if (MovementMode == NewMovementMode)
	{
		return;
	}

	const ERlMovementMode PrevMovementMode = MovementMode;

	MovementMode = NewMovementMode;

	// We allow setting movement mode before we have a component to update, in case this happens at startup.
	if (!HasValidData())
	{
		return;
	}

	// Handle change in movement mode
	OnMovementModeChanged(PrevMovementMode);
}

// meh
void URlCharacterMovementComponent::OnMovementModeChanged(ERlMovementMode PreviousMovementMode)
{
	if (!HasValidData())
	{
		return;
	}

	// React to changes in the movement mode.
	if (MovementMode == ERlMovementMode::Walking)
	{
		// Walking uses only XY velocity, and must be on a walkable floor, with a Base.
		Velocity.Z = 0.f;

		// make sure we update our new floor/base on initial entry of the walking physics
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
		AdjustFloorHeight();
		SetBaseFromFloor(CurrentFloor);

		CharacterOwner->ResetJumpState();
		CharacterOwner->bWallWalkToggle = true;
		LastSpeed = SprintMaxWalkSpeed;
	}
	else
	{
		CurrentFloor.Clear();

		if (MovementMode == ERlMovementMode::Falling && PreviousMovementMode == ERlMovementMode::Walking)
		{
			// Add the valocity of the base;
			Velocity += GetImpartedMovementBaseVelocity();
		}

		SetBase(NULL);
	}
	if (MovementMode == ERlMovementMode::WallWalking)
	{
		CharacterOwner->bWallWalkToggle = false;
	}
};

// meh
UPrimitiveComponent* URlCharacterMovementComponent::GetMovementBase() const
{
	return CharacterOwner ? CharacterOwner->GetMovementBase() : NULL;
}

// meh
void URlCharacterMovementComponent::SetBase(UPrimitiveComponent* NewBase, FName BoneName, bool bNotifyActor)
{
	if (CharacterOwner)
	{
		CharacterOwner->SetBase(NewBase, NewBase ? BoneName : NAME_None, bNotifyActor);
	}
}

// meh
void URlCharacterMovementComponent::SetBaseFromFloor(const FRlFindFloorResult& FloorResult)
{
	if (FloorResult.IsWalkableFloor())
	{
		SetBase(FloorResult.HitResult.GetComponent(), FloorResult.HitResult.BoneName);
	}
	else
	{
		SetBase(nullptr);
	}
}

// meh
void URlCharacterMovementComponent::UpdateBasedRotation(FRotator& FinalRotation, const FRotator& ReducedRotation)
{
	AController* Controller = CharacterOwner ? CharacterOwner->Controller : NULL;
	float ControllerRoll = 0.f;
	if (Controller && !bIgnoreBaseRotation)
	{
		FRotator const ControllerRot = Controller->GetControlRotation();
		ControllerRoll = ControllerRot.Roll;
		Controller->SetControlRotation(ControllerRot + ReducedRotation);
	}

	// Remove roll
	FinalRotation.Roll = 0.f;
	if (Controller)
	{
		FinalRotation.Roll = UpdatedComponent->GetComponentRotation().Roll;
		FRotator NewRotation = Controller->GetControlRotation();
		NewRotation.Roll = ControllerRoll;
		Controller->SetControlRotation(NewRotation);
	}
}

// meh
void URlCharacterMovementComponent::DisableMovement()
{
	//if (CharacterOwner)
	//{
	//	SetMovementMode(MOVE_None);
	//}
	//else
	//{
	//	MovementMode = MOVE_None;
	//	CustomMovementMode = 0;
	//}
}

// meh
void URlCharacterMovementComponent::SaveBaseLocation()
{
	if (!HasValidData())
	{
		return;
	}

	const UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
	if (RlMovementBaseUtility::UseRelativeLocation(MovementBase))
	{
		// Read transforms into OldBaseLocation, OldBaseQuat
		RlMovementBaseUtility::GetMovementBaseTransform(MovementBase, CharacterOwner->GetBasedMovement().BoneName, OldBaseLocation, OldBaseQuat);

		// Location
		const FVector RelativeLocation = UpdatedComponent->GetComponentLocation() - OldBaseLocation;

		// Rotation
		if (bIgnoreBaseRotation)
		{
			// Absolute rotation
			CharacterOwner->SaveRelativeBasedMovement(RelativeLocation, UpdatedComponent->GetComponentRotation(), false);
		}
		else
		{
			// Relative rotation
			const FRotator RelativeRotation = (FQuatRotationMatrix(UpdatedComponent->GetComponentQuat()) * FQuatRotationMatrix(OldBaseQuat).GetTransposed()).Rotator();
			CharacterOwner->SaveRelativeBasedMovement(RelativeLocation, RelativeRotation, true);
		}
	}
}

float URlCharacterMovementComponent::GetGravityZ() const
{
	return Super::GetGravityZ() * GravityScale;
}

// meh
float URlCharacterMovementComponent::GetMaxSpeed() const
{
	//switch (MovementMode)
	return MaxWalkSpeed;
}

// meh
float URlCharacterMovementComponent::GetMinAnalogSpeed() const
{
	return MinAnalogWalkSpeed;
}

// meh
FVector URlCharacterMovementComponent::GetPenetrationAdjustment(const FHitResult& Hit) const
{
	FVector Result = Super::GetPenetrationAdjustment(Hit);

	if (CharacterOwner)
	{
		float MaxDistance = MaxDepenetrationWithGeometry;
		const AActor* HitActor = Hit.GetActor();
		if (Cast<APawn>(HitActor))
		{
			MaxDistance = MaxDepenetrationWithPawn;
		}

		Result = Result.GetClampedToMaxSize(MaxDistance);
	}

	return Result;
}

// meh
bool URlCharacterMovementComponent::ResolvePenetrationImpl(const FVector& Adjustment, const FHitResult& Hit, const FQuat& NewRotation)
{
	// If movement occurs, mark that we teleported, so we don't incorrectly adjust velocity based on a potentially very different movement than our movement direction.
	bJustTeleported |= Super::ResolvePenetrationImpl(Adjustment, Hit, NewRotation);
	return bJustTeleported;
}

// meh
float URlCharacterMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult& Hit, bool bHandleImpact)
{
	if (!Hit.bBlockingHit)
	{
		return 0.f;
	}

	FVector Normal(InNormal);
	if (IsMovingOnGround())
	{
		// We don't want to be pushed up an unwalkable surface.
		if (Normal.Z > 0.f)
		{
			if (!IsWalkable(Hit))
			{
				Normal = Normal.GetSafeNormal2D();
			}
		}
		else if (Normal.Z < -KINDA_SMALL_NUMBER)
		{
			// Don't push down into the floor when the impact is on the upper portion of the capsule.
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				const FVector FloorNormal = CurrentFloor.HitResult.Normal;
				const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Z < 1.f - DELTA);
				if (bFloorOpposedToMovement)
				{
					Normal = FloorNormal;
				}

				Normal = Normal.GetSafeNormal2D();
			}
		}
	}

	return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
}

// meh
FVector URlCharacterMovementComponent::ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	FVector Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

	// prevent boosting up slopes
	if (IsFalling())
	{
		Result = HandleSlopeBoosting(Result, Delta, Time, Normal, Hit);
	}

	return Result;
}

// meh
FVector URlCharacterMovementComponent::HandleSlopeBoosting(const FVector& SlideResult, const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	FVector Result = SlideResult;

	if (Result.Z > 0.f)
	{
		// Don't move any higher than we originally intended.
		const float ZLimit = Delta.Z * Time;
		if (Result.Z - ZLimit > KINDA_SMALL_NUMBER)
		{
			if (ZLimit > 0.f)
			{
				// Rescale the entire vector (not just the Z component) otherwise we change the direction and likely head right back into the impact.
				const float UpPercent = ZLimit / Result.Z;
				Result *= UpPercent;
			}
			else
			{
				// We were heading down but were going to deflect upwards. Just make the deflection horizontal.
				Result = FVector::ZeroVector;
			}

			// Make remaining portion of original result horizontal and parallel to impact normal.
			const FVector RemainderXY = (SlideResult - Result) * FVector(1.f, 1.f, 0.f);
			const FVector NormalXY = Normal.GetSafeNormal2D();
			const FVector Adjust = Super::ComputeSlideVector(RemainderXY, 1.f, NormalXY, Hit);
			Result += Adjust;
		}
	}

	return Result;
}

// meh
bool URlCharacterMovementComponent::IsMovingOnGround() const
{
	return (MovementMode == ERlMovementMode::Walking) && UpdatedComponent;
}

// meh
bool URlCharacterMovementComponent::IsFalling() const
{
	return (MovementMode == ERlMovementMode::Falling) && UpdatedComponent;
}

// meh
bool URlCharacterMovementComponent::IsWallWalking() const
{
	return (MovementMode == ERlMovementMode::WallWalking) && UpdatedComponent;
}

// meh
void URlCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, float BrakingDeceleration)
{
	if (!HasValidData() || DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	Friction = FMath::Max(0.f, Friction);
	const float MaxAccel = GetMaxAcceleration();
	float MaxSpeed = GetMaxSpeed();


	if (bForceMaxAccel)
	{
		// Force acceleration at full speed.
		// In consideration order for direction: Acceleration, then Velocity, then Pawn's rotation.
		if (Acceleration.SizeSquared() > SMALL_NUMBER)
		{
			Acceleration = Acceleration.GetSafeNormal() * MaxAccel;
		}
		else
		{
			Acceleration = MaxAccel * (Velocity.SizeSquared() < SMALL_NUMBER ? UpdatedComponent->GetForwardVector() : Velocity.GetSafeNormal());
		}

		AnalogInputModifier = 1.f;
	}

	MaxSpeed = FMath::Max(MaxSpeed * AnalogInputModifier, GetMinAnalogSpeed());

	// Apply braking or deceleration
	const bool bZeroAcceleration = Acceleration.IsZero();
	const bool bVelocityOverMax = IsExceedingMaxSpeed(MaxSpeed);

	// Only apply braking if there is no acceleration, or we are over our max speed and need to slow down to it.
	if (bZeroAcceleration || bVelocityOverMax)
	{
		const FVector OldVelocity = Velocity;

		const float ActualBrakingFriction = (bUseSeparateBrakingFriction ? BrakingFriction : Friction);
		ApplyVelocityBraking(DeltaTime, ActualBrakingFriction, BrakingDeceleration);

		//// Don't allow braking to lower us below max speed if we started above it.
		//if (bVelocityOverMax && Velocity.SizeSquared() < FMath::Square(MaxSpeed) && FVector::DotProduct(Acceleration, OldVelocity) > 0.0f)
		//{
		//	Velocity = OldVelocity.GetSafeNormal() * MaxSpeed;
		//}
	}
	else if (!bZeroAcceleration)
	{
		// Friction affects our ability to change direction. This is only done for input acceleration, not path following.
		const FVector AccelDir = Acceleration.GetSafeNormal();
		const float VelSize = Velocity.Size();
		Velocity = Velocity - (Velocity - AccelDir * VelSize) * FMath::Min(DeltaTime * Friction, 1.f);
	}

	// Apply input acceleration
	if (!bZeroAcceleration)
	{
		Velocity += Acceleration * DeltaTime;

		// This allows a delay when clamping the velocity
		if (bSprintStop && FMath::Abs(Velocity.X) <= MaxSpeed)
		{
			bSprintStop = false;
		}

		if (!bSprintStop)
		{
			Velocity = Velocity.GetClampedToMaxSize(MaxSpeed);
		}
	}
}

// meh
float URlCharacterMovementComponent::GetMaxJumpHeight() const
{
	const float Gravity = GetGravityZ();
	if (FMath::Abs(Gravity) > KINDA_SMALL_NUMBER)
	{
		return FMath::Square(JumpZVelocity) / (-2.f * Gravity);
	}
	else
	{
		return 0.f;
	}
}

// meh
float URlCharacterMovementComponent::GetMaxJumpHeightWithJumpTime() const
{
	const float MaxJumpHeight = GetMaxJumpHeight();

	if (CharacterOwner)
	{
		// When bApplyGravityWhileJumping is true, the actual max height will be lower than this.
		// However, it will also be dependent on framerate (and substep iterations) so just return this
		// to avoid expensive calculations.

		// This can be imagined as the character being displaced to some height, then jumping from that height.
		return (CharacterOwner->JumpMaxHoldTime * JumpZVelocity) + MaxJumpHeight;
	}

	return MaxJumpHeight;
}

// meh
float URlCharacterMovementComponent::GetMaxAcceleration() const
{
	return MaxAcceleration;
}

// meh
float URlCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	switch (MovementMode)
	{
	case ERlMovementMode::Walking:
	case ERlMovementMode::WallWalking:
		return BrakingDecelerationWalking;
	case ERlMovementMode::Falling:
		return BrakingDecelerationFalling;
	default:
		return 0.f;
	}
}

// meh
FVector URlCharacterMovementComponent::GetCurrentAcceleration() const
{
	return Acceleration;
}

// meh
void URlCharacterMovementComponent::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
	if (Velocity.IsZero() || !HasValidData() || DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	const float FrictionFactor = FMath::Max(0.f, BrakingFrictionFactor);
	Friction = FMath::Max(0.f, Friction * FrictionFactor);
	BrakingDeceleration = FMath::Max(0.f, BrakingDeceleration);
	const bool bZeroFriction = (Friction == 0.f);
	const bool bZeroBraking = (BrakingDeceleration == 0.f);

	if (bZeroFriction && bZeroBraking)
	{
		return;
	}

	const FVector OldVel = Velocity;

	// subdivide braking to get reasonably consistent results at lower frame rates
	// (important for packet loss situations w/ networking)
	float RemainingTime = DeltaTime;
	const float MaxTimeStep = FMath::Clamp(BrakingSubStepTime, 1.0f / 75.0f, 1.0f / 20.0f);

	// Decelerate to brake to a stop
	const FVector RevAccel = (bZeroBraking ? FVector::ZeroVector : (-BrakingDeceleration * Velocity.GetSafeNormal()));
	while (RemainingTime >= MIN_TICK_TIME)
	{
		// Zero friction uses constant deceleration, so no need for iteration.
		const float dt = ((RemainingTime > MaxTimeStep && !bZeroFriction) ? FMath::Min(MaxTimeStep, RemainingTime * 0.5f) : RemainingTime);
		RemainingTime -= dt;

		// apply friction and braking
		Velocity = Velocity + ((-Friction) * Velocity + RevAccel) * dt;

		// Don't reverse direction
		if ((Velocity | OldVel) <= 0.f)
		{
			Velocity = FVector::ZeroVector;
			return;
		}
	}

	// Clamp to zero if nearly zero, or if below min threshold and braking.
	const float VSizeSq = Velocity.SizeSquared();
	if (VSizeSq <= KINDA_SMALL_NUMBER || (!bZeroBraking && VSizeSq <= FMath::Square(BRAKE_TO_STOP_VELOCITY)))
	{
		Velocity = FVector::ZeroVector;
	}
}

// meh
bool URlCharacterMovementComponent::CheckFall(const FRlFindFloorResult& OldFloor, const FHitResult& Hit, const FVector& Delta, const FVector& OldLocation, float RemainingTime, float TimeTick, int32 Iterations)
{
	if (!HasValidData())
	{
		return false;
	}

	if (IsMovingOnGround())
	{
		// If still walking, then fall. If not, assume the user set a different mode they want to keep.
		StartFalling(Iterations, RemainingTime, TimeTick, Delta, OldLocation);
	}
	return true;
}

// meh
void URlCharacterMovementComponent::StartFalling(int32 Iterations, float RemainingTime, float TimeTick, const FVector& Delta, const FVector& subLoc)
{
	// start falling
	const float DesiredDist = Delta.Size();
	const float ActualDist = (UpdatedComponent->GetComponentLocation() - subLoc).Size2D();
	RemainingTime = (DesiredDist < KINDA_SMALL_NUMBER)
		? 0.f
		: RemainingTime + TimeTick * (1.f - FMath::Min(1.f, ActualDist / DesiredDist));

	if (IsMovingOnGround())
	{
		// This is to catch cases where the first frame of PIE is executed, and the
		// level is not yet visible. In those cases, the player will fall out of the
		// world... So, don't set ERlMovementMode::Falling straight away.
		if (!GIsEditor || (GetWorld()->HasBegunPlay() && (GetWorld()->GetTimeSeconds() >= 1.f)))
		{
			SetMovementMode(ERlMovementMode::Falling); //default behavior if script didn't change physics
		}
		else
		{
			// Make sure that the floor check code continues processing during this delay.
			bForceNextFloorCheck = true;
		}
	}
	StartNewPhysics(RemainingTime, Iterations);
}

// meh
void URlCharacterMovementComponent::RevertMove(const FVector& OldLocation, UPrimitiveComponent* OldBase, const FVector& PreviousBaseLocation, const FRlFindFloorResult& OldFloor, bool bFailMove)
{
	//UE_LOG(LogCharacterMovement, Log, TEXT("RevertMove from %f %f %f to %f %f %f"), CharacterOwner->Location.X, CharacterOwner->Location.Y, CharacterOwner->Location.Z, OldLocation.X, OldLocation.Y, OldLocation.Z);
	UpdatedComponent->SetWorldLocation(OldLocation, false, nullptr, ETeleportType::None/*GetTeleportType()*/);

	//UE_LOG(LogCharacterMovement, Log, TEXT("Now at %f %f %f"), CharacterOwner->Location.X, CharacterOwner->Location.Y, CharacterOwner->Location.Z);
	bJustTeleported = false;
	// if our previous base couldn't have moved or changed in any physics-affecting way, restore it
	if (IsValid(OldBase) &&
		(!RlMovementBaseUtility::IsDynamicBase(OldBase) ||
		(OldBase->Mobility == EComponentMobility::Static) ||
			(OldBase->GetComponentLocation() == PreviousBaseLocation)
			)
		)
	{
		CurrentFloor = OldFloor;
		SetBase(OldBase, OldFloor.HitResult.BoneName);
	}
	else
	{
		SetBase(NULL);
	}

	if (bFailMove)
	{
		// end movement now
		Velocity = FVector::ZeroVector;
		Acceleration = FVector::ZeroVector;
		//UE_LOG(LogCharacterMovement, Log, TEXT("%s FAILMOVE RevertMove"), *CharacterOwner->GetName());
	}
}

// meh
void URlCharacterMovementComponent::OnCharacterStuckInGeometry(const FHitResult* Hit)
{
	// Don't update velocity based on our (failed) change in position this update since we're stuck.
	bJustTeleported = true;
}

// meh
void URlCharacterMovementComponent::MoveHorizontal(const FVector& InVelocity, float DeltaSeconds, bool bAlongFloor)
{
	if (bAlongFloor && !CurrentFloor.IsWalkableFloor())
	{
		return;
	}

	// Move along the current floor
	const FVector Delta = FVector(InVelocity.X, InVelocity.Y, 0.f) * DeltaSeconds;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

	float LastMoveTimeSlice = DeltaSeconds;

	if (Hit.bBlockingHit)
	{
		Velocity.X = 0.f;
	}

	if (Hit.bStartPenetrating)
	{
		// Allow this hit to be used as an impact we can deflect off, otherwise we do nothing the rest of the update and appear to hitch.
		SlideAlongSurface(Delta, 1.f, Hit.Normal, Hit, true);

		if (Hit.bStartPenetrating)
		{
			OnCharacterStuckInGeometry(&Hit);
		}
	}
}

// meh
void URlCharacterMovementComponent::MaintainHorizontalGroundVelocity()
{
	if (Velocity.Z != 0.f)
	{
		// Rescale velocity to be horizontal but maintain magnitude of last update.
		Velocity = Velocity.GetSafeNormal2D() * Velocity.Size();
	}
}

// meh
void URlCharacterMovementComponent::AdjustFloorHeight()
{
	// If we have a floor check that hasn't hit anything, don't adjust height.
	if (!CurrentFloor.IsWalkableFloor())
	{
		return;
	}

	float OldFloorDist = CurrentFloor.FloorDist;
	if (CurrentFloor.bLineTrace)
	{
		if (OldFloorDist < MIN_FLOOR_DIST && CurrentFloor.LineDist >= MIN_FLOOR_DIST)
		{
			// This would cause us to scale unwalkable walls
			UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("Adjust floor height aborting due to line trace with small floor distance (line: %.2f, sweep: %.2f)"), CurrentFloor.LineDist, CurrentFloor.FloorDist);
			return;
		}
		else
		{
			// Falling back to a line trace means the sweep was unwalkable (or in penetration). Use the line distance for the vertical adjustment.
			OldFloorDist = CurrentFloor.LineDist;
		}
	}

	// Move up or down to maintain floor height.
	//if (OldFloorDist < MIN_FLOOR_DIST || OldFloorDist > MAX_FLOOR_DIST)

	// Move character to the ground
	if (!FMath::IsNearlyZero(OldFloorDist) && OldFloorDist != 2.f)
	{
		FHitResult AdjustHit(1.f);
		FVector Location = UpdatedComponent->GetComponentLocation();

		// We leave the collision box 2 pixels above the floor in order to avoid stop collisions when walking
		Location.Z = (float)UKismetMathLibrary::Round(Location.Z - OldFloorDist) + 2.f;

		UpdatedComponent->SetRelativeLocation(Location);

		CurrentFloor.FloorDist = 2.f;

		//const float AvgFloorDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5f;
		//const float MoveDist = AvgFloorDist - OldFloorDist;
		//SafeMoveUpdatedComponent(FVector(0.f, 0.f, MoveDist), UpdatedComponent->GetComponentQuat(), true, AdjustHit);
		//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("Adjust floor height %.3f (Hit = %d)"), MoveDist, AdjustHit.bBlockingHit);

		UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("Adjust floor height %.3f"), OldFloorDist);

		CurrentFloor.FloorDist = 2.f;
		//CurrentFloor.FloorDist = KINDA_SMALL_NUMBER;

		//if (!AdjustHit.IsValidBlockingHit())
		//{
		//	CurrentFloor.FloorDist += MoveDist;
		//}
		//else if (MoveDist > 0.f)
		//{
		//	const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
		//	CurrentFloor.FloorDist += CurrentZ - InitialZ;
		//}
		//else
		//{
		//	checkSlow(MoveDist < 0.f);
		//	const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
		//	CurrentFloor.FloorDist = CurrentZ - AdjustHit.Location.Z;
		//	if (IsWalkable(AdjustHit))
		//	{
		//		CurrentFloor.SetFromSweep(AdjustHit, CurrentFloor.FloorDist, true);
		//	}
		//}

		// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
		// Also avoid it if we moved out of penetration
		bJustTeleported |= (OldFloorDist < 0.f);

		// If something caused us to adjust our height (especially a depentration) we should ensure another check next frame or we will keep a stale result.
		bForceNextFloorCheck = true;

		//bForceNextFloorCheck = false;
	}
}

// meh
void URlCharacterMovementComponent::StopActiveMovement()
{
	Super::StopActiveMovement();

	Acceleration = FVector::ZeroVector;
	Velocity = FVector::ZeroVector;
}

// meh
void URlCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float RemainingTime, int32 Iterations)
{
	if (IsFalling())
	{
		SetMovementMode(GroundMovementMode);
	}

	IPathFollowingAgentInterface* PFAgent = GetPathFollowingAgent();
	if (PFAgent)
	{
		PFAgent->OnLanded();
	}

	StartNewPhysics(RemainingTime, Iterations);
}

// meh
void URlCharacterMovementComponent::OnTeleported()
{
	if (!HasValidData())
	{
		return;
	}

	Super::OnTeleported();

	bJustTeleported = true;

	// Find floor at current location
	UpdateFloorFromAdjustment();

	// Validate it. We don't want to pop down to walking mode from very high off the ground, but we'd like to keep walking if possible.
	UPrimitiveComponent* OldBase = CharacterOwner->GetMovementBase();
	UPrimitiveComponent* NewBase = NULL;

	if (OldBase && CurrentFloor.IsWalkableFloor() && CurrentFloor.FloorDist <= MAX_FLOOR_DIST && Velocity.Z <= 0.f)
	{
		// Close enough to land or just keep walking.
		NewBase = CurrentFloor.HitResult.Component.Get();
	}
	else
	{
		CurrentFloor.Clear();
	}

	const bool bWasFalling = (MovementMode == ERlMovementMode::Falling);

	if (!CurrentFloor.IsWalkableFloor() || (OldBase && !NewBase))
	{
		if (!bWasFalling)
		{
			SetMovementMode(ERlMovementMode::Falling);
		}
	}
	else if (NewBase)
	{
		if (bWasFalling)
		{
			ProcessLanded(CurrentFloor.HitResult, 0.f, 0);
		}
	}

	SaveBaseLocation();
}

// meh
void URlCharacterMovementComponent::AddImpulse(FVector Impulse, bool bVelocityChange)
{
	if (!Impulse.IsZero() && IsActive() && HasValidData())
	{
		// handle scaling by mass
		FVector FinalImpulse = Impulse;
		if (!bVelocityChange)
		{
			if (Mass > SMALL_NUMBER)
			{
				FinalImpulse = FinalImpulse / Mass;
			}
			else
			{
				UE_LOG(LogCharacterMovement, Warning, TEXT("Attempt to apply impulse to zero or negative Mass in CharacterMovement"));
			}
		}

		PendingImpulseToApply += FinalImpulse;
	}
}

// meh
void URlCharacterMovementComponent::AddForce(FVector Force)
{
	if (!Force.IsZero() && IsActive() && HasValidData())
	{
		if (Mass > SMALL_NUMBER)
		{
			PendingForceToApply += Force / Mass;
		}
		else
		{
			UE_LOG(LogCharacterMovement, Warning, TEXT("Attempt to apply force to zero or negative Mass in CharacterMovement"));
		}
	}
}

bool URlCharacterMovementComponent::IsWalkable(const FHitResult& Hit) const
{
	if (!Hit.IsValidBlockingHit())
	{
		// No hit, or starting in penetration
		return false;
	}

	// Never walk up vertical surfaces.
	return Hit.ImpactNormal.Z > KINDA_SMALL_NUMBER;
}

bool URlCharacterMovementComponent::IsWithinEdgeTolerance(const FVector& BoxLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const
{
	const float DistFromCenterSq = (TestImpactPoint - BoxLocation).SizeSquared2D();
	const float ReducedRadiusSq = FMath::Square(FMath::Max(SWEEP_EDGE_REJECT_DISTANCE + KINDA_SMALL_NUMBER, CapsuleRadius - SWEEP_EDGE_REJECT_DISTANCE));
	return DistFromCenterSq < ReducedRadiusSq;
}

// meh
void URlCharacterMovementComponent::ComputeFloorDist(const FVector& BoxLocation, float LineDistance, float SweepDistance, FRlFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
{
	OutFloorResult.Clear();

	//float PawnRadius = CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().X;
	float PawnHalfWidth = CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().X;
	float PawnHalfHeight = CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().Z;
	//float PawnRadius, PawnHalfHeight;
	//CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	bool bSkipSweep = false;
	if (DownwardSweepResult != NULL && DownwardSweepResult->IsValidBlockingHit())
	{
		// Only if the supplied sweep was vertical and downward.
		if ((DownwardSweepResult->TraceStart.Z > DownwardSweepResult->TraceEnd.Z) &&
			(DownwardSweepResult->TraceStart - DownwardSweepResult->TraceEnd).SizeSquared2D() <= KINDA_SMALL_NUMBER)
		{
			// Reject hits that are barely on the cusp of the radius of the capsule
			//if (IsWithinEdgeTolerance(DownwardSweepResult->Location, DownwardSweepResult->ImpactPoint, PawnRadius))
			//{
			//	// Don't try a redundant sweep, regardless of whether this sweep is usable.
			//	bSkipSweep = true;

			//	const bool bIsWalkable = IsWalkable(*DownwardSweepResult);
			//	const float FloorDist = (BoxLocation.Z - DownwardSweepResult->Location.Z);
			//	OutFloorResult.SetFromSweep(*DownwardSweepResult, FloorDist, bIsWalkable);

			//	if (bIsWalkable)
			//	{
			//		// Use the supplied downward sweep as the floor hit result.
			//		return;
			//	}
			//}
		}
	}

	// We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
	if (SweepDistance < LineDistance)
	{
		ensure(SweepDistance >= LineDistance);
		return;
	}

	bool bBlockingHit = false;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ComputeFloorDist), false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	// Sweep test
	if (!bSkipSweep && SweepDistance > 0.f && SweepRadius > 0.f)
	{
		// Use a shorter height to avoid sweeps giving weird results if we start on a surface.
		// This also allows us to adjust out of penetrations.
		//const float ShrinkScale = 0.9f;
		//const float ShrinkScaleOverlap = 0.1f;
		//float ShrinkHeight = FMath::Abs(PawnHalfHeight - PawnRadius) * (1.f - ShrinkScale);
		//float TraceDist = SweepDistance + ShrinkHeight;
		//FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, PawnHalfHeight - ShrinkHeight);

		FHitResult Hit(1.f);
		//bBlockingHit = FloorSweepTest(Hit, BoxLocation, BoxLocation + FVector(0.f, 0.f, -TraceDist), CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
		bBlockingHit = FloorSweepTest(Hit, BoxLocation, BoxLocation + FVector(0.f, 0.f, -SweepDistance), CollisionChannel, CharacterOwner->GetBoxComponent()->GetCollisionShape(), QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			// Reject hits adjacent to us, we only care about hits on the bottom portion of our capsule.
			// Check 2D distance to impact point, reject if within a tolerance from radius.
			//if (Hit.bStartPenetrating || !IsWithinEdgeTolerance(BoxLocation, Hit.ImpactPoint, CapsuleShape.Capsule.Radius))
			//{
			//	// Use a capsule with a slightly smaller radius and shorter height to avoid the adjacent object.
			//	// Capsule must not be nearly zero or the trace will fall back to a line trace from the start point and have the wrong length.
			//	CapsuleShape.Capsule.Radius = FMath::Max(0.f, CapsuleShape.Capsule.Radius - SWEEP_EDGE_REJECT_DISTANCE - KINDA_SMALL_NUMBER);
			//	if (!CapsuleShape.IsNearlyZero())
			//	{
			//		ShrinkHeight = FMath::Abs(PawnHalfHeight - PawnRadius) * (1.f - ShrinkScaleOverlap);
			//		TraceDist = SweepDistance + ShrinkHeight;
			//		CapsuleShape.Capsule.HalfHeight = FMath::Max(FMath::Abs(PawnHalfHeight - ShrinkHeight), CapsuleShape.Capsule.Radius);
			//		Hit.Reset(1.f, false);

			//		bBlockingHit = FloorSweepTest(Hit, BoxLocation, BoxLocation + FVector(0.f, 0.f, -TraceDist), CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
			//	}
			//}

			// Reduce hit distance by ShrinkHeight because we shrank the capsule for the trace.
			// We allow negative distances here, because this allows us to pull out of penetrations.
			//const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, 2.f * PawnHalfWidth);
			//const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);
			const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * SweepDistance);

			OutFloorResult.SetFromSweep(Hit, SweepResult, false);
			if (Hit.IsValidBlockingHit() && IsWalkable(Hit))
			{
				if (SweepResult <= SweepDistance)
				{
					// Hit within test distance.
					OutFloorResult.bWalkableFloor = true;
					return;
				}
			}
		}
	}

	// Since we require a longer sweep than line trace, we don't want to run the line trace if the sweep missed everything.
	// We do however want to try a line trace if the sweep was stuck in penetration.
	if (!OutFloorResult.bBlockingHit && !OutFloorResult.HitResult.bStartPenetrating)
	{
		OutFloorResult.FloorDist = SweepDistance;
		return;
	}

	// Line trace
	if (LineDistance > 0.f)
	{
		//const float ShrinkHeight = PawnHalfHeight;
		//const FVector LineTraceStart = BoxLocation;
		//const float TraceDist = LineDistance + ShrinkHeight;
		//const FVector Down = FVector(0.f, 0.f, -TraceDist);
		//QueryParams.TraceTag = SCENE_QUERY_STAT_NAME_ONLY(FloorLineTrace);

		//FHitResult Hit(1.f);
		//bBlockingHit = GetWorld()->LineTraceSingleByChannel(Hit, LineTraceStart, LineTraceStart + Down, CollisionChannel, QueryParams, ResponseParam);

		//if (bBlockingHit)
		//{
		//	if (Hit.Time > 0.f)
		//	{
		//		// Reduce hit distance by ShrinkHeight because we started the trace higher than the base.
		//		// We allow negative distances here, because this allows us to pull out of penetrations.
		//		const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
		//		const float LineResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

		//		OutFloorResult.bBlockingHit = true;
		//		if (LineResult <= LineDistance && IsWalkable(Hit))
		//		{
		//			OutFloorResult.SetFromLineTrace(Hit, OutFloorResult.FloorDist, LineResult, true);
		//			return;
		//		}
		//	}
		//}
	}

	// No hits were acceptable.
	OutFloorResult.bWalkableFloor = false;
	OutFloorResult.FloorDist = SweepDistance;
}

// meh
void URlCharacterMovementComponent::FindFloor(const FVector& BoxLocation, FRlFindFloorResult& OutFloorResult, bool bCanUseCachedLocation, const FHitResult* DownwardSweepResult) const
{
	// No collision, no floor...
	if (!HasValidData() || !UpdatedComponent->IsQueryCollisionEnabled())
	{
		OutFloorResult.Clear();
		return;
	}

	UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("[Role:%d] FindFloor: %s at location %s"), (int32)CharacterOwner->Role, *GetNameSafe(CharacterOwner), *BoxLocation.ToString());
	check(CharacterOwner->GetBoxComponent());

	// Increase height check slightly if walking, to prevent floor height adjustment from later invalidating the floor result.
	const float HeightCheckAdjust = (IsMovingOnGround() ? MAX_FLOOR_DIST + KINDA_SMALL_NUMBER : -MAX_FLOOR_DIST);

	float FloorSweepTraceDist = FMath::Max(MAX_FLOOR_DIST,/* MaxStepHeight +*/ HeightCheckAdjust);
	float FloorLineTraceDist = FloorSweepTraceDist;
	bool bNeedToValidateFloor = true;

	// Sweep floor
	if (FloorLineTraceDist > 0.f || FloorSweepTraceDist > 0.f)
	{
		URlCharacterMovementComponent* MutableThis = const_cast<URlCharacterMovementComponent*>(this);

		if (bAlwaysCheckFloor || !bCanUseCachedLocation || bForceNextFloorCheck || bJustTeleported)
		{
			MutableThis->bForceNextFloorCheck = false;
			ComputeFloorDist(BoxLocation, FloorLineTraceDist, FloorSweepTraceDist, OutFloorResult, CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().X/*CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius()*/, DownwardSweepResult);
		}
		else
		{
			// Force floor check if base has collision disabled or if it does not block us.
			UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
			const AActor* BaseActor = MovementBase ? MovementBase->GetOwner() : NULL;
			const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

			if (MovementBase != NULL)
			{
				MutableThis->bForceNextFloorCheck = !MovementBase->IsQueryCollisionEnabled()
					|| MovementBase->GetCollisionResponseToChannel(CollisionChannel) != ECR_Block
					|| RlMovementBaseUtility::IsDynamicBase(MovementBase);
			}

			const bool IsActorBasePendingKill = BaseActor && BaseActor->IsPendingKill();

			if (!bForceNextFloorCheck && !IsActorBasePendingKill && MovementBase)
			{
				//UE_LOG(LogCharacterMovement, Log, TEXT("%s SKIP check for floor"), *CharacterOwner->GetName());
				OutFloorResult = CurrentFloor;
				bNeedToValidateFloor = false;
			}
			else
			{
				MutableThis->bForceNextFloorCheck = false;
				ComputeFloorDist(BoxLocation, FloorLineTraceDist, FloorSweepTraceDist, OutFloorResult, CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().X/*CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius()*/, DownwardSweepResult);
			}
		}
	}
}

// meh
bool URlCharacterMovementComponent::FloorSweepTest(
	FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	ECollisionChannel TraceChannel,
	const struct FCollisionShape& CollisionShape,
	const struct FCollisionQueryParams& Params,
	const struct FCollisionResponseParams& ResponseParam
) const
{
	return GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, TraceChannel, CollisionShape, Params, ResponseParam);
}

bool URlCharacterMovementComponent::IsValidLandingSpot(const FVector& BoxLocation, const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	// Skip some checks if penetrating. Penetration will be handled by the FindFloor call (using a smaller capsule)
	if (!Hit.bStartPenetrating)
	{
		// Reject unwalkable floor normals.
		if (!IsWalkable(Hit))
		{
			return false;
		}

		float PawnHalfWidth = CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().X;
		float PawnHalfHeight = CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().Z;
		//float PawnRadius, PawnHalfHeight;
		//CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

		// The it is using the same geometry as the main component. Previously a capsule, now a box
		//// Reject hits that are above our lower hemisphere (can happen when sliding down a vertical surface).
		//const float LowerHemisphereZ = Hit.Location.Z - PawnHalfHeight + PawnRadius;

		// Only the hits that are on the lower quarter of the box are processed.
		const float LowerHemisphereZ = Hit.Location.Z - PawnHalfHeight / 2.f;
		if (Hit.ImpactPoint.Z >= LowerHemisphereZ)
		{
			return false;
		}

		// Reject hits that are barely on the cusp of the radius of the capsule
		//if (!IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnHalfWidth))
		//{
		//	return false;
		//}
	}
	else
	{
		// Penetrating
		if (Hit.Normal.Z < KINDA_SMALL_NUMBER)
		{
			// Normal is nearly horizontal or downward, that's a penetration adjustment next to a vertical or overhanging wall. Don't pop to the floor.
			return false;
		}
	}

	FRlFindFloorResult FloorResult;
	FindFloor(BoxLocation, FloorResult, false, &Hit);

	if (!FloorResult.IsWalkableFloor())
	{
		return false;
	}

	return true;
}

bool URlCharacterMovementComponent::ShouldCheckForValidLandingSpot(float DeltaTime, const FVector& Delta, const FHitResult& Hit) const
{
	// See if we hit an edge of a surface on the lower portion of the capsule.
	// In this case the normal will not equal the impact normal, and a downward sweep may find a walkable surface on top of the edge.
	if (Hit.Normal.Z > KINDA_SMALL_NUMBER && !Hit.Normal.Equals(Hit.ImpactNormal))
	{
		const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
		if (IsWithinEdgeTolerance(PawnLocation, Hit.ImpactPoint, CharacterOwner->GetBoxComponent()->GetScaledBoxExtent().X/*CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius()*/))
		{
			return true;
		}
	}

	return false;
}

// meh
FString URlCharacterMovementComponent::GetMovementName() const
{
	if (CharacterOwner)
	{
		if (CharacterOwner->GetRootComponent() && CharacterOwner->GetRootComponent()->IsSimulatingPhysics())
		{
			return TEXT("Rigid Body");
		}
		else if (CharacterOwner->IsMatineeControlled())
		{
			return TEXT("Matinee");
		}
	}

	// Using character movement
	switch (MovementMode)
	{
	case ERlMovementMode::Walking:			return TEXT("Walking"); break;
	case ERlMovementMode::Falling:			return TEXT("Falling"); break;
	case ERlMovementMode::WallWalking:			return TEXT("Wall Walking"); break;
	}
	return TEXT("Unknown");
}

// meh
FVector URlCharacterMovementComponent::ConstrainInputAcceleration(const FVector& InputAcceleration) const
{
	// walking or falling pawns ignore up/down sliding
	if (InputAcceleration.Z != 0.f && (IsMovingOnGround() || IsFalling()))
	{
		return FVector(InputAcceleration.X, InputAcceleration.Y, 0.f);
	}

	return InputAcceleration;
}

// meh
FVector URlCharacterMovementComponent::ScaleInputAcceleration(const FVector& InputAcceleration) const
{
	return GetMaxAcceleration() * InputAcceleration.GetClampedToMaxSize(1.0f);
}

// meh
void URlCharacterMovementComponent::SprintStart()
{
	if (CharacterOwner->bRunEnabled)
	{
		CharacterOwner->bIsSprinting = true;
		MaxAcceleration = SprintMaxAcceleration;
		MaxWalkSpeed = SprintMaxWalkSpeed;
	}
}

// meh
void URlCharacterMovementComponent::SprintStop()
{
	CharacterOwner->bIsSprinting = false;
	MaxAcceleration = NormalMaxAcceleration;
	MaxWalkSpeed = NormalMaxWalkSpeed;
	bSprintStop = true;
}

// meh
bool URlCharacterMovementComponent::CheckSpikes()
{
	FVector Start = UpdatedComponent->GetComponentLocation();

	Start.X += FMath::Sign(Velocity.X) * 9.f;
	Start.Z -= 12.f;

	FVector End = Start;
	End.X -= FMath::Sign(Velocity.X) * 4.f;

	TArray<FHitResult> OutHits;
	UKismetSystemLibrary::LineTraceMulti(this, Start, End, UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_WorldStatic), false, TArray<AActor*>(), EDrawDebugTrace::None, OutHits, true);
	if (OutHits.Num() == 1)
	{
		if (Cast<ASpike>(OutHits[0].Actor))
		{
			CharacterOwner->Death();
			return true;
		}
	}

	return false;
}

// meh
float URlCharacterMovementComponent::ComputeAnalogInputModifier() const
{
	const float MaxAccel = GetMaxAcceleration();
	if (Acceleration.SizeSquared() > 0.f && MaxAccel > SMALL_NUMBER)
	{
		return FMath::Clamp(Acceleration.Size() / MaxAccel, 0.f, 1.f);
	}

	return 0.f;
}

// meh
float URlCharacterMovementComponent::GetAnalogInputModifier() const
{
	return AnalogInputModifier;
}

// meh
float URlCharacterMovementComponent::GetSimulationTimeStep(float RemainingTime, int32 Iterations) const
{
	if (RemainingTime > MaxSimulationTimeStep)
	{
		if (Iterations < MaxSimulationIterations)
		{
			// Subdivide moves to be no longer than MaxSimulationTimeStep seconds
			RemainingTime = FMath::Min(MaxSimulationTimeStep, RemainingTime * 0.5f);
		}
		else
		{
			// If this is the last iteration, just use all the remaining time. This is usually better than cutting things short, as the simulation won't move far enough otherwise.
		}
	}

	// no less than MIN_TICK_TIME (to avoid potential divide-by-zero during simulation).
	return FMath::Max(MIN_TICK_TIME, RemainingTime);
}

// meh
void URlCharacterMovementComponent::UpdateFloorFromAdjustment()
{
	if (!HasValidData())
	{
		return;
	}

	// If walking, try to update the cached floor so it is current. This is necessary for UpdateBasedMovement() and MoveAlongFloor() to work properly.
	// If base is now NULL, presumably we are no longer walking. If we had a valid floor but don't find one now, we'll likely start falling.
	if (CharacterOwner->GetMovementBase())
	{
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
	}
	else
	{
		CurrentFloor.Clear();
	}
}

// meh
void URlCharacterMovementComponent::ApplyAccumulatedForces(float DeltaSeconds)
{
	if (PendingImpulseToApply.Z != 0.f || PendingForceToApply.Z != 0.f)
	{
		// check to see if applied momentum is enough to overcome gravity
		if (IsMovingOnGround() && (PendingImpulseToApply.Z + (PendingForceToApply.Z * DeltaSeconds) + (GetGravityZ() * DeltaSeconds) > SMALL_NUMBER))
		{
			SetMovementMode(ERlMovementMode::Falling);
		}
	}

	Velocity += PendingImpulseToApply + (PendingForceToApply * DeltaSeconds);

	// Don't call ClearAccumulatedForces() because it could affect launch velocity
	PendingImpulseToApply = FVector::ZeroVector;
	PendingForceToApply = FVector::ZeroVector;
}

// meh
void URlCharacterMovementComponent::ClearAccumulatedForces()
{
	PendingImpulseToApply = FVector::ZeroVector;
	PendingForceToApply = FVector::ZeroVector;
	PendingLaunchVelocity = FVector::ZeroVector;
}

// meh
void URlCharacterMovementComponent::AddRadialForce(const FVector& Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff)
{
	FVector Delta = UpdatedComponent->GetComponentLocation() - Origin;
	const float DeltaMagnitude = Delta.Size();

	// Do nothing if outside radius
	if (DeltaMagnitude > Radius)
	{
		return;
	}

	Delta = Delta.GetSafeNormal();

	float ForceMagnitude = Strength;
	if (Falloff == RIF_Linear && Radius > 0.0f)
	{
		ForceMagnitude *= (1.0f - (DeltaMagnitude / Radius));
	}

	AddForce(Delta * ForceMagnitude);
}

// meh
void URlCharacterMovementComponent::AddRadialImpulse(const FVector& Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bVelChange)
{
	FVector Delta = UpdatedComponent->GetComponentLocation() - Origin;
	const float DeltaMagnitude = Delta.Size();

	// Do nothing if outside radius
	if (DeltaMagnitude > Radius)
	{
		return;
	}

	Delta = Delta.GetSafeNormal();

	float ImpulseMagnitude = Strength;
	if (Falloff == RIF_Linear && Radius > 0.0f)
	{
		ImpulseMagnitude *= (1.0f - (DeltaMagnitude / Radius));
	}

	AddImpulse(Delta * ImpulseMagnitude, bVelChange);
}