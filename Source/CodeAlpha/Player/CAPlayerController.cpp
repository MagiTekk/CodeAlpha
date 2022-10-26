// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CAPlayerController.h"
#include "CAPlayerState.h"
#include <AbilitySystemComponent.h>

void ACAPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	ACAPlayerState* CAPlayerState = GetPlayerState<ACAPlayerState>();
	if (CAPlayerState)
	{
		// Init ASC with PlayerState (Owner) and our new Pawn (AvatarActor)
		CAPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(CAPlayerState, InPawn);
	}
}

// #TODO_esonderblohm implement HUD