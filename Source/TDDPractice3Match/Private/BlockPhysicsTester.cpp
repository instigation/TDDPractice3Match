// Fill out your copyright notice in the Description page of Project Settings.

#include "BlockPhysicsTester.h"
#include "BlockPhysics.h"
#include "BlockMatrix.h"

BlockPhysicsTester::BlockPhysicsTester(const BlockMatrix& initialBlockMatrix, TFunction<int(void)> randomGenerator /*= rand*/)
	: blockPhysics(MakeUnique<BlockPhysics>(initialBlockMatrix, randomGenerator)), tickDivider(1)
{

}

void BlockPhysicsTester::DoSwipe(const FIntPoint& swipeStart, const FIntPoint& swipeEnd) const
{
	blockPhysics->ReceiveSwipeInput(swipeStart, swipeEnd);
}

void BlockPhysicsTester::TickUntilSwipeMoveAnimationEnd()
{
	TickFor(blockPhysics->GRID_SIZE / blockPhysics->SWIPE_MOVE_SPEED + VERY_SHORT_TIME);
}

void BlockPhysicsTester::TickForHalfSwipeMove()
{
	TickFor(blockPhysics->GRID_SIZE / blockPhysics->SWIPE_MOVE_SPEED / 2.f + VERY_SHORT_TIME);
}

void BlockPhysicsTester::TickUntilSwipeReturnAnimtaionEnd()
{
	TickUntilSwipeMoveAnimationEnd();
}

void BlockPhysicsTester::TickUntilBlockDestroyEnd()
{
	TickFor(blockPhysics->DESTROY_ANIMATION_TIME + VERY_SHORT_TIME);
}

void BlockPhysicsTester::TickUntilBlockFallEnd(int numGridsToFall)
{
	TickFor(GetFallTime(numGridsToFall) + VERY_SHORT_TIME);
}

void BlockPhysicsTester::TickUntilRollOneGrid()
{
	TickFor(blockPhysics->GRID_SIZE / blockPhysics->ROLL_SPEED + VERY_SHORT_TIME);
}

void BlockPhysicsTester::TestBlockOccurrence(const Block& expectedBlock, int expectedOccurance) const
{
	auto numFound = 0;
	const auto snapshots = blockPhysics->GetPhysicalBlockSnapShots();
	for (const auto& snapshot : snapshots) {
		if (snapshot.block == expectedBlock)
			numFound++;
	}
	if (numFound != expectedOccurance)
		UE_LOG(LogTemp, Error, TEXT("Expected %d of %s but there was %d"), expectedOccurance, *PrettyPrint(expectedBlock), numFound);
}

void BlockPhysicsTester::TestIfCorrectlyEmpty(const TSet<FIntPoint>& onlyPositionsThatShouldBeEmpty) const
{
	const auto numRows = blockPhysics->GetNumRows();
	const auto numCols = blockPhysics->GetNumCols();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto isEmpty = blockPhysics->IsEmpty(FIntPoint{ i, j });
			if (onlyPositionsThatShouldBeEmpty.Contains(FIntPoint{ i,j })) {
				if (!isEmpty) {
					UE_LOG(LogTemp, Error, TEXT("Cell is not empty at: (%d, %d)"), i, j);
					return;
				}
			}
			else {
				if (isEmpty) {
					UE_LOG(LogTemp, Error, TEXT("Cell is empty at (%d, %d)"), i, j);
					return;
				}
			}
		}
	}
}

void BlockPhysicsTester::TestIfBlockExistsAt(const Block& expectedBlock, const FIntPoint& expectedPosition) const
{
	const auto actualBlockType = blockPhysics->GetBlockMatrix().At(expectedPosition.X, expectedPosition.Y);
	if (actualBlockType != expectedBlock) {
		UE_LOG(LogTemp, Error, TEXT("expected: %s but was %s at (%d, %d)"), *PrettyPrint(expectedBlock), *PrettyPrint(actualBlockType), expectedPosition.X, expectedPosition.Y);
	}
}

void BlockPhysicsTester::TestIfBlockNotExistsAt(const Block& notExpectedBlock, const FIntPoint& positionToInspect) const
{
	const auto actualBlockType = blockPhysics->GetBlockMatrix().At(positionToInspect.X, positionToInspect.Y);
	if (actualBlockType == notExpectedBlock) {
		UE_LOG(LogTemp, Error, TEXT("Not expected: %s but was %s at (%d, %d)"), *PrettyPrint(notExpectedBlock), *PrettyPrint(actualBlockType), positionToInspect.X, positionToInspect.Y);
	}
}

void BlockPhysicsTester::TestIfAllBlocksAreFilled() const
{
	const auto numRow = blockPhysics->GetNumRows();
	const auto numCol = blockPhysics->GetNumCols();
	const auto block2DArray = blockPhysics->GetBlockMatrix().GetBlock2DArray();
	for (int i = 0; i < numRow; i++) {
		for (int j = 0; j < numCol; j++) {
			if (block2DArray[i][j] == Block::INVALID) {
				UE_LOG(LogTemp, Error, TEXT("Block at (%d, %d) is invalid, where all blocks are expected to be filled"), i, j);
			}
		}
	}
}

void BlockPhysicsTester::TestIfAlmostIdenticalTo(const BlockMatrix& blockMatrix, const TSet<FIntPoint>& exceptionalPositions) const
{
	const auto currentBlockMatrix = blockPhysics->GetBlockMatrix();
	const auto numRows = blockPhysics->GetNumRows();
	const auto numCols = blockPhysics->GetNumCols();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto position = FIntPoint{ i, j };
			if (!exceptionalPositions.Contains(position)) {
				const auto currentBlock = blockMatrix.At(i, j);
				const auto originalBlock = currentBlockMatrix.At(i, j);
				if (currentBlock != originalBlock) {
					UE_LOG(LogTemp, Error, TEXT("Block (%s) at (%d, %d) differs from original block (%s)"),
						*PrettyPrint(currentBlock), i, j, *PrettyPrint(originalBlock));
				}
			}
		}
	}
}

void BlockPhysicsTester::TestIfNewBlocksNotSpawnedAtCol(int colToInspect) const
{
	if (blockPhysics->ExistsBlockNear(FIntPoint{ -1, colToInspect }, blockPhysics->GRID_SIZE / 3.f)) {
		UE_LOG(LogTemp, Error, TEXT("Block should not exist at (%d,%d) but present"),
			-1, colToInspect);
	}
}

void BlockPhysicsTester::TestIfNewBlocksSpawnedAtCol(int colToInspect, int expectedNewBlocksCount) const
{
	for (int i = -1; i >= -expectedNewBlocksCount; i--) {
		if (!blockPhysics->ExistsBlockNear(FIntPoint{ i, colToInspect }, blockPhysics->GRID_SIZE / 3.f)) {
			UE_LOG(LogTemp, Error, TEXT("Block expected to be exist at (%d,%d) but not present"),
				i, colToInspect);
		}
	}
}

void BlockPhysicsTester::TestIfNewBlocksSpawnedAtCol(const TSet<int>& newBlockSpawnExpectedCols) const
{
	for (int colIndex = 0; colIndex < blockPhysics->GetNumRows(); colIndex++) {
		const auto pos = FIntPoint{ -1,colIndex };
		const auto isNewBlockCreated = !blockPhysics->IsEmpty(pos) || blockPhysics->ExistsBlockNear(pos, blockPhysics->GRID_SIZE / 3.f);
		if (newBlockSpawnExpectedCols.Contains(colIndex)) {
			if (!isNewBlockCreated) {
				UE_LOG(LogTemp, Error, TEXT("New blocks are not created at (%d, %d)"), pos.X, pos.Y);
			}
		}
		else {
			if (isNewBlockCreated) {
				UE_LOG(LogTemp, Error, TEXT("New blocks are created at (%d, %d)"), pos.X, pos.Y);
			}
		}
	}

}

void BlockPhysicsTester::TestIfCorrectlyGettingDestroyed(const TSet<FIntPoint>& onlyPositionsThatShouldBeDestroyed) const
{
	const auto numRows = blockPhysics->GetNumRows();
	const auto numCols = blockPhysics->GetNumCols();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto isGettingDestroyed = blockPhysics->IsPlayingDestroyAnimAt(FIntPoint{ i,j });
			if (onlyPositionsThatShouldBeDestroyed.Contains(FIntPoint{ i,j })) {
				if (!isGettingDestroyed) {
					UE_LOG(LogTemp, Error, TEXT("Block destroy animation not playing at: (%d, %d)"), i, j);
				}
			}
			else {
				if (isGettingDestroyed) {
					UE_LOG(LogTemp, Error, TEXT("unmatched block is getting destroyed at: (%d, %d)"), i, j);
				}
			}
		}
	}

}

void BlockPhysicsTester::TickFor(float deltaSeconds)
{
	for (int i = 0; i < tickDivider; i++) {
		blockPhysics->Tick(deltaSeconds / tickDivider);
		if (i != tickDivider - 1)
			duringFrequentTickTest(*this);
		onTickEndTest(*this);
	}
}

float BlockPhysicsTester::GetFallTime(int numGridsToFall)
{
	return FMath::Sqrt(2 * blockPhysics->GRID_SIZE * numGridsToFall / blockPhysics->GRAVITY_ACCELERATION);
}
