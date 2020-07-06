// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "MatchRules.h"


class Match {
public:
	Match(FIntPoint location, Formation formation) : location(location), formation(formation) {}
	Match(const Match& other) = default;
	bool IsSubcompatibleOf(const TSet<Match>& matches) const;
	bool IsSubcompatibleOf(const Match& otherMatch) const;
	TSet<FIntPoint> GetMatchedPositions() const;
	FIntPoint GetLocation() const { return location; }
	Formation GetFormation() const { return formation; }
	bool operator==(const Match& otherMatch) const;
private:
	FIntPoint location;
	Formation formation;
};
void AddAndRemoveSubcompatibles(TSet<Match>& matches, const Match& matchToAdd);
uint32 GetTypeHash(const Match& match);

class TDDPRACTICE3MATCH_API MatchResult {
public:
	TSet<FIntPoint> GetMatchedPositions() const { return allMatchedPositions; }
	TSet<TPair<Block, FIntPoint>> GetSpecialBlockAndItsSpawnPositions() const { return specialBlockSpawnPositions; }
	void AddMatchedPositions(const TSet<FIntPoint>& matchedPositions);
	void AddSpecialBlockWith(Block specialBlock, FIntPoint defaultSpawnPosition, const TSet<FIntPoint>& matchedPositions, const TSet<FIntPoint>& specialBlockSpawnCandidatePositions);
private:
	void AddMatchedPosition(FIntPoint matchedPosition) { allMatchedPositions.Add(matchedPosition); }
	void AddSpecialBlockSpawn(FIntPoint spawnPosition, Block specialBlockType) { specialBlockSpawnPositions.Add(TPair<Block, FIntPoint>{specialBlockType, spawnPosition}); }

	TSet<FIntPoint> allMatchedPositions;
	TSet<TPair<Block, FIntPoint>> specialBlockSpawnPositions;
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
	void RemoveBlocksAt(const TSet<FIntPoint>& positions);
	static int GetRow(FIntPoint point) { return point.X; }
	static int GetCol(FIntPoint point) { return point.Y; }
	bool IsOutOfMatrix(FIntPoint point) const;
	TSet<Match> GetMatches() const;
	TArray<Match> FindAMatchAt(FIntPoint point) const;
	bool IsFormationOutOfMatrix(const Formation& formation, FIntPoint point) const;
	bool ColorOfBlocksConsistentIn(const Formation& formation, FIntPoint point) const;
	int numRows = 0;
	int numCols = 0;
	TMap<FIntPoint, Block> blockMatrix;
	TArray<TArray<Block>> block2DArray;
};