// Fill out your copyright notice in the Description page of Project Settings.


#include "CodeAlpha/Core/CAAssetManager.h"
#include <AbilitySystemGlobals.h>

void UCAAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	
	UAbilitySystemGlobals::Get().InitGlobalData();
}
