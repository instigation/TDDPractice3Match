// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "BlockMatrix.h"

enum class ActionType {
	Idle,
	SwipeMove,
	SwipeReturn,
	Fall,
	GetsDestroyed,
	Roll
};

FString PrettyPrint(ActionType actionType);

class BlockAction {
public:
	BlockAction(FVector2D initialPos) :position(initialPos) {}
	virtual ~BlockAction() {}
	virtual void Tick(float deltaSeconds) = 0;
	virtual bool IsJustCompleted() const = 0;
	virtual bool ShouldCheckMatch() const = 0;
	virtual bool IsEligibleForMatching() const = 0;
	virtual bool ShouldBeRemoved() const { return false; }
	// returns nullptr if there's no next action
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const = 0;
	virtual Block GetNextBlock(Block originalBlock) const { return originalBlock; }
	FVector2D GetPosition() const { return position; }
	virtual FVector2D GetOccupiedPosition() const { return GetPosition(); }
	// lower int means lower layer
	virtual int GetLayer() const { return 0; }

	virtual ActionType GetType() const = 0;
protected:
	FVector2D position;
};

class IdleBlockAction : public BlockAction {
public:
	IdleBlockAction(FVector2D initialPos) : BlockAction(initialPos) {}
	virtual void Tick(float deltaSeconds) override {}
	virtual bool IsJustCompleted() const override { return false; }
	virtual bool ShouldCheckMatch() const { return false; }
	virtual bool IsEligibleForMatching() const { return true; }
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const { return nullptr; }

	virtual ActionType GetType() const { return ActionType::Idle; }
};

class SwipeMoveBlockAction : public BlockAction {
public:
	SwipeMoveBlockAction(FIntPoint initialPos, FIntPoint destPos);
	virtual void Tick(float deltaSeconds) override;
	virtual bool IsJustCompleted() const override { return isJustCompleted; }
	virtual bool ShouldCheckMatch() const { return IsJustCompleted(); }
	virtual bool IsEligibleForMatching() const { return isJustCompleted; }
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const;
	virtual FVector2D GetOccupiedPosition() const override { return initialPos; }

	virtual ActionType GetType() const { return ActionType::SwipeMove; }
private:
	FIntPoint initialPos, destPos;
	bool isJustCompleted = false;
};

class SwipeReturnBlockAction : public BlockAction {
public:
	SwipeReturnBlockAction(FIntPoint initialPos, FIntPoint destPos);
	virtual void Tick(float deltaSeconds) override;
	virtual bool IsJustCompleted() const override { return isJustCompleted; }
	virtual bool ShouldCheckMatch() const { return false; }
	virtual bool IsEligibleForMatching() const { return isJustCompleted; }
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const { return MakeUnique<IdleBlockAction>(position); }
	virtual FVector2D GetOccupiedPosition() const override { return initialPos; }

	virtual ActionType GetType() const { return ActionType::SwipeReturn; }
private:
	FIntPoint initialPos, destPos;
	bool isJustCompleted = false;
};

class FallingBlockAction : public BlockAction {
public:
	FallingBlockAction(FIntPoint initialPos, FIntPoint destPos);
	virtual void Tick(float deltaSeconds) override;
	virtual bool IsJustCompleted() const override { return isJustCompleted; }
	virtual bool ShouldCheckMatch() const { return IsJustCompleted(); }
	virtual bool IsEligibleForMatching() const { return IsJustCompleted(); }
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const { return MakeUnique<IdleBlockAction>(position); }

	virtual ActionType GetType() const { return ActionType::Fall; }
private:
	FIntPoint initialPos, destPos;
	float currentSpeed = 0.f;
	bool isJustCompleted = false;
};

class GetsDestroyedBlockAction : public BlockAction {
public:
	GetsDestroyedBlockAction(FVector2D initialPos) : BlockAction(initialPos) {}
	virtual void Tick(float deltaSeconds) override;
	virtual bool IsJustCompleted() const override { return completed; };
	virtual bool ShouldCheckMatch() const { return false; }
	virtual bool IsEligibleForMatching() const { return false; }
	virtual bool ShouldBeRemoved() const { return completed; }
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const { return nullptr; }

	virtual ActionType GetType() const { return ActionType::GetsDestroyed; }
private:
	float elapsedTime = 0.0f;
	bool completed = false;
};

class GetsDestroyedAndSpawnBlockAfterAction : public GetsDestroyedBlockAction {
public:
	GetsDestroyedAndSpawnBlockAfterAction(FVector2D initialPos, Block blockToSpawnAfterDestroy);
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const override;
	virtual bool ShouldBeRemoved() const override { return false; }
	virtual Block GetNextBlock(Block originalBlock) const override { return blockToSpawnAfterDestroy; }
private:
	Block blockToSpawnAfterDestroy;
};

class BlockPhysics;
class MunchickenRollAction : public BlockAction {
public:
	MunchickenRollAction(FVector2D initialPos, FIntPoint rollDirection, BlockPhysics& blockPhysics);

	void Tick(float deltaSeconds) override;
	bool IsJustCompleted() const override;
	bool ShouldCheckMatch() const override;
	bool IsEligibleForMatching() const override;
	TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const override;
	ActionType GetType() const override;

private:
	void UpdatePosition(float deltaSeconds);
	TSet<FIntPoint> GetCellPositionsRolledOver() const;
	static TSet<int> GetIntegersBetween(float bound1, float bound2);
	void DestroyBlocksInBackground(const TSet<FIntPoint>& destroyPositions);
	bool IsOutOfTheMap() const;
	FVector2D previousPosition;
	FIntPoint rollDirection;
	BlockPhysics& blockPhysics;
	enum RollType {
		Invalid,
		Vertical,
		Horizontal
	};
	RollType rollType;
};

class GetsDestroyedInBackgroundBlockAction : public GetsDestroyedBlockAction {

public:
	GetsDestroyedInBackgroundBlockAction(FVector2D initialPos) : GetsDestroyedBlockAction(initialPos) {}
	int GetLayer() const override { return -1; }
	FVector2D GetOccupiedPosition() const override { return INVALID_POSITION; }
private:
	const static FIntPoint INVALID_POSITION;
};

class PhysicalBlockSnapShot {
public:
	PhysicalBlockSnapShot(int id, Block block, ActionType actionType, FVector2D position) : id(id), block(block), actionType(actionType), position(position) {}
	int id;
	Block block;
	ActionType actionType;
	FVector2D position;
};


class PhysicalBlock {
public:
	PhysicalBlock(Block block, FIntPoint initialPosition);
	PhysicalBlock(Block block, FIntPoint initialPosition, TUniquePtr<BlockAction>&& action);
	PhysicalBlock(const PhysicalBlock& other) = delete;
	PhysicalBlock(PhysicalBlock&& other);
	int GetId() const { return id; }
	PhysicalBlockSnapShot GetSnapShot() const;
	Block block;
	TUniquePtr<BlockAction> currentAction;
private:
	int id;
	static int lastIssuedId;
};

class BlockMatrix;

/**
 *
 */
class TDDPRACTICE3MATCH_API BlockPhysics
{
public:
	BlockPhysics(const BlockMatrix& blockMatrix, TFunction<int(void)> newBlockGenerator = rand);
	BlockPhysics(const BlockPhysics& other) = delete;
	BlockPhysics(BlockPhysics&& other);
	~BlockPhysics();

public:
	void Tick(float deltaSeconds);
private:
	void TickBlockActions(float deltaSeconds);
	bool ShouldCheckMatch();
	bool CheckAndProcessMatch();
	TSet<FIntPoint> GetBlockInflowPositions();
	void RemoveDeadBlocks();
	void ChangeCompletedActionsToNextActions(bool thereIsAMatch);
	void SetFallingActionsAndGenerateNewBlocks();

public:
	void RecieveSwipeInput(FIntPoint swipeStart, FIntPoint swipeEnd);

	void DisableTickDebugLog() { enableTickDebugLog = false; }
	bool enableTickDebugLog = true;

	constexpr static int MAX_ROW_COL_SIZE = 50;
	constexpr static float DELTA_COSINE = 0.001f;
	constexpr static float DELTA_DISTANCE = 0.0001f;
	constexpr static float GRID_SIZE = 1.0f;
	constexpr static float GRAVITY_ACCELERATION = 10.0f;
	constexpr static float SWIPE_MOVE_SPEED = 1.0f;
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

	int NumOccupiedCellsInColumn(int colIndex) const;
	void MakeBlockFallToDestination(PhysicalBlock& blockStatus, FIntPoint destination);

	static int ToInt(float value);
	Block GetRandomBlock();

	TArray<PhysicalBlock> physicalBlocks;
	int numRows = 0;
	int numCols = 0;
	float elapsedTime = 0.0f;
	TFunction<int(void)> newBlockGenerator;


};
