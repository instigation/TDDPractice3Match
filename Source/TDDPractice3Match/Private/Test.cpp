// Fill out your copyright notice in the Description page of Project Settings.


#include "../Public/Test.h"
#include "../Public/Block.h"
#include "../Public/BlockPhysics.h"
#include "Misc/AutomationTest.h"
#include "../Public/BlockPhysicsTester.h"


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

	const auto swipeStart = FIntPoint{ 1, 2 };
	const auto swipeEnd = FIntPoint{ 1, 3 };
	const auto movingBlockPositions = TSet<FIntPoint>{ swipeStart, swipeEnd };
	auto blockMatrix = TestUtils::blockMatrix5x5;
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	blockPhysicsTester.TestIfAlmostIdenticalTo(blockMatrix, movingBlockPositions);
	blockPhysicsTester.TickForHalfSwipeMove();
	blockPhysicsTester.TestIfCorrectlyEmpty(movingBlockPositions);
	blockPhysicsTester.TickForHalfSwipeMove();
	blockPhysicsTester.TestIfAlmostIdenticalTo(blockMatrix, TSet<FIntPoint>{});
	return true;
}

bool OnSwipeMatchCheckShouldOccurTest(int tickDivider) {

	const auto swipeStart = FIntPoint{ 0,3 };
	const auto swipeEnd = FIntPoint{ 0,2 };
	auto blockMatrix = TestUtils::blockMatrix5x5;
	auto newBlockCount = 0;
	auto newBlockGenerator = [&newBlockCount]() -> int {
		const auto blockOne = static_cast<int>(BlockColor::ONE);
		const auto blockTwo = static_cast<int>(BlockColor::TWO);
		const auto newBlocks = TArray<int>{ blockOne, blockOne, blockTwo };
		if(newBlockCount < newBlocks.Num())
			return newBlocks[newBlockCount++];
		else {
			UE_LOG(LogTemp, Error, TEXT("More than three blocks are randomly generated"));
			return 0;
		}
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.SetTickDivider(tickDivider);
	blockPhysicsTester.SetDuringFrequentTickTest([blockMatrix, swipeStart, swipeEnd](const BlockPhysicsTester& tester) -> void {
		tester.TestIfAlmostIdenticalTo(blockMatrix, TSet<FIntPoint>{swipeStart, swipeEnd});
		});
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	const auto matchedBlockPositions = TSet<FIntPoint>{ FIntPoint{0,0}, FIntPoint{0,1}, FIntPoint{0,2} };
	blockPhysicsTester.TestIfCorrectlyGettingDestroyed(matchedBlockPositions);
	const auto matchedAndSwipedBlockPositions = TSet<FIntPoint>{ FIntPoint{0,0}, FIntPoint{0,1}, FIntPoint{0,2}, FIntPoint{0,3} };
	blockPhysicsTester.SetDuringFrequentTickTest(
		[matchedBlockPositions, blockMatrix, matchedAndSwipedBlockPositions](const BlockPhysicsTester& tester) {
			tester.TestIfCorrectlyGettingDestroyed(matchedBlockPositions);
			tester.TestIfAlmostIdenticalTo(blockMatrix, matchedAndSwipedBlockPositions);
		}
	);
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	blockPhysicsTester.TestIfCorrectlyEmpty(matchedBlockPositions);
	const auto shouldBeSpawnedCols = TSet<int>{ 0, 1, 2 };
	blockPhysicsTester.TestIfNewBlocksSpawnedAtCol(shouldBeSpawnedCols);
	blockPhysicsTester.ResetDuringFrequentTickTest();
	blockPhysicsTester.TickUntilBlockFallEnd(1);
	blockPhysicsTester.TestIfAlmostIdenticalTo(blockMatrix, matchedAndSwipedBlockPositions);
	blockPhysicsTester.TestIfAllBlocksAreFilled();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(OnSwipeMatchCheckShouldOccur, "Board.OnSwipe.Match should occur when there's a match", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool OnSwipeMatchCheckShouldOccur::RunTest(const FString& Parameters)
{
	return OnSwipeMatchCheckShouldOccurTest(1);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(TickFrequencyShouldNotMatter, "Board.OnSwipe.Match should occur even if Tick is frequent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool TickFrequencyShouldNotMatter::RunTest(const FString& Parameters) {
	if (!OnSwipeMatchCheckShouldOccurTest(10))
		return false;
	if (!OnSwipeMatchCheckShouldOccurTest(60))
		return false;
	return true;
}

const BlockMatrix TestUtils::blockMatrix5x5 = BlockMatrix(
	TArray<TArray<Block>>{
		{ Block::ONE, Block::ONE, Block::TWO, Block::ONE, Block::THREE },
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

const BlockMatrix TestUtils::oneByFourMatchTest = BlockMatrix{
	TArray<TArray<Block>>{
		{Block::ZERO, Block::ONE, Block::ZERO, Block::ZERO},
		{Block::ONE, Block::ZERO, Block::TWO, Block::TWO},
		{Block::TWO, Block::ONE, Block::ZERO, Block::TWO},
		{Block::TWO, Block::TWO, Block::ZERO, Block::FOUR}
	}
};

const BlockMatrix TestUtils::lineClearerTest = BlockMatrix{
	TArray<TArray<Block>>{
		{Block::ONE, Block::TWO, Block(BlockColor::ONE, BlockSpecialAttribute::VERTICAL_LINE_CLEAR)},
		{Block::THREE, Block::ONE, Block::THREE},
		{Block::FOUR, Block::ZERO, Block(BlockColor::TWO, BlockSpecialAttribute::HORIZONTAL_LINE_CLEAR)}
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MultipleNewBlocksShouldBeGenerated, "Board.BlockSpawn.Multiple new blocks should be generated if needed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MultipleNewBlocksShouldBeGenerated::RunTest(const FString& Parameters) {

	const auto swipeColumn = 2;
	const auto swipeStart = FIntPoint{ 3,swipeColumn };
	const auto swipeEnd = FIntPoint{ 2,swipeColumn };
	auto blockMatrix = TestUtils::blockMatrix5x5;
	auto newBlockCount = 0;
	auto newBlockGenerator = [&newBlockCount]() -> int {
		const auto blockOne = static_cast<int>(BlockColor::ONE);
		const auto blockTwo = static_cast<int>(BlockColor::TWO);
		const auto newBlocks = TArray<int>{ blockOne, blockOne, blockTwo };
		if (newBlockCount < newBlocks.Num())
			return newBlocks[newBlockCount++];
		else {
			UE_LOG(LogTemp, Error, TEXT("More than three blocks are randomly generated"));
			return 0;
		}
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	const auto expectedNewBlocksCount = 3;
	blockPhysicsTester.TestIfNewBlocksSpawnedAtCol(swipeColumn, expectedNewBlocksCount);
	blockPhysicsTester.TickUntilBlockFallEnd(3);
	blockPhysicsTester.TestIfAllBlocksAreFilled();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenShouldBeGenerated, "Board.MatchRule.Rollable should be generated if 2x2 matched", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenShouldBeGenerated::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 1, 3 };
	const auto swipeEnd = FIntPoint{ 1, 2 };
	auto blockMatrix = TestUtils::twoByTwoMatchTest1;
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	const auto destroyExpectedPositions = TSet<FIntPoint>{
		FIntPoint{0,1}, FIntPoint{0,2}, FIntPoint{1,1}, FIntPoint{1,2}
	};
	blockPhysicsTester.TestIfCorrectlyGettingDestroyed(destroyExpectedPositions);
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	const auto expectedMunchickenSpawnPosition = FIntPoint{ 1, 2 };
	blockPhysicsTester.TestIfBlockExistsAt(Block::MUNCHICKEN, expectedMunchickenSpawnPosition);
	blockPhysicsTester.TestIfNewBlocksSpawnedAtCol(1, 2);
	blockPhysicsTester.TestIfNewBlocksSpawnedAtCol(2, 1);
	blockPhysicsTester.TickUntilBlockFallEnd(1);
	blockPhysicsTester.TestIfBlockExistsAt(Block::MUNCHICKEN, FIntPoint{ 1, 2 });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenShouldBeGeneratedAtBlockInflowPosition, "Board.Rollable.Rollable should be generated at block inflow position", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenShouldBeGeneratedAtBlockInflowPosition::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 1, 0 };
	const auto swipeEnd = FIntPoint{ 1, 1 };
	auto blockMatrix = TestUtils::twoByTwoMatchTest2;
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	const auto expectedMunchickenSpawnPosition = FIntPoint{ 1, 1 };
	blockPhysicsTester.TestIfBlockExistsAt(Block::MUNCHICKEN, expectedMunchickenSpawnPosition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(SwipeOnMunchickenShouldRollIt, "Board.Rollable.Swipe on Rollable should roll it", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool SwipeOnMunchickenShouldRollIt::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 2, 2 };
	const auto swipeEnd = FIntPoint{ 2, 1 };
	const auto swipeDirectionVec = FIntPoint{ 0, -1 };
	auto blockMatrix = TestUtils::munchickenRollTest;
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	const auto numGridsToGo = 3;
	// for each cells this rollable enter
	for (int i = 0; i < numGridsToGo; i++) {
		blockPhysicsTester.TickUntilRollOneGrid();
		const auto munchickenPreviousPosition = swipeStart + swipeDirectionVec*i;
		blockPhysicsTester.TestIfNewBlocksSpawnedAtCol(munchickenPreviousPosition.Y, 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(SwipeOnMunchickenShouldRollItForFrequentTick, "Board.Rollable.Swipe on Rollable should roll it for frequent tick", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool SwipeOnMunchickenShouldRollItForFrequentTick::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 2, 2 };
	const auto swipeEnd = FIntPoint{ 2, 1 };
	auto blockMatrix = TestUtils::munchickenRollTest;
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.SetTickDivider(20);
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.TestIfBlockNotExistsAt(Block::ONE, swipeEnd);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenVerticalRollShouldNotSpawnNewBlocks, "Board.Rollable.Rollable vertical roll should not spawn new blocks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenVerticalRollShouldNotSpawnNewBlocks::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 2, 2 };
	const auto swipeEnd = FIntPoint{ 1, 2 };
	const auto movingColIndex = 2;
	auto blockMatrix = TestUtils::munchickenRollTest;
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.SetTickDivider(20); 
	const auto areNewBlockNotSpawned = [movingColIndex](const BlockPhysicsTester& tester) -> void {
		tester.TestIfNewBlocksNotSpawnedAtCol(movingColIndex);
	};
	blockPhysicsTester.SetOnTickEndTest(areNewBlockNotSpawned);
	const auto numGridsToGo = 3;
	// for each cells this rollable enter
	for (int i = 0; i < numGridsToGo - 1; i++) {
		blockPhysicsTester.TickUntilRollOneGrid();
	}
	blockPhysicsTester.ResetOnTickEndTest();
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	blockPhysicsTester.TestIfNewBlocksSpawnedAtCol(movingColIndex, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(MunchickenShouldFallIfIdle, "Board.Rollable.Rollable should fall if idle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool MunchickenShouldFallIfIdle::RunTest(const FString& Parameters) {
	const auto swipeStart = FIntPoint{ 0, 0 };
	const auto swipeEnd = FIntPoint{ 1, 0 };
	auto blockMatrix = BlockMatrix(TArray<TArray<Block>>{
		{ Block::ONE, Block::MUNCHICKEN, Block::TWO },
		{ Block::TWO, Block::ONE, Block::ONE }
	});
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	blockPhysicsTester.TickUntilBlockFallEnd(1);
	const auto munchickenExpectedPosition = FIntPoint{ 1, 1 };
	blockPhysicsTester.TestIfBlockExistsAt(Block::MUNCHICKEN, munchickenExpectedPosition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(RollableShouldRollOtherRollables, "Board.Rollable.Rollable should roll other rollables", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool RollableShouldRollOtherRollables::RunTest(const FString& Parameters) {
	const auto swipeStart = FIntPoint{ 1, 0 };
	const auto swipeEnd = FIntPoint{ 1, 1 };
	const auto blockMatrix = BlockMatrix(TArray<TArray<Block>>{
		{ Block::ZERO, Block::ONE, Block::TWO, Block::THREE },
		{ Block::MUNCHICKEN, Block::TWO, Block::MUNCHICKEN, Block::TWO },
		{ Block::ONE, Block::TWO, Block::THREE, Block::FOUR }
	});
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	const auto randomDirectionGenerator = []() -> int {
		return 0;
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator, randomDirectionGenerator);
	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.TestIfBlockExistsAt(Block::MUNCHICKEN, FIntPoint{ 1,3 });
	blockPhysicsTester.TestIfBlockExistsAt(Block::MUNCHICKEN, FIntPoint{ 0,2 });
	blockPhysicsTester.TestIfBlockNotExistsAt(Block::MUNCHICKEN, FIntPoint{ 1, 2 });
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(OverlappedRollablesShouldNotMakeOtherBlockRise, "Board.Rollable.Overlapped rollables should not make other block rise", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool OverlappedRollablesShouldNotMakeOtherBlockRise::RunTest(const FString& Parameters) {
	const auto swipeStart = FIntPoint{ 2, 0 };
	const auto swipeEnd = FIntPoint{ 2, 1 };
	const auto blockMatrix = BlockMatrix(TArray<TArray<Block>>{
		{ Block::ZERO, Block::ONE, Block::TWO, Block::THREE },
		{ Block::ONE, Block::TWO, Block::THREE, Block::FOUR },
		{ Block::MUNCHICKEN, Block::TWO, Block::MUNCHICKEN, Block::TWO }
	});
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	const auto randomDirectionGenerator = []() -> int {
		return 1;
	};
	auto blockPhysicsTester = BlockPhysicsTester(blockMatrix, newBlockGenerator, randomDirectionGenerator);
	blockPhysicsTester.SetTickDivider(3);
	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.TickUntilRollOneGrid();
	blockPhysicsTester.SetDuringFrequentTickTest(
		[](const BlockPhysicsTester& tester) -> void {
			tester.TestIfBlockExistsBetween(FIntPoint{ 1, 2 }, FIntPoint{ 2,2 });
		}
	);
	blockPhysicsTester.TickUntilRollOneGrid();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(OneByFourMatchShouldSpawnLineClearBlock, "Board.MatchRule.Line clearer should be generated if 1x4 matched", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool OneByFourMatchShouldSpawnLineClearBlock::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 1, 1 };
	const auto swipeEnd = FIntPoint{ 0, 1 };
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysicsTester = BlockPhysicsTester(TestUtils::oneByFourMatchTest, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	const auto lineClearBlockExpectedPosition = FIntPoint{ 0, 1 };
	const auto expectedLineClearBlock = Block(BlockColor::ZERO, BlockSpecialAttribute::VERTICAL_LINE_CLEAR);
	blockPhysicsTester.TestIfBlockExistsAt(expectedLineClearBlock, lineClearBlockExpectedPosition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FourByOneMatchShouldSpawnLineClearBlock, "Board.MatchRule.Line clearer should be generated if 4x1 matched", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FourByOneMatchShouldSpawnLineClearBlock::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 1, 1 };
	const auto swipeEnd = FIntPoint{ 1, 2 };
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysicsTester = BlockPhysicsTester(TestUtils::oneByFourMatchTest, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	const auto expectedLineClearBlock = Block(BlockColor::ZERO, BlockSpecialAttribute::HORIZONTAL_LINE_CLEAR);
	blockPhysicsTester.TestBlockOccurrence(expectedLineClearBlock, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(LineClearerShouldClearALineOnDestroy, "Board.LineClearer.Line clearer should clear a line on destory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool LineClearerShouldClearALineOnDestroy::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 1, 1 };
	const auto swipeEnd = FIntPoint{ 0, 1 };
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		return counter++;
	};
	auto blockPhysicsTester = BlockPhysicsTester(TestUtils::lineClearerTest, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	const auto blockDestroyExpectedPositions = TSet<FIntPoint>{
		{0,0}, {0,1}, {0,2}, 
		{1,2}, {2,2},
		{2,1}, {2,0}
	};
	blockPhysicsTester.TestIfCorrectlyEmpty(blockDestroyExpectedPositions);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(OnlyOneSpecialBlockShouldBeGeneratedEvenIfManyCandidatePositions, "Board.MatchRule.Only one special block should be generated even if candidate positions are many", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool OnlyOneSpecialBlockShouldBeGeneratedEvenIfManyCandidatePositions::RunTest(const FString& Parameters) {

	const auto swipeStart = FIntPoint{ 1, 0 };
	const auto swipeEnd = FIntPoint{ 0, 0 };
	auto counter = 0;
	const auto newBlockGenerator = [&counter]() -> int {
		const auto blockOne = static_cast<int>(BlockColor::ONE);
		const auto blockZero = static_cast<int>(BlockColor::ZERO);
		const auto newBlocks = TArray<int>{ blockOne, blockOne, blockZero };
		return newBlocks[(counter++)%3];
	};
	auto blockPhysicsTester = BlockPhysicsTester(TestUtils::twoByTwoMatchTest2, newBlockGenerator);

	blockPhysicsTester.DoSwipe(swipeStart, swipeEnd);
	blockPhysicsTester.TickUntilSwipeMoveAnimationEnd();
	// 1x3 match occurs
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	blockPhysicsTester.TickUntilBlockFallEnd(1);
	// 2x2 match occurs
	blockPhysicsTester.TickUntilBlockDestroyEnd();
	blockPhysicsTester.TestBlockOccurrence(Block(BlockColor::NONE, BlockSpecialAttribute::ROLLABLE), 1);
	return true;
}