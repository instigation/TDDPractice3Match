// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/Test.h"
#include "../Public/Block.h"
#include "../Public/BlockPhysics.h"
#include "Misc/AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(HasNoMatchShouldReturnTrueGivenNoMatch, "Blocks.BlockMatrix.HasNoMatch should return true when no match", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool HasNoMatchShouldReturnTrueGivenNoMatch::RunTest(const FString& Parameters)
{
	const auto block2DArrayWithNoMatch = TArray<TArray<Block>>{
		{Block::ONE, Block::ONE, Block::TWO },
		{Block::ONE, Block::TWO, Block::TWO},
		{Block::THREE, Block::FOUR, Block::THREE}
	};
	const auto blockMatrixWithNoMatch = BlockMatrix(block2DArrayWithNoMatch);
	return blockMatrixWithNoMatch.HasNoMatch();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(HasNoMatchShouldReturnTrueGiven2x2, "Blocks.BlockMatrix.HasNoMatch should return true when 2x2", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool HasNoMatchShouldReturnTrueGiven2x2::RunTest(const FString& Parameters)
{
	const auto block2DArrayWithNoMatch = TArray<TArray<Block>>{
		{Block::ONE, Block::ONE},
		{Block::ONE, Block::TWO}
	};
	const auto blockMatrixWithNoMatch = BlockMatrix(block2DArrayWithNoMatch);
	return blockMatrixWithNoMatch.HasNoMatch();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(HasNoMatchShouldReturnFalseGivenMatch, "Blocks.BlockMatrix.HasNoMatch should return false when match", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool HasNoMatchShouldReturnFalseGivenMatch::RunTest(const FString& Parameters)
{
	const auto block2DArrayWithMatch = TArray<TArray<Block>>{
		{Block::ONE, Block::ONE, Block::ONE },
		{Block::ONE, Block::TWO, Block::TWO},
		{Block::THREE, Block::FOUR, Block::THREE}
	};
	const auto blockMatrixWithMatch = BlockMatrix(block2DArrayWithMatch);
	return !blockMatrixWithMatch.HasNoMatch();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(IfNoMatchOnSwipeThenBlocksShouldReturn, "Board.OnSwipe.Blocks should return when there's no match", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool IfNoMatchOnSwipeThenBlocksShouldReturn::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeStart = FIntPoint{ 1, 2 };
	const auto swipeEnd = FIntPoint{ 1, 3 };
	const auto movingBlockPositions = TArray<FIntPoint>{ swipeStart, swipeEnd };
	auto blockMatrix = TestUtils::blockMatrix5x5;
	auto blockPhysics = BlockPhysics(blockMatrix);

	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);

	// After swipe animation end
	const auto swipeMoveTime = blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED;
	blockPhysics.Tick(swipeMoveTime + TestUtils::veryShortTime);
	const auto blockMatrixAfterSwipe = blockPhysics.GetBlockMatrix();
	if (!TestUtils::AreIdenticalExcept(blockMatrix, blockMatrixAfterSwipe, movingBlockPositions))
		return false;

	// After returning back animation end
	blockPhysics.Tick(swipeMoveTime + TestUtils::veryShortTime);
	const auto blockMatrixAfterSwipeReturn = blockPhysics.GetBlockMatrix();
	if (!TestUtils::AreIdenticalExcept(blockMatrix, blockMatrixAfterSwipeReturn, TArray<FIntPoint>()))
		return false;

	return true;
}

bool OnSwipeMatchCheckShouldOccurTest(int tickDivider) {
	// Setup
	const auto swipeStart = FIntPoint{ 0,3 };
	const auto swipeEnd = FIntPoint{ 0,2 };
	auto blockMatrix = TestUtils::blockMatrix5x5;
	auto blockPhysics = BlockPhysics(blockMatrix);

	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);

	// During swipe animation
	const auto swipeMoveTime = blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED;
	for (int i = 0; i < tickDivider - 1; i++) {
		blockPhysics.Tick(swipeMoveTime / tickDivider);
		if (!TestUtils::AreIdenticalExcept(blockPhysics.GetBlockMatrix(), blockMatrix, TArray<FIntPoint>{swipeStart, swipeEnd}))
			return false;
	}

	// After swipe animation end
	blockPhysics.Tick(swipeMoveTime / tickDivider + TestUtils::veryShortTime);
	const auto matchedBlockPositions = TArray<FIntPoint>{ FIntPoint{0,0}, FIntPoint{0,1}, FIntPoint{0,2} };
	if (!TestUtils::IsCorrectlyGettingDestroyed(blockPhysics, matchedBlockPositions))
		return false;

	// During destroy animation
	const auto matchedAndSwipedBlockPositions = TArray<FIntPoint>{ FIntPoint{0,0}, FIntPoint{0,1}, FIntPoint{0,2}, FIntPoint{0,3} };
	for (int i = 0; i < tickDivider - 1; i++) {
		blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME / tickDivider);
		if (!TestUtils::IsCorrectlyGettingDestroyed(blockPhysics, matchedBlockPositions))
			return false;
		if (!TestUtils::AreIdenticalExcept(blockPhysics.GetBlockMatrix(), blockMatrix, matchedAndSwipedBlockPositions))
			return false;
	}

	// After destroy animation end
	blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME / tickDivider + TestUtils::veryShortTime);
	if (!TestUtils::IsCorrectlyEmpty(blockPhysics, matchedBlockPositions))
		return false;
	const auto shouldBeSpawnedCols = TArray<int>{ 0, 1, 2 };
	if (!TestUtils::AreNewBlocksSpawnedAtTop(blockPhysics, shouldBeSpawnedCols))
		return false;

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(OnSwipeMatchCheckShouldOccur, "Board.OnSwipe.Match should occur when there's a match", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool OnSwipeMatchCheckShouldOccur::RunTest(const FString& Parameters)
{
	return OnSwipeMatchCheckShouldOccurTest(0);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(TickFrequencyShouldNotMatter, "Board.OnSwipe.Should work fine even if Tick is frequent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool TickFrequencyShouldNotMatter::RunTest(const FString& Parameters) {
	if (!OnSwipeMatchCheckShouldOccurTest(10))
		return false;
	if (!OnSwipeMatchCheckShouldOccurTest(60))
		return false;
	return true;
}

const float TestUtils::veryShortTime = 0.00001f;

const BlockMatrix TestUtils::blockMatrix5x5 = BlockMatrix(
	TArray<TArray<Block>>{
		{ Block::ONE, Block::ONE, Block::TWO, Block::ONE, Block::TWO },
		{ Block::ONE, Block::TWO, Block::TWO, Block::THREE, Block::FOUR },
		{ Block::THREE, Block::FOUR, Block::THREE, Block::TWO, Block::FOUR },
		{ Block::ONE, Block::ONE, Block::TWO, Block::ONE, Block::THREE },
		{ Block::TWO, Block::THREE, Block::ONE, Block::TWO, Block::THREE }
	}
);

bool TestUtils::IsCorrectlyGettingDestroyed(const BlockPhysics& blockPhysics, const TArray<FIntPoint>& onlyPositionsThatShouldBeDestroyed)
{
	const auto numRows = blockPhysics.GetNumRows();
	const auto numCols = blockPhysics.GetNumCols();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto isGettingDestroyed = blockPhysics.IsPlayingDestroyAnimAt(FIntPoint{ i,j });
			if (onlyPositionsThatShouldBeDestroyed.Contains(FIntPoint{ i,j })) {
				if (!isGettingDestroyed) {
					UE_LOG(LogTemp, Error, TEXT("Block destroy animation not playing at: (%d, %d)"), i, j);
					return false;
				}
			}
			else {
				if (isGettingDestroyed) {
					UE_LOG(LogTemp, Error, TEXT("unmatched block is getting destroyed at: (%d, %d)"), i, j);
					return false;
				}
			}
		}
	}
	return true;
}

bool TestUtils::IsCorrectlyEmpty(const BlockPhysics& blockPhysics, const TArray<FIntPoint>& onlyPositionsThatShouldBeEmpty)
{
	const auto numRows = blockPhysics.GetNumRows();
	const auto numCols = blockPhysics.GetNumCols();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto isEmpty = blockPhysics.IsEmpty(FIntPoint{ i, j });
			if (onlyPositionsThatShouldBeEmpty.Contains(FIntPoint{ i,j })) {
				if (!isEmpty) {
					UE_LOG(LogTemp, Error, TEXT("Cell is not empty at: (%d, %d)"), i, j);
					return false;
				}
			}
			else {
				if (isEmpty) {
					UE_LOG(LogTemp, Error, TEXT("Cell is empty at (%d, %d)"), i, j);
					return false;
				}
			}
		}
	}
	return true;
}

bool TestUtils::AreNewBlocksSpawnedAtTop(const BlockPhysics& blockPhysics, const TArray<int>& newBlockSpawnExpectedCols)
{
	for (int colIndex = 0; colIndex < blockPhysics.GetNumRows(); colIndex++) {
		const auto pos = FIntPoint{ -1,colIndex };
		const auto isNewBlockCreated = !blockPhysics.IsEmpty(pos) || blockPhysics.ExistsBlockBetween(pos, pos + FIntPoint{ 1,0 });
		if (newBlockSpawnExpectedCols.Contains(colIndex)) {
			if (!isNewBlockCreated) {
				UE_LOG(LogTemp, Error, TEXT("New blocks are not created at (%d, %d)"), pos.X, pos.Y);
				return false;
			}
		}
		else {
			if (isNewBlockCreated) {
				UE_LOG(LogTemp, Error, TEXT("New blocks are created at (%d, %d)"), pos.X, pos.Y);
				return false;
			}
		}
	}
	return true;
}

bool TestUtils::AreIdenticalExcept(const BlockMatrix& currentMatrix, const BlockMatrix& originalMatrix, const TArray<FIntPoint>& exceptionalPositions)
{
	const auto numRows = currentMatrix.GetNumRows();
	const auto numCols = currentMatrix.GetNumCols();
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto position = FIntPoint{ i, j };
			if (!exceptionalPositions.Contains(position)) {
				const auto currentBlock = currentMatrix.At(i, j);
				const auto originalBlock = originalMatrix.At(i, j);
				if (currentBlock != originalBlock) {
					UE_LOG(LogTemp, Error, TEXT("Block (type: %d) at (%d, %d) differs from original block (type: %d)"),
						currentBlock, i, j, originalBlock);
					return false;
				}
			}
		}
	}
	return true;
}
