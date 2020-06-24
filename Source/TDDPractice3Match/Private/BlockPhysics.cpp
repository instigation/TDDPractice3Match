// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/BlockPhysics.h"
#include "GenericPlatform/GenericPlatformMath.h"

BlockPhysics::BlockPhysics(const BlockMatrix& blockMatrix, TFunction<int(void)> newBlockGenerator)
	:newBlockGenerator(newBlockGenerator)
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
	TickBlockActions(deltaSeconds);
	auto thereIsAMatch = false;
	if (ShouldCheckMatch()) {
		thereIsAMatch = CheckAndProcessMatch();
	}
	RemoveDeadBlocks();
	ChangeCompletedActionsToNextActions(thereIsAMatch);
	SetFallingActionsAndGenerateNewBlocks();
}

void BlockPhysics::TickBlockActions(float deltaSeconds)
{
	for (auto& block : blocks) {
		block.currentAction->Tick(deltaSeconds);
		if (block.currentAction->IsJustCompleted()) {
			UE_LOG(LogTemp, Display, TEXT("action completed. type: %s"), *PrettyPrint(block.currentAction->GetType()));
		}
	}
}

bool BlockPhysics::ShouldCheckMatch()
{
	for (auto& block : blocks) {
		if (block.currentAction->ShouldCheckMatch())
			return true;
	}
	return false;
}

bool BlockPhysics::CheckAndProcessMatch()
{
	UE_LOG(LogTemp, Display, TEXT("match check"));
	auto blockMatrix = GetBlockMatrix();
	auto thereIsAMatch = !blockMatrix.HasNoMatch();
	if (thereIsAMatch) {
		UE_LOG(LogTemp, Display, TEXT("match occured"));
		auto matchResult = blockMatrix.ProcessMatch(TSet<FIntPoint>());
		StartDestroyingMatchedBlocksAccordingTo(matchResult);
		SetSpecialBlocksSpawnAccordingTo(matchResult);
	}
	return thereIsAMatch;
}

void BlockPhysics::RemoveDeadBlocks()
{
	blocks.RemoveAll([](const BlockPhysicalStatus& target) -> bool {
		return target.currentAction->ShouldBeRemoved();
		});
}

void BlockPhysics::ChangeCompletedActionsToNextActions(bool thereIsAMatch)
{
	for (auto& block : blocks) {
		if (block.currentAction->IsJustCompleted()) {
			block.block = block.currentAction->GetNextBlock(block.block);
			block.currentAction = block.currentAction->GetNextAction(thereIsAMatch);
		}
	}
}

void BlockPhysics::SetFallingActionsAndGenerateNewBlocks()
{
	class BlocksInColumn {
	public:
		BlocksInColumn(BlockPhysics& blockPhysics, int col) : blockPhysics(blockPhysics), col(col) {
			for (int row = 0; row < blockPhysics.GetNumRows(); row++) {
				auto pBlockStatus = blockPhysics.GetBlockAt(FIntPoint{ row, col });
				if (pBlockStatus != nullptr)
					rowIndicesOfBlocks.Add(row);
			}
		}
		bool IsEmpty() const { return rowIndicesOfBlocks.Num() == 0; }
		BlockPhysicalStatus& PopLowest() {
			const auto lowestRow = rowIndicesOfBlocks.Pop();
			auto pBlockStatus = blockPhysics.GetBlockAt(FIntPoint{ lowestRow, col });
			if (pBlockStatus == nullptr)
				UE_LOG(LogTemp, Error, TEXT("Queryed GetBlockAt with non-empty location and got nullptr: (%d, %d)"), lowestRow, col);
			return *pBlockStatus;
		}
	private:
		BlockPhysics& blockPhysics;
		int col;
		TArray<int> rowIndicesOfBlocks;
	};

	class PositionsInColumn {
	public:
		PositionsInColumn(int colIndex, int lowestRow) : col(colIndex), lowestRow(lowestRow) {}
		bool IsEmpty() const {
			return lowestRow < 0;
		}
		FIntPoint PopLowest() {
			return FIntPoint{ lowestRow--, col };
		}
		int col;
		int lowestRow;
	};

	for (int col = 0; col < numCols; col++) {
		if (NumOccupiedCellsInColumn(col) == numRows) {
			UE_LOG(LogTemp, Display, TEXT("Column %d has all cells occupied"), col);
			continue;
		}

		auto blocksInCol = BlocksInColumn(*this, col);
		auto positionsInCol = PositionsInColumn(col, numRows-1);
		auto topRow = -1;
		while (!positionsInCol.IsEmpty()) {
			auto destination = positionsInCol.PopLowest();
			if (blocksInCol.IsEmpty()) {
				auto newBlock = BlockPhysicalStatus(GetRandomBlock(), FIntPoint{ topRow--, col });
				UE_LOG(LogTemp, Display, TEXT("New block generated at: (%d, %d)"), topRow, col);
				MakeBlockFallToDestination(newBlock, destination);
				blocks.Add(MoveTemp(newBlock));
			}
			else {
				auto& currentBlock = blocksInCol.PopLowest();
				if (currentBlock.currentAction->GetPosition() != destination)
					MakeBlockFallToDestination(currentBlock, destination);
			}
		}
	}
}


int BlockPhysics::NumOccupiedCellsInColumn(int colIndex) const
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
		auto blockPosToStart = FVector2D(startPos) - blockPos;
		if (blockPosToStart.IsNearlyZero(DELTA_DISTANCE)) {
			UE_LOG(LogTemp, Display, TEXT("blockPosToStart Nearly zero"));
			return true;
		}
		blockPosToStart.Normalize();
		auto blockPosToEnd = FVector2D(endPos) - blockPos;
		if (blockPosToEnd.IsNearlyZero(DELTA_DISTANCE)) {
			UE_LOG(LogTemp, Display, TEXT("blockPosToEnd Nearly zero. block (%f,%f), end (%d,%d)"),
				blockPos.X, blockPos.Y, endPos.X, endPos.Y);
			return true;
		}
		blockPosToEnd.Normalize();
		const auto dotProduct = FVector2D::DotProduct(blockPosToStart, blockPosToEnd);
		if (FGenericPlatformMath::Abs(dotProduct + 1) < DELTA_COSINE) {
			UE_LOG(LogTemp, Display, TEXT("start:(%d,%d),end:(%d,%d),dot:%f"),
				startPos.X, startPos.Y, endPos.X, endPos.Y, dotProduct);
			return true;
		}
	}
	return false;
}

bool BlockPhysics::ExistsBlockNear(FIntPoint searchPosition, float threshold) const
{
	for (const auto& block : blocks) {
		const auto blockPos = block.currentAction->GetPosition();
		const auto distance = (blockPos - FVector2D(searchPosition)).Size();
		if (distance < threshold)
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

void BlockPhysics::StartDestroyingMatchedBlocksAccordingTo(const MatchResult& matchResult)
{
	for (const auto matchedPos : matchResult.GetMatchedPositions()) {
		const auto row = matchedPos.X;
		const auto col = matchedPos.Y;
		auto blockStatus = GetBlockAt(matchedPos);
		if (blockStatus == nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("block to update does not exist at (%d, %d)"), row, col);
			continue;
		}
		UE_LOG(LogTemp, Display, TEXT("block to destroy at (%d, %d)"), row, col);
		blockStatus->currentAction = MakeUnique<GetsDestroyedBlockAction>(blockStatus->currentAction->GetPosition());
	}
}

void BlockPhysics::SetSpecialBlocksSpawnAccordingTo(const MatchResult& matchResult)
{
	for (const auto& munchickenSpawnPosition : matchResult.GetSpawnPositionsOf(Block::MUNCHICKEN)) {
		const auto row = munchickenSpawnPosition.X;
		const auto col = munchickenSpawnPosition.Y;
		auto blockStatus = GetBlockAt(munchickenSpawnPosition);
		if (blockStatus == nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("block to update does not exist at (%d, %d)"), row, col);
			continue;
		}
		UE_LOG(LogTemp, Display, TEXT("Special block generation reserved at (%d, %d)"), row, col);
		blockStatus->currentAction = MakeUnique<GetsDestroyedAndSpawnBlockAfterAction>(FVector2D(munchickenSpawnPosition), Block::MUNCHICKEN);
	}
}

void BlockPhysics::MakeBlockFallToDestination(BlockPhysicalStatus& blockStatus, FIntPoint destination)
{
	const auto initialPosition = ToFIntPoint(blockStatus.currentAction->GetPosition());
	blockStatus.currentAction = MakeUnique<FallingBlockAction>(initialPosition, destination);
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
	const auto normalBlocks = GetNormalBlocks();
	return normalBlocks[newBlockGenerator() % normalBlocks.Num()];
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

GetsDestroyedAndSpawnBlockAfterAction::GetsDestroyedAndSpawnBlockAfterAction(FVector2D initialPos, Block blockToSpawnAfterDestroy)
	: GetsDestroyedBlockAction(initialPos), blockToSpawnAfterDestroy(blockToSpawnAfterDestroy)
{

}

TUniquePtr<BlockAction> GetsDestroyedAndSpawnBlockAfterAction::GetNextAction(bool thereIsAMatch) const
{
	return MakeUnique<IdleBlockAction>(position);
}
