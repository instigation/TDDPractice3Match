#pragma once

#include "CoreMinimal.h"
#include "ExplosionArea.h"

// When this enum gets updated, below arrays should be too.
enum class TDDPRACTICE3MATCH_API BlockColor {
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    NONE,
    INVALID
};

const static TArray<FString> blockColorEnumStrings = TArray<FString>{
	TEXT("ZERO"), TEXT("ONE"), TEXT("TWO"), TEXT("THREE"), TEXT("FOUR"), TEXT("NONE"), TEXT("INVALID")
};
const static TArray<BlockColor> validColors = TArray<BlockColor>{
	BlockColor::ZERO, BlockColor::ONE, BlockColor::TWO, BlockColor::THREE, BlockColor::FOUR
};

// When this enum gets updated, below arrays should be too.
enum class TDDPRACTICE3MATCH_API BlockSpecialAttribute {
    ROLLABLE,
    ONE_COLOR_CLEAR,
    VERTICAL_LINE_CLEAR,
    HORIZONTAL_LINE_CLEAR,
	DIAMOND_NEIGHBORHOOD_CLEAR,
	NONE,
    INVALID
};
const static TArray<FString> blockSpecialAttributeEnumStrings = TArray<FString>{
	TEXT("ROLLABLE"), TEXT("ONE_COLOR_CLEAR"), TEXT("VERTICAL_LINE_CLEAR"), TEXT("HORIZONTAL_LINE_CLEAR"),
	TEXT("DIAMOND_NEIGHBORHOOD_CLEAR"), TEXT("NONE"), TEXT("INVALID")
};
bool HasColor(BlockSpecialAttribute specialAttribute);
const static TArray<BlockSpecialAttribute> decoratorAttributes = TArray<BlockSpecialAttribute>{
    BlockSpecialAttribute::VERTICAL_LINE_CLEAR, BlockSpecialAttribute::HORIZONTAL_LINE_CLEAR,
    BlockSpecialAttribute::DIAMOND_NEIGHBORHOOD_CLEAR
};


class TDDPRACTICE3MATCH_API Block {
public:
    Block() : color(BlockColor::NONE), specialAttribute(BlockSpecialAttribute::NONE) {}
    Block(BlockColor color, BlockSpecialAttribute specialAttribute) : color(color), specialAttribute(specialAttribute) {}
    BlockColor GetColor() const { return color; }
    BlockSpecialAttribute GetSpecialAttribute() const { return specialAttribute; }
	bool IsSpecial() const { return specialAttribute != BlockSpecialAttribute::NONE; }
	bool HasDecoratorAttribute() const { return decoratorAttributes.Contains(specialAttribute);	}
    TUniquePtr<ExplosionArea> GetExplosionArea(const FVector2D& blockPosition, float gridSize) const {
        if (specialAttribute == BlockSpecialAttribute::VERTICAL_LINE_CLEAR)
            return MakeUnique<VerticalLineExplosionArea>(blockPosition, gridSize);
        else if (specialAttribute == BlockSpecialAttribute::HORIZONTAL_LINE_CLEAR)
            return MakeUnique<HorizontalLineExplosionArea>(blockPosition, gridSize);
        else
            return MakeUnique<EmptyExplosionArea>();
    }

    bool operator==(const Block& otherBlock) const;
    bool operator!=(const Block& otherBlock) const { return !(*this == otherBlock); }
    
    const static Block INVALID;
    const static Block ZERO;
    const static Block ONE;
    const static Block TWO;
    const static Block THREE;
    const static Block FOUR;
    const static Block MUNCHICKEN;

private:
    BlockColor color;
    BlockSpecialAttribute specialAttribute;
};
uint32 GetTypeHash(const Block& block);

FString PrettyPrint(BlockColor color);
FString PrettyPrint(BlockSpecialAttribute specialAttribute);
FString PrettyPrint(Block block);
TArray<Block> GetNormalBlocks();