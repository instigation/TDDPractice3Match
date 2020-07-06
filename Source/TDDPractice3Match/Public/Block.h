#pragma once

#include "CoreMinimal.h"

/**
 *
 */
enum class TDDPRACTICE3MATCH_API Block {
    MIN,
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    MAX_NORMAL,
	MUNCHICKEN,
	VERTICAL_LINE_CLEAR,
    HORIZONTAL_LINE_CLEAR,
    MAX_SPECIAL,
    INVALID,
    MAX
};

FString PrettyPrint(Block block);

TArray<Block> GetNormalBlocks();
bool IsSpecial(Block block);

enum class TDDPRACTICE3MATCH_API BlockColor {
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    MAX
};

BlockColor GetColor(Block block);