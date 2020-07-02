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