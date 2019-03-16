// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "UnrealFPInventoryGameMode.h"
#include "UnrealFPInventoryHUD.h"
#include "UnrealFPInventoryCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUnrealFPInventoryGameMode::AUnrealFPInventoryGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AUnrealFPInventoryHUD::StaticClass();
}
