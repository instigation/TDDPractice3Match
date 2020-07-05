// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "BlockMatrix.h"

class BlockPhysics;

static class TestUtils {
public:
	static const float veryShortTime;
	static const BlockMatrix blockMatrix5x5;
	static const BlockMatrix twoByTwoMatchTest1;
	static const BlockMatrix twoByTwoMatchTest2;
	static const BlockMatrix munchickenRollTest;

	static bool IsCorrectlyGettingDestroyed(const BlockPhysics& blockPhysics, const TArray<FIntPoint>& onlyPositionsThatShouldBeDestroyed);
	static bool IsCorrectlyEmpty(const BlockPhysics& blockPhysics, const TArray<FIntPoint>& onlyPositionsThatShouldBeEmpty);
	static bool AreNewBlocksSpawnedAtTopAsExpected(const BlockPhysics& blockPhysics, const TArray<int>& newBlockSpawnExpectedCols);
	static bool AreNewBlocksSpawnedAsExpected(const BlockPhysics& blockPhysics, int colToInspect, int expectedNewBlocksCount);
	static bool AreNewBlocksNotSpawnedAsExpected(const BlockPhysics& blockPhysics, int colToInspect);
	static bool AreAlmostIdenticalAsExpected(const BlockMatrix& blockMatrix1, const BlockMatrix& blockMatrix2, const TArray<FIntPoint>& exceptionalPositions);
	static bool AllBlocksAreFilledAsExpected(const BlockMatrix& blockMatrix);
	static bool IsExpectedBlockExistsAt(const BlockMatrix& blockMatrix, FIntPoint positionToInspect, Block expectedBlockType);
	static bool IsNotExpectedBlockNotExistAt(const BlockMatrix& blockMatrix, FIntPoint positionToInspect, Block notExpectedBlockType);

	class FrequentTicker {
	public:
		FrequentTicker(BlockPhysics& blockPhysics, int tickDivider, float veryShortTimeToAdd);
		bool TickFrequent(float deltaSeconds, const TFunction<bool(void)>& isExpectationMet = []() { return true; });
	private:
		BlockPhysics& blockPhysics;
		int tickDivider;
		float veryShortTimeToAdd;
	};

	static float GetFallTime(const BlockPhysics& blockPhysics, int howManyGridsToFall);
};