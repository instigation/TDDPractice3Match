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
	const auto matchedFormations = GetMatches();
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
	const auto matches = GetMatches();
	for (const auto& match : matches) {
		const auto matchedLocation = match.GetLocation();
		const auto matchedFormation = match.GetFormation();
		const auto matchedPositions = match.GetMatchedPositions();
		const auto matchedColor = match.GetMatchedColor();
		const auto specialBlockColor = HasColor(matchedFormation.GetBlockSpecialAttribute()) ? matchedColor : BlockColor::NONE;
		RemoveBlocksAt(matchedPositions);
		matchResult.AddMatchedPositions(matchedPositions);
		if (matchedFormation.NeedSpecialBlockSpawn())
			matchResult.AddSpecialBlockWith(
				Block(specialBlockColor, matchedFormation.GetBlockSpecialAttribute()),
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
			UE_LOG(LogTemp, Error, TEXT("removed block does not exist or duplicated in block map: position (%d, %d)"), position.X, position.Y);
		}
	}
}

bool BlockMatrix::IsOutOfMatrix(FIntPoint point) const
{
	return (GetRow(point) < 0) || (GetRow(point) >= numRows) ||
		(GetCol(point) < 0) || (GetCol(point) >= numCols) ||
		(block2DArray[point.X][point.Y] == Block::INVALID);
}

TSet<Match> BlockMatrix::GetMatches() const
{
	auto ret = TSet<Match>();

	for (int i = 0; i < numRows; i++) {
		for (int j = 0; j < numCols; j++) {
			const auto point = FIntPoint{ i, j };
			const auto matchAtHereInArray = FindAMatchAt(point);
			if (matchAtHereInArray.Num() == 0)
				continue;
			const auto matchAtHere = matchAtHereInArray.Last();
			AddAndRemoveSubcompatibles(ret, matchAtHere);
		}
	}

	return ret;
}

TArray<Match> BlockMatrix::FindAMatchAt(FIntPoint point) const
{
	auto ret = TArray<Match>();
	for (const auto& rule : MatchRules::rules) {
		for (const auto& formation : rule) {
			if (IsFormationOutOfMatrix(formation, point))
				continue;
			if (ColorOfBlocksConsistentIn(formation, point)) {
				const auto color = At(point.X, point.Y).GetColor();
				ret.Add(Match(point, formation, color));
				return ret;
			}
		}
	}
	return ret;
}

bool BlockMatrix::IsFormationOutOfMatrix(const Formation& formation, FIntPoint point) const
{
	for (const auto& vector : formation.vectors) {
		if (IsOutOfMatrix(point + vector)) {
			return true;
		}
	}
	return false;
}

bool BlockMatrix::ColorOfBlocksConsistentIn(const Formation& formation, FIntPoint point) const
{
	auto colors = TSet<BlockColor>();
	for (const auto& vector : formation.vectors) {
		if (!blockMatrix.Contains(point + vector)) {
			UE_LOG(LogTemp, Warning, TEXT("IsOutOfMatrix를 통과했는데 blockMatrix에 값이 없음"));
			colors.Empty();
			break;
		}
		colors.Add(blockMatrix.FindRef(point + vector).GetColor());
	}
	return (colors.Num() == 1);
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
			return;
		}
	}
	if (!spawnedSpecialBlock) {
		AddSpecialBlockSpawn(defaultSpawnPosition, specialBlock);
	}
}

bool Match::IsSubcompatibleOf(const TSet<Match>& matches) const
{
	for (const auto& match : matches) {
		if (IsSubcompatibleOf(match))
			return true;
	}
	return false;
}

bool Match::IsSubcompatibleOf(const Match& otherMatch) const
{
	for (const auto& matchedPosition : GetMatchedPositions()) {
		if (!otherMatch.GetMatchedPositions().Contains(matchedPosition))
			return false;
	}
	return true;
}

TSet<FIntPoint> Match::GetMatchedPositions() const
{
	auto matchedPositions = TSet<FIntPoint>();
	for (const auto& vector : formation.vectors) {
		matchedPositions.Add(location + vector);
	}
	return matchedPositions;
}

bool Match::operator==(const Match& otherMatch) const
{
	return (location == otherMatch.location) && (formation == otherMatch.formation);
}

void AddAndRemoveSubcompatibles(TSet<Match>& originalSet, const Match& newElement)
{
	if (newElement.IsSubcompatibleOf(originalSet))
		return;

	auto subcompatiblesInOriginalSet = TSet<Match>();
	for (const auto& originalElement : originalSet) {
		if (originalElement.IsSubcompatibleOf(newElement)) {
			subcompatiblesInOriginalSet.Add(originalElement);
		}
	}
	originalSet = originalSet.Difference(subcompatiblesInOriginalSet);
	originalSet.Add(newElement);
}

uint32 GetTypeHash(const Match& match)
{
	return GetTypeHash(match.GetFormation()) + GetTypeHash(match.GetLocation());
}
