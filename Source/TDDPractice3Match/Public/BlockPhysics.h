// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Block.h"

enum class ActionType {
	Idle,
	SwipeMove,
	SwipeReturn,
	Fall,
	GetsDestroyed
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
	FVector2D GetPosition() const { return position; }
	virtual FVector2D GetOccupiedPosition() const { return GetPosition(); }

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

	virtual ActionType GetType() const { return ActionType::SwipeReturn; }
private:
	FIntPoint initialPos, destPos;
	bool isJustCompleted = false;
};

class FallingBlockAction : public BlockAction {
public:
	FallingBlockAction(FIntPoint initialPos, FIntPoint destPos);
	virtual void Tick(float deltaSeconds) override {}
	virtual bool IsJustCompleted() const override { return false; }
	virtual bool ShouldCheckMatch() const { return IsJustCompleted(); }
	virtual bool IsEligibleForMatching() const { return isJustCompleted; }
	virtual TUniquePtr<BlockAction> GetNextAction(bool thereIsAMatch) const { return MakeUnique<IdleBlockAction>(position); }

	virtual ActionType GetType() const { return ActionType::Fall; }
private:
	FIntPoint initialPos, destPos;
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


class BlockPhysicalStatus {
public:
	BlockPhysicalStatus(Block block, FIntPoint initialPosition);
	BlockPhysicalStatus(Block block, FIntPoint initialPosition, TUniquePtr<BlockAction>&& action);
	BlockPhysicalStatus(const BlockPhysicalStatus& other) = delete;
	BlockPhysicalStatus(BlockPhysicalStatus&& other);
	int GetId() const { return id; }
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
	BlockPhysics(const BlockMatrix& blockMatrix);
	BlockPhysics(const BlockPhysics& other) = delete;
	BlockPhysics(BlockPhysics&& other);
	~BlockPhysics();

	void Tick(float deltaSeconds);

	void RecieveSwipeInput(FIntPoint swipeStart, FIntPoint swipeEnd);

	constexpr static int MAX_ROW_COL_SIZE = 50;
	constexpr static float DELTA_COSINE = 0.001f;
	constexpr static float DELTA_DISTANCE = 0.0001f;
	constexpr static float GRID_SIZE = 1.0f;
	constexpr static float SWIPE_MOVE_SPEED = 1.0f;
	constexpr static float DESTROY_ANIMATION_TIME = 0.2f;

	bool IsEmpty(FIntPoint position) const;
	bool ExistsBlockBetween(FIntPoint startPos, FIntPoint endPos) const;
	bool IsPlayingDestroyAnimAt(FIntPoint position) const;
	bool IsIdleAt(FIntPoint position) const;

	BlockMatrix GetBlockMatrix() const;
	int GetNumRows() const { return numRows; }
	int GetNumCols() const { return numCols; }

private:
	const BlockPhysicalStatus* GetBlockAt(FIntPoint position) const;
	BlockPhysicalStatus* GetBlockAt(FIntPoint position);
	void UpdateBlockStatus(const BlockMatrix& blockMatrix);

	int NumBlocksInColumn(int colIndex);

	static FIntPoint ToFIntPoint(FVector2D position);
	static int ToInt(float value);
	static Block GetRandomBlock();

	TArray<BlockPhysicalStatus> blocks;
	int numRows = 0;
	int numCols = 0;
	float elapsedTime = 0.0f;
};
