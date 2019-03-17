// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "UnrealFPInventoryCharacter.h"
#include "UnrealFPInventoryProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"

#include "PickupActor.h"
#include "ActorPickupLocation.h"

#include <EngineGlobals.h> //Needed for GEngine->AddOnScreenDebugMessage()
#include <Runtime/Engine/Classes/Engine/Engine.h> //Needed for GEngine->AddOnScreenDebugMessage()


#define DebugPrint(text) {if (GEngine) {GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text); UE_LOG(LogTemp,Log,TEXT(text))}}
#define ErrorPrint(text) {if (GEngine) {GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::Red,text); UE_LOG(LogTemp,Error,TEXT(text))}}



DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AUnrealFPInventoryCharacter

AUnrealFPInventoryCharacter::AUnrealFPInventoryCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);
	Ammo = 0; //No Ammo
}

void AUnrealFPInventoryCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	Mesh1P->SetHiddenInGame(false, true); //Unhide Gun
}

void AUnrealFPInventoryCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) //Tidy up after play by removing dynamically added Abilities
{


	Super::EndPlay(EndPlayReason);
}


//////////////////////////////////////////////////////////////////////////
// Input

void AUnrealFPInventoryCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AUnrealFPInventoryCharacter::OnFire);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AUnrealFPInventoryCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AUnrealFPInventoryCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AUnrealFPInventoryCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AUnrealFPInventoryCharacter::LookUpAtRate);
}


void AUnrealFPInventoryCharacter::OnFire()
{
	if (Ammo <= 0) //If we are on Empty play click
	{
		if (ClickSound != NULL)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetActorLocation());
		}
		return;
	}

	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			const FRotator SpawnRotation = GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// spawn the projectile at the muzzle
			World->SpawnActor<AUnrealFPInventoryProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
	Ammo--; //Decrement Ammo count
}


void AUnrealFPInventoryCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AUnrealFPInventoryCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AUnrealFPInventoryCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AUnrealFPInventoryCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

//Inventory


bool AUnrealFPInventoryCharacter::OnPickup_Implementation(APickupActor* tPickup)
{
	DebugPrint("Default OnPickup_Implementation()");
	TArray<UActorPickupLocation*> tLocations;	//Where items can go on Actor
	GetComponents(tLocations,true);
	for (int tI=0; tI< tLocations.Num();tI++)
	{
		TArray<USceneComponent*> tCurrentAttachments;	
		tLocations[tI]->GetChildrenComponents(false, tCurrentAttachments);
		if (tCurrentAttachments.Num() == 0)
		{
			tPickup->AttachToComponent(tLocations[tI], FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			UE_LOG(LogTemp, Log, TEXT("Attached to %d out of %d"), tI, tLocations.Num());
			Pickups.Add(tPickup);
			tPickup->OnPickedup(this); //Signal object who picked up
			return	true;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("All %d attach points used"), tLocations.Num());
	return	false;
}


int AUnrealFPInventoryCharacter::ItemCount()
{
	int		tCount = 0;
	for (int tI=0;tI <Pickups.Num();tI++)
	{
		if (IsValid(Pickups[tI])) tCount++;
	}
	return	tCount;
}

int AUnrealFPInventoryCharacter::UpdateAmmo(int Delta)
{
	Ammo += Delta;
	if (Ammo < 0) Ammo = 0; //Dont Allow Ammo to be less than 0
	return Ammo;
}

int AUnrealFPInventoryCharacter::AmmoGetter()
{
	return Ammo;
}


