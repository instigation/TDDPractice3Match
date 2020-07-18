// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDDPractice3MatchGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class TDDPRACTICE3MATCH_API ATDDPractice3MatchGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	ATDDPractice3MatchGameModeBase();
	
public:
	UPROPERTY(BlueprintReadWrite, Category=Score)
	int score = 0;
};
