#include "MatchRules.h"


bool Formation::operator==(const Formation& other) const
{
	if (vectors.Num() != other.vectors.Num())
		return false;

	for (int i = 0; i < vectors.Num(); i++) {
		if (vectors[i] != other.vectors[i])
			return false;
	}
	return true;
}

uint32 GetTypeHash(const Formation& formation) {
	const auto factor = uint32(20);
	auto accumulatedFactor = uint32(1);
	auto ret = uint32(0);
	for (const auto& vector : formation.vectors) {
		ret += GetTypeHash(vector) * accumulatedFactor;
		accumulatedFactor *= factor;
	}
	return ret;
}