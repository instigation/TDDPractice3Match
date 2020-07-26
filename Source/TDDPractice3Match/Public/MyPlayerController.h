// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class BlockPhysics;
class PhysicalBlockSnapShot;
class AActor;
class Block;
enum class ActionType;
class UPaperSprite;
class USoundWave;
class Match;

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
	constexpr static int NUM_SPECIAL_BLOCK_TYPES = 1;
	UPROPERTY(EditAnywhere, Category = Block)
	TSubclassOf<class AActor> specialBlockActorBlueprintType[NUM_SPECIAL_BLOCK_TYPES];
	UPROPERTY(EditAnywhere, Category = Block)
	TSubclassOf<class AActor> explosionActorBlutprintType;
	UPROPERTY(EditAnywhere, Category = Block)
	UPaperSprite* horizontalRibbonSprite;
	constexpr static int NUM_NORMAL_BLOCK_POP_SOUNDS = 3;
	UPROPERTY(EditAnywhere, Category = BlockSound)
	USoundWave* normalBlockPopSounds[NUM_NORMAL_BLOCK_POP_SOUNDS];
	UPROPERTY(EditAnywhere, Category = BlockSound)
	USoundWave* specialBlockPopSound;
	UPROPERTY(EditAnywhere, Category = BlockOrganization)
	float GRID_SIZE = 60.0f;
	UPROPERTY(EditAnywhere, Category = BlockOrganization)
	float DEFAULT_DEPTH = -500.0f;

private:
	void SpawnInitialBlocks();
	void SpawnBlockActor(const PhysicalBlockSnapShot& physicalBlockSnapShot);
	AActor* SpawnBlockActor(const PhysicalBlockSnapShot& physicalBlockSnapShot, const FVector* spawnPosition, const FRotator* spawnRotation);
	UClass* GetBlockActorClassToSpawn(const PhysicalBlockSnapShot& physicalBlockSnapShot);
	UClass* GetSpecialBlockActorClassToSpawn(const Block& block);
	void PlayDestroySoundIfNeeded(const PhysicalBlockSnapShot& updatedPhysicalBlock);
	void PlaySpecialDestroySound();
	void RandomlyPlayNormalDestorySound();
	void UpdateBlockStatus(AActor* pBlock, const PhysicalBlockSnapShot& updatedPhysicalBlock);
	void UpdateBlocks();
	void DeleteBlockActor(int blockId);

	void AddScore(int score);
	static int ComputeScore(const TSet<Match>& matches);

	BlockPhysics* blockPhysics;
	TMap<int, AActor*> idToBlockActorMap;
	TMap<int, ActionType> idToActionTypeMap;
#pragma endregion
};
