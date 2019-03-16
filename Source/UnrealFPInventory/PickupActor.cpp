// Fill out your copyright notice in the Description page of Project Settings.

#include "PickupActor.h"

#include "Runtime/Engine/Classes/Components/StaticMeshComponent.h" //Needed for UStaticMeshComponent
#include "Runtime/Engine/Classes/Engine/EngineTypes.h" //Needed for ECollisionResponse::ECR_Overlap
#include "UnrealFPInventoryCharacter.h" //Need this to talk to the actor we collided with

#include <EngineGlobals.h> //Needed for GEngine->AddOnScreenDebugMessage()
#include <Runtime/Engine/Classes/Engine/Engine.h> //Needed for GEngine->AddOnScreenDebugMessage()


#define DebugPrint(text) {if (GEngine) {GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White,text); UE_LOG(LogTemp,Log,TEXT(text))}}
#define ErrorPrint(text) {if (GEngine) {GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::Red,text); UE_LOG(LogTemp,Error,TEXT(text))}}



// Sets default values
APickupActor::APickupActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PickupRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PickupRoot")); //Root for PickupMesh, used as its got a transform
	PickupRoot->SetMobility(EComponentMobility::Movable); //Make sure its movable, or when it disappears shadow will stay
	RootComponent = PickupRoot;	//Make a fake root

	WorldDepiction = CreateDefaultSubobject<USceneComponent>(TEXT("WorldDepiction")); //WorldDepiction parent
	WorldDepiction->SetupAttachment(PickupRoot);	//Parent Mesh to Pickup Root, so we can locally transform it
	WorldDepiction->SetMobility(EComponentMobility::Movable); //Make sure its movable, or when it disappears shadow will stay

	OnPlayerDepiction = CreateDefaultSubobject<USceneComponent>(TEXT("OnPlayerDepiction")); //OnPlayerDepiction parent
	OnPlayerDepiction->SetupAttachment(PickupRoot);	//Parent Mesh to Pickup Root, so we can locally transform it
	OnPlayerDepiction->SetMobility(EComponentMobility::Movable); //Make sure its movable, or when it disappears shadow will stay

	CurrentBobTime = 0;
	isPickedUp = false;
}

// Called when the game starts or when spawned
void APickupActor::BeginPlay()
{
	Super::BeginPlay();
	WorldDepiction->SetHiddenInGame(false, true);

	OnPlayerDepiction->SetHiddenInGame(true, true);

	OnActorBeginOverlap.AddDynamic(this, &APickupActor::OnOverlap); //Link Overlap action handler to our code
}

void APickupActor::OnOverlap(AActor * MyActor, AActor * OtherActor)
{
	APickupActor* tMyPickup = Cast<APickupActor>(MyActor);
	if(tMyPickup != nullptr && !isPickedUp) //Only pick up if pick up and not already picked up
	{ 
		AUnrealFPInventoryCharacter* tInventoryActor = Cast<AUnrealFPInventoryCharacter>(OtherActor);
		if (tInventoryActor != nullptr) //Check its the player
		{
			if (GEngine != nullptr) GEngine->AddOnScreenDebugMessage(-1, 1.5, FColor::White, FString::Printf(TEXT("OnOverlap() with %s"), *OtherActor->GetName()));
			isPickedUp=tInventoryActor->OnPickup(this); //Ask Character to pickup, they can refuse by returning false
			if (isPickedUp)
			{
				WorldDepiction->SetHiddenInGame(true, true);
				OnPlayerDepiction->SetHiddenInGame(false, true);
				OnPlayerDepiction->ResetRelativeTransform();
			}
		}
	}
}

// Called every frame
void APickupActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PickupRoot != nullptr && !isPickedUp)
	{
		FVector	tRelativeLocation = FVector(0, 0, BobHeight* sin(CurrentBobTime*BobSpeed)); //Bob Mesh up and down
		WorldDepiction->SetRelativeLocation(tRelativeLocation);
		CurrentBobTime += DeltaTime;
	}
}

