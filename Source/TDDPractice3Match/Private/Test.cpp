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
	const auto veryShortTime = 0.00001f;
	const auto swipeStart = FIntPoint{ 1, 2 };
	const auto swipeEnd = FIntPoint{ 1, 3 };
	const auto movingBlockPositions = TSet<FIntPoint>{ swipeStart, swipeEnd };
	auto blockMatrix = BlockMatrix( 
		TArray<TArray<Block>>{
			{Block::ONE, Block::ONE, Block::TWO, Block::ONE, Block::TWO},
			{Block::ONE, Block::TWO, Block::TWO, Block::THREE, Block::FOUR},
			{Block::THREE, Block::FOUR, Block::THREE, Block::TWO, Block::FOUR},
			{Block::ONE, Block::ONE, Block::TWO, Block::ONE, Block::THREE},
			{Block::TWO, Block::THREE, Block::ONE, Block::TWO, Block::THREE}
		}
	);
	auto blockPhysics = BlockPhysics(blockMatrix);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	// After swipe animation end
	const auto swipeMoveTime = blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED;
	blockPhysics.Tick(swipeMoveTime + veryShortTime);
	const auto blockMatrixAfterSwipe = blockPhysics.GetBlockMatrix();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			const auto position = FIntPoint{ i, j };
			if (!movingBlockPositions.Contains(position)) {
				const auto currentBlock = blockMatrixAfterSwipe.At(i, j);
				const auto originalBlock = blockMatrix.At(i, j);
				if (!blockPhysics.IsIdleAt(position) || (currentBlock != originalBlock)) {
					UE_LOG(LogTemp, Error, TEXT("After swipe, block (type: %d) at (%d, %d) is either not idle or differs from original block (type: %d)"),
						currentBlock, i, j, originalBlock);
					return false;
				}
			}
		}
	}
	// After returning back animation end
	blockPhysics.Tick(swipeMoveTime + veryShortTime);
	const auto blockMatrixAfterReturn = blockPhysics.GetBlockMatrix();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			const auto currentBlock = blockMatrixAfterReturn.At(i, j);
			const auto originalBlock = blockMatrix.At(i, j);
			if (!blockPhysics.IsIdleAt(FIntPoint{ i, j }) || (currentBlock != originalBlock)) {
				UE_LOG(LogTemp, Error, TEXT("After swipe return, block (type: %d) at (%d, %d) is either not idle or differs from original block (type: %d)"),
					currentBlock, i, j, originalBlock);
				return false;
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(OnSwipeMatchCheckShouldOccur, "Board.OnSwipe.Match should occur when there's a match", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool OnSwipeMatchCheckShouldOccur::RunTest(const FString& Parameters)
{
	// Setup
	const auto veryShortTime = 0.00001f;
	const auto swipeStart = FIntPoint{ 0,3 };
	const auto swipeEnd = FIntPoint{ 0,2 };
	auto blockMatrix = BlockMatrix(
		TArray<TArray<Block>>{
			{Block::ONE, Block::ONE, Block::TWO, Block::ONE, Block::TWO},
			{ Block::ONE, Block::TWO, Block::TWO, Block::THREE, Block::FOUR },
			{ Block::THREE, Block::FOUR, Block::THREE, Block::TWO, Block::FOUR },
			{ Block::ONE, Block::ONE, Block::TWO, Block::ONE, Block::THREE },
			{ Block::TWO, Block::THREE, Block::ONE, Block::TWO, Block::THREE }
		}
	);
	auto blockPhysics = BlockPhysics(blockMatrix);
	// Swipe
	blockPhysics.RecieveSwipeInput(swipeStart, swipeEnd);
	// After swipe animation end
	const auto swipeMoveTime = blockPhysics.GRID_SIZE / blockPhysics.SWIPE_MOVE_SPEED;
	blockPhysics.Tick(swipeMoveTime + veryShortTime);
	const auto matchedBlockPositions = TArray<FIntPoint>{ FIntPoint{0,0}, FIntPoint{0,1}, FIntPoint{0,2} };
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			const auto isGettingDestroyed = blockPhysics.IsPlayingDestroyAnimAt(FIntPoint{ i,j });
			if (matchedBlockPositions.Contains(FIntPoint{ i,j })) {
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
	// After destroy animation end
	blockPhysics.Tick(blockPhysics.DESTROY_ANIMATION_TIME + veryShortTime);
	// block destroy check
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			const auto isDestroyed = blockPhysics.IsEmpty(FIntPoint{ i, j });
			if (matchedBlockPositions.Contains(FIntPoint{ i,j })) {
				if (!isDestroyed) {
					UE_LOG(LogTemp, Error, TEXT("Block destory animation finished, but the cell is not empty. cell: (%d, %d)"), i, j);
					return false;
				}
			}
			else {
				if (isDestroyed) {
					UE_LOG(LogTemp, Error, TEXT("Idle block gets destroyed at (%d, %d)"), i, j);
					return false;
				}
			}
		}
	}
	// new block spawn test
	const auto newBlockSpawnPositions = TArray<FIntPoint>{ FIntPoint{-1,0}, FIntPoint{-1,1}, FIntPoint{-1,2} };
	for (int i = 0; i < 5; i++) {
		const auto pos = FIntPoint{ -1,i };
		const auto isNewBlockCreated = !blockPhysics.IsEmpty(pos) || blockPhysics.ExistsBlockBetween(pos, pos + FIntPoint{ 1,0 });
		if (newBlockSpawnPositions.Contains(pos)) {
			if (!isNewBlockCreated) {
				UE_LOG(LogTemp, Error, TEXT("New blocks are not created at (%d, %d)"), pos.X, pos.Y);
				return false;
			}
		}
		else {
			if (isNewBlockCreated) {
				UE_LOG(LogTemp, Error, TEXT("New blocks are created at (%d, %d)"), pos.X, pos.Y);
			}
		}
	}
	return true;
}