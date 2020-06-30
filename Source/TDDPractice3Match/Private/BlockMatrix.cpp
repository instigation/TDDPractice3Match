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
		for (const auto& vector : matchedFormation.vectors) {
			auto matchedBlockPosition = matchedLocation + vector;
			auto removedNum = blockMatrix.Remove(matchedBlockPosition);
			if (removedNum != 1) {
				UE_LOG(LogTemp, Error, TEXT("Match reported block does not exist or duplicated in block map"));
			}
			matchResult.AddMatchedPosition(matchedBlockPosition);
		}
		if (matchedFormation.NeedSpecialBlockSpawn()) {
			bool spawnedSpecialBlock = false;
			for (const auto& spawnCandidatePos : specialBlockSpawnCandidatePositions) {
				if (matchedFormation.vectors.Contains(spawnCandidatePos)) {
					matchResult.AddSpecialBlockSpawn(spawnCandidatePos, matchedFormation.GetSpecialBlock());
					spawnedSpecialBlock = true;
				}
			}
			if (!spawnedSpecialBlock) {
				matchResult.AddSpecialBlockSpawn(matchedLocation, matchedFormation.GetSpecialBlock());
			}
		}
	}
	return matchResult;
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
								UE_LOG(LogTemp, Warning, TEXT("IsOutOfMatrix�� ����ߴµ� blockMatrix�� ���� ����"));
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