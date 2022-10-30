// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CAPlayerState.h"
#include "Character/Abilities/CAAbilitySystemComponent.h"

ACAPlayerState::ACAPlayerState()
{
	// Create ability system component, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UCAAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// Mixed mode means we only are replicated the GEs to ourselves, not the GEs to simulated proxies. If another GDPlayerState (Hero) receives a GE,
	// we won't be told about it by the Server. Attributes, GameplayTags, and GameplayCues will still replicate to us.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Create the attribute set, this replicates by default
	// Adding it as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	AttributeSetBase = CreateDefaultSubobject<UCACharacterAttributeSetBase>(TEXT("AttributeSetBase"));

	// Set PlayerState's NetUpdateFrequency to the same as the Character.
	// Default is very low for PlayerStates and introduces perceived lag in the ability system.
	// 100 is probably way too high for a shipping game, you can adjust to fit your needs.
	// #TODO_esonderblohm adjust update frequency, try some values.
	NetUpdateFrequency = 100.0f;

	// Cache tags
	DeadTag = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
}

UAbilitySystemComponent* ACAPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

class UCACharacterAttributeSetBase* ACAPlayerState::GetAttributeSetBase() const
{
	return AttributeSetBase;
}

bool ACAPlayerState::IsAlive() const
{
	return GetHealth() > 0.0f;
}

void ACAPlayerState::ShowAbilityConfirmCancelText(bool ShowText)
{
	// #TODO_esonderblohm implement HUD
}

float ACAPlayerState::GetHealth() const
{
	return AttributeSetBase->GetHealth();
}

float ACAPlayerState::GetMaxHealth() const
{
	return AttributeSetBase->GetMaxHealth();
}

float ACAPlayerState::GetMana() const
{
	return AttributeSetBase->GetMana();
}

float ACAPlayerState::GetMaxMana() const
{
	return AttributeSetBase->GetMaxMana();
}

int32 ACAPlayerState::GetCharacterLevel() const
{
	return AttributeSetBase->GetCharacterLevel();
}

void ACAPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// Attribute change callbacks
		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetHealthAttribute()).AddUObject(this, &ACAPlayerState::HealthChanged);
		MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetMaxHealthAttribute()).AddUObject(this, &ACAPlayerState::MaxHealthChanged);
		ManaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetManaAttribute()).AddUObject(this, &ACAPlayerState::ManaChanged);
		MaxManaChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetMaxManaAttribute()).AddUObject(this, &ACAPlayerState::MaxManaChanged);
		CharacterLevelChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSetBase->GetCharacterLevelAttribute()).AddUObject(this, &ACAPlayerState::CharacterLevelChanged);

		// Tag change callbacks
		AbilitySystemComponent->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Stun")), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ACAPlayerState::StunTagChanged);
	}
}

void ACAPlayerState::HealthChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("<<CodeAlpha>> %s -- AT LINE :: %d"), *FString(__FUNCTION__), __LINE__);
}

void ACAPlayerState::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("<<CodeAlpha>> %s -- AT LINE :: %d"), *FString(__FUNCTION__), __LINE__);
}

void ACAPlayerState::ManaChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("<<CodeAlpha>> %s -- AT LINE :: %d"), *FString(__FUNCTION__), __LINE__);
}

void ACAPlayerState::MaxManaChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("<<CodeAlpha>> %s -- AT LINE :: %d"), *FString(__FUNCTION__), __LINE__);
}

void ACAPlayerState::CharacterLevelChanged(const FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("<<CodeAlpha>> %s -- AT LINE :: %d"), *FString(__FUNCTION__), __LINE__);
}

void ACAPlayerState::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		FGameplayTagContainer AbilityTagsToCancel;
		AbilityTagsToCancel.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability")));

		FGameplayTagContainer AbilityTagsToIgnore;
		AbilityTagsToIgnore.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.NotCanceledByStun")));

		AbilitySystemComponent->CancelAbilities(&AbilityTagsToCancel, &AbilityTagsToIgnore);
	}
}
