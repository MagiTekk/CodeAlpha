// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CAPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CODEALPHA_API ACAPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	virtual void OnPossess(APawn* InPawn) override;

};
