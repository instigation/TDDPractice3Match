#include "../Public/Block.h"


uint32 GetTypeHash(const Block& block)
{
	return (1 + static_cast<int>(block.GetColor())) * (1 + static_cast<int>(block.GetSpecialAttribute()));
}

FString PrettyPrint(Block block)
{
	return PrettyPrint(block.GetColor()) + FString(TEXT("-")) + PrettyPrint(block.GetSpecialAttribute());
}

FString PrettyPrint(BlockColor color)
{
	return blockColorEnumStrings[static_cast<int>(color)];
}

FString PrettyPrint(BlockSpecialAttribute specialAttribute)
{
	return blockSpecialAttributeEnumStrings[static_cast<int>(specialAttribute)];
}

TArray<Block> GetNormalBlocks() {
	auto ret = TArray<Block>();
	for (const auto validColor : validColors) {
		ret.Add(Block(validColor, BlockSpecialAttribute::NONE));
	}
	return ret;
}

bool Block::operator==(const Block& otherBlock) const
{
	return (color == otherBlock.color) && (specialAttribute == otherBlock.specialAttribute);
}

const Block Block::INVALID = Block(BlockColor::NONE, BlockSpecialAttribute::NONE);

const Block Block::ZERO = Block(BlockColor::ZERO, BlockSpecialAttribute::NONE);

const Block Block::ONE = Block(BlockColor::ONE, BlockSpecialAttribute::NONE);

const Block Block::TWO = Block(BlockColor::TWO, BlockSpecialAttribute::NONE);

const Block Block::THREE = Block(BlockColor::THREE, BlockSpecialAttribute::NONE);

const Block Block::FOUR = Block(BlockColor::FOUR, BlockSpecialAttribute::NONE);

const Block Block::MUNCHICKEN = Block(BlockColor::NONE, BlockSpecialAttribute::ROLLABLE);

bool HasColor(BlockSpecialAttribute specialAttribute)
{
	return (specialAttribute != BlockSpecialAttribute::ROLLABLE) &&
		(specialAttribute != BlockSpecialAttribute::ONE_COLOR_CLEAR);
}
