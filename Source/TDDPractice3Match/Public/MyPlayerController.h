// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class BlockPhysics;
class PhysicalBlockSnapShot;
class AActor;
enum class Block;

/**
 * 
 */
UCLASS()
class TDDPRACTICE3MATCH_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AMyPlayerController();
	~AMyPlayerController();

	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupInputComponent() override;

#pragma region Drag functionality
public:
	TWeakObjectPtr<AActor> GetBlockUnderCursor(bool Debug = true);
private:
	void DebugOutputHitResult(const FHitResult& hitResult);
	void ProcessSwipeInput(bool debug);
	void HandleLeftMouseInput(float value);
	bool CannotBeNormalizedAsHorizontalOrVertical(FIntPoint dragDirection);
	FIntPoint NormalizeAsHorizontalOrVertical(FIntPoint dragDirection);
	float FromCameraToBackgroundDistance = 2000.0f;
	TWeakObjectPtr<AActor> draggedActor;
	bool isDragging;
	FVector dragStart;
	FVector dragEnd;
#pragma endregion

#pragma region Cell Coordinates
	FIntPoint WorldPositionToCellCoordinate(FVector position);
	FVector CellCoordinaeToWorldPosition(FVector2D coordinate);
#pragma endregion

#pragma region Blocks
public:
	constexpr static int NUM_NORMAL_BLOCK_TYPES = 6;
	UPROPERTY(EditAnywhere, Category = Block)
	TSubclassOf<class AActor> blockActorBlueprintType[NUM_NORMAL_BLOCK_TYPES];
	UPROPERTY(EditAnywhere, Category = BlockOrganization)
	float GRID_SIZE = 60.0f;
	UPROPERTY(EditAnywhere, Category = BlockOrganization)
	float DEFAULT_DEPTH = -500.0f;

private:
	void SpawnInitialBlocks();
	void SpawnBlock(const PhysicalBlockSnapShot& physicalBlockSnapShot);
	void UpdateBlockStatus(AActor* pBlock, const PhysicalBlockSnapShot& updatedPhysicalBlock);
	void UpdateBlocks();
	void DeleteBlock(int blockId);


	BlockPhysics* blockPhysics;
	TMap<int, AActor*> idToBlockActorMap;
#pragma endregion
};
