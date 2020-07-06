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
			physicalBlocks.Add(PhysicalBlock(block2DArray[i][j], FIntPoint{ i, j }));
		}
	}
}

BlockPhysics::BlockPhysics(BlockPhysics&& other)
	:physicalBlocks(MoveTemp(other.physicalBlocks)), numRows(other.numRows), numCols(other.numCols)
{

}

BlockPhysics::~BlockPhysics()
{
}

void BlockPhysics::Tick(float deltaSeconds)
{
	elapsedTime += deltaSeconds;
	if(enableTickDebugLog)
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
	for (auto& block : physicalBlocks) {
		block.currentAction->Tick(deltaSeconds);
		if (block.currentAction->IsJustCompleted()) {
			UE_LOG(LogTemp, Display, TEXT("action completed. Action type: %s, block type: %s, position: %f, %f"), 
				*PrettyPrint(block.currentAction->GetType()), 
				*PrettyPrint(block.block), 
				block.currentAction->GetPosition().X,
				block.currentAction->GetPosition().Y);
		}
	}
}

bool BlockPhysics::ShouldCheckMatch()
{
	for (auto& block : physicalBlocks) {
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
		auto matchResult = blockMatrix.ProcessMatch(GetBlockInflowPositions());
		StartDestroyingMatchedBlocksAccordingTo(matchResult);
		SetSpecialBlocksSpawnAccordingTo(matchResult);
	}
	return thereIsAMatch;
}

TSet<FIntPoint> BlockPhysics::GetBlockInflowPositions()
{
	auto ret = TSet<FIntPoint>();
	for (const auto& block : physicalBlocks) {
		if (block.currentAction->IsJustCompleted() && IsNearLatticePoint(block.currentAction->GetPosition())) {
			ret.Add(ToFIntPoint(block.currentAction->GetPosition()));
		}
	}
	return ret;
}

void BlockPhysics::RemoveDeadBlocks()
{
	physicalBlocks.RemoveAll([](const PhysicalBlock& target) -> bool {
		return target.currentAction->ShouldBeRemoved();
		});
}

void BlockPhysics::ChangeCompletedActionsToNextActions(bool thereIsAMatch)
{
	for (auto& physicalBlock : physicalBlocks) {
		if (physicalBlock.currentAction->IsJustCompleted()) {
			physicalBlock.block = physicalBlock.currentAction->GetNextBlock(physicalBlock.block);
			physicalBlock.currentAction = physicalBlock.currentAction->GetNextAction(thereIsAMatch);
		}
	}
}

void BlockPhysics::SetFallingActionsAndGenerateNewBlocks()
{
	class BlocksInColumn {
	public:
		BlocksInColumn(BlockPhysics& blockPhysics, int col) : col(col) {
			for (auto& block : blockPhysics.physicalBlocks) {
				const auto isOccupyingPositionInThisColumn = FGenericPlatformMath::Abs(block.currentAction->GetOccupiedPosition().Y - col) < blockPhysics.DELTA_DISTANCE;
				if (isOccupyingPositionInThisColumn) {
					blocksInCol.Add(&block);
				}
			}
			blocksInCol.Sort([](const PhysicalBlock& block1, const PhysicalBlock& block2) -> bool {
				return block1.currentAction->GetPosition().X < block2.currentAction->GetPosition().X;
			});
		}
		int NumOccupiedCellsInColumn() const { return blocksInCol.Num(); }
		bool IsEmpty() const { return blocksInCol.Num() == 0; }
		PhysicalBlock& PopLowest() {
			auto* physicalBlock = blocksInCol.Pop();
			if (physicalBlock == nullptr)
				UE_LOG(LogTemp, Error, TEXT("Queryed GetTopmostBlockAt with non-empty location and got nullptr"));
			return *physicalBlock;
		}
	private:
		TArray<PhysicalBlock*> blocksInCol;
		int col;
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
		void PopUpTo(float rowCoordinate) {
			while (lowestRow >= static_cast<int>(rowCoordinate)) {
				lowestRow--;
			}
		}
		int col;
		int lowestRow;
	};

	for (int col = 0; col < numCols; col++) {
		auto blocksInCol = BlocksInColumn(*this, col);
		if (blocksInCol.NumOccupiedCellsInColumn() == numRows) {
			if (enableTickDebugLog)
				UE_LOG(LogTemp, Display, TEXT("Column %d has all cells occupied"), col);
			continue;
		}
		auto positionsInCol = PositionsInColumn(col, numRows-1);
		auto topRow = -1;
		while (!positionsInCol.IsEmpty()) {
			auto destination = positionsInCol.PopLowest();
			if (blocksInCol.IsEmpty()) {
				UE_LOG(LogTemp, Display, TEXT("New physicalBlock generated at: (%d, %d)"), topRow, col);
				auto newBlock = PhysicalBlock(GetRandomBlock(), FIntPoint{ topRow--, col });
				MakeBlockFallToDestination(newBlock, destination);
				physicalBlocks.Add(MoveTemp(newBlock));
			}
			else {
				auto& currentBlock = blocksInCol.PopLowest();
				const auto isGettingDestroyedMunchicken = (currentBlock.block == Block::MUNCHICKEN) && (currentBlock.currentAction->GetType() == ActionType::GetsDestroyed);
				const auto isRollingMunchicken = currentBlock.currentAction->GetType() == ActionType::Roll;
				if (isGettingDestroyedMunchicken || isRollingMunchicken) {
					positionsInCol.PopUpTo(currentBlock.currentAction->GetPosition().X);
					continue;
				}

				if (currentBlock.currentAction->GetPosition() != destination)
					MakeBlockFallToDestination(currentBlock, destination);
			}
		}
	}
}

void BlockPhysics::RecieveSwipeInput(FIntPoint swipeStart, FIntPoint swipeEnd)
{
	auto startBlock = GetTopmostBlockAt(swipeStart);
	if (startBlock == nullptr) {
		UE_LOG(LogTemp, Warning, TEXT("RecieveSwipeInput precondition: block should exist at start position: (%d, %d)"),
			swipeStart.X, swipeStart.Y);
		return;
	}

	if (startBlock->block != Block::MUNCHICKEN) {
		auto endBlock = GetTopmostBlockAt(swipeEnd);
		if (endBlock == nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("RecieveSwipeInput precondition: block should exist at end position if swiping block is not munchicken: (%d, %d)"),
				swipeEnd.X, swipeEnd.Y);
			return;
		}
		startBlock->currentAction = MakeUnique<SwipeMoveBlockAction>(swipeStart, swipeEnd);
		endBlock->currentAction = MakeUnique<SwipeMoveBlockAction>(swipeEnd, swipeStart);
	}
	else {
		FIntPoint rollDirection = swipeEnd - swipeStart;
		startBlock->currentAction = MakeUnique<MunchickenRollAction>(swipeStart, rollDirection, *this);
	}
}

bool BlockPhysics::IsEmpty(FIntPoint position) const
{
	return GetTopmostBlockAt(position) == nullptr;
}

bool BlockPhysics::ExistsBlockBetween(FIntPoint startPos, FIntPoint endPos) const
{
	for (const auto& block : physicalBlocks) {
		const auto blockPos = block.currentAction->GetPosition();
		auto blockPosToStart = FVector2D(startPos) - blockPos;
		if (blockPosToStart.IsNearlyZero(DELTA_DISTANCE)) {
			UE_LOG(LogTemp, Display, TEXT("blockPosToStart Nearly zero"));
			return true;
		}
		blockPosToStart.Normalize();
		auto blockPosToEnd = FVector2D(endPos) - blockPos;
		if (blockPosToEnd.IsNearlyZero(DELTA_DISTANCE)) {
			UE_LOG(LogTemp, Display, TEXT("blockPosToEnd Nearly zero. physicalBlock (%f,%f), end (%d,%d)"),
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
	for (const auto& block : physicalBlocks) {
		const auto blockPos = block.currentAction->GetPosition();
		const auto distance = (blockPos - FVector2D(searchPosition)).Size();
		if (distance < threshold)
			return true;
	}
	return false;
}

bool BlockPhysics::IsPlayingDestroyAnimAt(FIntPoint position) const
{
	auto blockStatus = GetTopmostBlockAt(position);
	if (blockStatus == nullptr)
		return false;
	return blockStatus->currentAction->GetType() == ActionType::GetsDestroyed;
}

bool BlockPhysics::IsIdleAt(FIntPoint position) const
{
	auto pBlock = GetTopmostBlockAt(position);
	return (pBlock != nullptr) && (pBlock->currentAction->GetType() == ActionType::Idle);
}

void BlockPhysics::DestroyBlocksInBackgroundAt(const TSet<FIntPoint>& destroyPositions, const TSet<Block>& exceptionalBlocks)
{
	for (const auto& destroyPosition : destroyPositions) {
		auto blocksAtDestroyPosition = GetBlocksAt(destroyPosition);
		for (auto physicalBlock : blocksAtDestroyPosition) {
			if (!exceptionalBlocks.Contains(physicalBlock->block)) {
				physicalBlock->currentAction = MakeUnique<GetsDestroyedInBackgroundBlockAction>(destroyPosition);
				UE_LOG(LogTemp, Display, TEXT("Destroying block: %s at (%d, %d) in background"), *PrettyPrint(physicalBlock->block), destroyPosition.X, destroyPosition.Y);
			}
		}
	}
}

TArray<PhysicalBlockSnapShot> BlockPhysics::GetPhysicalBlockSnapShots() const
{
	auto ret = TArray<PhysicalBlockSnapShot>();
	for (const auto& block : physicalBlocks) {
		ret.Add(block.GetSnapShot());
	}
	return ret;
}

PhysicalBlock* BlockPhysics::GetTopmostBlockAt(FIntPoint position)
{
	auto highestLayerSoFar = INT_MIN;
	PhysicalBlock* ret = nullptr;
	for (auto& block : physicalBlocks) {
		if ((block.currentAction->GetPosition() - position).SizeSquared() <= DELTA_DISTANCE) {
			if (highestLayerSoFar < block.currentAction->GetLayer()) {
				ret = &block;
				highestLayerSoFar = block.currentAction->GetLayer();
			}
		}
	}
	return ret;
}

const PhysicalBlock* BlockPhysics::GetTopmostBlockAt(FIntPoint position) const
{
	auto highestLayerSoFar = INT_MIN;
	const PhysicalBlock* ret = nullptr;
	for (const auto& block : physicalBlocks) {
		if ((block.currentAction->GetPosition() - position).SizeSquared() <= DELTA_DISTANCE) {
			if (highestLayerSoFar < block.currentAction->GetLayer()) {
				ret = &block;
				highestLayerSoFar = block.currentAction->GetLayer();
			}
		}
	}
	return ret;
}

TArray<PhysicalBlock*> BlockPhysics::GetBlocksAt(FIntPoint position)
{
	auto ret = TArray<PhysicalBlock*>();
	for (auto& block : physicalBlocks) {
		if ((block.currentAction->GetPosition() - position).SizeSquared() <= DELTA_DISTANCE) {
			ret.Add(&block);
		}
	}
	return ret;

}

BlockMatrix BlockPhysics::GetBlockMatrix() const
{
	auto blockMatrix = TMap<FIntPoint, Block>();
	for (const auto& physicalBlock : physicalBlocks) {
		if (physicalBlock.currentAction->IsEligibleForMatching()) {
			blockMatrix.Add(ToFIntPoint(physicalBlock.currentAction->GetPosition()), physicalBlock.block);
		}
	}
	return BlockMatrix(numRows, numCols, blockMatrix);
}

void BlockPhysics::StartDestroyingMatchedBlocksAccordingTo(const MatchResult& matchResult)
{
	for (const auto matchedPos : matchResult.GetMatchedPositions()) {
		const auto row = matchedPos.X;
		const auto col = matchedPos.Y;
		auto* physicalBlock = GetTopmostBlockAt(matchedPos);
		if (physicalBlock == nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("physicalBlock to update does not exist at (%d, %d)"), row, col);
			continue;
		}
		UE_LOG(LogTemp, Display, TEXT("physicalBlock to destroy at (%d, %d)"), row, col);
		physicalBlock->currentAction = MakeUnique<GetsDestroyedBlockAction>(physicalBlock->currentAction->GetPosition());
	}
}

void BlockPhysics::SetSpecialBlocksSpawnAccordingTo(const MatchResult& matchResult)
{
	for (const auto& specialBlockAndItsSpawnPosition : matchResult.GetSpecialBlockAndItsSpawnPositions()) {
		const auto specialBlock = specialBlockAndItsSpawnPosition.Key;
		const auto spawnPosition = specialBlockAndItsSpawnPosition.Value;
		auto* physicalBlock = GetTopmostBlockAt(spawnPosition);
		if (physicalBlock == nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("physicalBlock to update does not exist at (%d, %d)"), spawnPosition.X, spawnPosition.Y);
			continue;
		}
		UE_LOG(LogTemp, Display, TEXT("Special physicalBlock generation reserved at (%d, %d)"), spawnPosition.X, spawnPosition.Y);
		physicalBlock->currentAction = MakeUnique<GetsDestroyedAndSpawnBlockAfterAction>(FVector2D(spawnPosition), specialBlock);
	}
}

void BlockPhysics::MakeBlockFallToDestination(PhysicalBlock& blockStatus, FIntPoint destination)
{
	const auto initialPosition = ToFIntPoint(blockStatus.currentAction->GetPosition());
	blockStatus.currentAction = MakeUnique<FallingBlockAction>(initialPosition, destination);
}

FIntPoint BlockPhysics::ToFIntPoint(FVector2D position)
{
	return FIntPoint{ ToInt(position.X), ToInt(position.Y) };
}

bool BlockPhysics::IsNearLatticePoint(FVector2D position)
{
	return (position - ToFIntPoint(position)).Size() <= DELTA_DISTANCE;
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

PhysicalBlock::PhysicalBlock(Block block, FIntPoint initialPosition, TUniquePtr<BlockAction>&& action)
	: block(block), currentAction(MoveTemp(action)), id(++lastIssuedId)
{
}

PhysicalBlock::PhysicalBlock(Block block, FIntPoint initialPosition)
	: block(block), currentAction(MakeUnique<IdleBlockAction>(initialPosition)), id(++lastIssuedId)
{

}

PhysicalBlock::PhysicalBlock(PhysicalBlock&& other)
	: block(other.block), currentAction(MoveTemp(other.currentAction)), id(++lastIssuedId)
{

}

PhysicalBlockSnapShot PhysicalBlock::GetSnapShot() const
{
	return PhysicalBlockSnapShot(id, block, currentAction->GetType(), currentAction->GetPosition());
}

int PhysicalBlock::lastIssuedId = -1;

