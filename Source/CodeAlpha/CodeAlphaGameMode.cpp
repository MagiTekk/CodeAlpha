// Copyright Epic Games, Inc. All Rights Reserved.

#include "CodeAlphaGameMode.h"
#include "CodeAlphaCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACodeAlphaGameMode::ACodeAlphaGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
