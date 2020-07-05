// Fill out your copyright notice in the Description page of Project Settings.

#include "MyPlayerController.h"
#include "Engine/World.h"
#include "Engine/Public/DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "../TDDPractice3MatchGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "BlockPhysics.h"
#include "BlockMatrix.h"
#include "BlockActor.h"
#include "Block.h"


AMyPlayerController::AMyPlayerController() 
	: APlayerController(), isDragging(false),
	blockPhysics(nullptr)
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	const auto  newBlockGenerator = []() -> int {
		return static_cast<int>(Block::TWO) - static_cast<int>(Block::ZERO);
	};
	blockPhysics = new BlockPhysics(BlockMatrix(TArray<TArray<Block>>{
		{Block::TWO, Block::TWO, Block::THREE},
		{ Block::ONE, Block::TWO, Block::THREE },
		{ Block::TWO, Block::MUNCHICKEN, Block::ONE }
	}), newBlockGenerator);
	blockPhysics->DisableTickDebugLog();
}

AMyPlayerController::~AMyPlayerController()
{
	delete blockPhysics;
}

void AMyPlayerController::Tick(float DeltaSeconds)
{
	if (blockPhysics)
		blockPhysics->Tick(DeltaSeconds);
	UpdateBlocks();
}

TWeakObjectPtr<AActor> AMyPlayerController::GetBlockUnderCursor(bool Debug)
{
	FVector Start, Dir, End;
	DeprojectMousePositionToWorld(Start, Dir);
	End = Start + Dir * FromCameraToBackgroundDistance;

	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);

	if (Debug) {
		DebugOutputHitResult(HitResult);
	}

	if (HitResult.Actor.IsValid()) {
		return HitResult.Actor;
	}
	else
		return nullptr;
}

void AMyPlayerController::DebugOutputHitResult(const FHitResult& hitResult)
{
	DrawDebugSolidBox(GetWorld(), hitResult.Location, FVector(20.0f), FColor::Red);
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, hitResult.Location.ToString());
		if (hitResult.Actor.IsValid())
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, hitResult.Actor->GetName());
	}

	if (hitResult.Actor.IsValid())
		DrawDebugBox(GetWorld(), hitResult.Actor->GetActorLocation(), hitResult.Actor->GetPlacementExtent(), FColor::Blue, false, 3.f, 0, 1.f);
}

void AMyPlayerController::ProcessSwipeInput(bool debug)
{
	const auto dragStartCellCoordinate = WorldPositionToCellCoordinate(dragStart);
	const auto dragEndCellCoordinate = WorldPositionToCellCoordinate(dragEnd);
	const auto dragDirection = dragEndCellCoordinate - dragStartCellCoordinate;
	if (CannotBeNormalizedAsHorizontalOrVertical(dragDirection))
		return;
	const auto normalizedDragDirection = NormalizeAsHorizontalOrVertical(dragDirection);
	const auto normalizedDragEndCellCoordinate = dragStartCellCoordinate + normalizedDragDirection;
	blockPhysics->RecieveSwipeInput(dragStartCellCoordinate, normalizedDragEndCellCoordinate);

	if (debug) {
		UE_LOG(LogTemp, Display, TEXT("dragStart: (%d, %d), dragEnd: (%d, %d)"),
			dragStartCellCoordinate.X, dragStartCellCoordinate.Y,
			normalizedDragEndCellCoordinate.X, normalizedDragEndCellCoordinate.Y);
		UE_LOG(LogTemp, Display, TEXT("dragStart: (%f, %f, %f), dragEnd: (%f, %f, %f)"),
			dragStart.X, dragStart.Y, dragStart.Z,
			dragEnd.X, dragEnd.Y, dragEnd.Z);
	}
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis("LeftMouse", this, &AMyPlayerController::HandleLeftMouseInput);
	InputComponent->BindAction("StartGame", IE_Pressed, this, &AMyPlayerController::SpawnInitialBlocks);
}

void AMyPlayerController::HandleLeftMouseInput(float value)
{
	if (isDragging && (value < 0.5)) {
		FVector unusedDir;
		DeprojectMousePositionToWorld(dragEnd, unusedDir);
		ProcessSwipeInput(true);
		isDragging = false;
	}
	else if (!isDragging && (value > 0.5)) {
		draggedActor = GetBlockUnderCursor(false);
		if (draggedActor == nullptr)
			return;
		FVector unusedDir;
		DeprojectMousePositionToWorld(dragStart, unusedDir);
		isDragging = true;
	}
}

bool AMyPlayerController::CannotBeNormalizedAsHorizontalOrVertical(FIntPoint dragDirection)
{
	return NormalizeAsHorizontalOrVertical(dragDirection) == FIntPoint{ 0, 0 };
}

FIntPoint AMyPlayerController::NormalizeAsHorizontalOrVertical(FIntPoint dragDirection)
{
	if (dragDirection.Size() == 0)
		return FIntPoint{ 0, 0 };

	auto dragDirectionAbs = FVector2D(FGenericPlatformMath::Abs(dragDirection.X), FGenericPlatformMath::Abs(dragDirection.Y));
	dragDirectionAbs.Normalize();
	const auto xAxisVector = FVector2D(1.f, 0.f);
	if (FGenericPlatformMath::Abs(FVector2D::CrossProduct(xAxisVector, dragDirectionAbs)) <= 0.5f)
		return FIntPoint{ dragDirection.X / FGenericPlatformMath::Abs(dragDirection.X), 0 };
	const auto yAxisVector = FVector2D(0.f, 1.f);
	if (FGenericPlatformMath::Abs(FVector2D::CrossProduct(yAxisVector, dragDirectionAbs)) <= 0.5f)
		return FIntPoint{ 0, dragDirection.Y / FGenericPlatformMath::Abs(dragDirection.Y) };

	return FIntPoint{ 0, 0 };
}

FIntPoint AMyPlayerController::WorldPositionToCellCoordinate(FVector position)
{
	return FIntPoint{
		FGenericPlatformMath::FloorToInt((-position.Z / GRID_SIZE) + 0.5),
		FGenericPlatformMath::FloorToInt((position.X / GRID_SIZE) + 0.5)
	};
}

FVector AMyPlayerController::CellCoordinaeToWorldPosition(FVector2D coordinate)
{
	return FVector{ coordinate.Y * GRID_SIZE, DEFAULT_DEPTH, -coordinate.X * GRID_SIZE };
}

void AMyPlayerController::SpawnInitialBlocks()
{
	if (GEngine == nullptr)
		return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Spawn Initial Blocks");

	if (blockPhysics == nullptr)
		return;

	const auto physicalBlockSnapShots = blockPhysics->GetPhysicalBlockSnapShots();
	for (const auto& physicalBlockSnapShot : physicalBlockSnapShots) {
		SpawnBlockActor(physicalBlockSnapShot);
	}
}

void AMyPlayerController::SpawnBlockActor(const PhysicalBlockSnapShot& physicalBlockSnapShot)
{
	auto world = GetWorld();
	if (world == nullptr)
		return;
	auto spawnPosition = CellCoordinaeToWorldPosition(physicalBlockSnapShot.position);
	auto spawnRotation = FRotator::ZeroRotator;
	auto spawnResult = world->SpawnActor(GetBlockActorClassToSpawn(physicalBlockSnapShot), &spawnPosition, &spawnRotation);
	idToBlockActorMap.Add(physicalBlockSnapShot.id, spawnResult);
	idToActionTypeMap.Add(physicalBlockSnapShot.id, physicalBlockSnapShot.actionType);
}

UClass* AMyPlayerController::GetBlockActorClassToSpawn(const PhysicalBlockSnapShot& physicalBlockSnapShot)
{
	if (physicalBlockSnapShot.actionType == ActionType::GetsDestroyed)
		return explosionActorBlutprintType.Get();
	if (IsSpecial(physicalBlockSnapShot.block))
		return specialBlockActorBlueprintType[static_cast<int>(physicalBlockSnapShot.block) - static_cast<int>(Block::MAX_NORMAL) - 1].Get();
	return blockActorBlueprintType[static_cast<int>(physicalBlockSnapShot.block)].Get();
}

void AMyPlayerController::UpdateBlockStatus(AActor* pBlockActor, const PhysicalBlockSnapShot& physicalBlockSnapShot)
{
	if (pBlockActor == nullptr)
		return;

	auto originalActionType = idToActionTypeMap.FindRef(physicalBlockSnapShot.id);
	auto justStartedGettingDestroyed = (physicalBlockSnapShot.actionType == ActionType::GetsDestroyed) && (originalActionType != ActionType::GetsDestroyed);
	auto justEndedGettingDestroyed = (physicalBlockSnapShot.actionType != ActionType::GetsDestroyed) && (originalActionType == ActionType::GetsDestroyed);
	if (justStartedGettingDestroyed || justEndedGettingDestroyed) {
		DeleteBlockActor(physicalBlockSnapShot.id);
		SpawnBlockActor(physicalBlockSnapShot);
	}
	else {
		pBlockActor->SetActorLocation(CellCoordinaeToWorldPosition(physicalBlockSnapShot.position));
	}
	idToActionTypeMap.Emplace(physicalBlockSnapShot.id, physicalBlockSnapShot.actionType);
}

void AMyPlayerController::UpdateBlocks()
{
	if (blockPhysics == nullptr)
		return;

	auto notUpdatedBlockIds = TSet<int>();
	for (const auto& idAndBlockActor : idToBlockActorMap) {
		notUpdatedBlockIds.Add(idAndBlockActor.Key);
	}

	const auto physicalBlockSnapShots = blockPhysics->GetPhysicalBlockSnapShots();
	for (const auto& physicalBlockSnapShot : physicalBlockSnapShots) {
		const auto ppBlockActor = idToBlockActorMap.Find(physicalBlockSnapShot.id);
		if (ppBlockActor == nullptr) {
			SpawnBlockActor(physicalBlockSnapShot);
		}
		else if (ppBlockActor != nullptr){
			UpdateBlockStatus(*ppBlockActor, physicalBlockSnapShot);
			notUpdatedBlockIds.Remove(physicalBlockSnapShot.id);
		}
	}

	for (const auto blockId : notUpdatedBlockIds) {
		DeleteBlockActor(blockId);
	}
}

void AMyPlayerController::DeleteBlockActor(int blockId)
{
	const auto ppBlockActor = idToBlockActorMap.Find(blockId);
	if (ppBlockActor == nullptr)
		return;

	auto pBlock = *ppBlockActor;
	if (pBlock == nullptr)
		return;

	pBlock->Destroy();
	idToBlockActorMap.Remove(blockId);
	idToActionTypeMap.Remove(blockId);
}
