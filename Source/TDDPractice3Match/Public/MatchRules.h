#pragma once

#include "CoreMinimal.h"
#include "Block.h"

struct TDDPRACTICE3MATCH_API Formation {
	Formation(const TArray<FIntPoint>& vectors, Block specialBlock = Block::INVALID) : vectors(vectors), specialBlock(specialBlock) {};
	bool NeedSpecialBlockSpawn() const { return vectors.Num() == 4; };
	Block GetSpecialBlock() const { return specialBlock; };
	bool operator==(const Formation& other) const;
	TArray<FIntPoint> vectors;
	Block specialBlock;
};

uint32 GetTypeHash(const Formation& formation);

static class TDDPRACTICE3MATCH_API MatchRules {
public:
	const static TArray<Formation> threeBlockLineFormations;
	const static TArray<Formation> fourBlockSquareFormations;
	const static TArray<Formation> fourBlockLineFormations;

	const static TArray<TArray<Formation>> rules;
};

__declspec(selectany) const TArray<Formation> MatchRules::threeBlockLineFormations =
{
	Formation({{-1,0}, {0,0}, {1,0}}),
	Formation({{0,-1}, {0,0}, {0,1}})
};

__declspec(selectany) const TArray<Formation> MatchRules::fourBlockSquareFormations =
{
	Formation({{-1,-1}, {-1,0}, {0,-1}, {0,0}}, Block::MUNCHICKEN)
};

__declspec(selectany) const TArray<Formation> MatchRules::fourBlockLineFormations =
{
	Formation({{1,0}, {0,0}, {-1,0}, {-2,0}}, Block::HORIZONTAL_LINE_CLEAR),
	Formation({{0,1}, {0,0}, {0,-1}, {0,-2}}, Block::VERTICAL_LINE_CLEAR)
};

__declspec(selectany) const TArray<TArray<Formation>> MatchRules::rules =
{
	fourBlockLineFormations,
	threeBlockLineFormations,
	fourBlockSquareFormations
};