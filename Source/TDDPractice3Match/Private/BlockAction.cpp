// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/BlockAction.h"
#include "../Public/BlockPhysics.h"
#include "../Public/MyMathUtils.h"

void GetsDestroyedBlockAction::Tick(float deltaSeconds)
{
	elapsedTime += deltaSeconds;
	if (elapsedTime >= BlockPhysics::DESTROY_ANIMATION_TIME)
		completed = true;
}

SwipeMoveBlockAction::SwipeMoveBlockAction(FIntPoint initialPos, FIntPoint destPos)
	: BlockAction(initialPos), initialPos(initialPos), destPos(destPos)
{

}

void SwipeMoveBlockAction::Tick(float deltaSeconds)
{
	const auto distanceToDestination = (FVector2D(destPos) - position).Size();
	const auto moveDistance = deltaSeconds * BlockPhysics::SWIPE_MOVE_SPEED;
	if (distanceToDestination < moveDistance) {
		position = destPos;
		isJustCompleted = true;
	}
	else {
		auto moveDirection = FVector2D(destPos - initialPos);
		moveDirection.Normalize();
		position += moveDirection * moveDistance;
	}
}

TUniquePtr<BlockAction> SwipeMoveBlockAction::GetNextAction(bool thereIsAMatch) const
{
	if (thereIsAMatch) {
		return MakeUnique<IdleBlockAction>(position);
	}
	else {
		return MakeUnique<SwipeReturnBlockAction>(destPos, initialPos);
	}
}

FallingBlockAction::FallingBlockAction(FIntPoint initialPos, FIntPoint destPos)
	:BlockAction(initialPos), initialPos(initialPos), destPos(destPos)
{

}

void FallingBlockAction::Tick(float deltaSeconds)
{
	const auto averageSpeed = currentSpeed + deltaSeconds * BlockPhysics::GRAVITY_ACCELERATION / 2.f;
	const auto fallDistance = averageSpeed * deltaSeconds;
	const auto distanceLeftToGo = (FVector2D(destPos) - position).Size();
	if (distanceLeftToGo < fallDistance) {
		position = destPos;
		isJustCompleted = true;
	}
	else {
		auto fallDirection = FVector2D(destPos - initialPos);
		fallDirection.Normalize();
		position += fallDirection * fallDistance;
	}
	currentSpeed += deltaSeconds * BlockPhysics::GRAVITY_ACCELERATION;
}

SwipeReturnBlockAction::SwipeReturnBlockAction(FIntPoint initialPos, FIntPoint destPos)
	: BlockAction(initialPos), initialPos(initialPos), destPos(destPos)
{

}

void SwipeReturnBlockAction::Tick(float deltaSeconds)
{
	const auto distanceToDestination = (FVector2D(destPos) - position).Size();
	const auto moveDistance = deltaSeconds * BlockPhysics::SWIPE_MOVE_SPEED;
	if (distanceToDestination < moveDistance) {
		position = destPos;
		isJustCompleted = true;
	}
	else {
		auto moveDirection = FVector2D(destPos - initialPos);
		moveDirection.Normalize();
		position += moveDirection * moveDistance;
	}
}

FString PrettyPrint(ActionType actionType)
{
	switch (actionType) {
	case ActionType::Idle:
		return TEXT("Idle");
	case ActionType::SwipeMove:
		return TEXT("SwipeMove");
	case ActionType::SwipeReturn:
		return TEXT("SwipeReturn");
	case ActionType::Fall:
		return TEXT("Fall");
	case ActionType::GetsDestroyed:
		return TEXT("GetsDestroyed");
	default:
		return TEXT("");
	}
}

GetsDestroyedAndSpawnBlockAfterAction::GetsDestroyedAndSpawnBlockAfterAction(FVector2D initialPos, Block blockToSpawnAfterDestroy)
	: GetsDestroyedBlockAction(initialPos), blockToSpawnAfterDestroy(blockToSpawnAfterDestroy)
{

}

TUniquePtr<BlockAction> GetsDestroyedAndSpawnBlockAfterAction::GetNextAction(bool thereIsAMatch) const
{
	return MakeUnique<IdleBlockAction>(position);
}

MunchickenRollAction::MunchickenRollAction(FVector2D initialPos, FIntPoint rollDirection, BlockPhysics& blockPhysics, int rollableId)
	: BlockAction(initialPos), lastRolledOverPosition(BlockPhysics::ToFIntPoint(initialPos)), rollDirection(rollDirection), blockPhysics(blockPhysics), rollableId(rollableId)
{
	if (rollDirection.X == 0)
		rollType = Horizontal;
	else if (rollDirection.Y == 0)
		rollType = Vertical;
	else {
		UE_LOG(LogTemp, Warning, TEXT("Initialized MunchickenRollAction with wrong rollDirection: (%d, %d)"), rollDirection.X, rollDirection.Y);
		rollType = Invalid;
	}
}

void MunchickenRollAction::Tick(float deltaSeconds)
{
	previousPosition = position;
	UpdatePosition(deltaSeconds);
	const auto cellPositionsRolledOver = GetCellPositionsRolledOver();
	for (const auto& rolledOverPosition : cellPositionsRolledOver)
		lastRolledOverPosition = rolledOverPosition;
	ApplyRollOverEffectAt(cellPositionsRolledOver);
}

bool MunchickenRollAction::IsJustCompleted() const
{
	return IsOutOfTheMap();
}

bool MunchickenRollAction::ShouldCheckMatch() const
{
	return false;
}

bool MunchickenRollAction::IsEligibleForMatching() const
{
	return false;
}

TUniquePtr<BlockAction> MunchickenRollAction::GetNextAction(bool thereIsAMatch) const
{
	return MakeUnique<GetsDestroyedBlockAction>(position);
}

ActionType MunchickenRollAction::GetType() const
{
	return ActionType::Roll;
}

void MunchickenRollAction::UpdatePosition(float deltaSeconds)
{
	const auto rollDistance = BlockPhysics::ROLL_SPEED * deltaSeconds;
	position += FVector2D(rollDirection) * rollDistance;
}

TSet<FIntPoint> MunchickenRollAction::GetCellPositionsRolledOver() const
{
	TSet<FIntPoint> ret;
	switch (rollType) {
	case Horizontal: {
		const auto movingRow = BlockPhysics::ToFIntPoint(position).X;
		const auto rolledOverCols = GetIntegersBetween(previousPosition.Y, position.Y);
		for (const auto& rolledOverCol : rolledOverCols) {
			const auto rolledOverCellPos = FIntPoint{ movingRow, rolledOverCol };
			ret.Add(rolledOverCellPos);
		}
		break;
	}
	case Vertical: {
		const auto movingCol = BlockPhysics::ToFIntPoint(position).Y;
		const auto rolledOverRows = GetIntegersBetween(previousPosition.X, position.X);
		for (const auto& rolledOverRow : rolledOverRows) {
			const auto rolledOverCellPos = FIntPoint{ rolledOverRow, movingCol };
			ret.Add(rolledOverCellPos);
		}
		break;
	}
	default: {
	}
	}
	return ret;
}

TSet<int> MunchickenRollAction::GetIntegersBetween(float bound1, float bound2)
{
	// Want to output N s.t. lowerBound <= N < upperBound

	float lowerBound = FGenericPlatformMath::Min(bound1, bound2);
	float upperBound = FGenericPlatformMath::Max(bound1, bound2);
	int lowerIntBound = FGenericPlatformMath::CeilToInt(lowerBound);
	int upperIntBound = FGenericPlatformMath::CeilToInt(upperBound);

	TSet<int> ret;
	for (int i = lowerIntBound; i < upperIntBound; i++)
		ret.Add(i);
	return ret;
}

void MunchickenRollAction::ApplyRollOverEffectAt(const TSet<FIntPoint>& destroyPositions)
{
	for (const auto& destroyPosition : destroyPositions) {
		UE_LOG(LogTemp, Display, TEXT("destroyed by munchicken at (%d, %d)"), destroyPosition.X, destroyPosition.Y);
	}
	blockPhysics.ApplyRollOverEffectAt(destroyPositions, TSet<int>{rollableId}, rollDirection);
}

bool MunchickenRollAction::IsOutOfTheMap() const
{
	const auto rowNum = blockPhysics.GetNumRows();
	const auto colNum = blockPhysics.GetNumCols();
	const auto square = MyMathUtils::Square(FIntPoint(-1, -1), FIntPoint(rowNum, colNum));
	if (!square.Includes(position))
		return true;
	return false;
}

const FIntPoint GetsDestroyedInBackgroundBlockAction::INVALID_POSITION = { INT_MIN, INT_MIN };
