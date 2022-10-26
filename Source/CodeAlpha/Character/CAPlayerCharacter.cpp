// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CAPlayerCharacter.h"
#include "AI/CAAIController.h"
#include <Camera/CameraComponent.h>
#include "Player/CAPlayerState.h"
#include "Abilities/CAAbilitySystemComponent.h"
#include <GameFramework/SpringArmComponent.h>
#include <Components/CapsuleComponent.h>
#include <Kismet/KismetMathLibrary.h>

ACAPlayerCharacter::ACAPlayerCharacter(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(FName("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SetRelativeLocation(FVector(0, 0, 68.492264));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(FName("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->FieldOfView = 80.0f;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	// Makes sure that the animations play on the Server so that we can use bone and socket transforms
	// to do things like spawning projectiles and other FX.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(FName("NoCollision"));

	AIControllerClass = ACAAIController::StaticClass();

	DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
}

void ACAPlayerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ACAPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACAPlayerCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ACAPlayerCharacter::LookUp);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ACAPlayerCharacter::LookUpRate);
	PlayerInputComponent->BindAxis("Turn", this, &ACAPlayerCharacter::Turn);
	PlayerInputComponent->BindAxis("TurnRate", this, &ACAPlayerCharacter::TurnRate);

	// Bind player input to the AbilitySystemComponent. Also called in OnRep_PlayerState because of a potential race condition.
	BindAbilitySystemComponentInput();
}

void ACAPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	ACAPlayerState* CAPlayerState = GetPlayerState<ACAPlayerState>();
	if (CAPlayerState)
	{
		// Set the AbilitySystemComponent on the Server. Clients do this in OnRep_PlayerState()
		AbilitySystemComponent = Cast<UCAAbilitySystemComponent>(CAPlayerState->GetAbilitySystemComponent());

		// AI won't have PlayerControllers so we can init again here just to be sure. No harm in setting up twice for characters that have PlayerControllers.
		CAPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(CAPlayerState, this);

		// Set the AttributeSetBase for convenience attribute functions
		AttributeSetBase = CAPlayerState->GetAttributeSetBase();

		// #TODO_esonderblohm If we handle players disconnecting and rejoining in the future, we'll have to change this so that possession from rejoining doesn't reset attributes.
		// For now assume possession = spawn/respawn.
		InitializeAttributes();

		// -- Respawn specific things that won't affect first possession.

		// Forcibly set the DeadTag count to 0
		AbilitySystemComponent->SetTagMapCount(DeadTag, 0);

		// Set Health/Mana/Stamina to their max. This is only necessary for *Respawn*.
		SetHealth(GetMaxHealth());
		SetMana(GetMaxMana());

		// -- End respawn specific things

		AddStartupEffects();
		AddCharacterAbilities();
	}
}

class USpringArmComponent* ACAPlayerCharacter::GetCameraBoom()
{
	return CameraBoom;
}

class UCameraComponent* ACAPlayerCharacter::GetFollowCamera()
{
	return FollowCamera;
}

float ACAPlayerCharacter::GetStartingCameraBoomArmLength()
{
	return StartingCameraBoomArmLength;
}

FVector ACAPlayerCharacter::GetStartingCameraBoomLocation()
{
	return StartingCameraBoomLocation;
}

/**
* On the Server, Possession happens before BeginPlay.
* On the Client, BeginPlay happens before Possession.
* So we can't use BeginPlay to do anything with the AbilitySystemComponent because we don't have it until the PlayerState replicates from possession.
*/
void ACAPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Only needed for Characters placed in world and when the player is the Server.
	// On respawn, they are set up in PossessedBy.

	StartingCameraBoomArmLength = CameraBoom->TargetArmLength;
	StartingCameraBoomLocation = CameraBoom->GetRelativeLocation();
}

void ACAPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	ACAPlayerState* CAPlayerState = GetPlayerState<ACAPlayerState>();
	if (CAPlayerState)
	{
		// Set the ASC for clients. Server does this in PossessedBy.
		AbilitySystemComponent = Cast<UCAAbilitySystemComponent>(CAPlayerState->GetAbilitySystemComponent());

		// Init ASC Actor Info for clients. Server will init its ASC when it possesses a new Actor.
		AbilitySystemComponent->InitAbilityActorInfo(CAPlayerState, this);

		// Bind player input to the AbilitySystemComponent. Also called in SetupPlayerInputComponent because of a potential race condition.
		BindAbilitySystemComponentInput();

		// Set the AttributeSetBase for convenience attribute functions
		AttributeSetBase = CAPlayerState->GetAttributeSetBase();

		// If we handle players disconnecting and rejoining in the future, we'll have to change this so that possession from rejoining doesn't reset attributes.
		// For now assume possession = spawn/respawn.
		InitializeAttributes();


		// -- Respawn specific things that won't affect first possession.

		// Forcibly set the DeadTag count to 0
		AbilitySystemComponent->SetTagMapCount(DeadTag, 0);

		// Set Health/Mana to their max. This is only necessary for *Respawn*.
		SetHealth(GetMaxHealth());
		SetMana(GetMaxMana());

		// -- End respawn specific things
	}
}

void ACAPlayerCharacter::LookUp(float Value)
{
	if (IsAlive())
	{
		AddControllerPitchInput(Value);
	}
}

void ACAPlayerCharacter::LookUpRate(float Value)
{
	if (IsAlive())
	{
		AddControllerPitchInput(Value * BaseLookUpRate * GetWorld()->DeltaTimeSeconds);
	}
}

void ACAPlayerCharacter::Turn(float Value)
{
	if (IsAlive())
	{
		AddControllerYawInput(Value);
	}
}

void ACAPlayerCharacter::TurnRate(float Value)
{
	if (IsAlive())
	{
		AddControllerYawInput(Value * BaseTurnRate * GetWorld()->DeltaTimeSeconds);
	}
}

void ACAPlayerCharacter::MoveForward(float Value)
{
	if (IsAlive())
	{
		AddMovementInput(UKismetMathLibrary::GetForwardVector(FRotator(0, GetControlRotation().Yaw, 0)), Value);
	}
}

void ACAPlayerCharacter::MoveRight(float Value)
{
	if (IsAlive())
	{
		AddMovementInput(UKismetMathLibrary::GetRightVector(FRotator(0, GetControlRotation().Yaw, 0)), Value);
	}
}

void ACAPlayerCharacter::BindAbilitySystemComponentInput()
{
	if (!ASCInputBound && AbilitySystemComponent.IsValid() && IsValid(InputComponent))
	{
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBinds(FString("ConfirmTarget"),
			FString("CancelTarget"), FString("EAbilityInputID"), static_cast<int32>(EAbilityInputID::Confirm), static_cast<int32>(EAbilityInputID::Cancel)));

		ASCInputBound = true;
	}
}
