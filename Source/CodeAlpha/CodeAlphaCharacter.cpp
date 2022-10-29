// Copyright Epic Games, Inc. All Rights Reserved.

#include "CodeAlphaCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Character/Abilities/CAGameplayAbility.h"
#include "Character/Abilities/CAAbilitySystemComponent.h"
#include "Character/Abilities/AttributeSets/CACharacterAttributeSetBase.h"
#include "Player/CAPlayerState.h"
#include <GameplayAbilitySpec.h>

//////////////////////////////////////////////////////////////////////////
// ACodeAlphaCharacter

ACodeAlphaCharacter::ACodeAlphaCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	bAlwaysRelevant = true;
	DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
	EffectRemoveOnDeathTag = FGameplayTag::RequestGameplayTag(FName("State.RemoveOnDeath"));
}

UAbilitySystemComponent* ACodeAlphaCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

bool ACodeAlphaCharacter::IsAlive() const
{
	return GetHealth() > 0.0f;
}

int32 ACodeAlphaCharacter::GetAbilityLevel(EAbilityInputID AbilityID) const
{
	return 1;	//#TODO_esonderblohm make ability system progression
}

void ACodeAlphaCharacter::RemoveCharacterAbilities()
{
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent.IsValid() || !AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	// Remove any abilities added from a previous call. This checks to make sure the ability is in the startup 'CharacterAbilities' array.
	TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if ((Spec.SourceObject == this) && CharacterAbilities.Contains(Spec.Ability->GetClass()))
		{
			AbilitiesToRemove.Add(Spec.Handle);
		}
	}

	// Do in two passes so the removal happens after we have the full list
	for (int32 i = 0; i < AbilitiesToRemove.Num(); i++)
	{
		AbilitySystemComponent->ClearAbility(AbilitiesToRemove[i]);
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = false;
}

void ACodeAlphaCharacter::Die()
{
	// Only runs on Server
	RemoveCharacterAbilities();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->Velocity = FVector(0);

	OnCharacterDied.Broadcast(this);

	if (AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->CancelAllAbilities();

		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	}

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
	else
	{
		FinishDying();
	}
}

void ACodeAlphaCharacter::FinishDying()
{
	Destroy();
}

int32 ACodeAlphaCharacter::GetCharacterLevel() const
{
	if (AttributeSetBase.IsValid())
	{
		return static_cast<int32>(AttributeSetBase->GetCharacterLevel());
	}

	return 0;
}

float ACodeAlphaCharacter::GetHealth() const
{
	if (AttributeSetBase.IsValid())
	{
		return AttributeSetBase->GetHealth();
	}

	return 0.0f;
}

float ACodeAlphaCharacter::GetMaxHealth() const
{
	if (AttributeSetBase.IsValid())
	{
		return AttributeSetBase->GetMaxHealth();
	}

	return 0.0f;
}

float ACodeAlphaCharacter::GetMana() const
{
	if (AttributeSetBase.IsValid())
	{
		return AttributeSetBase->GetMana();
	}

	return 0.0f;
}

float ACodeAlphaCharacter::GetMaxMana() const
{
	if (AttributeSetBase.IsValid())
	{
		return AttributeSetBase->GetMaxMana();
	}

	return 0.0f;
}

void ACodeAlphaCharacter::PossessedBy(AController* NewController)
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

void ACodeAlphaCharacter::OnRep_PlayerState()
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

void ACodeAlphaCharacter::Jump()
{
	if (IsAlive())
	{
		Super::Jump();
	}
}

void ACodeAlphaCharacter::AddControllerPitchInput(float Val)
{
	if (IsAlive())
	{
		Super::AddControllerPitchInput(Val);
	}
}

void ACodeAlphaCharacter::AddControllerYawInput(float Val)
{
	if (IsAlive())
	{
		Super::AddControllerYawInput(Val);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACodeAlphaCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ACodeAlphaCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ACodeAlphaCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &ACodeAlphaCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &ACodeAlphaCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ACodeAlphaCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ACodeAlphaCharacter::TouchStopped);

	// Bind player input to the AbilitySystemComponent. Also called in OnRep_PlayerState because of a potential race condition.
	BindAbilitySystemComponentInput();
}

void ACodeAlphaCharacter::BindAbilitySystemComponentInput()
{
	if (!ASCInputBound && AbilitySystemComponent.IsValid() && IsValid(InputComponent))
	{
		AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, FGameplayAbilityInputBinds(FString("ConfirmTarget"),
			FString("CancelTarget"), FString("EAbilityInputID"), static_cast<int32>(EAbilityInputID::Confirm), static_cast<int32>(EAbilityInputID::Cancel)));

		ASCInputBound = true;
	}
}

void ACodeAlphaCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (IsAlive())
	{
		Jump();
	}
}

void ACodeAlphaCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void ACodeAlphaCharacter::TurnAtRate(float Rate)
{
	if (IsAlive())
	{
		// calculate delta for this frame from the rate information
		AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
	}
}

void ACodeAlphaCharacter::LookUpAtRate(float Rate)
{
	if (IsAlive())
	{
		// calculate delta for this frame from the rate information
		AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
	}
}

void ACodeAlphaCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ACodeAlphaCharacter::AddCharacterAbilities()
{
	// Grant abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent.IsValid() || AbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		return;
	}

	for (TSubclassOf<UCAGameplayAbility>& StartupAbility : CharacterAbilities)
	{
		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(StartupAbility, GetAbilityLevel(StartupAbility.GetDefaultObject()->AbilityInputID), static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID), this));
	}

	AbilitySystemComponent->bCharacterAbilitiesGiven = true;
}

void ACodeAlphaCharacter::InitializeAttributes()
{
	if (!AbilitySystemComponent.IsValid())
	{
		return;
	}

	if (!DefaultAttributes)
	{
		UE_LOG(LogTemp, Error, TEXT("%s() Missing DefaultAttributes for %s. Please fill in the character's Blueprint."), *FString(__FUNCTION__), *GetName());
		return;
	}

	// Can run on Server and Client
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributes, GetCharacterLevel(), EffectContext);
	if (NewHandle.IsValid())
	{
		FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent.Get());
	}
}

void ACodeAlphaCharacter::AddStartupEffects()
{
	if (GetLocalRole() != ROLE_Authority || !AbilitySystemComponent.IsValid() || AbilitySystemComponent->bStartupEffectsApplied)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> GameplayEffect : StartupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, GetCharacterLevel(), EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent.Get());
		}
	}

	AbilitySystemComponent->bStartupEffectsApplied = true;
}

void ACodeAlphaCharacter::SetHealth(float Health)
{
	if (AttributeSetBase.IsValid())
	{
		AttributeSetBase->SetHealth(Health);
	}
}

void ACodeAlphaCharacter::SetMana(float Mana)
{
	if (AttributeSetBase.IsValid())
	{
		AttributeSetBase->SetMana(Mana);
	}
}

void ACodeAlphaCharacter::MoveForward(float Value)
{
	if (IsAlive() && (Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ACodeAlphaCharacter::MoveRight(float Value)
{
	if (IsAlive() && (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
