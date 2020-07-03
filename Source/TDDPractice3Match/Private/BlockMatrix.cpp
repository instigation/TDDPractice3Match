// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/BlockMatrix.h"

BlockMatrix::BlockMatrix(const TArray<TArray<Block>>& block2DArray)
	: numRows(block2DArray.Num()), numCols(numRows == 0 ? 0 : block2DArray[0].Num()), block2DArray(block2DArray)
{
	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto point = FIntPoint{ i, j };
			blockMatrix.Add(point, block2DArray[i][j]);
		}
	}
}

BlockMatrix::BlockMatrix(int numRows, int numCols, const TMap<FIntPoint, Block> blockMatrix)
	:numRows(numRows), numCols(numCols), blockMatrix(blockMatrix)
{
	for (int i = 0; i < numRows; i++) {
		block2DArray.Add(TArray<Block>());
		for (int j = 0; j < numCols; j++) {
			if (nullptr == blockMatrix.Find(FIntPoint{ i,j })) {
				block2DArray[i].Add(Block::INVALID);
			}
			else {
				block2DArray[i].Add(*blockMatrix.Find(FIntPoint{ i, j }));
			}
		}
	}
}

bool BlockMatrix::HasNoMatch() const
{
	const auto matchedFormations = GetMatchedLocationAndFormations();
	return matchedFormations.Num() == 0;
}

Block BlockMatrix::At(int row, int col) const
{
	auto pBlock = blockMatrix.Find(FIntPoint{ row, col });
	return pBlock == nullptr ? Block::INVALID : *pBlock;
}

MatchResult BlockMatrix::ProcessMatch(const TSet<FIntPoint>& specialBlockSpawnCandidatePositions)
{
	auto matchResult = MatchResult();
	const auto matchedLocationAndFormations = GetMatchedLocationAndFormations();
	for (const auto& locationAndFormation : matchedLocationAndFormations) {
		const auto matchedLocation = locationAndFormation.Key;
		const auto matchedFormation = locationAndFormation.Value;
		auto matchedPositions = TSet<FIntPoint>();
		for (const auto& vector : matchedFormation.vectors) {
			matchedPositions.Add(matchedLocation + vector);
		}

		RemoveBlocksAt(matchedPositions);
		matchResult.AddMatchedPositions(matchedPositions);
		if (matchedFormation.NeedSpecialBlockSpawn())
			matchResult.AddSpecialBlockWith(
				matchedFormation.GetSpecialBlock(),
				matchedLocation,
				matchedPositions, 
				specialBlockSpawnCandidatePositions);
	}
	return matchResult;
}

void BlockMatrix::RemoveBlocksAt(const TSet<FIntPoint>& positions)
{
	for (const auto& position : positions) {
		auto removedNum = blockMatrix.Remove(position);
		if (removedNum != 1) {
			UE_LOG(LogTemp, Error, TEXT("removed block does not exist or duplicated in block map."));
		}
	}
}

bool BlockMatrix::IsOutOfMatrix(FIntPoint point) const
{
	return (GetRow(point) < 0) || (GetRow(point) >= numRows) ||
		(GetCol(point) < 0) || (GetCol(point) >= numCols) ||
		(block2DArray[point.X][point.Y] == Block::INVALID);
}

TArray<TPair<FIntPoint, Formation>> BlockMatrix::GetMatchedLocationAndFormations() const
{
	auto ret = TArray<TPair<FIntPoint, Formation>>();

	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto point = FIntPoint{ i, j };
			for (const auto& rule : MatchRules::rules) {
				for (const auto& formation : rule) {
					auto isFormationOutOfMatrix = false;
					for (const auto& vector : formation.vectors) {
						if (IsOutOfMatrix(point + vector)) {
							isFormationOutOfMatrix = true;
							break;
						}
					}

					if (!isFormationOutOfMatrix) {
						auto colors = TSet<BlockColor>();
						for (const auto& vector : formation.vectors) {
							if (!blockMatrix.Contains(point + vector)) {
								UE_LOG(LogTemp, Warning, TEXT("IsOutOfMatrix를 통과했는데 blockMatrix에 값이 없음"));
								colors.Empty();
								break;
							}
							colors.Add(GetColor(blockMatrix.FindRef(point + vector)));
						}
						if (colors.Num() == 1) {
							ret.Add(TPair<FIntPoint, Formation>{point, formation});
						}
					}
				}
			}
		}
	}

	return ret;
}

TSet<FIntPoint> MatchResult::GetSpawnPositionsOf(Block block) const
{
	auto ret = TSet<FIntPoint>();
	for (const auto& spawnPositionAndBlock : specialBlockSpawnPositions) {
		const auto spawnPosition = spawnPositionAndBlock.Key;
		const auto specialBlockToSpawn = spawnPositionAndBlock.Value;
		if (specialBlockToSpawn == block)
			ret.Add(spawnPosition);
	}
	return ret;
}

void MatchResult::AddMatchedPositions(const TSet<FIntPoint>& matchedPositions)
{
	for (const auto& matchedBlockPosition : matchedPositions) {
		AddMatchedPosition(matchedBlockPosition);
	}
}

void MatchResult::AddSpecialBlockWith(Block specialBlock, FIntPoint defaultSpawnPosition, const TSet<FIntPoint>& matchedPositions, const TSet<FIntPoint>& specialBlockSpawnCandidatePositions)
{
	bool spawnedSpecialBlock = false;
	for (const auto& spawnCandidatePos : specialBlockSpawnCandidatePositions) {
		if (matchedPositions.Contains(spawnCandidatePos)) {
			AddSpecialBlockSpawn(spawnCandidatePos, specialBlock);
			spawnedSpecialBlock = true;
		}
	}
	if (!spawnedSpecialBlock) {
		AddSpecialBlockSpawn(defaultSpawnPosition, specialBlock);
	}
}
