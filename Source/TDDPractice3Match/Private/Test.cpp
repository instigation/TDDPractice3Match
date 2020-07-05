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
	if (!TestUtils::AreAlmostIdenticalAsExpected(blockMatrix, blockMatrixAfterSwipe, movingBlockPositions))
		return false;

	// During return back animation
	blockPhysics.Tick(swipeMoveTime / 2.f);
	if (!TestUtils::IsCorrectlyEmpty(blockPhysics, movingBlockPositions))
		return false;

	// After returning back animation end
	blockPhysics.Tick(swipeMoveTime / 2.f + TestUtils::veryShortTime);
	const auto blockMatrixAfterSwipeReturn = blockPhysics.GetBlockMatrix();
	if (!TestUtils::AreAlmostIdenticalAsExpected(blockMatrix, blockMatrixAfterSwipeReturn, TArray<FIntPoint>()))
		return false;

	return true;
}

bool OnSwipeMatchCheckShouldOccurTest(int tickDivider) {
	// Setup
	const auto swipeStart = FIntPoint{ 0,3 };
	const auto swipeEnd = FIntPoint{ 0,2 };
	auto blockMatrix = TestUtils::blockMatrix5x5;
	auto newBlockCount = 0;
	auto newBlockGenerator = [&newBlockCount]() -> int {
		const auto blockOne = static_cast<int>(Block::ONE);
		const auto blockTwo = static_cast<int>(Block::TWO);
		const auto newBlocks = TArray<int>{ blockOne, blockOne, blockTwo };
		if(newBlockCount < newBlocks.Num())
			return newBlocks[newBlockCount++];
		else {
			UE_LOG(LogTemp, Error, TEXT("More than three blocks are randomly generated"));
			return 0;
		}
	};
	auto blockPhysics = BlockPhysics(blockMatrix, newBlockGenerator);

	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);

	// During swipe animation
	const auto swipeMoveTime = blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED;
	for (int i = 0; i < tickDivider - 1; i++) {
		blockPhysics.Tick(swipeMoveTime / tickDivider);
		if (!TestUtils::AreAlmostIdenticalAsExpected(blockPhysics.GetBlockMatrix(), blockMatrix, TArray<FIntPoint>{swipeStart, swipeEnd}))
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
		if (!TestUtils::AreAlmostIdenticalAsExpected(blockPhysics.GetBlockMatrix(), blockMatrix, matchedAndSwipedBlockPositions))
			return false;
	}

	// After destroy animation end
	blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME / tickDivider + TestUtils::veryShortTime);
	if (!TestUtils::IsCorrectlyEmpty(blockPhysics, matchedBlockPositions))
		return false;
	const auto shouldBeSpawnedCols = TArray<int>{ 0, 1, 2 };
	if (!TestUtils::AreNewBlocksSpawnedAtTopAsExpected(blockPhysics, shouldBeSpawnedCols))
		return false;

	// After sufficient time to fall
	for (int i = 0; i < tickDivider - 1; i++) {
		blockPhysics.Tick(TestUtils::GetFallTime(blockPhysics, 1) / tickDivider);
	}
	blockPhysics.Tick(TestUtils::GetFallTime(blockPhysics, 1) / tickDivider + TestUtils::veryShortTime);
	if (!TestUtils::AreAlmostIdenticalAsExpected(blockPhysics.GetBlockMatrix(), blockMatrix, matchedAndSwipedBlockPositions))
		return false;
	if (!TestUtils::AllBlocksAreFilledAsExpected(blockPhysics.GetBlockMatrix()))
		return false;

	return true;
}

float TestUtils::GetFallTime(const BlockPhysics& blockPhysics, int howManyGridsToFall) {
	return FMath::Sqrt(2 * blockPhysics.GRID_SIZE * howManyGridsToFall / blockPhysics.GRAVITY_ACCELERATION);
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

const BlockMatrix TestUtils::twoByTwoMatchTest1 = BlockMatrix{
	TArray<TArray<Block>>{
		{ Block::ONE, Block::TWO, Block::TWO, Block::ONE, Block::ONE},
		{ Block::ONE, Block::TWO, Block::THREE, Block::TWO, Block::ONE},
		{ Block::THREE, Block::ONE, Block::THREE, Block::FOUR, Block::THREE}
	}
};

const BlockMatrix TestUtils::twoByTwoMatchTest2 = BlockMatrix{
	TArray<TArray<Block>>{
		{Block::ONE, Block::TWO, Block::TWO},
		{Block::TWO, Block::ONE, Block::TWO},
		{Block::THREE, Block::FOUR, Block::THREE}
	}
};

const BlockMatrix TestUtils::munchickenRollTest = BlockMatrix{
	TArray<TArray<Block>>{
		{Block::ONE, Block::ONE, Block::TWO, Block::TWO},
		{Block::THREE, Block::THREE, Block::FOUR, Block::FOUR},
		{Block::ONE, Block::ONE, Block::MUNCHICKEN, Block::TWO},
		{Block::THREE, Block::THREE, Block::FOUR, Block::FOUR}
	}
};

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

bool TestUtils::AreNewBlocksSpawnedAtTopAsExpected(const BlockPhysics& blockPhysics, const TArray<int>& newBlockSpawnExpectedCols)
{
	for (int colIndex = 0; colIndex < blockPhysics.GetNumRows(); colIndex++) {
		const auto pos = FIntPoint{ -1,colIndex };
		const auto isNewBlockCreated = !blockPhysics.IsEmpty(pos) || blockPhysics.ExistsBlockNear(pos, blockPhysics.GRID_SIZE/3.f);
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

bool TestUtils::AreNewBlocksSpawnedAsExpected(const BlockPhysics& blockPhysics, int colToInspect, int expectedNewBlocksCount)
{
	for (int i = -1; i >= -expectedNewBlocksCount; i--) {
		if (!blockPhysics.ExistsBlockNear(FIntPoint{ i, colToInspect }, blockPhysics.GRID_SIZE/3.f)) {
			UE_LOG(LogTemp, Error, TEXT("Block expected to be exist at (%d,%d) but not present"),
				i, colToInspect);
			return false;
		}
	}
	return true;
}

bool TestUtils::AreNewBlocksNotSpawnedAsExpected(const BlockPhysics& blockPhysics, int colToInspect)
{
	if (blockPhysics.ExistsBlockNear(FIntPoint{ -1, colToInspect }, blockPhysics.GRID_SIZE / 3.f)) {
		UE_LOG(LogTemp, Error, TEXT("Block should not exist at (%d,%d) but present"),
			-1, colToInspect);
		return false;
	}
	return true;
}

bool TestUtils::AreAlmostIdenticalAsExpected(const BlockMatrix& currentMatrix, const BlockMatrix& originalMatrix, const TArray<FIntPoint>& exceptionalPositions)
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
					UE_LOG(LogTemp, Error, TEXT("Block (type: %d) at (%d, %d) differs from original block (type: %s)"),
						currentBlock, i, j, *PrettyPrint(originalBlock));
					return false;
				}
			}
		}
	}
	return true;
}

bool TestUtils::AllBlocksAreFilledAsExpected(const BlockMatrix& blockMatrix)
{
	const auto numRow = blockMatrix.GetNumRows();
	const auto numCol = blockMatrix.GetNumCols();
	const auto block2DArray = blockMatrix.GetBlock2DArray();
	for (int i = 0; i < numRow; i++) {
		for (int j = 0; j < numCol; j++) {
			if (block2DArray[i][j] == Block::INVALID) {
				UE_LOG(LogTemp, Error, TEXT("Block at (%d, %d) is invalid, where all blocks are expected to be filled"), i, j);
				return false;
			}
		}
	}
	return true;
}

bool TestUtils::IsExpectedBlockExistsAt(const BlockMatrix& blockMatrix, FIntPoint positionToInspect, Block expectedBlockType)
{
	const auto actualBlockType = blockMatrix.At(positionToInspect.X, positionToInspect.Y);
	if (actualBlockType != expectedBlockType) {
		UE_LOG(LogTemp, Error, TEXT("expected: %s but was %s at (%d, %d)"), *PrettyPrint(expectedBlockType), *PrettyPrint(actualBlockType), positionToInspect.X, positionToInspect.Y);
		return false;
	}
	return true;
}

bool TestUtils::IsNotExpectedBlockNotExistAt(const BlockMatrix& blockMatrix, FIntPoint positionToInspect, Block notExpectedBlockType)
{
	const auto actualBlockType = blockMatrix.At(positionToInspect.X, positionToInspect.Y);
	if (actualBlockType == notExpectedBlockType) {
		UE_LOG(LogTemp, Error, TEXT("Not expected: %s but was %s at (%d, %d)"), *PrettyPrint(notExpectedBlockType), *PrettyPrint(actualBlockType), positionToInspect.X, positionToInspect.Y);
		return false;
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(MultipleNewBlocksShouldBeGenerated, "Board.OnSwipe.Multiple new blocks should be generated if needed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MultipleNewBlocksShouldBeGenerated::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeColumn = 2;
	const auto swipeStart = FIntPoint{ 3,swipeColumn };
	const auto swipeEnd = FIntPoint{ 2,swipeColumn };
	auto blockMatrix = TestUtils::blockMatrix5x5;
	auto newBlockCount = 0;
	auto newBlockGenerator = [&newBlockCount]() -> int {
		const auto blockOne = static_cast<int>(Block::ONE);
		const auto blockTwo = static_cast<int>(Block::TWO);
		const auto newBlocks = TArray<int>{ blockOne, blockOne, blockTwo };
		if (newBlockCount < newBlocks.Num())
			return newBlocks[newBlockCount++];
		else {
			UE_LOG(LogTemp, Error, TEXT("More than three blocks are randomly generated"));
			return 0;
		}
	};
	auto blockPhysics = BlockPhysics(blockMatrix, newBlockGenerator);

	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	// After swipe move end
	blockPhysics.Tick((blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED) + TestUtils::veryShortTime);
	// After destroy animation end
	blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME + TestUtils::veryShortTime);
	const auto expectedNewBlocksCount = 3;
	if (!TestUtils::AreNewBlocksSpawnedAsExpected(blockPhysics, swipeColumn, expectedNewBlocksCount))
		return false;
	// After falling end
	blockPhysics.Tick(TestUtils::GetFallTime(blockPhysics, 3) + TestUtils::veryShortTime);
	if (!TestUtils::AllBlocksAreFilledAsExpected(blockPhysics.GetBlockMatrix()))
		return false;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenShouldBeGenerated, "Board.OnSwipe.Munchicken should be generated if 2x2 matched", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenShouldBeGenerated::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeStart = FIntPoint{ 1, 3 };
	const auto swipeEnd = FIntPoint{ 1, 2 };
	auto blockMatrix = TestUtils::twoByTwoMatchTest1;
	auto blockPhysics = BlockPhysics(blockMatrix);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	// After swipe move end
	blockPhysics.Tick((blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED) + TestUtils::veryShortTime);
	const auto destroyExpectedPositions = TArray<FIntPoint>{
		FIntPoint{0,1}, FIntPoint{0,2}, FIntPoint{1,1}, FIntPoint{1,2}
	};
	if (!TestUtils::IsCorrectlyGettingDestroyed(blockPhysics, destroyExpectedPositions))
		return false;
	// After destroy animation end
	blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME + TestUtils::veryShortTime);
	const auto expectedMunchickenSpawnPosition = FIntPoint{ 1, 2 };
	if (!TestUtils::IsExpectedBlockExistsAt(blockPhysics.GetBlockMatrix(), expectedMunchickenSpawnPosition, Block::MUNCHICKEN))
		return false;
	if (!TestUtils::AreNewBlocksSpawnedAsExpected(blockPhysics, 1, 2) || !TestUtils::AreNewBlocksSpawnedAsExpected(blockPhysics, 2, 1))
		return false;
	// After falling end
	blockPhysics.Tick(TestUtils::GetFallTime(blockPhysics, 1) + TestUtils::veryShortTime);
	if (!TestUtils::IsExpectedBlockExistsAt(blockPhysics.GetBlockMatrix(), FIntPoint{ 1,2 }, Block::MUNCHICKEN))
		return false;

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenShouldBeGeneratedAtBlockInflowPosition, "Board.OnSwipe.Munchicken should be generated at block inflow position", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenShouldBeGeneratedAtBlockInflowPosition::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeStart = FIntPoint{ 1, 0 };
	const auto swipeEnd = FIntPoint{ 1, 1 };
	auto blockMatrix = TestUtils::twoByTwoMatchTest2;
	auto blockPhysics = BlockPhysics(blockMatrix);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	// After swipe move end
	blockPhysics.Tick((blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED) + TestUtils::veryShortTime);
	// After destroy animation end
	blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME + TestUtils::veryShortTime);
	const auto expectedMunchickenSpawnPosition = FIntPoint{ 1, 1 };
	if (!TestUtils::IsExpectedBlockExistsAt(blockPhysics.GetBlockMatrix(), expectedMunchickenSpawnPosition, Block::MUNCHICKEN))
		return false;

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(SwipeOnMunchickenShouldRollIt, "Board.OnSwipe.Swipe on Munchicken should roll it", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool SwipeOnMunchickenShouldRollIt::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeStart = FIntPoint{ 2, 2 };
	const auto swipeEnd = FIntPoint{ 2, 1 };
	const auto swipeDirectionVec = FIntPoint{ 0, -1 };
	auto blockMatrix = TestUtils::munchickenRollTest;
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysics = BlockPhysics(blockMatrix, newBlockGenerator);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	const auto numGridsToGo = 3;
	// for each cells this munchicken enter
	for (int i = 0; i < numGridsToGo; i++) {
		blockPhysics.Tick((blockPhysics.GRID_SIZE / blockPhysics.ROLL_SPEED) + TestUtils::veryShortTime);
		const auto munchickenPreviousPosition = swipeStart + swipeDirectionVec*i;
		if (!TestUtils::AreNewBlocksSpawnedAsExpected(blockPhysics, munchickenPreviousPosition.Y, 1))
			return false;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(SwipeOnMunchickenShouldRollItForFrequentTick, "Board.OnSwipe.Swipe on Munchicken should roll it for frequent tick", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool SwipeOnMunchickenShouldRollItForFrequentTick::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeStart = FIntPoint{ 2, 2 };
	const auto swipeEnd = FIntPoint{ 2, 1 };
	const auto swipeDirectionVec = FIntPoint{ 0, -1 };
	auto blockMatrix = TestUtils::munchickenRollTest;
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysics = BlockPhysics(blockMatrix, newBlockGenerator);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	// After time to pass one cell with frequent tick
	auto divisor = 100;
	for (int i = 0; i < divisor; i++)
		blockPhysics.Tick(blockPhysics.GRID_SIZE / (blockPhysics.ROLL_SPEED * divisor));
	blockPhysics.Tick(TestUtils::veryShortTime);
	if (!TestUtils::IsNotExpectedBlockNotExistAt(blockPhysics.GetBlockMatrix(), swipeEnd, Block::ONE))
		return false;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenVerticalRollShouldNotSpawnNewBlocks, "Board.OnSwipe.Munchicken vertical roll should not spawn new blocks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenVerticalRollShouldNotSpawnNewBlocks::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeStart = FIntPoint{ 2, 2 };
	const auto swipeEnd = FIntPoint{ 1, 2 };
	const auto swipeDirectionVec = FIntPoint{ -1, 0 };
	const auto movingColIndex = 2;
	auto blockMatrix = TestUtils::munchickenRollTest;
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysics = BlockPhysics(blockMatrix, newBlockGenerator);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	const auto numGridsToGo = 3;
	// for each cells this munchicken enter
	for (int i = 0; i < numGridsToGo-1; i++) {
		blockPhysics.Tick((blockPhysics.GRID_SIZE / blockPhysics.ROLL_SPEED) + TestUtils::veryShortTime);
		if (!TestUtils::AreNewBlocksNotSpawnedAsExpected(blockPhysics, movingColIndex))
			return false;
	}
	// when this munchicken gets destroyed
	blockPhysics.Tick((blockPhysics.GRID_SIZE / blockPhysics.ROLL_SPEED) + TestUtils::veryShortTime);
	if (!TestUtils::AreNewBlocksSpawnedAsExpected(blockPhysics, movingColIndex, 3))
		return false;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenShouldFallIfIdle, "Board.OnSwipe.Munchicken should fall if idle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenShouldFallIfIdle::RunTest(const FString& Parameters) {
	// Setup
	const auto swipeStart = FIntPoint{ 0, 0 };
	const auto swipeEnd = FIntPoint{ 1, 0 };
	const auto swipeDirectionVec = FIntPoint{ 1, 0 };
	auto blockMatrix = BlockMatrix(TArray<TArray<Block>>{
		{ Block::ONE, Block::MUNCHICKEN, Block::TWO },
		{ Block::TWO, Block::ONE, Block::ONE }
	});
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysics = BlockPhysics(blockMatrix, newBlockGenerator);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	// Swipe move end
	blockPhysics.Tick(blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED + TestUtils::veryShortTime);
	// block destroy end
	blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME + TestUtils::veryShortTime);
	// Fall
	blockPhysics.Tick(TestUtils::GetFallTime(blockPhysics, 1) + TestUtils::veryShortTime);
	const auto munchickenExpectedPosition = FIntPoint{ 1, 1 };
	if (!TestUtils::IsExpectedBlockExistsAt(blockPhysics.GetBlockMatrix(), munchickenExpectedPosition, Block::MUNCHICKEN))
		return false;
	return true;
}