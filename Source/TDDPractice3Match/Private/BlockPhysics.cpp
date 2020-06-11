// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/BlockPhysics.h"
#include "GenericPlatform/GenericPlatformMath.h"

BlockPhysics::BlockPhysics(const BlockMatrix& blockMatrix)
{
	const auto block2DArray = blockMatrix.GetBlock2DArray();
	numRows = block2DArray.Num();
	numCols = numRows == 0 ? 0 : block2DArray[0].Num();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			blocks.Add(BlockPhysicalStatus(block2DArray[i][j], FIntPoint{ i, j }));
		}
	}
}

BlockPhysics::BlockPhysics(BlockPhysics&& other)
	:blocks(MoveTemp(other.blocks)), numRows(other.numRows), numCols(other.numCols)
{

}

BlockPhysics::~BlockPhysics()
{
}

void BlockPhysics::Tick(float deltaSeconds)
{
	elapsedTime += deltaSeconds;
	UE_LOG(LogTemp, Display, TEXT("Tick start. Elapsed time: %f"), elapsedTime);
	bool needToCheckMatch = false;
	// Tick actions
	for (auto& block : blocks) {
		block.currentAction->Tick(deltaSeconds);
		if (block.currentAction->IsJustCompleted()) {
			UE_LOG(LogTemp, Display, TEXT("action completed. type: %s"), *PrettyPrint(block.currentAction->GetType()));
		}
		if (block.currentAction->ShouldCheckMatch()) {
			needToCheckMatch = true;
		}
	}
	// Check match
	auto thereIsAMatch = false;
	if (needToCheckMatch) {
		UE_LOG(LogTemp, Display, TEXT("match check"));
		auto blockMatrix = GetBlockMatrix();
		thereIsAMatch = !blockMatrix.HasNoMatch();
		if (thereIsAMatch) {
			UE_LOG(LogTemp, Display, TEXT("match occured"));
			blockMatrix.ProcessMatch();
			UpdateBlockStatus(blockMatrix);
		}
	}
	// Remove dead blocks
	blocks.RemoveAll([](const BlockPhysicalStatus& target) -> bool {
		return target.currentAction->ShouldBeRemoved();
		});
	// Set next action
	for (auto& block : blocks) {
		if (block.currentAction->IsJustCompleted()) {
			block.currentAction = block.currentAction->GetNextAction(thereIsAMatch);
		}
	}
	// Set blocks to falling && Generate new blocks
	for (int col = 0; col < numCols; col++) {
		if (NumBlocksInColumn(col) < numCols) {
			UE_LOG(LogTemp, Display, TEXT("Generating blocks to fill the empty cells.."));
			int destinationRowIndex = numRows - 1;
			int blockToFallRowIndex = numRows - 1;
			while (destinationRowIndex >= 0) {
				if ((blockToFallRowIndex >= 0) && IsEmpty(FIntPoint{ blockToFallRowIndex, col })) {
					blockToFallRowIndex--;
					continue;
				}
				else {
					if (blockToFallRowIndex < destinationRowIndex) {
						const auto blockToFallPos = FIntPoint{ blockToFallRowIndex, col };
						const auto destinationPos = FIntPoint{ destinationRowIndex, col };
						auto blockToFall = GetBlockAt(blockToFallPos);
						if (blockToFallRowIndex < 0) {
							blocks.Add(BlockPhysicalStatus(GetRandomBlock(), blockToFallPos, MakeUnique<FallingBlockAction>(blockToFallPos, destinationPos)));
						}
						else if (blockToFall == nullptr) {
							UE_LOG(LogTemp, Warning, TEXT("Queryed GetBlockAt with non-empty location and got nullptr: (%d, %d)"), blockToFallRowIndex, col);
						}
						else {
							blockToFall->currentAction = MakeUnique<FallingBlockAction>(blockToFallPos, destinationPos);
						}
					}
					destinationRowIndex--;
					blockToFallRowIndex--;
				}
			}
		}
	}
}

void BlockPhysics::RecieveSwipeInput(FIntPoint swipeStart, FIntPoint swipeEnd)
{
	auto startBlockStatus = GetBlockAt(swipeStart);
	auto endBlockStatus = GetBlockAt(swipeEnd);
	if ((startBlockStatus == nullptr) || (endBlockStatus == nullptr)) {
		UE_LOG(LogTemp, Warning, TEXT("RecieveSwipeInput precondition: blocks should exist at start and end points: (%d, %d)->(%d, %d)"),
			swipeStart.X, swipeStart.Y, swipeEnd.X, swipeEnd.Y);
		return;
	}
	startBlockStatus->currentAction = MakeUnique<SwipeMoveBlockAction>(swipeStart, swipeEnd);
	endBlockStatus->currentAction = MakeUnique<SwipeMoveBlockAction>(swipeEnd, swipeStart);
}

bool BlockPhysics::IsEmpty(FIntPoint position) const
{
	return GetBlockAt(position) == nullptr;
}

bool BlockPhysics::ExistsBlockBetween(FIntPoint startPos, FIntPoint endPos) const
{
	for (const auto& block : blocks) {
		const auto blockPos = block.currentAction->GetPosition();
		auto startToEnd = FVector2D(endPos - startPos);
		if (startToEnd.IsNearlyZero(DELTA_DISTANCE))
			return true;
		startToEnd.Normalize();
		auto startToBlockPos = FVector2D(blockPos - startPos);
		if (startToBlockPos.IsNearlyZero(DELTA_DISTANCE))
			return true;
		startToBlockPos.Normalize();
		const auto dotProduct = FVector2D::DotProduct(startToEnd, startToBlockPos);
		if (FGenericPlatformMath::Abs(dotProduct + 1) < DELTA_COSINE)
			return true;
	}
	return false;
}

bool BlockPhysics::IsPlayingDestroyAnimAt(FIntPoint position) const
{
	auto blockStatus = GetBlockAt(position);
	if (blockStatus == nullptr)
		return false;
	return blockStatus->currentAction->GetType() == ActionType::GetsDestroyed;
}

bool BlockPhysics::IsIdleAt(FIntPoint position) const
{
	auto pBlock = GetBlockAt(position);
	return (pBlock != nullptr) && (pBlock->currentAction->GetType() == ActionType::Idle);
}

BlockPhysicalStatus* BlockPhysics::GetBlockAt(FIntPoint position)
{
	for (auto& block : blocks) {
		if ((block.currentAction->GetPosition() - position).SizeSquared() <= DELTA_DISTANCE) {
			return &block;
		}
	}
	return nullptr;
}

const BlockPhysicalStatus* BlockPhysics::GetBlockAt(FIntPoint position) const
{
	for (const auto& block : blocks) {
		if ((block.currentAction->GetPosition() - position).SizeSquared() <= DELTA_DISTANCE) {
			return &block;
		}
	}
	return nullptr;
}

BlockMatrix BlockPhysics::GetBlockMatrix() const
{
	auto blockMatrix = TMap<FIntPoint, Block>();
	for (const auto& block : blocks) {
		if (block.currentAction->IsEligibleForMatching()) {
			blockMatrix.Add(ToFIntPoint(block.currentAction->GetPosition()), block.block);
		}
	}
	return BlockMatrix(numRows, numCols, blockMatrix);
}

void BlockPhysics::UpdateBlockStatus(const BlockMatrix& blockMatrix)
{
	const auto updatedBlocks = blockMatrix.GetBlock2DArray();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			if (updatedBlocks[i][j] == Block::INVALID) {
				auto blockStatus = GetBlockAt(FIntPoint{ i, j });
				if (blockStatus == nullptr) {
					UE_LOG(LogTemp, Warning, TEXT("block to update does not exist at (%d, %d)"), i, j);
					continue;
				}
				UE_LOG(LogTemp, Display, TEXT("block to destroy at (%d, %d)"), i, j);
				blockStatus->currentAction = MakeUnique<GetsDestroyedBlockAction>(blockStatus->currentAction->GetPosition());
			}
		}
	}
}

int BlockPhysics::NumBlocksInColumn(int colIndex)
{
	auto count = 0;
	auto occupiedRowIndices = TSet<int>();
	for (const auto& block : blocks) {
		if (FGenericPlatformMath::Abs(block.currentAction->GetOccupiedPosition().Y - colIndex) < DELTA_DISTANCE) {
			occupiedRowIndices.Add(ToInt(block.currentAction->GetOccupiedPosition().X));
			count++;
		}
	}
	if (count != occupiedRowIndices.Num()) {
		UE_LOG(LogTemp, Warning, TEXT("'count' and 'occupied row indices count' differ"));
	}
	return count;
}

FIntPoint BlockPhysics::ToFIntPoint(FVector2D position)
{
	return FIntPoint{ ToInt(position.X), ToInt(position.Y) };
}

int BlockPhysics::ToInt(float value)
{
	return FGenericPlatformMath::RoundToInt(value);
}

Block BlockPhysics::GetRandomBlock()
{
	return Block(rand() % static_cast<int>(Block::MAX));
}

BlockPhysicalStatus::BlockPhysicalStatus(Block block, FIntPoint initialPosition, TUniquePtr<BlockAction>&& action)
	: block(block), currentAction(MoveTemp(action)), id(++lastIssuedId)
{
}

BlockPhysicalStatus::BlockPhysicalStatus(Block block, FIntPoint initialPosition)
	: block(block), currentAction(MakeUnique<IdleBlockAction>(initialPosition)), id(++lastIssuedId)
{

}

BlockPhysicalStatus::BlockPhysicalStatus(BlockPhysicalStatus&& other)
	: block(other.block), currentAction(MoveTemp(other.currentAction)), id(++lastIssuedId)
{

}

int BlockPhysicalStatus::lastIssuedId = -1;

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
		position += moveDirection * BlockPhysics::SWIPE_MOVE_SPEED;
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
