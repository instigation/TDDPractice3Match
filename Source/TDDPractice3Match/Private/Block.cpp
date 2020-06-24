#include "../Public/Block.h"

FString PrettyPrint(Block block)
{
	if (block == Block::MUNCHICKEN)
		return TEXT("MUNCHICKEN");
	else if (block == Block::INVALID)
		return TEXT("INVALID");
	else
		return FString::FromInt(static_cast<int>(block) - static_cast<int>(Block::MIN));
}

TArray<Block> GetNormalBlocks() {
	auto ret = TArray<Block>();
	for (int i = static_cast<int>(Block::MIN) + 1; i < static_cast<int>(Block::MAX_NORMAL); i++)
		ret.Add(Block(i));
	return ret;
}

BlockColor GetColor(Block block)
{
	return BlockColor(static_cast<int>(block));
}