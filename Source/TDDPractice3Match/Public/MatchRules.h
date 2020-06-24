#pragma once

#include "CoreMinimal.h"
#include "Block.h"

struct TDDPRACTICE3MATCH_API Formation {
	Formation(const TArray<FIntPoint>& vectors) : vectors(vectors) {};
	bool NeedSpecialBlockSpawn() const { return vectors.Num() == 4; };
	Block GetSpecialBlock() const { return Block::MUNCHICKEN; };
	TArray<FIntPoint> vectors;
};

static class TDDPRACTICE3MATCH_API MatchRules {
public:
	const static TArray<Formation> threeBlockLineFormations;

	const static TArray<TArray<Formation>> rules;
};

__declspec(selectany) const TArray<Formation> MatchRules::threeBlockLineFormations =
{
	Formation({{-1,0}, {0,0}, {1,0}}),
	Formation({{0,-1}, {0,0}, {0,1}}),
	Formation({{-1,-1}, {-1,0}, {0,-1}, {0,0}})
};

__declspec(selectany) const TArray<TArray<Formation>> MatchRules::rules =
{
	threeBlockLineFormations
};
