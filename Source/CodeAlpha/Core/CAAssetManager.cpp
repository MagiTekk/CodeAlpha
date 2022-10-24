// Fill out your copyright notice in the Description page of Project Settings.


#include "CodeAlpha/Core/CAAssetManager.h"
#include <AbilitySystemGlobals.h>

void UCAAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	
	UAbilitySystemGlobals::Get().InitGlobalData();
	UE_LOG(LogTemp, Warning, TEXT("<<CA>> %s -- AT LINE :: %d"), *FString(__FUNCTION__), __LINE__);
}
