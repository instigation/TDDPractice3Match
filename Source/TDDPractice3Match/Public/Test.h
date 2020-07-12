// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "BlockMatrix.h"

class BlockPhysics;

static class TestUtils {
public:
	static const BlockMatrix blockMatrix5x5;
	static const BlockMatrix twoByTwoMatchTest1;
	static const BlockMatrix twoByTwoMatchTest2;
	static const BlockMatrix munchickenRollTest;
	static const BlockMatrix oneByFourMatchTest;
	static const BlockMatrix lineClearerTest;
};