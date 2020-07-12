// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class BlockMatrix;
class Block;
class BlockPhysics;

class TDDPRACTICE3MATCH_API BlockPhysicsTester
{
public:
	explicit BlockPhysicsTester(const BlockMatrix& initialBlockMatrix, TFunction<int(void)> randomGenerator = rand);

	void DoSwipe(const FIntPoint& swipeStart, const FIntPoint& swipeEnd) const;

	void SetTickDivider(int newTickDivider) { tickDivider = newTickDivider; }
	void SetOnTickEndTest(TFunction<void(const BlockPhysicsTester&)> test) { onTickEndTest = test; }
	void SetDuringFrequentTickTest(TFunction<void(const BlockPhysicsTester&)> test) { duringFrequentTickTest = test; }
	void ResetOnTickEndTest() { onTickEndTest = [](const BlockPhysicsTester&) {}; }
	void ResetDuringFrequentTickTest() { duringFrequentTickTest = [](const BlockPhysicsTester&) {}; }

	void TickUntilSwipeMoveAnimationEnd();
	void TickForHalfSwipeMove();
	void TickUntilSwipeReturnAnimtaionEnd();
	void TickUntilBlockDestroyEnd();
	void TickUntilBlockFallEnd(int numGridsToFall);
	void TickUntilRollOneGrid();

	void TestBlockOccurrence(const Block& expectedBlock, int expectedOccurance) const;
	void TestIfCorrectlyEmpty(const TSet<FIntPoint>& onlyPositionsThatShouldBeEmpty) const;
	void TestIfBlockExistsAt(const Block& expectedBlock, const FIntPoint& expectedPosition) const;
	void TestIfBlockNotExistsAt(const Block& notExpectedBlock, const FIntPoint& positionToInspect) const;
	void TestIfAllBlocksAreFilled() const;
	void TestIfAlmostIdenticalTo(const BlockMatrix& blockMatrix, const TSet<FIntPoint>& exceptionalPositions) const;
	void TestIfNewBlocksNotSpawnedAtCol(int colToInspect) const;
	void TestIfNewBlocksSpawnedAtCol(int colToInspect, int expectedNewBlocksCount) const;
	void TestIfNewBlocksSpawnedAtCol(const TSet<int>& newBlockSpawnExpectedCols) const;
	void TestIfCorrectlyGettingDestroyed(const TSet<FIntPoint>& onlyPositionsThatShouldBeDestroyed) const;

private:
	void TickFor(float deltaSeconds);

	float GetFallTime(int numGridsToFall);

	TUniquePtr<BlockPhysics> blockPhysics;

	TFunction<void(const BlockPhysicsTester&)> onTickEndTest = [](const BlockPhysicsTester&) {};
	TFunction<void(const BlockPhysicsTester&)> duringFrequentTickTest = [](const BlockPhysicsTester&) {};

	int tickDivider = 1;

	constexpr static float VERY_SHORT_TIME = 0.00001f;
};
