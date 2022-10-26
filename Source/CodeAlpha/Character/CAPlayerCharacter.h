// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CACharacterBase.h"
#include "CAPlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class CODEALPHA_API ACAPlayerCharacter : public ACACharacterBase
{
	GENERATED_BODY()

public:

	ACAPlayerCharacter(const class FObjectInitializer& ObjectInitializer);
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Only called on the Server. Calls before Server's AcknowledgePossession.
	virtual void PossessedBy(AController* NewController) override;

	class USpringArmComponent* GetCameraBoom();

	class UCameraComponent* GetFollowCamera();

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Camera")
	float GetStartingCameraBoomArmLength();

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Camera")
	FVector GetStartingCameraBoomLocation();

protected:

	virtual void BeginPlay() override;

	// Client only
	virtual void OnRep_PlayerState() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CodeAlpha|Camera")
	float BaseTurnRate = 45.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CodeAlpha|Camera")
	float BaseLookUpRate = 45.0f;

	UPROPERTY(BlueprintReadOnly, Category = "CodeAlpha|Camera")
	float StartingCameraBoomArmLength;

	UPROPERTY(BlueprintReadOnly, Category = "CodeAlpha|Camera")
	FVector StartingCameraBoomLocation;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CodeAlpha|Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CodeAlpha|Camera")
	class UCameraComponent* FollowCamera;

	// Is the AbilitySystemComponent input bound or not?
	bool ASCInputBound = false;

	FGameplayTag DeadTag;

	// Mouse
	void LookUp(float Value);

	// Gamepad
	void LookUpRate(float Value);

	// Mouse
	void Turn(float Value);

	// Gamepad
	void TurnRate(float Value);

	// Mouse + Gamepad
	void MoveForward(float Value);

	// Mouse + Gamepad
	void MoveRight(float Value);

	// Called from both SetupPlayerInputComponent and OnRep_PlayerState because of a potential race condition where the PlayerController might
	// call ClientRestart which calls SetupPlayerInputComponent before the PlayerState is repped to the client so the PlayerState would be null in SetupPlayerInputComponent.
	// Conversely, the PlayerState might be repped before the PlayerController calls ClientRestart so the Actor's InputComponent would be null in OnRep_PlayerState.
	void BindAbilitySystemComponentInput();

};
