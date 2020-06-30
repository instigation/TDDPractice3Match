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

	blockPhysics = new BlockPhysics(BlockMatrix(TArray<TArray<Block>>{
		{Block::ONE, Block::TWO, Block::THREE},
		{ Block::ONE, Block::TWO, Block::THREE },
		{ Block::TWO, Block::FOUR, Block::ONE }
	}));
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
	// TODO
	return false;
}

FIntPoint AMyPlayerController::NormalizeAsHorizontalOrVertical(FIntPoint dragDirection)
{
	// TODO
	return FIntPoint{ 1, 0 };
}

FIntPoint AMyPlayerController::WorldPositionToCellCoordinate(FVector position)
{
	return FIntPoint{
		FGenericPlatformMath::FloorToInt((position.X / GRID_SIZE) + 0.5),
		FGenericPlatformMath::FloorToInt((position.Z / GRID_SIZE) + 0.5)
	};
}

FVector AMyPlayerController::CellCoordinaeToWorldPosition(FVector2D coordinate)
{
	return FVector{ coordinate.X * GRID_SIZE, DEFAULT_DEPTH, coordinate.Y * GRID_SIZE };
}

void AMyPlayerController::SpawnInitialBlocks()
{
	if (!GEngine)
		return;

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Spawn Initial Blocks");

	if (!blockPhysics)
		return;

	const auto physicalBlockSnapShots = blockPhysics->GetPhysicalBlockSnapShots();
	for (const auto& physicalBlockSnapShot : physicalBlockSnapShots) {
		SpawnBlock(physicalBlockSnapShot);
	}
}

void AMyPlayerController::SpawnBlock(const PhysicalBlockSnapShot& physicalBlockSnapShot)
{
	auto world = GetWorld();
	if (!world)
		return;
	auto spawnPosition = CellCoordinaeToWorldPosition(physicalBlockSnapShot.position);
	auto spawnRotation = FRotator();
	auto spawnResult = world->SpawnActor(blockActorBlueprintType[static_cast<int>(physicalBlockSnapShot.block)].Get(), &spawnPosition, &spawnRotation);
	idToBlockActorMap.Add(physicalBlockSnapShot.id, spawnResult);
}

void AMyPlayerController::UpdateBlockStatus(AActor* pBlock, const PhysicalBlockSnapShot& physicalBlockSnapShot)
{
	if (!pBlock)
		return;

	pBlock->SetActorLocation(CellCoordinaeToWorldPosition(physicalBlockSnapShot.position));
}

void AMyPlayerController::UpdateBlocks()
{
	if (!blockPhysics)
		return;

	auto notUpdatedBlockIds = TSet<int>();
	for (const auto& idAndBlockActor : idToBlockActorMap) {
		notUpdatedBlockIds.Add(idAndBlockActor.Key);
	}

	const auto physicalBlockSnapShots = blockPhysics->GetPhysicalBlockSnapShots();
	for (const auto& physicalBlockSnapShot : physicalBlockSnapShots) {
		const auto ppBlockActor = idToBlockActorMap.Find(physicalBlockSnapShot.id);
		if (ppBlockActor == nullptr) {
			SpawnBlock(physicalBlockSnapShot);
		}
		else {
			UpdateBlockStatus(*ppBlockActor, physicalBlockSnapShot);
			notUpdatedBlockIds.Remove(physicalBlockSnapShot.id);
		}
	}

	for (const auto blockId : notUpdatedBlockIds) {
		const auto ppBlockActor = idToBlockActorMap.Find(blockId);
		if (ppBlockActor == nullptr)
			continue;

		DeleteBlock(*ppBlockActor);
	}
}

void AMyPlayerController::DeleteBlock(AActor* pBlock)
{
	if (!pBlock)
		return;

	pBlock->Destroy();
}

