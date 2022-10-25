// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Abilities/CAAbilitySystemComponent.h"

void UCAAbilitySystemComponent::ReceiveDamage(UCAAbilitySystemComponent* SourceASC, float UnmitigatedDamage, float MitigatedDamage)
{
	ReceivedDamage.Broadcast(SourceASC, UnmitigatedDamage, MitigatedDamage);
}
