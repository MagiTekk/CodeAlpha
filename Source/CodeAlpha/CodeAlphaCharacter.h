// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CodeAlpha.h"
#include <AbilitySystemInterface.h>
#include <GameplayTagContainer.h>
#include "CodeAlphaCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, ACodeAlphaCharacter*, Character);

UCLASS(config=Game)
class ACodeAlphaCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

public:
	ACodeAlphaCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Character")
	virtual bool IsAlive() const;

	// Switch on AbilityID to return individual ability levels.
	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Character")
	virtual int32 GetAbilityLevel(EAbilityInputID AbilityID) const;

	// Removes all CharacterAbilities. Can only be called by the Server. Removing on the Server will remove from Client too.
	virtual void RemoveCharacterAbilities();

	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Character")
	virtual void FinishDying();

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Character|Attributes")
	int32 GetCharacterLevel() const;

	UFUNCTION(BlueprintCallable, Category="CodeAlpha|Character|Attributes")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Character|Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Character|Attributes")
	float GetMana() const;

	UFUNCTION(BlueprintCallable, Category = "CodeAlpha|Character|Attributes")
	float GetMaxMana() const;

	UPROPERTY(BlueprintAssignable, Category = "CodeAlpha|Character")
	FCharacterDiedDelegate OnCharacterDied;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

	// Only called on the Server. Calls before Server's AcknowledgePossession.
	virtual void PossessedBy(AController* NewController) override;

	// Client only
	virtual void OnRep_PlayerState() override;

	virtual void Jump() override;

	void AddControllerPitchInput(float Val) override;

	void AddControllerYawInput(float Val) override;

protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TWeakObjectPtr<class UCAAbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<class UCACharacterAttributeSetBase> AttributeSetBase;

	FGameplayTag DeadTag;
	FGameplayTag EffectRemoveOnDeathTag;

	// Is the AbilitySystemComponent input bound or not?
	bool ASCInputBound = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="CodeAlpha|Character")
	FText CharacterName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CodeAlpha|Animation")
	UAnimMontage* DeathMontage;

	// Default abilities for this Character. These will be removed on Character death and regiven if Character respawns.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CodeAlpha|Abilities")
	TArray<TSubclassOf<class UCAGameplayAbility>> CharacterAbilities;

	// Default attributes for a character for initializing on spawn/respawn.
	// This is an instant GE that overrides the values for attributes that get reset on spawn/respawn.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CodeAlpha|Abilities")
	TSubclassOf<class UGameplayEffect> DefaultAttributes;

	// These effects are only applied one time on startup
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CodeAlpha|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> StartupEffects;

	// Grant abilities on the Server. The Ability Specs will be replicated to the owning client.
	virtual void AddCharacterAbilities();

	// Initialize the Character's attributes. Must run on Server but we run it on Client too
	// so that we don't have to wait. The Server's replication to the Client won't matter since
	// the values should be the same.
	virtual void InitializeAttributes();

	virtual void AddStartupEffects();

	/**
	* Setters for Attributes. Only use these in special cases like Respawning, otherwise use a GE to change Attributes.
	* These change the Attribute's Base Value.
	*/

	virtual void SetHealth(float Health);
	virtual void SetMana(float Mana);

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	// Called from both SetupPlayerInputComponent and OnRep_PlayerState because of a potential race condition where the PlayerController might
	// call ClientRestart which calls SetupPlayerInputComponent before the PlayerState is repped to the client so the PlayerState would be null in SetupPlayerInputComponent.
	// Conversely, the PlayerState might be repped before the PlayerController calls ClientRestart so the Actor's InputComponent would be null in OnRep_PlayerState.
	void BindAbilitySystemComponentInput();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

