// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "BlockMatrix.h"
#include "BlockAction.h"

class PhysicalBlockSnapShot {
public:
	PhysicalBlockSnapShot(int id, Block block, ActionType actionType, FVector2D position) : id(id), block(block), actionType(actionType), position(position) {}
	int id;
	Block block;
	ActionType actionType;
	FVector2D position;
};

class ExplosionArea;

class PhysicalBlock {
public:
	PhysicalBlock(Block block, FIntPoint initialPosition);
	PhysicalBlock(Block block, FIntPoint initialPosition, TUniquePtr<BlockAction>&& action);
	PhysicalBlock(const PhysicalBlock& other) = delete;
	PhysicalBlock(PhysicalBlock&& other);
	int GetId() const { return id; }
	PhysicalBlockSnapShot GetSnapShot() const;
	TUniquePtr<ExplosionArea> GetExplosionArea(float gridSize) const { return block.GetExplosionArea(currentAction->GetPosition(), gridSize); }
	Block block;
	TUniquePtr<BlockAction> currentAction;
private:
	int id;
	static int lastIssuedId;
};

class PhysicalBlocksSnapShotDiff {
public:
	PhysicalBlocksSnapShotDiff(const TArray<PhysicalBlockSnapShot>& before, const TArray<PhysicalBlockSnapShot>& after);
	TSet<int> GetJustDestroyedBlockIds() const;
private:
	bool WasNotDestroyingBefore(int blockId) const;
	TArray<PhysicalBlockSnapShot> before;
	TArray<PhysicalBlockSnapShot> after;
};

class BlockMatrix;

class TDDPRACTICE3MATCH_API BlockPhysics
{
public:
	explicit BlockPhysics(const BlockMatrix& blockMatrix, TFunction<int(void)> newBlockGenerator = rand);
	BlockPhysics(const BlockPhysics& other) = delete;
	BlockPhysics(BlockPhysics&& other);
	~BlockPhysics();

public:
	void Tick(float deltaSeconds);
	TSet<Match> GetMatchesInThisTick() const;
	int GetNumDestroyedBlocksInThisTick() const {
		return numDestroyedBlocksInThisTick;
	}
private:
	void TickBlockActions(float deltaSeconds);
	bool ShouldCheckMatch();
	bool CheckAndProcessMatch();
	TSet<FIntPoint> GetBlockInflowPositions();
	void RecursivelyApplyExplosionEffects(const TSet<int>& destroyedBlockIds);
	TSet<int> DestroyBlocksAndGetTheirIds(const ExplosionArea& explosionArea);
	void RemoveDeadBlocks();
	void ChangeCompletedActionsToNextActions(bool thereIsAMatch);
	void SetFallingActionsAndGenerateNewBlocks();
	TSet<Match> matchesOccuredInThisTick;
	int numDestroyedBlocksInThisTick;

public:
	void ReceiveSwipeInput(FIntPoint swipeStart, FIntPoint swipeEnd);

	void DisableTickDebugLog() { enableTickDebugLog = false; }
	bool enableTickDebugLog = true;

	constexpr static int MAX_ROW_COL_SIZE = 50;
	constexpr static float DELTA_COSINE = 0.001f;
	constexpr static float DELTA_DISTANCE = 0.0001f;
	constexpr static float GRID_SIZE = 1.0f;
	constexpr static float GRAVITY_ACCELERATION = 10.0f;
	constexpr static float SWIPE_MOVE_SPEED = 2.0f;
	constexpr static float ROLL_SPEED = SWIPE_MOVE_SPEED;
	constexpr static float DESTROY_ANIMATION_TIME = 0.35f;

	bool IsEmpty(FIntPoint position) const;
	bool ExistsBlockBetween(FIntPoint startPos, FIntPoint endPos) const;
	bool ExistsBlockNear(FIntPoint searchPosition, float threshold) const;
	bool IsPlayingDestroyAnimAt(FIntPoint position) const;
	bool IsIdleAt(FIntPoint position) const;

	void DestroyBlocksInBackgroundAt(const TSet<FIntPoint>& destroyPositions, const TSet<Block>& exceptionalBlocks);

	TArray<PhysicalBlockSnapShot> GetPhysicalBlockSnapShots() const;
	BlockMatrix GetBlockMatrix() const;
	int GetNumRows() const { return numRows; }
	int GetNumCols() const { return numCols; }

	static FIntPoint ToFIntPoint(FVector2D position);
	static bool IsNearLatticePoint(FVector2D position);

private:
	const PhysicalBlock* GetTopmostBlockAt(FIntPoint position) const;
	PhysicalBlock* GetTopmostBlockAt(FIntPoint position);
	TArray<PhysicalBlock*> GetBlocksAt(FIntPoint position);

	void StartDestroyingMatchedBlocksAccordingTo(const MatchResult& blockMatrix);
	void SetSpecialBlocksSpawnAccordingTo(const MatchResult& blockMatrix);

	void MakeBlockFallToDestination(PhysicalBlock& blockStatus, FIntPoint destination);

	static int ToInt(float value);
	Block GetRandomBlock();

	TArray<PhysicalBlock> physicalBlocks;
	int numRows = 0;
	int numCols = 0;
	float elapsedTime = 0.0f;
	TFunction<int(void)> newBlockGenerator;


};
