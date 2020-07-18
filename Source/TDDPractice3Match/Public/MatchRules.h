#pragma once

#include "CoreMinimal.h"
#include "Block.h"

struct TDDPRACTICE3MATCH_API Formation {
	Formation(const TArray<FIntPoint>& vectors, BlockSpecialAttribute specialBlock = BlockSpecialAttribute::NONE) : vectors(vectors), specialBlock(specialBlock) {};
	bool NeedSpecialBlockSpawn() const { return vectors.Num() == 4; };
	BlockSpecialAttribute GetBlockSpecialAttribute() const { return specialBlock; };
	int GetScore() const { return 100 * vectors.Num(); }
	bool operator==(const Formation& other) const;
	TArray<FIntPoint> vectors;
	BlockSpecialAttribute specialBlock;
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
	Formation({{-1,-1}, {-1,0}, {0,-1}, {0,0}}, BlockSpecialAttribute::ROLLABLE)
};

__declspec(selectany) const TArray<Formation> MatchRules::fourBlockLineFormations =
{
	Formation({{1,0}, {0,0}, {-1,0}, {-2,0}}, BlockSpecialAttribute::HORIZONTAL_LINE_CLEAR),
	Formation({{0,1}, {0,0}, {0,-1}, {0,-2}}, BlockSpecialAttribute::VERTICAL_LINE_CLEAR)
};

__declspec(selectany) const TArray<TArray<Formation>> MatchRules::rules =
{
	fourBlockLineFormations,
	fourBlockSquareFormations,
	threeBlockLineFormations
};