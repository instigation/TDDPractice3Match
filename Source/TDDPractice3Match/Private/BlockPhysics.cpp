// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/BlockPhysics.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "../Public/MyMathUtils.h"

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
	for (const auto& munchickenSpawnPosition : matchResult.GetSpawnPositionsOf(Block::MUNCHICKEN)) {
		const auto row = munchickenSpawnPosition.X;
		const auto col = munchickenSpawnPosition.Y;
		auto* physicalBlock = GetTopmostBlockAt(munchickenSpawnPosition);
		if (physicalBlock == nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("physicalBlock to update does not exist at (%d, %d)"), row, col);
			continue;
		}
		UE_LOG(LogTemp, Display, TEXT("Special physicalBlock generation reserved at (%d, %d)"), row, col);
		physicalBlock->currentAction = MakeUnique<GetsDestroyedAndSpawnBlockAfterAction>(FVector2D(munchickenSpawnPosition), Block::MUNCHICKEN);
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

MunchickenRollAction::MunchickenRollAction(FVector2D initialPos, FIntPoint rollDirection, BlockPhysics& blockPhysics)
	: BlockAction(initialPos), rollDirection(rollDirection), blockPhysics(blockPhysics)
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
	DestroyBlocksInBackground(cellPositionsRolledOver);
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

void MunchickenRollAction::DestroyBlocksInBackground(const TSet<FIntPoint>& destroyPositions)
{
	for (const auto& destroyPosition : destroyPositions) {
		UE_LOG(LogTemp, Display, TEXT("destroyed by munchicken at (%d, %d)"), destroyPosition.X, destroyPosition.Y);
	}
	blockPhysics.DestroyBlocksInBackgroundAt(destroyPositions, TSet<Block>{Block::MUNCHICKEN});
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
