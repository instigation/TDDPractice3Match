// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "MatchRules.h"


class TDDPRACTICE3MATCH_API MatchResult {
public:
	TSet<FIntPoint> GetMatchedPositions() const { return matchedPositions; }
	TSet<FIntPoint> GetSpawnPositionsOf(Block block) const;
	void AddMatchedPosition(FIntPoint matchedPosition) { matchedPositions.Add(matchedPosition); }
	void AddSpecialBlockSpawn(FIntPoint spawnPosition, Block specialBlockType) { specialBlockSpawnPositions.Add(TPair<FIntPoint, Block>{spawnPosition, specialBlockType}); }
private:
	TSet<FIntPoint> matchedPositions;
	TSet<TPair<FIntPoint, Block>> specialBlockSpawnPositions;
};

class TDDPRACTICE3MATCH_API BlockMatrix {
public:
	BlockMatrix() {}
	BlockMatrix(int numRows, int numCols, const TMap<FIntPoint, Block> blockMatrix);
	BlockMatrix(const TArray<TArray<Block>>& block2DArray);
	TArray<TArray<Block>> GetBlock2DArray() const { return block2DArray; }
	bool HasNoMatch() const;
	Block At(int row, int col) const;
	MatchResult ProcessMatch(const TSet<FIntPoint>& specialBlockSpawnCandidatePositions);
	int GetNumRows() const { return numRows; }
	int GetNumCols() const { return numCols; }
private:
	static int GetRow(FIntPoint point) { return point.X; }
	static int GetCol(FIntPoint point) { return point.Y; }
	bool IsOutOfMatrix(FIntPoint point) const;
	TArray<TPair<FIntPoint, Formation>> GetMatchedLocationAndFormations() const;
	int numRows = 0;
	int numCols = 0;
	TMap<FIntPoint, Block> blockMatrix;
	TArray<TArray<Block>> block2DArray;
};