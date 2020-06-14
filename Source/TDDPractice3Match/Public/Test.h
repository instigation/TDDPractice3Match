// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"

class BlockPhysics;

static class TestUtils {
public:
	static const float veryShortTime;
	static const BlockMatrix blockMatrix5x5;

	static bool IsCorrectlyGettingDestroyed(const BlockPhysics& blockPhysics, const TArray<FIntPoint>& onlyPositionsThatShouldBeDestroyed);
	static bool IsCorrectlyEmpty(const BlockPhysics& blockPhysics, const TArray<FIntPoint>& onlyPositionsThatShouldBeEmpty);
	static bool AreNewBlocksSpawnedAtTop(const BlockPhysics& blockPhysics, const TArray<int>& newBlockSpawnExpectedCols);
	static bool AreIdenticalExcept(const BlockMatrix& blockMatrix1, const BlockMatrix& blockMatrix2, const TArray<FIntPoint>& exceptionalPositions);
	static bool AllBlocksAreFilled(const BlockMatrix& blockMatrix);
};