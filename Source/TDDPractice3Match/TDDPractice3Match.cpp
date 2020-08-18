// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TDDPractice3Match.h"
#include "Modules/ModuleManager.h"
#include "Misc/AutomationTest.h"
#include "Logging/LogMacros.h"

void FTDDPractice3MatchModuleImpl::StartupModule() {
	UE_LOG(LogTemp, Warning, TEXT("StartupModule"));

}

void FTDDPractice3MatchModuleImpl::ShutdownModule() {
	/* Workaround for UE-25350 */
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("HasNoMatchShouldReturnTrueGivenNoMatch"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("HasNoMatchShouldReturnFalseGivenMatch"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("HasNoMatchShouldReturnTrueGiven2x2"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("OnSwipeMatchCheckShouldOccur"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("IfNoMatchOnSwipeThenBlocksShouldReturn"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("TickFrequencyShouldNotMatter"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("MultipleNewBlocksShouldBeGenerated"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("MunchickenShouldBeGenerated"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("SwipeOnMunchickenShouldRollIt"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("SwipeOnMunchickenShouldRollItForFrequentTick"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("MunchickenShouldBeGeneratedAtBlockInflowPosition"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("MunchickenVerticalRollShouldNotSpawnNewBlocks"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("MunchickenShouldFallIfIdle"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("RollableShouldRollOtherRollables"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("OverlappedRollablesShouldNotMakeOtherBlockRise"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("OneByFourMatchShouldSpawnLineClearBlock"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("FourByOneMatchShouldSpawnLineClearBlock"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("LineClearerShouldClearALineOnDestroy"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("OnlyOneSpecialBlockShouldBeGeneratedEvenIfManyCandidatePositions"));
	FAutomationTestFramework::Get().UnregisterAutomationTest(TEXT("BlockPhysicsShouldReturnInActionWhenBlockMoving"));	
	
	
	UE_LOG(LogTemp, Warning, TEXT("ShoutdownModule"));
}

IMPLEMENT_PRIMARY_GAME_MODULE(FTDDPractice3MatchModuleImpl, TDDPractice3Match, "TDDPractice3Match" );
