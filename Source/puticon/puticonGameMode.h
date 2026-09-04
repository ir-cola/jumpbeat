// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "puticonGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AputiconGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AputiconGameMode();
};



